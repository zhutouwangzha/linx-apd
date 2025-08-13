#include <linux/types.h>
#include <uapi/linux/in.h>
#include <net/inet_sock.h>
#include <net/ipv6.h>
#include <linux/ip.h>
#include <linux/ipv6.h>
#include <net/af_unix.h>
#include <asm-generic/access_ok.h>
#include <linux/version.h>

#include "linx_events.h"


#ifdef _DEBUG
#define ASSERT(expr) WARN_ON(!(expr))
#else
#define ASSERT(expr)
#endif /* _DEBUG */

unsigned long linx_copy_from_user(void *to, const void __user *from, unsigned long n) {
	unsigned long res = n;

	pagefault_disable();

	if(likely(access_ok(from, n)))
		res = __copy_from_user_inatomic(to, from, n);

	pagefault_enable();

	return res;
}


static void unix_socket_path(char *dest, const char *path, size_t size) {
	if(path == NULL) {
		dest[0] = '\0';
		return;
	}

	if(path[0] == '\0') {
		snprintf(dest, size - 1, "%s", path + 1);
	} else {
		snprintf(dest,
		         size,
		         "%s",
		         path); /* we assume this will be smaller than (targetbufsize - (1 + 8 + 8)) */
	}
}

inline int sock_getname(struct socket *sock, struct sockaddr *sock_address, int peer) {
	struct sock *sk = sock->sk;

	switch(sk->sk_family) {
	case AF_INET: {
		struct sockaddr_in *sin = (struct sockaddr_in *)sock_address;
		struct inet_sock *inet = (struct inet_sock *)sk;

		sin->sin_family = AF_INET;
		if(peer) {
			sin->sin_port = inet->inet_dport;
			sin->sin_addr.s_addr = inet->inet_daddr;
		} else {
			uint32_t addr = inet->inet_rcv_saddr;
			if(!addr) {
				addr = inet->inet_saddr;
			}
			sin->sin_port = inet->inet_sport;
			sin->sin_addr.s_addr = addr;
		}
		break;
	}
	case AF_INET6: {
		struct sockaddr_in6 *sin = (struct sockaddr_in6 *)sock_address;
		struct inet_sock *inet = (struct inet_sock *)sk;
		struct ipv6_pinfo *np = (struct ipv6_pinfo *)inet->pinet6;

		sin->sin6_family = AF_INET6;
		if(peer) {
			sin->sin6_port = inet->inet_dport;
			sin->sin6_addr = sk->sk_v6_daddr;
		} else {
			sin->sin6_addr = sk->sk_v6_rcv_saddr;
			if(ipv6_addr_any(&sin->sin6_addr)) {
				sin->sin6_addr = np->saddr;
			}
			sin->sin6_port = inet->inet_sport;
		}
		break;
	}

	case AF_UNIX: {
		struct sockaddr_un *sunaddr = (struct sockaddr_un *)sock_address;
		struct unix_sock *u;

		if(peer)
			sk = ((struct unix_sock *)sk)->peer;

		u = (struct unix_sock *)sk;
		if(u && u->addr) {
			unsigned int len = u->addr->len;
			if(unlikely(len > sizeof(struct sockaddr_storage))) {
				len = sizeof(struct sockaddr_storage);
			}
			memcpy(sunaddr, u->addr->name, len);
		} else {
			sunaddr->sun_family = AF_UNIX;
			sunaddr->sun_path[0] = 0;
			// The first byte to 0 can be confused with an `abstract socket address` for this reason
			// we put also the second byte to 0 to comunicate to the caller that the address is not
			// valid.
			sunaddr->sun_path[1] = 0;
		}
		break;
	}

	default:
		return -ENOTCONN;
	}

	return 0;
}


uint16_t fd_to_socktuple(int fd,
                         struct sockaddr *usrsockaddr,
                         int ulen,
                         bool use_userdata,
                         bool is_inbound,
                         char *targetbuf,
                         uint16_t targetbufsize) {
	int err = 0;
	sa_family_t family;
	uint32_t sip;
	uint32_t dip;
	uint8_t *sip6;
	uint8_t *dip6;
	uint16_t sport;
	uint16_t dport;
	struct sockaddr_in *usrsockaddr_in;
	struct sockaddr_in6 *usrsockaddr_in6;
	uint16_t size;
	struct sockaddr_storage sock_address = {};
	struct sockaddr_storage peer_address = {};
	struct socket *sock;
	char *dest;
	struct unix_sock *us;
	char *us_name = NULL;
	struct sock *speer;
	struct sockaddr_un *usrsockaddr_un;

	/*
	 * Get the socket from the fd
	 * NOTE: sockfd_lookup() locks the socket, so we don't need to worry when we dig in it
	 */
	sock = sockfd_lookup(fd, &err);

	if(unlikely(!sock || !(sock->sk))) {
		/*
		 * This usually happens if the call failed without being able to establish a connection,
		 * i.e. if it didn't return something like SE_EINPROGRESS.
		 */
		if(sock)
			sockfd_put(sock);
		return 0;
	}
	err = sock_getname(sock, (struct sockaddr *)&sock_address, 0);
	ASSERT(err == 0);

	family = sock->sk->sk_family;

	/*
	 * Extract and pack the info, based on the family
	 */
	switch(family) {
	case AF_INET:
		if(!use_userdata) {
			err = sock_getname(sock, (struct sockaddr *)&peer_address, 1);
			if(err == 0) {
				if(is_inbound) {
					sip = ((struct sockaddr_in *)&peer_address)->sin_addr.s_addr;
					sport = ntohs(((struct sockaddr_in *)&peer_address)->sin_port);
					dip = ((struct sockaddr_in *)&sock_address)->sin_addr.s_addr;
					dport = ntohs(((struct sockaddr_in *)&sock_address)->sin_port);
				} else {
					sip = ((struct sockaddr_in *)&sock_address)->sin_addr.s_addr;
					sport = ntohs(((struct sockaddr_in *)&sock_address)->sin_port);
					dip = ((struct sockaddr_in *)&peer_address)->sin_addr.s_addr;
					dport = ntohs(((struct sockaddr_in *)&peer_address)->sin_port);
				}
			} else {
				sip = 0;
				sport = 0;
				dip = 0;
				dport = 0;
			}
		} else {
			/*
			 * Map the user-provided address to a sockaddr_in
			 */
			usrsockaddr_in = (struct sockaddr_in *)usrsockaddr;

			if(is_inbound) {
				/* To take peer address info we try to use the kernel where possible.
				 * TCP allows us to obtain the right information, while the kernel doesn't fill
				 * `sk->__sk_common.skc_daddr` for UDP connection.
				 * Instead of having a custom logic for each protocol we try to read from
				 * kernel structs and if we don't find valid data we fallback to userspace
				 * structs.
				 */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 13, 0)
				sport = ntohs(sock->sk->__sk_common.skc_dport);
				if(sport != 0) {
					/* We can read from the kernel */
					sip = sock->sk->__sk_common.skc_daddr;
				} else
#endif
				{
					sip = usrsockaddr_in->sin_addr.s_addr;
					sport = ntohs(usrsockaddr_in->sin_port);
				}
				dip = ((struct sockaddr_in *)&sock_address)->sin_addr.s_addr;
				dport = ntohs(((struct sockaddr_in *)&sock_address)->sin_port);
			} else {
				sip = ((struct sockaddr_in *)&sock_address)->sin_addr.s_addr;
				sport = ntohs(((struct sockaddr_in *)&sock_address)->sin_port);
				dip = usrsockaddr_in->sin_addr.s_addr;
				dport = ntohs(usrsockaddr_in->sin_port);
			}
		}

		/*
		 * Pack the tuple info in the temporary buffer
		 */
		size = 1 + 4 + 4 + 2 + 2; /* family + sip + dip + sport + dport */

		*targetbuf = (uint8_t)family;
		*(uint32_t *)(targetbuf + 1) = sip;
		*(uint16_t *)(targetbuf + 5) = sport;
		*(uint32_t *)(targetbuf + 7) = dip;
		*(uint16_t *)(targetbuf + 11) = dport;

		break;
	case AF_INET6:
		if(!use_userdata) {
			err = sock_getname(sock, (struct sockaddr *)&peer_address, 1);
			ASSERT(err == 0);

			if(is_inbound) {
				sip6 = ((struct sockaddr_in6 *)&peer_address)->sin6_addr.s6_addr;
				sport = ntohs(((struct sockaddr_in6 *)&peer_address)->sin6_port);
				dip6 = ((struct sockaddr_in6 *)&sock_address)->sin6_addr.s6_addr;
				dport = ntohs(((struct sockaddr_in6 *)&sock_address)->sin6_port);
			} else {
				sip6 = ((struct sockaddr_in6 *)&sock_address)->sin6_addr.s6_addr;
				sport = ntohs(((struct sockaddr_in6 *)&sock_address)->sin6_port);
				dip6 = ((struct sockaddr_in6 *)&peer_address)->sin6_addr.s6_addr;
				dport = ntohs(((struct sockaddr_in6 *)&peer_address)->sin6_port);
			}
		} else {
			/*
			 * Map the user-provided address to a sockaddr_in6
			 */
			usrsockaddr_in6 = (struct sockaddr_in6 *)usrsockaddr;

			if(is_inbound) {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 13, 0)
				sport = ntohs(sock->sk->__sk_common.skc_dport);
				if(sport != 0) {
					/* We can read from the kernel */
					sip6 = sock->sk->__sk_common.skc_v6_daddr.in6_u.u6_addr8;
				} else
#endif
				{
					/* Fallback to userspace struct */
					sip6 = usrsockaddr_in6->sin6_addr.s6_addr;
					sport = ntohs(usrsockaddr_in6->sin6_port);
				}
				dip6 = ((struct sockaddr_in6 *)&sock_address)->sin6_addr.s6_addr;
				dport = ntohs(((struct sockaddr_in6 *)&sock_address)->sin6_port);
			} else {
				sip6 = ((struct sockaddr_in6 *)&sock_address)->sin6_addr.s6_addr;
				sport = ntohs(((struct sockaddr_in6 *)&sock_address)->sin6_port);
				dip6 = usrsockaddr_in6->sin6_addr.s6_addr;
				dport = ntohs(usrsockaddr_in6->sin6_port);
			}
		}

		/*
		 * Pack the tuple info in the temporary buffer
		 */
		size = 1 + 16 + 16 + 2 + 2; /* family + sip + dip + sport + dport */

		*targetbuf = ((uint8_t)family);
		memcpy(targetbuf + 1, sip6, 16);
		*(uint16_t *)(targetbuf + 17) = sport;
		memcpy(targetbuf + 19, dip6, 16);
		*(uint16_t *)(targetbuf + 35) = dport;

		break;
	case AF_UNIX:
		/*
		 * Retrieve the addresses
		 */
		us = unix_sk(sock->sk);
		speer = us->peer;

		*targetbuf = (uint8_t)(family);

		if(is_inbound) {
			*(uint64_t *)(targetbuf + 1) = (uint64_t)(unsigned long)us;
			*(uint64_t *)(targetbuf + 1 + 8) = (uint64_t)(unsigned long)speer;
			us_name = ((struct sockaddr_un *)&sock_address)->sun_path;
		} else {
			*(uint64_t *)(targetbuf + 1) = (uint64_t)(unsigned long)speer;
			*(uint64_t *)(targetbuf + 1 + 8) = (uint64_t)(unsigned long)us;
			err = sock_getname(sock, (struct sockaddr *)&peer_address, 1);
			ASSERT(err == 0);
			us_name = ((struct sockaddr_un *)&peer_address)->sun_path;
		}

		// `us_name` should contain the socket path extracted from the kernel if we cannot retrieve
		// it we can fallback to the user-provided address
		// Note that we check the second byte of `us_name`, see `sock_getname` for more details.
		// Some times `usrsockaddr` is provided as a NULL pointer, checking `use_userdata` should be
		// enough but just to be sure we check also `usrsockaddr != NULL`
		if((!us_name || (us_name[0] == '\0' && us_name[1] == '\0')) && usrsockaddr != NULL) {
			usrsockaddr_un = (struct sockaddr_un *)usrsockaddr;

			/*
			 * Put a 0 at the end of struct sockaddr_un because
			 * the user might not have considered it in the length
			 */
			if(ulen == sizeof(struct sockaddr_storage))
				*(((char *)usrsockaddr_un) + ulen - 1) = 0;
			else
				*(((char *)usrsockaddr_un) + ulen) = 0;

			if(is_inbound)
				us_name = ((struct sockaddr_un *)&sock_address)->sun_path;
			else
				us_name = usrsockaddr_un->sun_path;
		}
		size = 1 + 8 + 8;
		dest = targetbuf + size;
		unix_socket_path(dest, us_name, UNIX_PATH_MAX);
		size += strlen(dest) + 1;
		break;
	default:
		size = 0;
		break;
	}

	/*
	 * Digging finished. We can release the fd.
	 */
	sockfd_put(sock);

	return size;
}

int addr_to_kernel(void __user *uaddr, int ulen, struct sockaddr *kaddr) {
	if(unlikely(ulen < 0 || ulen > sizeof(struct sockaddr_storage)))
		return -EINVAL;

	if(unlikely(ulen == 0))
		return 0;

	if(unlikely(linx_copy_from_user(kaddr, uaddr, ulen)))
		return -EFAULT;

	return 0;
}
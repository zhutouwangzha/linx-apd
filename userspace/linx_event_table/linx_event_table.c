#include "linx_event_table.h"

const linx_event_table_t g_linx_event_table[LINX_EVENT_TYPE_MAX] = {
	[LINX_EVENT_TYPE_READ_E] = {
		"read", 2,
		{
			{"fd", LINX_FIELD_TYPE_INT64},
			{"size", LINX_FIELD_TYPE_UINT32},
		}
	},
	[LINX_EVENT_TYPE_READ_X] = {
		"read", 4,
		{
			{"res", LINX_FIELD_TYPE_INT64},
			{"data", LINX_FIELD_TYPE_BYTEBUF},
			{"fd", LINX_FIELD_TYPE_INT64},
			{"size", LINX_FIELD_TYPE_UINT32},
		},
	},
	[LINX_EVENT_TYPE_WRITE_E] = {
		"write", 2,
		{
			{"fd", LINX_FIELD_TYPE_INT64},
			{"size", LINX_FIELD_TYPE_UINT32},
		}
	},
	[LINX_EVENT_TYPE_WRITE_X] = {
		"write", 4,
		{
			{"res", LINX_FIELD_TYPE_INT64},
			{"data", LINX_FIELD_TYPE_BYTEBUF},
			{"fd", LINX_FIELD_TYPE_INT64},
			{"size", LINX_FIELD_TYPE_UINT32},
		},
	},
	[LINX_EVENT_TYPE_OPEN_E] = {
		"open", 0,
		{}
	},
	[LINX_EVENT_TYPE_OPEN_X] = {
		"open", 3,
		{
			{"filename", LINX_FIELD_TYPE_CHARBUF},
			{"flags", LINX_FIELD_TYPE_INT32},
			{"mode", LINX_FIELD_TYPE_UINT16},
		},
	},
	[LINX_EVENT_TYPE_CLOSE_E] = {
		"close", 0,
		{}
	},
	[LINX_EVENT_TYPE_CLOSE_X] = {
		"close", 1,
		{
			{"fd", LINX_FIELD_TYPE_UINT32},
		},
	},
	[LINX_EVENT_TYPE_STAT_E] = {
		"stat", 0,
		{}
	},
	[LINX_EVENT_TYPE_STAT_X] = {
		"stat", 2,
		{
			{"pathname", LINX_FIELD_TYPE_CHARBUF},
			{"buf", LINX_FIELD_TYPE_UNKNOWN},
		},
	},
	[LINX_EVENT_TYPE_FSTAT_E] = {
		"fstat", 0,
		{}
	},
	[LINX_EVENT_TYPE_FSTAT_X] = {
		"fstat", 2,
		{
			{"fd", LINX_FIELD_TYPE_UINT32},
			{"buf", LINX_FIELD_TYPE_UNKNOWN},
		},
	},
	[LINX_EVENT_TYPE_LSTAT_E] = {
		"lstat", 0,
		{}
	},
	[LINX_EVENT_TYPE_LSTAT_X] = {
		"lstat", 2,
		{
			{"pathname", LINX_FIELD_TYPE_CHARBUF},
			{"buf", LINX_FIELD_TYPE_UNKNOWN},
		},
	},
	[LINX_EVENT_TYPE_POLL_E] = {
		"poll", 0,
		{}
	},
	[LINX_EVENT_TYPE_POLL_X] = {
		"poll", 3,
		{
			{"ufds", LINX_FIELD_TYPE_UNKNOWN},
			{"nfds", LINX_FIELD_TYPE_UINT32},
			{"timeout_msecs", LINX_FIELD_TYPE_INT32},
		},
	},
	[LINX_EVENT_TYPE_LSEEK_E] = {
		"lseek", 0,
		{}
	},
	[LINX_EVENT_TYPE_LSEEK_X] = {
		"lseek", 3,
		{
			{"fd", LINX_FIELD_TYPE_UINT32},
			{"offset", LINX_FIELD_TYPE_INT64},
			{"whence", LINX_FIELD_TYPE_UINT32},
		},
	},
	[LINX_EVENT_TYPE_MMAP_E] = {
		"mmap", 0,
		{}
	},
	[LINX_EVENT_TYPE_MMAP_X] = {
		"mmap", 6,
		{
			{"addr", LINX_FIELD_TYPE_UINT64},
			{"len", LINX_FIELD_TYPE_UINT64},
			{"prot", LINX_FIELD_TYPE_UINT64},
			{"flags", LINX_FIELD_TYPE_UINT64},
			{"fd", LINX_FIELD_TYPE_UINT64},
			{"off", LINX_FIELD_TYPE_UINT64},
		},
	},
	[LINX_EVENT_TYPE_MPROTECT_E] = {
		"mprotect", 0,
		{}
	},
	[LINX_EVENT_TYPE_MPROTECT_X] = {
		"mprotect", 3,
		{
			{"start", LINX_FIELD_TYPE_UINT64},
			{"len", LINX_FIELD_TYPE_UINT64},
			{"prot", LINX_FIELD_TYPE_UINT64},
		},
	},
	[LINX_EVENT_TYPE_MUNMAP_E] = {
		"munmap", 0,
		{}
	},
	[LINX_EVENT_TYPE_MUNMAP_X] = {
		"munmap", 2,
		{
			{"addr", LINX_FIELD_TYPE_UINT64},
			{"len", LINX_FIELD_TYPE_UINT64},
		},
	},
	[LINX_EVENT_TYPE_BRK_E] = {
		"brk", 0,
		{}
	},
	[LINX_EVENT_TYPE_BRK_X] = {
		"brk", 1,
		{
			{"brk", LINX_FIELD_TYPE_UINT64},
		},
	},
	[LINX_EVENT_TYPE_RT_SIGACTION_E] = {
		"rt_sigaction", 0,
		{}
	},
	[LINX_EVENT_TYPE_RT_SIGACTION_X] = {
		"rt_sigaction", 4,
		{
			{"sig", LINX_FIELD_TYPE_INT32},
			{"act", LINX_FIELD_TYPE_UNKNOWN},
			{"oact", LINX_FIELD_TYPE_UNKNOWN},
			{"sigsetsize", LINX_FIELD_TYPE_UINT64},
		},
	},
	[LINX_EVENT_TYPE_RT_SIGPROCMASK_E] = {
		"rt_sigprocmask", 0,
		{}
	},
	[LINX_EVENT_TYPE_RT_SIGPROCMASK_X] = {
		"rt_sigprocmask", 4,
		{
			{"how", LINX_FIELD_TYPE_INT32},
			{"nset", LINX_FIELD_TYPE_UNKNOWN},
			{"oset", LINX_FIELD_TYPE_UNKNOWN},
			{"sigsetsize", LINX_FIELD_TYPE_UINT64},
		},
	},
	[LINX_EVENT_TYPE_RT_SIGRETURN_E] = {
		"rt_sigreturn", 0,
		{}
	},
	[LINX_EVENT_TYPE_RT_SIGRETURN_X] = {
		"rt_sigreturn", 0,
		{
		},
	},
	[LINX_EVENT_TYPE_IOCTL_E] = {
		"ioctl", 0,
		{}
	},
	[LINX_EVENT_TYPE_IOCTL_X] = {
		"ioctl", 3,
		{
			{"fd", LINX_FIELD_TYPE_UINT32},
			{"cmd", LINX_FIELD_TYPE_UINT32},
			{"arg", LINX_FIELD_TYPE_UINT64},
		},
	},
	[LINX_EVENT_TYPE_PREAD64_E] = {
		"pread64", 0,
		{}
	},
	[LINX_EVENT_TYPE_PREAD64_X] = {
		"pread64", 4,
		{
			{"fd", LINX_FIELD_TYPE_UINT32},
			{"buf", LINX_FIELD_TYPE_CHARBUF},
			{"count", LINX_FIELD_TYPE_UINT64},
			{"pos", LINX_FIELD_TYPE_INT64},
		},
	},
	[LINX_EVENT_TYPE_PWRITE64_E] = {
		"pwrite64", 0,
		{}
	},
	[LINX_EVENT_TYPE_PWRITE64_X] = {
		"pwrite64", 4,
		{
			{"fd", LINX_FIELD_TYPE_UINT32},
			{"buf", LINX_FIELD_TYPE_CHARBUF},
			{"count", LINX_FIELD_TYPE_UINT64},
			{"pos", LINX_FIELD_TYPE_INT64},
		},
	},
	[LINX_EVENT_TYPE_READV_E] = {
		"readv", 0,
		{}
	},
	[LINX_EVENT_TYPE_READV_X] = {
		"readv", 3,
		{
			{"fd", LINX_FIELD_TYPE_UINT64},
			{"vec", LINX_FIELD_TYPE_UNKNOWN},
			{"vlen", LINX_FIELD_TYPE_UINT64},
		},
	},
	[LINX_EVENT_TYPE_WRITEV_E] = {
		"writev", 0,
		{}
	},
	[LINX_EVENT_TYPE_WRITEV_X] = {
		"writev", 3,
		{
			{"fd", LINX_FIELD_TYPE_UINT64},
			{"vec", LINX_FIELD_TYPE_UNKNOWN},
			{"vlen", LINX_FIELD_TYPE_UINT64},
		},
	},
	[LINX_EVENT_TYPE_ACCESS_E] = {
		"access", 0,
		{}
	},
	[LINX_EVENT_TYPE_ACCESS_X] = {
		"access", 2,
		{
			{"filename", LINX_FIELD_TYPE_CHARBUF},
			{"mode", LINX_FIELD_TYPE_INT32},
		},
	},
	[LINX_EVENT_TYPE_PIPE_E] = {
		"pipe", 0,
		{}
	},
	[LINX_EVENT_TYPE_PIPE_X] = {
		"pipe", 1,
		{
			{"fildes", LINX_FIELD_TYPE_UNKNOWN},
		},
	},
	[LINX_EVENT_TYPE_SELECT_E] = {
		"select", 0,
		{}
	},
	[LINX_EVENT_TYPE_SELECT_X] = {
		"select", 5,
		{
			{"n", LINX_FIELD_TYPE_INT32},
			{"inp", LINX_FIELD_TYPE_UNKNOWN},
			{"outp", LINX_FIELD_TYPE_UNKNOWN},
			{"exp", LINX_FIELD_TYPE_UNKNOWN},
			{"tvp", LINX_FIELD_TYPE_UNKNOWN},
		},
	},
	[LINX_EVENT_TYPE_SCHED_YIELD_E] = {
		"sched_yield", 0,
		{}
	},
	[LINX_EVENT_TYPE_SCHED_YIELD_X] = {
		"sched_yield", 0,
		{
		},
	},
	[LINX_EVENT_TYPE_MREMAP_E] = {
		"mremap", 0,
		{}
	},
	[LINX_EVENT_TYPE_MREMAP_X] = {
		"mremap", 5,
		{
			{"addr", LINX_FIELD_TYPE_UINT64},
			{"old_len", LINX_FIELD_TYPE_UINT64},
			{"new_len", LINX_FIELD_TYPE_UINT64},
			{"flags", LINX_FIELD_TYPE_UINT64},
			{"new_addr", LINX_FIELD_TYPE_UINT64},
		},
	},
	[LINX_EVENT_TYPE_MSYNC_E] = {
		"msync", 0,
		{}
	},
	[LINX_EVENT_TYPE_MSYNC_X] = {
		"msync", 3,
		{
			{"start", LINX_FIELD_TYPE_UINT64},
			{"len", LINX_FIELD_TYPE_UINT64},
			{"flags", LINX_FIELD_TYPE_INT32},
		},
	},
	[LINX_EVENT_TYPE_MINCORE_E] = {
		"mincore", 0,
		{}
	},
	[LINX_EVENT_TYPE_MINCORE_X] = {
		"mincore", 3,
		{
			{"start", LINX_FIELD_TYPE_UINT64},
			{"len", LINX_FIELD_TYPE_UINT64},
			{"vec", LINX_FIELD_TYPE_UNKNOWN},
		},
	},
	[LINX_EVENT_TYPE_MADVISE_E] = {
		"madvise", 0,
		{}
	},
	[LINX_EVENT_TYPE_MADVISE_X] = {
		"madvise", 3,
		{
			{"start", LINX_FIELD_TYPE_UINT64},
			{"len_in", LINX_FIELD_TYPE_UINT64},
			{"behavior", LINX_FIELD_TYPE_INT32},
		},
	},
	[LINX_EVENT_TYPE_SHMGET_E] = {
		"shmget", 0,
		{}
	},
	[LINX_EVENT_TYPE_SHMGET_X] = {
		"shmget", 3,
		{
			{"key", LINX_FIELD_TYPE_INT32},
			{"size", LINX_FIELD_TYPE_UINT64},
			{"shmflg", LINX_FIELD_TYPE_INT32},
		},
	},
	[LINX_EVENT_TYPE_SHMAT_E] = {
		"shmat", 0,
		{}
	},
	[LINX_EVENT_TYPE_SHMAT_X] = {
		"shmat", 3,
		{
			{"shmid", LINX_FIELD_TYPE_INT32},
			{"shmaddr", LINX_FIELD_TYPE_CHARBUF},
			{"shmflg", LINX_FIELD_TYPE_INT32},
		},
	},
	[LINX_EVENT_TYPE_SHMCTL_E] = {
		"shmctl", 0,
		{}
	},
	[LINX_EVENT_TYPE_SHMCTL_X] = {
		"shmctl", 3,
		{
			{"shmid", LINX_FIELD_TYPE_INT32},
			{"cmd", LINX_FIELD_TYPE_INT32},
			{"buf", LINX_FIELD_TYPE_UNKNOWN},
		},
	},
	[LINX_EVENT_TYPE_DUP_E] = {
		"dup", 1,
		{
			{"fd", LINX_FIELD_TYPE_INT64},
		}
	},
	[LINX_EVENT_TYPE_DUP_X] = {
		"dup", 2,
		{
			{"res", LINX_FIELD_TYPE_INT64},
			{"oldfd", LINX_FIELD_TYPE_INT64},
		},
	},
	[LINX_EVENT_TYPE_DUP2_E] = {
		"dup2", 1,
		{
			{"fd", LINX_FIELD_TYPE_INT64},
		}
	},
	[LINX_EVENT_TYPE_DUP2_X] = {
		"dup2", 3,
		{
			{"res", LINX_FIELD_TYPE_INT64},
			{"oldfd", LINX_FIELD_TYPE_INT64},
			{"newfd", LINX_FIELD_TYPE_INT64}
		},
	},
	[LINX_EVENT_TYPE_PAUSE_E] = {
		"pause", 0,
		{}
	},
	[LINX_EVENT_TYPE_PAUSE_X] = {
		"pause", 0,
		{
		},
	},
	[LINX_EVENT_TYPE_NANOSLEEP_E] = {
		"nanosleep", 0,
		{}
	},
	[LINX_EVENT_TYPE_NANOSLEEP_X] = {
		"nanosleep", 2,
		{
			{"rqtp", LINX_FIELD_TYPE_UNKNOWN},
			{"rmtp", LINX_FIELD_TYPE_UNKNOWN},
		},
	},
	[LINX_EVENT_TYPE_GETITIMER_E] = {
		"getitimer", 0,
		{}
	},
	[LINX_EVENT_TYPE_GETITIMER_X] = {
		"getitimer", 2,
		{
			{"which", LINX_FIELD_TYPE_INT32},
			{"value", LINX_FIELD_TYPE_UNKNOWN},
		},
	},
	[LINX_EVENT_TYPE_ALARM_E] = {
		"alarm", 0,
		{}
	},
	[LINX_EVENT_TYPE_ALARM_X] = {
		"alarm", 1,
		{
			{"seconds", LINX_FIELD_TYPE_UINT32},
		},
	},
	[LINX_EVENT_TYPE_SETITIMER_E] = {
		"setitimer", 0,
		{}
	},
	[LINX_EVENT_TYPE_SETITIMER_X] = {
		"setitimer", 3,
		{
			{"which", LINX_FIELD_TYPE_INT32},
			{"value", LINX_FIELD_TYPE_UNKNOWN},
			{"ovalue", LINX_FIELD_TYPE_UNKNOWN},
		},
	},
	[LINX_EVENT_TYPE_GETPID_E] = {
		"getpid", 0,
		{}
	},
	[LINX_EVENT_TYPE_GETPID_X] = {
		"getpid", 0,
		{
		},
	},
	[LINX_EVENT_TYPE_SENDFILE_E] = {
		"sendfile", 0,
		{}
	},
	[LINX_EVENT_TYPE_SENDFILE_X] = {
		"sendfile", 4,
		{
			{"out_fd", LINX_FIELD_TYPE_INT32},
			{"in_fd", LINX_FIELD_TYPE_INT32},
			{"offset", LINX_FIELD_TYPE_UNKNOWN},
			{"count", LINX_FIELD_TYPE_UINT64},
		},
	},
	[LINX_EVENT_TYPE_SOCKET_E] = {
		"socket", 0,
		{}
	},
	[LINX_EVENT_TYPE_SOCKET_X] = {
		"socket", 3,
		{
			{"family", LINX_FIELD_TYPE_INT32},
			{"type", LINX_FIELD_TYPE_INT32},
			{"protocol", LINX_FIELD_TYPE_INT32},
		},
	},
	[LINX_EVENT_TYPE_CONNECT_E] = {
		"connect", 0,
		{}
	},
	[LINX_EVENT_TYPE_CONNECT_X] = {
		"connect", 3,
		{
			{"fd", LINX_FIELD_TYPE_INT32},
			{"uservaddr", LINX_FIELD_TYPE_UNKNOWN},
			{"addrlen", LINX_FIELD_TYPE_INT32},
		},
	},
	[LINX_EVENT_TYPE_ACCEPT_E] = {
		"accept", 0,
		{}
	},
	[LINX_EVENT_TYPE_ACCEPT_X] = {
		"accept", 3,
		{
			{"fd", LINX_FIELD_TYPE_INT32},
			{"upeer_sockaddr", LINX_FIELD_TYPE_UNKNOWN},
			{"upeer_addrlen", LINX_FIELD_TYPE_UNKNOWN},
		},
	},
	[LINX_EVENT_TYPE_SENDTO_E] = {
		"sendto", 3,
		{
			{"fd", LINX_FIELD_TYPE_INT64},
			{"size", LINX_FIELD_TYPE_UINT32},
			{"tuple", LINX_FIELD_TYPE_SOCKTUPLE}
		}
	},
	[LINX_EVENT_TYPE_SENDTO_X] = {
		"sendto", 2,
		{
			{"res", LINX_FIELD_TYPE_INT64},
			{"data", LINX_FIELD_TYPE_BYTEBUF},
		},
	},
	[LINX_EVENT_TYPE_RECVFROM_E] = {
		"recvfrom", 2,
		{
			{"fd", LINX_FIELD_TYPE_INT64},
			{"size", LINX_FIELD_TYPE_UINT32}
		}
	},
	[LINX_EVENT_TYPE_RECVFROM_X] = {
		"recvfrom", 3,
		{
			{"res", LINX_FIELD_TYPE_INT32},
			{"data", LINX_FIELD_TYPE_BYTEBUF},
			{"tuple", LINX_FIELD_TYPE_SOCKTUPLE}
		},
	},
	[LINX_EVENT_TYPE_SENDMSG_E] = {
		"sendmsg", 0,
		{}
	},
	[LINX_EVENT_TYPE_SENDMSG_X] = {
		"sendmsg", 3,
		{
			{"fd", LINX_FIELD_TYPE_INT32},
			{"msg", LINX_FIELD_TYPE_UNKNOWN},
			{"flags", LINX_FIELD_TYPE_UINT32},
		},
	},
	[LINX_EVENT_TYPE_RECVMSG_E] = {
		"recvmsg", 0,
		{}
	},
	[LINX_EVENT_TYPE_RECVMSG_X] = {
		"recvmsg", 3,
		{
			{"fd", LINX_FIELD_TYPE_INT32},
			{"msg", LINX_FIELD_TYPE_UNKNOWN},
			{"flags", LINX_FIELD_TYPE_UINT32},
		},
	},
	[LINX_EVENT_TYPE_SHUTDOWN_E] = {
		"shutdown", 0,
		{}
	},
	[LINX_EVENT_TYPE_SHUTDOWN_X] = {
		"shutdown", 2,
		{
			{"fd", LINX_FIELD_TYPE_INT32},
			{"how", LINX_FIELD_TYPE_INT32},
		},
	},
	[LINX_EVENT_TYPE_BIND_E] = {
		"bind", 0,
		{}
	},
	[LINX_EVENT_TYPE_BIND_X] = {
		"bind", 3,
		{
			{"fd", LINX_FIELD_TYPE_INT32},
			{"umyaddr", LINX_FIELD_TYPE_UNKNOWN},
			{"addrlen", LINX_FIELD_TYPE_INT32},
		},
	},
	[LINX_EVENT_TYPE_LISTEN_E] = {
		"listen", 0,
		{}
	},
	[LINX_EVENT_TYPE_LISTEN_X] = {
		"listen", 2,
		{
			{"fd", LINX_FIELD_TYPE_INT32},
			{"backlog", LINX_FIELD_TYPE_INT32},
		},
	},
	[LINX_EVENT_TYPE_GETSOCKNAME_E] = {
		"getsockname", 0,
		{}
	},
	[LINX_EVENT_TYPE_GETSOCKNAME_X] = {
		"getsockname", 3,
		{
			{"fd", LINX_FIELD_TYPE_INT32},
			{"usockaddr", LINX_FIELD_TYPE_UNKNOWN},
			{"usockaddr_len", LINX_FIELD_TYPE_UNKNOWN},
		},
	},
	[LINX_EVENT_TYPE_GETPEERNAME_E] = {
		"getpeername", 0,
		{}
	},
	[LINX_EVENT_TYPE_GETPEERNAME_X] = {
		"getpeername", 3,
		{
			{"fd", LINX_FIELD_TYPE_INT32},
			{"usockaddr", LINX_FIELD_TYPE_UNKNOWN},
			{"usockaddr_len", LINX_FIELD_TYPE_UNKNOWN},
		},
	},
	[LINX_EVENT_TYPE_SOCKETPAIR_E] = {
		"socketpair", 0,
		{}
	},
	[LINX_EVENT_TYPE_SOCKETPAIR_X] = {
		"socketpair", 4,
		{
			{"family", LINX_FIELD_TYPE_INT32},
			{"type", LINX_FIELD_TYPE_INT32},
			{"protocol", LINX_FIELD_TYPE_INT32},
			{"usockvec", LINX_FIELD_TYPE_UNKNOWN},
		},
	},
	[LINX_EVENT_TYPE_SETSOCKOPT_E] = {
		"setsockopt", 0,
		{}
	},
	[LINX_EVENT_TYPE_SETSOCKOPT_X] = {
		"setsockopt", 5,
		{
			{"fd", LINX_FIELD_TYPE_INT32},
			{"level", LINX_FIELD_TYPE_INT32},
			{"optname", LINX_FIELD_TYPE_INT32},
			{"optval", LINX_FIELD_TYPE_CHARBUF},
			{"optlen", LINX_FIELD_TYPE_INT32},
		},
	},
	[LINX_EVENT_TYPE_GETSOCKOPT_E] = {
		"getsockopt", 0,
		{}
	},
	[LINX_EVENT_TYPE_GETSOCKOPT_X] = {
		"getsockopt", 5,
		{
			{"fd", LINX_FIELD_TYPE_INT32},
			{"level", LINX_FIELD_TYPE_INT32},
			{"optname", LINX_FIELD_TYPE_INT32},
			{"optval", LINX_FIELD_TYPE_CHARBUF},
			{"optlen", LINX_FIELD_TYPE_UNKNOWN},
		},
	},
	[LINX_EVENT_TYPE_CLONE_E] = {
		"clone", 0,
		{}
	},
	[LINX_EVENT_TYPE_CLONE_X] = {
		"clone", 5,
		{
			{"clone_flags", LINX_FIELD_TYPE_UINT64},
			{"newsp", LINX_FIELD_TYPE_UINT64},
			{"parent_tidptr", LINX_FIELD_TYPE_UNKNOWN},
			{"child_tidptr", LINX_FIELD_TYPE_UNKNOWN},
			{"tls", LINX_FIELD_TYPE_UINT64},
		},
	},
	[LINX_EVENT_TYPE_FORK_E] = {
		"fork", 0,
		{}
	},
	[LINX_EVENT_TYPE_FORK_X] = {
		"fork", 0,
		{
		},
	},
	[LINX_EVENT_TYPE_VFORK_E] = {
		"vfork", 0,
		{}
	},
	[LINX_EVENT_TYPE_VFORK_X] = {
		"vfork", 0,
		{
		},
	},
	[LINX_EVENT_TYPE_EXECVE_E] = {
		"execve", 1,
		{
			{"filename", LINX_FIELD_TYPE_CHARBUF}
		}
	},
	[LINX_EVENT_TYPE_EXECVE_X] = {
		"execve", 8,
		{
			{"exe", LINX_FIELD_TYPE_CHARBUF},
			{"vm_size", LINX_FIELD_TYPE_UINT32},
			{"vm_rss", LINX_FIELD_TYPE_UINT32},
			{"comm", LINX_FIELD_TYPE_CHARBUF},
			{"tty", LINX_FIELD_TYPE_UINT32},
			{"env", LINX_FIELD_TYPE_BYTEBUF},
			{"loginuid", LINX_FIELD_TYPE_UID},
			{"pgid", LINX_FIELD_TYPE_PID}
		},
	},
	[LINX_EVENT_TYPE_EXIT_E] = {
		"exit", 0,
		{}
	},
	[LINX_EVENT_TYPE_EXIT_X] = {
		"exit", 1,
		{
			{"error_code", LINX_FIELD_TYPE_INT32},
		},
	},
	[LINX_EVENT_TYPE_WAIT4_E] = {
		"wait4", 0,
		{}
	},
	[LINX_EVENT_TYPE_WAIT4_X] = {
		"wait4", 4,
		{
			{"upid", LINX_FIELD_TYPE_INT32},
			{"stat_addr", LINX_FIELD_TYPE_UNKNOWN},
			{"options", LINX_FIELD_TYPE_INT32},
			{"ru", LINX_FIELD_TYPE_UNKNOWN},
		},
	},
	[LINX_EVENT_TYPE_KILL_E] = {
		"kill", 0,
		{}
	},
	[LINX_EVENT_TYPE_KILL_X] = {
		"kill", 2,
		{
			{"pid", LINX_FIELD_TYPE_INT32},
			{"sig", LINX_FIELD_TYPE_INT32},
		},
	},
	[LINX_EVENT_TYPE_UNAME_E] = {
		"uname", 0,
		{}
	},
	[LINX_EVENT_TYPE_UNAME_X] = {
		"uname", 1,
		{
			{"name", LINX_FIELD_TYPE_UNKNOWN},
		},
	},
	[LINX_EVENT_TYPE_SEMGET_E] = {
		"semget", 0,
		{}
	},
	[LINX_EVENT_TYPE_SEMGET_X] = {
		"semget", 3,
		{
			{"key", LINX_FIELD_TYPE_INT32},
			{"nsems", LINX_FIELD_TYPE_INT32},
			{"semflg", LINX_FIELD_TYPE_INT32},
		},
	},
	[LINX_EVENT_TYPE_SEMOP_E] = {
		"semop", 0,
		{}
	},
	[LINX_EVENT_TYPE_SEMOP_X] = {
		"semop", 3,
		{
			{"semid", LINX_FIELD_TYPE_INT32},
			{"tsops", LINX_FIELD_TYPE_UNKNOWN},
			{"nsops", LINX_FIELD_TYPE_UINT32},
		},
	},
	[LINX_EVENT_TYPE_SEMCTL_E] = {
		"semctl", 0,
		{}
	},
	[LINX_EVENT_TYPE_SEMCTL_X] = {
		"semctl", 4,
		{
			{"semid", LINX_FIELD_TYPE_INT32},
			{"semnum", LINX_FIELD_TYPE_INT32},
			{"cmd", LINX_FIELD_TYPE_INT32},
			{"arg", LINX_FIELD_TYPE_UINT64},
		},
	},
	[LINX_EVENT_TYPE_SHMDT_E] = {
		"shmdt", 0,
		{}
	},
	[LINX_EVENT_TYPE_SHMDT_X] = {
		"shmdt", 1,
		{
			{"shmaddr", LINX_FIELD_TYPE_CHARBUF},
		},
	},
	[LINX_EVENT_TYPE_MSGGET_E] = {
		"msgget", 0,
		{}
	},
	[LINX_EVENT_TYPE_MSGGET_X] = {
		"msgget", 2,
		{
			{"key", LINX_FIELD_TYPE_INT32},
			{"msgflg", LINX_FIELD_TYPE_INT32},
		},
	},
	[LINX_EVENT_TYPE_MSGSND_E] = {
		"msgsnd", 0,
		{}
	},
	[LINX_EVENT_TYPE_MSGSND_X] = {
		"msgsnd", 4,
		{
			{"msqid", LINX_FIELD_TYPE_INT32},
			{"msgp", LINX_FIELD_TYPE_UNKNOWN},
			{"msgsz", LINX_FIELD_TYPE_UINT64},
			{"msgflg", LINX_FIELD_TYPE_INT32},
		},
	},
	[LINX_EVENT_TYPE_MSGRCV_E] = {
		"msgrcv", 0,
		{}
	},
	[LINX_EVENT_TYPE_MSGRCV_X] = {
		"msgrcv", 5,
		{
			{"msqid", LINX_FIELD_TYPE_INT32},
			{"msgp", LINX_FIELD_TYPE_UNKNOWN},
			{"msgsz", LINX_FIELD_TYPE_UINT64},
			{"msgtyp", LINX_FIELD_TYPE_INT64},
			{"msgflg", LINX_FIELD_TYPE_INT32},
		},
	},
	[LINX_EVENT_TYPE_MSGCTL_E] = {
		"msgctl", 0,
		{}
	},
	[LINX_EVENT_TYPE_MSGCTL_X] = {
		"msgctl", 3,
		{
			{"msqid", LINX_FIELD_TYPE_INT32},
			{"cmd", LINX_FIELD_TYPE_INT32},
			{"buf", LINX_FIELD_TYPE_UNKNOWN},
		},
	},
	[LINX_EVENT_TYPE_FCNTL_E] = {
		"fcntl", 0,
		{}
	},
	[LINX_EVENT_TYPE_FCNTL_X] = {
		"fcntl", 3,
		{
			{"fd", LINX_FIELD_TYPE_UINT32},
			{"cmd", LINX_FIELD_TYPE_UINT32},
			{"arg", LINX_FIELD_TYPE_UINT64},
		},
	},
	[LINX_EVENT_TYPE_FLOCK_E] = {
		"flock", 0,
		{}
	},
	[LINX_EVENT_TYPE_FLOCK_X] = {
		"flock", 2,
		{
			{"fd", LINX_FIELD_TYPE_UINT32},
			{"cmd", LINX_FIELD_TYPE_UINT32},
		},
	},
	[LINX_EVENT_TYPE_FSYNC_E] = {
		"fsync", 0,
		{}
	},
	[LINX_EVENT_TYPE_FSYNC_X] = {
		"fsync", 1,
		{
			{"fd", LINX_FIELD_TYPE_UINT32},
		},
	},
	[LINX_EVENT_TYPE_FDATASYNC_E] = {
		"fdatasync", 0,
		{}
	},
	[LINX_EVENT_TYPE_FDATASYNC_X] = {
		"fdatasync", 1,
		{
			{"fd", LINX_FIELD_TYPE_UINT32},
		},
	},
	[LINX_EVENT_TYPE_TRUNCATE_E] = {
		"truncate", 0,
		{}
	},
	[LINX_EVENT_TYPE_TRUNCATE_X] = {
		"truncate", 2,
		{
			{"path", LINX_FIELD_TYPE_CHARBUF},
			{"length", LINX_FIELD_TYPE_INT64},
		},
	},
	[LINX_EVENT_TYPE_FTRUNCATE_E] = {
		"ftruncate", 0,
		{}
	},
	[LINX_EVENT_TYPE_FTRUNCATE_X] = {
		"ftruncate", 2,
		{
			{"fd", LINX_FIELD_TYPE_UINT32},
			{"length", LINX_FIELD_TYPE_INT64},
		},
	},
	[LINX_EVENT_TYPE_GETDENTS_E] = {
		"getdents", 0,
		{}
	},
	[LINX_EVENT_TYPE_GETDENTS_X] = {
		"getdents", 3,
		{
			{"fd", LINX_FIELD_TYPE_UINT32},
			{"dirent", LINX_FIELD_TYPE_UNKNOWN},
			{"count", LINX_FIELD_TYPE_UINT32},
		},
	},
	[LINX_EVENT_TYPE_GETCWD_E] = {
		"getcwd", 0,
		{}
	},
	[LINX_EVENT_TYPE_GETCWD_X] = {
		"getcwd", 2,
		{
			{"buf", LINX_FIELD_TYPE_CHARBUF},
			{"size", LINX_FIELD_TYPE_UINT64},
		},
	},
	[LINX_EVENT_TYPE_CHDIR_E] = {
		"chdir", 0,
		{}
	},
	[LINX_EVENT_TYPE_CHDIR_X] = {
		"chdir", 1,
		{
			{"filename", LINX_FIELD_TYPE_CHARBUF},
		},
	},
	[LINX_EVENT_TYPE_FCHDIR_E] = {
		"fchdir", 0,
		{}
	},
	[LINX_EVENT_TYPE_FCHDIR_X] = {
		"fchdir", 1,
		{
			{"fd", LINX_FIELD_TYPE_UINT32},
		},
	},
	[LINX_EVENT_TYPE_RENAME_E] = {
		"rename", 0,
		{}
	},
	[LINX_EVENT_TYPE_RENAME_X] = {
		"rename", 2,
		{
			{"oldname", LINX_FIELD_TYPE_CHARBUF},
			{"newname", LINX_FIELD_TYPE_CHARBUF},
		},
	},
	[LINX_EVENT_TYPE_MKDIR_E] = {
		"mkdir", 0,
		{}
	},
	[LINX_EVENT_TYPE_MKDIR_X] = {
		"mkdir", 2,
		{
			{"pathname", LINX_FIELD_TYPE_CHARBUF},
			{"mode", LINX_FIELD_TYPE_UINT16},
		},
	},
	[LINX_EVENT_TYPE_RMDIR_E] = {
		"rmdir", 0,
		{}
	},
	[LINX_EVENT_TYPE_RMDIR_X] = {
		"rmdir", 1,
		{
			{"pathname", LINX_FIELD_TYPE_CHARBUF},
		},
	},
	[LINX_EVENT_TYPE_CREAT_E] = {
		"creat", 0,
		{}
	},
	[LINX_EVENT_TYPE_CREAT_X] = {
		"creat", 2,
		{
			{"pathname", LINX_FIELD_TYPE_CHARBUF},
			{"mode", LINX_FIELD_TYPE_UINT16},
		},
	},
	[LINX_EVENT_TYPE_LINK_E] = {
		"link", 0,
		{}
	},
	[LINX_EVENT_TYPE_LINK_X] = {
		"link", 2,
		{
			{"oldname", LINX_FIELD_TYPE_CHARBUF},
			{"newname", LINX_FIELD_TYPE_CHARBUF},
		},
	},
	[LINX_EVENT_TYPE_UNLINK_E] = {
		"unlink", 0,
		{}
	},
	[LINX_EVENT_TYPE_UNLINK_X] = {
		"unlink", 1,
		{
			{"pathname", LINX_FIELD_TYPE_CHARBUF},
		},
	},
	[LINX_EVENT_TYPE_SYMLINK_E] = {
		"symlink", 0,
		{}
	},
	[LINX_EVENT_TYPE_SYMLINK_X] = {
		"symlink", 2,
		{
			{"oldname", LINX_FIELD_TYPE_CHARBUF},
			{"newname", LINX_FIELD_TYPE_CHARBUF},
		},
	},
	[LINX_EVENT_TYPE_READLINK_E] = {
		"readlink", 0,
		{}
	},
	[LINX_EVENT_TYPE_READLINK_X] = {
		"readlink", 3,
		{
			{"path", LINX_FIELD_TYPE_CHARBUF},
			{"buf", LINX_FIELD_TYPE_CHARBUF},
			{"bufsiz", LINX_FIELD_TYPE_INT32},
		},
	},
	[LINX_EVENT_TYPE_CHMOD_E] = {
		"chmod", 0,
		{}
	},
	[LINX_EVENT_TYPE_CHMOD_X] = {
		"chmod", 2,
		{
			{"filename", LINX_FIELD_TYPE_CHARBUF},
			{"mode", LINX_FIELD_TYPE_UINT16},
		},
	},
	[LINX_EVENT_TYPE_FCHMOD_E] = {
		"fchmod", 0,
		{}
	},
	[LINX_EVENT_TYPE_FCHMOD_X] = {
		"fchmod", 2,
		{
			{"fd", LINX_FIELD_TYPE_UINT32},
			{"mode", LINX_FIELD_TYPE_UINT16},
		},
	},
	[LINX_EVENT_TYPE_CHOWN_E] = {
		"chown", 0,
		{}
	},
	[LINX_EVENT_TYPE_CHOWN_X] = {
		"chown", 3,
		{
			{"filename", LINX_FIELD_TYPE_CHARBUF},
			{"user", LINX_FIELD_TYPE_UINT32},
			{"group", LINX_FIELD_TYPE_UINT32},
		},
	},
	[LINX_EVENT_TYPE_FCHOWN_E] = {
		"fchown", 0,
		{}
	},
	[LINX_EVENT_TYPE_FCHOWN_X] = {
		"fchown", 3,
		{
			{"fd", LINX_FIELD_TYPE_UINT32},
			{"user", LINX_FIELD_TYPE_UINT32},
			{"group", LINX_FIELD_TYPE_UINT32},
		},
	},
	[LINX_EVENT_TYPE_LCHOWN_E] = {
		"lchown", 0,
		{}
	},
	[LINX_EVENT_TYPE_LCHOWN_X] = {
		"lchown", 3,
		{
			{"filename", LINX_FIELD_TYPE_CHARBUF},
			{"user", LINX_FIELD_TYPE_UINT32},
			{"group", LINX_FIELD_TYPE_UINT32},
		},
	},
	[LINX_EVENT_TYPE_UMASK_E] = {
		"umask", 0,
		{}
	},
	[LINX_EVENT_TYPE_UMASK_X] = {
		"umask", 1,
		{
			{"mask", LINX_FIELD_TYPE_INT32},
		},
	},
	[LINX_EVENT_TYPE_GETTIMEOFDAY_E] = {
		"gettimeofday", 0,
		{}
	},
	[LINX_EVENT_TYPE_GETTIMEOFDAY_X] = {
		"gettimeofday", 2,
		{
			{"tv", LINX_FIELD_TYPE_UNKNOWN},
			{"tz", LINX_FIELD_TYPE_UNKNOWN},
		},
	},
	[LINX_EVENT_TYPE_GETRLIMIT_E] = {
		"getrlimit", 0,
		{}
	},
	[LINX_EVENT_TYPE_GETRLIMIT_X] = {
		"getrlimit", 2,
		{
			{"resource", LINX_FIELD_TYPE_UINT32},
			{"rlim", LINX_FIELD_TYPE_UNKNOWN},
		},
	},
	[LINX_EVENT_TYPE_GETRUSAGE_E] = {
		"getrusage", 0,
		{}
	},
	[LINX_EVENT_TYPE_GETRUSAGE_X] = {
		"getrusage", 2,
		{
			{"who", LINX_FIELD_TYPE_INT32},
			{"ru", LINX_FIELD_TYPE_UNKNOWN},
		},
	},
	[LINX_EVENT_TYPE_SYSINFO_E] = {
		"sysinfo", 0,
		{}
	},
	[LINX_EVENT_TYPE_SYSINFO_X] = {
		"sysinfo", 1,
		{
			{"info", LINX_FIELD_TYPE_UNKNOWN},
		},
	},
	[LINX_EVENT_TYPE_TIMES_E] = {
		"times", 0,
		{}
	},
	[LINX_EVENT_TYPE_TIMES_X] = {
		"times", 1,
		{
			{"tbuf", LINX_FIELD_TYPE_UNKNOWN},
		},
	},
	[LINX_EVENT_TYPE_PTRACE_E] = {
		"ptrace", 0,
		{}
	},
	[LINX_EVENT_TYPE_PTRACE_X] = {
		"ptrace", 4,
		{
			{"request", LINX_FIELD_TYPE_INT64},
			{"pid", LINX_FIELD_TYPE_INT64},
			{"addr", LINX_FIELD_TYPE_UINT64},
			{"data", LINX_FIELD_TYPE_UINT64},
		},
	},
	[LINX_EVENT_TYPE_GETUID_E] = {
		"getuid", 0,
		{}
	},
	[LINX_EVENT_TYPE_GETUID_X] = {
		"getuid", 0,
		{
		},
	},
	[LINX_EVENT_TYPE_SYSLOG_E] = {
		"syslog", 0,
		{}
	},
	[LINX_EVENT_TYPE_SYSLOG_X] = {
		"syslog", 3,
		{
			{"type", LINX_FIELD_TYPE_INT32},
			{"buf", LINX_FIELD_TYPE_CHARBUF},
			{"len", LINX_FIELD_TYPE_INT32},
		},
	},
	[LINX_EVENT_TYPE_GETGID_E] = {
		"getgid", 0,
		{}
	},
	[LINX_EVENT_TYPE_GETGID_X] = {
		"getgid", 0,
		{
		},
	},
	[LINX_EVENT_TYPE_SETUID_E] = {
		"setuid", 0,
		{}
	},
	[LINX_EVENT_TYPE_SETUID_X] = {
		"setuid", 1,
		{
			{"uid", LINX_FIELD_TYPE_UINT32},
		},
	},
	[LINX_EVENT_TYPE_SETGID_E] = {
		"setgid", 0,
		{}
	},
	[LINX_EVENT_TYPE_SETGID_X] = {
		"setgid", 1,
		{
			{"gid", LINX_FIELD_TYPE_UINT32},
		},
	},
	[LINX_EVENT_TYPE_GETEUID_E] = {
		"geteuid", 0,
		{}
	},
	[LINX_EVENT_TYPE_GETEUID_X] = {
		"geteuid", 0,
		{
		},
	},
	[LINX_EVENT_TYPE_GETEGID_E] = {
		"getegid", 0,
		{}
	},
	[LINX_EVENT_TYPE_GETEGID_X] = {
		"getegid", 0,
		{
		},
	},
	[LINX_EVENT_TYPE_SETPGID_E] = {
		"setpgid", 0,
		{}
	},
	[LINX_EVENT_TYPE_SETPGID_X] = {
		"setpgid", 2,
		{
			{"pid", LINX_FIELD_TYPE_INT32},
			{"pgid", LINX_FIELD_TYPE_INT32},
		},
	},
	[LINX_EVENT_TYPE_GETPPID_E] = {
		"getppid", 0,
		{}
	},
	[LINX_EVENT_TYPE_GETPPID_X] = {
		"getppid", 0,
		{
		},
	},
	[LINX_EVENT_TYPE_GETPGRP_E] = {
		"getpgrp", 0,
		{}
	},
	[LINX_EVENT_TYPE_GETPGRP_X] = {
		"getpgrp", 0,
		{
		},
	},
	[LINX_EVENT_TYPE_SETSID_E] = {
		"setsid", 0,
		{}
	},
	[LINX_EVENT_TYPE_SETSID_X] = {
		"setsid", 0,
		{
		},
	},
	[LINX_EVENT_TYPE_SETREUID_E] = {
		"setreuid", 0,
		{}
	},
	[LINX_EVENT_TYPE_SETREUID_X] = {
		"setreuid", 2,
		{
			{"ruid", LINX_FIELD_TYPE_UINT32},
			{"euid", LINX_FIELD_TYPE_UINT32},
		},
	},
	[LINX_EVENT_TYPE_SETREGID_E] = {
		"setregid", 0,
		{}
	},
	[LINX_EVENT_TYPE_SETREGID_X] = {
		"setregid", 2,
		{
			{"rgid", LINX_FIELD_TYPE_UINT32},
			{"egid", LINX_FIELD_TYPE_UINT32},
		},
	},
	[LINX_EVENT_TYPE_GETGROUPS_E] = {
		"getgroups", 0,
		{}
	},
	[LINX_EVENT_TYPE_GETGROUPS_X] = {
		"getgroups", 2,
		{
			{"gidsetsize", LINX_FIELD_TYPE_INT32},
			{"grouplist", LINX_FIELD_TYPE_UNKNOWN},
		},
	},
	[LINX_EVENT_TYPE_SETGROUPS_E] = {
		"setgroups", 0,
		{}
	},
	[LINX_EVENT_TYPE_SETGROUPS_X] = {
		"setgroups", 2,
		{
			{"gidsetsize", LINX_FIELD_TYPE_INT32},
			{"grouplist", LINX_FIELD_TYPE_UNKNOWN},
		},
	},
	[LINX_EVENT_TYPE_SETRESUID_E] = {
		"setresuid", 0,
		{}
	},
	[LINX_EVENT_TYPE_SETRESUID_X] = {
		"setresuid", 3,
		{
			{"ruid", LINX_FIELD_TYPE_UINT32},
			{"euid", LINX_FIELD_TYPE_UINT32},
			{"suid", LINX_FIELD_TYPE_UINT32},
		},
	},
	[LINX_EVENT_TYPE_GETRESUID_E] = {
		"getresuid", 0,
		{}
	},
	[LINX_EVENT_TYPE_GETRESUID_X] = {
		"getresuid", 3,
		{
			{"ruidp", LINX_FIELD_TYPE_UNKNOWN},
			{"euidp", LINX_FIELD_TYPE_UNKNOWN},
			{"suidp", LINX_FIELD_TYPE_UNKNOWN},
		},
	},
	[LINX_EVENT_TYPE_SETRESGID_E] = {
		"setresgid", 0,
		{}
	},
	[LINX_EVENT_TYPE_SETRESGID_X] = {
		"setresgid", 3,
		{
			{"rgid", LINX_FIELD_TYPE_UINT32},
			{"egid", LINX_FIELD_TYPE_UINT32},
			{"sgid", LINX_FIELD_TYPE_UINT32},
		},
	},
	[LINX_EVENT_TYPE_GETRESGID_E] = {
		"getresgid", 0,
		{}
	},
	[LINX_EVENT_TYPE_GETRESGID_X] = {
		"getresgid", 3,
		{
			{"rgidp", LINX_FIELD_TYPE_UNKNOWN},
			{"egidp", LINX_FIELD_TYPE_UNKNOWN},
			{"sgidp", LINX_FIELD_TYPE_UNKNOWN},
		},
	},
	[LINX_EVENT_TYPE_GETPGID_E] = {
		"getpgid", 0,
		{}
	},
	[LINX_EVENT_TYPE_GETPGID_X] = {
		"getpgid", 1,
		{
			{"pid", LINX_FIELD_TYPE_INT32},
		},
	},
	[LINX_EVENT_TYPE_SETFSUID_E] = {
		"setfsuid", 0,
		{}
	},
	[LINX_EVENT_TYPE_SETFSUID_X] = {
		"setfsuid", 1,
		{
			{"uid", LINX_FIELD_TYPE_UINT32},
		},
	},
	[LINX_EVENT_TYPE_SETFSGID_E] = {
		"setfsgid", 0,
		{}
	},
	[LINX_EVENT_TYPE_SETFSGID_X] = {
		"setfsgid", 1,
		{
			{"gid", LINX_FIELD_TYPE_UINT32},
		},
	},
	[LINX_EVENT_TYPE_GETSID_E] = {
		"getsid", 0,
		{}
	},
	[LINX_EVENT_TYPE_GETSID_X] = {
		"getsid", 1,
		{
			{"pid", LINX_FIELD_TYPE_INT32},
		},
	},
	[LINX_EVENT_TYPE_CAPGET_E] = {
		"capget", 0,
		{}
	},
	[LINX_EVENT_TYPE_CAPGET_X] = {
		"capget", 2,
		{
			{"header", LINX_FIELD_TYPE_UNKNOWN},
			{"dataptr", LINX_FIELD_TYPE_UNKNOWN},
		},
	},
	[LINX_EVENT_TYPE_CAPSET_E] = {
		"capset", 0,
		{}
	},
	[LINX_EVENT_TYPE_CAPSET_X] = {
		"capset", 2,
		{
			{"header", LINX_FIELD_TYPE_UNKNOWN},
			{"data", LINX_FIELD_TYPE_UNKNOWN},
		},
	},
	[LINX_EVENT_TYPE_RT_SIGPENDING_E] = {
		"rt_sigpending", 0,
		{}
	},
	[LINX_EVENT_TYPE_RT_SIGPENDING_X] = {
		"rt_sigpending", 2,
		{
			{"uset", LINX_FIELD_TYPE_UNKNOWN},
			{"sigsetsize", LINX_FIELD_TYPE_UINT64},
		},
	},
	[LINX_EVENT_TYPE_RT_SIGTIMEDWAIT_E] = {
		"rt_sigtimedwait", 0,
		{}
	},
	[LINX_EVENT_TYPE_RT_SIGTIMEDWAIT_X] = {
		"rt_sigtimedwait", 4,
		{
			{"uthese", LINX_FIELD_TYPE_UNKNOWN},
			{"uinfo", LINX_FIELD_TYPE_UNKNOWN},
			{"uts", LINX_FIELD_TYPE_UNKNOWN},
			{"sigsetsize", LINX_FIELD_TYPE_UINT64},
		},
	},
	[LINX_EVENT_TYPE_RT_SIGQUEUEINFO_E] = {
		"rt_sigqueueinfo", 0,
		{}
	},
	[LINX_EVENT_TYPE_RT_SIGQUEUEINFO_X] = {
		"rt_sigqueueinfo", 3,
		{
			{"pid", LINX_FIELD_TYPE_INT32},
			{"sig", LINX_FIELD_TYPE_INT32},
			{"uinfo", LINX_FIELD_TYPE_UNKNOWN},
		},
	},
	[LINX_EVENT_TYPE_RT_SIGSUSPEND_E] = {
		"rt_sigsuspend", 0,
		{}
	},
	[LINX_EVENT_TYPE_RT_SIGSUSPEND_X] = {
		"rt_sigsuspend", 2,
		{
			{"unewset", LINX_FIELD_TYPE_UNKNOWN},
			{"sigsetsize", LINX_FIELD_TYPE_UINT64},
		},
	},
	[LINX_EVENT_TYPE_SIGALTSTACK_E] = {
		"sigaltstack", 0,
		{}
	},
	[LINX_EVENT_TYPE_SIGALTSTACK_X] = {
		"sigaltstack", 2,
		{
			{"uss", LINX_FIELD_TYPE_UNKNOWN},
			{"uoss", LINX_FIELD_TYPE_UNKNOWN},
		},
	},
	[LINX_EVENT_TYPE_UTIME_E] = {
		"utime", 0,
		{}
	},
	[LINX_EVENT_TYPE_UTIME_X] = {
		"utime", 2,
		{
			{"filename", LINX_FIELD_TYPE_CHARBUF},
			{"times", LINX_FIELD_TYPE_UNKNOWN},
		},
	},
	[LINX_EVENT_TYPE_MKNOD_E] = {
		"mknod", 0,
		{}
	},
	[LINX_EVENT_TYPE_MKNOD_X] = {
		"mknod", 3,
		{
			{"filename", LINX_FIELD_TYPE_CHARBUF},
			{"mode", LINX_FIELD_TYPE_UINT16},
			{"dev", LINX_FIELD_TYPE_UINT32},
		},
	},
	[LINX_EVENT_TYPE_USELIB_E] = {
		"uselib", 0,
		{}
	},
	[LINX_EVENT_TYPE_USELIB_X] = {
		"uselib", 1,
		{
			{"library", LINX_FIELD_TYPE_CHARBUF},
		},
	},
	[LINX_EVENT_TYPE_PERSONALITY_E] = {
		"personality", 0,
		{}
	},
	[LINX_EVENT_TYPE_PERSONALITY_X] = {
		"personality", 1,
		{
			{"personality", LINX_FIELD_TYPE_UINT32},
		},
	},
	[LINX_EVENT_TYPE_USTAT_E] = {
		"ustat", 0,
		{}
	},
	[LINX_EVENT_TYPE_USTAT_X] = {
		"ustat", 2,
		{
			{"dev", LINX_FIELD_TYPE_UINT32},
			{"ubuf", LINX_FIELD_TYPE_UNKNOWN},
		},
	},
	[LINX_EVENT_TYPE_STATFS_E] = {
		"statfs", 0,
		{}
	},
	[LINX_EVENT_TYPE_STATFS_X] = {
		"statfs", 2,
		{
			{"pathname", LINX_FIELD_TYPE_CHARBUF},
			{"buf", LINX_FIELD_TYPE_UNKNOWN},
		},
	},
	[LINX_EVENT_TYPE_FSTATFS_E] = {
		"fstatfs", 0,
		{}
	},
	[LINX_EVENT_TYPE_FSTATFS_X] = {
		"fstatfs", 2,
		{
			{"fd", LINX_FIELD_TYPE_UINT32},
			{"buf", LINX_FIELD_TYPE_UNKNOWN},
		},
	},
	[LINX_EVENT_TYPE_SYSFS_E] = {
		"sysfs", 0,
		{}
	},
	[LINX_EVENT_TYPE_SYSFS_X] = {
		"sysfs", 3,
		{
			{"option", LINX_FIELD_TYPE_INT32},
			{"arg1", LINX_FIELD_TYPE_UINT64},
			{"arg2", LINX_FIELD_TYPE_UINT64},
		},
	},
	[LINX_EVENT_TYPE_GETPRIORITY_E] = {
		"getpriority", 0,
		{}
	},
	[LINX_EVENT_TYPE_GETPRIORITY_X] = {
		"getpriority", 2,
		{
			{"which", LINX_FIELD_TYPE_INT32},
			{"who", LINX_FIELD_TYPE_INT32},
		},
	},
	[LINX_EVENT_TYPE_SETPRIORITY_E] = {
		"setpriority", 0,
		{}
	},
	[LINX_EVENT_TYPE_SETPRIORITY_X] = {
		"setpriority", 3,
		{
			{"which", LINX_FIELD_TYPE_INT32},
			{"who", LINX_FIELD_TYPE_INT32},
			{"niceval", LINX_FIELD_TYPE_INT32},
		},
	},
	[LINX_EVENT_TYPE_SCHED_SETPARAM_E] = {
		"sched_setparam", 0,
		{}
	},
	[LINX_EVENT_TYPE_SCHED_SETPARAM_X] = {
		"sched_setparam", 2,
		{
			{"pid", LINX_FIELD_TYPE_INT32},
			{"param", LINX_FIELD_TYPE_UNKNOWN},
		},
	},
	[LINX_EVENT_TYPE_SCHED_GETPARAM_E] = {
		"sched_getparam", 0,
		{}
	},
	[LINX_EVENT_TYPE_SCHED_GETPARAM_X] = {
		"sched_getparam", 2,
		{
			{"pid", LINX_FIELD_TYPE_INT32},
			{"param", LINX_FIELD_TYPE_UNKNOWN},
		},
	},
	[LINX_EVENT_TYPE_SCHED_SETSCHEDULER_E] = {
		"sched_setscheduler", 0,
		{}
	},
	[LINX_EVENT_TYPE_SCHED_SETSCHEDULER_X] = {
		"sched_setscheduler", 3,
		{
			{"pid", LINX_FIELD_TYPE_INT32},
			{"policy", LINX_FIELD_TYPE_INT32},
			{"param", LINX_FIELD_TYPE_UNKNOWN},
		},
	},
	[LINX_EVENT_TYPE_SCHED_GETSCHEDULER_E] = {
		"sched_getscheduler", 0,
		{}
	},
	[LINX_EVENT_TYPE_SCHED_GETSCHEDULER_X] = {
		"sched_getscheduler", 1,
		{
			{"pid", LINX_FIELD_TYPE_INT32},
		},
	},
	[LINX_EVENT_TYPE_SCHED_GET_PRIORITY_MAX_E] = {
		"sched_get_priority_max", 0,
		{}
	},
	[LINX_EVENT_TYPE_SCHED_GET_PRIORITY_MAX_X] = {
		"sched_get_priority_max", 1,
		{
			{"policy", LINX_FIELD_TYPE_INT32},
		},
	},
	[LINX_EVENT_TYPE_SCHED_GET_PRIORITY_MIN_E] = {
		"sched_get_priority_min", 0,
		{}
	},
	[LINX_EVENT_TYPE_SCHED_GET_PRIORITY_MIN_X] = {
		"sched_get_priority_min", 1,
		{
			{"policy", LINX_FIELD_TYPE_INT32},
		},
	},
	[LINX_EVENT_TYPE_SCHED_RR_GET_INTERVAL_E] = {
		"sched_rr_get_interval", 0,
		{}
	},
	[LINX_EVENT_TYPE_SCHED_RR_GET_INTERVAL_X] = {
		"sched_rr_get_interval", 2,
		{
			{"pid", LINX_FIELD_TYPE_INT32},
			{"interval", LINX_FIELD_TYPE_UNKNOWN},
		},
	},
	[LINX_EVENT_TYPE_MLOCK_E] = {
		"mlock", 0,
		{}
	},
	[LINX_EVENT_TYPE_MLOCK_X] = {
		"mlock", 2,
		{
			{"start", LINX_FIELD_TYPE_UINT64},
			{"len", LINX_FIELD_TYPE_UINT64},
		},
	},
	[LINX_EVENT_TYPE_MUNLOCK_E] = {
		"munlock", 0,
		{}
	},
	[LINX_EVENT_TYPE_MUNLOCK_X] = {
		"munlock", 2,
		{
			{"start", LINX_FIELD_TYPE_UINT64},
			{"len", LINX_FIELD_TYPE_UINT64},
		},
	},
	[LINX_EVENT_TYPE_MLOCKALL_E] = {
		"mlockall", 0,
		{}
	},
	[LINX_EVENT_TYPE_MLOCKALL_X] = {
		"mlockall", 1,
		{
			{"flags", LINX_FIELD_TYPE_INT32},
		},
	},
	[LINX_EVENT_TYPE_MUNLOCKALL_E] = {
		"munlockall", 0,
		{}
	},
	[LINX_EVENT_TYPE_MUNLOCKALL_X] = {
		"munlockall", 0,
		{
		},
	},
	[LINX_EVENT_TYPE_VHANGUP_E] = {
		"vhangup", 0,
		{}
	},
	[LINX_EVENT_TYPE_VHANGUP_X] = {
		"vhangup", 0,
		{
		},
	},
	[LINX_EVENT_TYPE_MODIFY_LDT_E] = {
		"modify_ldt", 0,
		{}
	},
	[LINX_EVENT_TYPE_MODIFY_LDT_X] = {
		"modify_ldt", 3,
		{
			{"func", LINX_FIELD_TYPE_INT32},
			{"ptr", LINX_FIELD_TYPE_UNKNOWN},
			{"bytecount", LINX_FIELD_TYPE_UINT64},
		},
	},
	[LINX_EVENT_TYPE_PIVOT_ROOT_E] = {
		"pivot_root", 0,
		{}
	},
	[LINX_EVENT_TYPE_PIVOT_ROOT_X] = {
		"pivot_root", 2,
		{
			{"new_root", LINX_FIELD_TYPE_CHARBUF},
			{"put_old", LINX_FIELD_TYPE_CHARBUF},
		},
	},
	[LINX_EVENT_TYPE__SYSCTL_E] = {
		"_sysctl", 0,
		{}
	},
	[LINX_EVENT_TYPE__SYSCTL_X] = {
		"_sysctl", 1,
		{
			{"args", LINX_FIELD_TYPE_UNKNOWN},
		},
	},
	[LINX_EVENT_TYPE_PRCTL_E] = {
		"prctl", 0,
		{}
	},
	[LINX_EVENT_TYPE_PRCTL_X] = {
		"prctl", 5,
		{
			{"option", LINX_FIELD_TYPE_INT32},
			{"arg2", LINX_FIELD_TYPE_UINT64},
			{"arg3", LINX_FIELD_TYPE_UINT64},
			{"arg4", LINX_FIELD_TYPE_UINT64},
			{"arg5", LINX_FIELD_TYPE_UINT64},
		},
	},
	[LINX_EVENT_TYPE_ARCH_PRCTL_E] = {
		"arch_prctl", 0,
		{}
	},
	[LINX_EVENT_TYPE_ARCH_PRCTL_X] = {
		"arch_prctl", 2,
		{
			{"option", LINX_FIELD_TYPE_INT32},
			{"arg2", LINX_FIELD_TYPE_UINT64},
		},
	},
	[LINX_EVENT_TYPE_ADJTIMEX_E] = {
		"adjtimex", 0,
		{}
	},
	[LINX_EVENT_TYPE_ADJTIMEX_X] = {
		"adjtimex", 1,
		{
			{"txc_p", LINX_FIELD_TYPE_UNKNOWN},
		},
	},
	[LINX_EVENT_TYPE_SETRLIMIT_E] = {
		"setrlimit", 0,
		{}
	},
	[LINX_EVENT_TYPE_SETRLIMIT_X] = {
		"setrlimit", 2,
		{
			{"resource", LINX_FIELD_TYPE_UINT32},
			{"rlim", LINX_FIELD_TYPE_UNKNOWN},
		},
	},
	[LINX_EVENT_TYPE_CHROOT_E] = {
		"chroot", 0,
		{}
	},
	[LINX_EVENT_TYPE_CHROOT_X] = {
		"chroot", 1,
		{
			{"filename", LINX_FIELD_TYPE_CHARBUF},
		},
	},
	[LINX_EVENT_TYPE_SYNC_E] = {
		"sync", 0,
		{}
	},
	[LINX_EVENT_TYPE_SYNC_X] = {
		"sync", 0,
		{
		},
	},
	[LINX_EVENT_TYPE_ACCT_E] = {
		"acct", 0,
		{}
	},
	[LINX_EVENT_TYPE_ACCT_X] = {
		"acct", 1,
		{
			{"name", LINX_FIELD_TYPE_CHARBUF},
		},
	},
	[LINX_EVENT_TYPE_SETTIMEOFDAY_E] = {
		"settimeofday", 0,
		{}
	},
	[LINX_EVENT_TYPE_SETTIMEOFDAY_X] = {
		"settimeofday", 2,
		{
			{"tv", LINX_FIELD_TYPE_UNKNOWN},
			{"tz", LINX_FIELD_TYPE_UNKNOWN},
		},
	},
	[LINX_EVENT_TYPE_MOUNT_E] = {
		"mount", 0,
		{}
	},
	[LINX_EVENT_TYPE_MOUNT_X] = {
		"mount", 5,
		{
			{"dev_name", LINX_FIELD_TYPE_CHARBUF},
			{"dir_name", LINX_FIELD_TYPE_CHARBUF},
			{"type", LINX_FIELD_TYPE_CHARBUF},
			{"flags", LINX_FIELD_TYPE_UINT64},
			{"data", LINX_FIELD_TYPE_UNKNOWN},
		},
	},
	[LINX_EVENT_TYPE_UMOUNT2_E] = {
		"umount2", 0,
		{}
	},
	[LINX_EVENT_TYPE_UMOUNT2_X] = {
		"umount2", 2,
		{
			{"target", LINX_FIELD_TYPE_CHARBUF},
			{"flags", LINX_FIELD_TYPE_INT32},
		},
	},
	[LINX_EVENT_TYPE_SWAPON_E] = {
		"swapon", 0,
		{}
	},
	[LINX_EVENT_TYPE_SWAPON_X] = {
		"swapon", 2,
		{
			{"specialfile", LINX_FIELD_TYPE_CHARBUF},
			{"swap_flags", LINX_FIELD_TYPE_INT32},
		},
	},
	[LINX_EVENT_TYPE_SWAPOFF_E] = {
		"swapoff", 0,
		{}
	},
	[LINX_EVENT_TYPE_SWAPOFF_X] = {
		"swapoff", 1,
		{
			{"specialfile", LINX_FIELD_TYPE_CHARBUF},
		},
	},
	[LINX_EVENT_TYPE_REBOOT_E] = {
		"reboot", 0,
		{}
	},
	[LINX_EVENT_TYPE_REBOOT_X] = {
		"reboot", 4,
		{
			{"magic1", LINX_FIELD_TYPE_INT32},
			{"magic2", LINX_FIELD_TYPE_INT32},
			{"cmd", LINX_FIELD_TYPE_UINT32},
			{"arg", LINX_FIELD_TYPE_UNKNOWN},
		},
	},
	[LINX_EVENT_TYPE_SETHOSTNAME_E] = {
		"sethostname", 0,
		{}
	},
	[LINX_EVENT_TYPE_SETHOSTNAME_X] = {
		"sethostname", 2,
		{
			{"name", LINX_FIELD_TYPE_CHARBUF},
			{"len", LINX_FIELD_TYPE_INT32},
		},
	},
	[LINX_EVENT_TYPE_SETDOMAINNAME_E] = {
		"setdomainname", 0,
		{}
	},
	[LINX_EVENT_TYPE_SETDOMAINNAME_X] = {
		"setdomainname", 2,
		{
			{"name", LINX_FIELD_TYPE_CHARBUF},
			{"len", LINX_FIELD_TYPE_INT32},
		},
	},
	[LINX_EVENT_TYPE_IOPL_E] = {
		"iopl", 0,
		{}
	},
	[LINX_EVENT_TYPE_IOPL_X] = {
		"iopl", 1,
		{
			{"level", LINX_FIELD_TYPE_UINT32},
		},
	},
	[LINX_EVENT_TYPE_IOPERM_E] = {
		"ioperm", 0,
		{}
	},
	[LINX_EVENT_TYPE_IOPERM_X] = {
		"ioperm", 3,
		{
			{"from", LINX_FIELD_TYPE_UINT64},
			{"num", LINX_FIELD_TYPE_UINT64},
			{"turn_on", LINX_FIELD_TYPE_INT32},
		},
	},
	[LINX_EVENT_TYPE_CREATE_MODULE_E] = {
		"create_module", 0,
		{}
	},
	[LINX_EVENT_TYPE_CREATE_MODULE_X] = {
		"create_module", 2,
		{
			{"name", LINX_FIELD_TYPE_CHARBUF},
			{"size", LINX_FIELD_TYPE_UINT64},
		},
	},
	[LINX_EVENT_TYPE_INIT_MODULE_E] = {
		"init_module", 0,
		{}
	},
	[LINX_EVENT_TYPE_INIT_MODULE_X] = {
		"init_module", 3,
		{
			{"umod", LINX_FIELD_TYPE_UNKNOWN},
			{"len", LINX_FIELD_TYPE_UINT64},
			{"uargs", LINX_FIELD_TYPE_CHARBUF},
		},
	},
	[LINX_EVENT_TYPE_DELETE_MODULE_E] = {
		"delete_module", 0,
		{}
	},
	[LINX_EVENT_TYPE_DELETE_MODULE_X] = {
		"delete_module", 2,
		{
			{"name_user", LINX_FIELD_TYPE_CHARBUF},
			{"flags", LINX_FIELD_TYPE_UINT32},
		},
	},
	[LINX_EVENT_TYPE_GET_KERNEL_SYMS_E] = {
		"get_kernel_syms", 0,
		{}
	},
	[LINX_EVENT_TYPE_GET_KERNEL_SYMS_X] = {
		"get_kernel_syms", 1,
		{
			{"table", LINX_FIELD_TYPE_UNKNOWN},
		},
	},
	[LINX_EVENT_TYPE_QUERY_MODULE_E] = {
		"query_module", 0,
		{}
	},
	[LINX_EVENT_TYPE_QUERY_MODULE_X] = {
		"query_module", 5,
		{
			{"name", LINX_FIELD_TYPE_CHARBUF},
			{"which", LINX_FIELD_TYPE_INT32},
			{"buf", LINX_FIELD_TYPE_UNKNOWN},
			{"bufsize", LINX_FIELD_TYPE_UINT64},
			{"ret", LINX_FIELD_TYPE_UNKNOWN},
		},
	},
	[LINX_EVENT_TYPE_QUOTACTL_E] = {
		"quotactl", 0,
		{}
	},
	[LINX_EVENT_TYPE_QUOTACTL_X] = {
		"quotactl", 4,
		{
			{"cmd", LINX_FIELD_TYPE_UINT32},
			{"special", LINX_FIELD_TYPE_CHARBUF},
			{"id", LINX_FIELD_TYPE_UINT32},
			{"addr", LINX_FIELD_TYPE_UNKNOWN},
		},
	},
	[LINX_EVENT_TYPE_NFSSERVCTL_E] = {
		"nfsservctl", 0,
		{}
	},
	[LINX_EVENT_TYPE_NFSSERVCTL_X] = {
		"nfsservctl", 3,
		{
			{"cmd", LINX_FIELD_TYPE_INT32},
			{"argp", LINX_FIELD_TYPE_UNKNOWN},
			{"resp", LINX_FIELD_TYPE_UNKNOWN},
		},
	},
	[LINX_EVENT_TYPE_GETTID_E] = {
		"gettid", 0,
		{}
	},
	[LINX_EVENT_TYPE_GETTID_X] = {
		"gettid", 0,
		{
		},
	},
	[LINX_EVENT_TYPE_READAHEAD_E] = {
		"readahead", 0,
		{}
	},
	[LINX_EVENT_TYPE_READAHEAD_X] = {
		"readahead", 3,
		{
			{"fd", LINX_FIELD_TYPE_INT32},
			{"offset", LINX_FIELD_TYPE_INT64},
			{"count", LINX_FIELD_TYPE_UINT64},
		},
	},
	[LINX_EVENT_TYPE_SETXATTR_E] = {
		"setxattr", 0,
		{}
	},
	[LINX_EVENT_TYPE_SETXATTR_X] = {
		"setxattr", 5,
		{
			{"pathname", LINX_FIELD_TYPE_CHARBUF},
			{"name", LINX_FIELD_TYPE_CHARBUF},
			{"value", LINX_FIELD_TYPE_UNKNOWN},
			{"size", LINX_FIELD_TYPE_UINT64},
			{"flags", LINX_FIELD_TYPE_INT32},
		},
	},
	[LINX_EVENT_TYPE_LSETXATTR_E] = {
		"lsetxattr", 0,
		{}
	},
	[LINX_EVENT_TYPE_LSETXATTR_X] = {
		"lsetxattr", 5,
		{
			{"pathname", LINX_FIELD_TYPE_CHARBUF},
			{"name", LINX_FIELD_TYPE_CHARBUF},
			{"value", LINX_FIELD_TYPE_UNKNOWN},
			{"size", LINX_FIELD_TYPE_UINT64},
			{"flags", LINX_FIELD_TYPE_INT32},
		},
	},
	[LINX_EVENT_TYPE_FSETXATTR_E] = {
		"fsetxattr", 0,
		{}
	},
	[LINX_EVENT_TYPE_FSETXATTR_X] = {
		"fsetxattr", 5,
		{
			{"fd", LINX_FIELD_TYPE_INT32},
			{"name", LINX_FIELD_TYPE_CHARBUF},
			{"value", LINX_FIELD_TYPE_UNKNOWN},
			{"size", LINX_FIELD_TYPE_UINT64},
			{"flags", LINX_FIELD_TYPE_INT32},
		},
	},
	[LINX_EVENT_TYPE_GETXATTR_E] = {
		"getxattr", 0,
		{}
	},
	[LINX_EVENT_TYPE_GETXATTR_X] = {
		"getxattr", 4,
		{
			{"pathname", LINX_FIELD_TYPE_CHARBUF},
			{"name", LINX_FIELD_TYPE_CHARBUF},
			{"value", LINX_FIELD_TYPE_UNKNOWN},
			{"size", LINX_FIELD_TYPE_UINT64},
		},
	},
	[LINX_EVENT_TYPE_LGETXATTR_E] = {
		"lgetxattr", 0,
		{}
	},
	[LINX_EVENT_TYPE_LGETXATTR_X] = {
		"lgetxattr", 4,
		{
			{"pathname", LINX_FIELD_TYPE_CHARBUF},
			{"name", LINX_FIELD_TYPE_CHARBUF},
			{"value", LINX_FIELD_TYPE_UNKNOWN},
			{"size", LINX_FIELD_TYPE_UINT64},
		},
	},
	[LINX_EVENT_TYPE_FGETXATTR_E] = {
		"fgetxattr", 0,
		{}
	},
	[LINX_EVENT_TYPE_FGETXATTR_X] = {
		"fgetxattr", 4,
		{
			{"fd", LINX_FIELD_TYPE_INT32},
			{"name", LINX_FIELD_TYPE_CHARBUF},
			{"value", LINX_FIELD_TYPE_UNKNOWN},
			{"size", LINX_FIELD_TYPE_UINT64},
		},
	},
	[LINX_EVENT_TYPE_LISTXATTR_E] = {
		"listxattr", 0,
		{}
	},
	[LINX_EVENT_TYPE_LISTXATTR_X] = {
		"listxattr", 3,
		{
			{"pathname", LINX_FIELD_TYPE_CHARBUF},
			{"list", LINX_FIELD_TYPE_CHARBUF},
			{"size", LINX_FIELD_TYPE_UINT64},
		},
	},
	[LINX_EVENT_TYPE_LLISTXATTR_E] = {
		"llistxattr", 0,
		{}
	},
	[LINX_EVENT_TYPE_LLISTXATTR_X] = {
		"llistxattr", 3,
		{
			{"pathname", LINX_FIELD_TYPE_CHARBUF},
			{"list", LINX_FIELD_TYPE_CHARBUF},
			{"size", LINX_FIELD_TYPE_UINT64},
		},
	},
	[LINX_EVENT_TYPE_FLISTXATTR_E] = {
		"flistxattr", 0,
		{}
	},
	[LINX_EVENT_TYPE_FLISTXATTR_X] = {
		"flistxattr", 3,
		{
			{"fd", LINX_FIELD_TYPE_INT32},
			{"list", LINX_FIELD_TYPE_CHARBUF},
			{"size", LINX_FIELD_TYPE_UINT64},
		},
	},
	[LINX_EVENT_TYPE_REMOVEXATTR_E] = {
		"removexattr", 0,
		{}
	},
	[LINX_EVENT_TYPE_REMOVEXATTR_X] = {
		"removexattr", 2,
		{
			{"pathname", LINX_FIELD_TYPE_CHARBUF},
			{"name", LINX_FIELD_TYPE_CHARBUF},
		},
	},
	[LINX_EVENT_TYPE_LREMOVEXATTR_E] = {
		"lremovexattr", 0,
		{}
	},
	[LINX_EVENT_TYPE_LREMOVEXATTR_X] = {
		"lremovexattr", 2,
		{
			{"pathname", LINX_FIELD_TYPE_CHARBUF},
			{"name", LINX_FIELD_TYPE_CHARBUF},
		},
	},
	[LINX_EVENT_TYPE_FREMOVEXATTR_E] = {
		"fremovexattr", 0,
		{}
	},
	[LINX_EVENT_TYPE_FREMOVEXATTR_X] = {
		"fremovexattr", 2,
		{
			{"fd", LINX_FIELD_TYPE_INT32},
			{"name", LINX_FIELD_TYPE_CHARBUF},
		},
	},
	[LINX_EVENT_TYPE_TKILL_E] = {
		"tkill", 0,
		{}
	},
	[LINX_EVENT_TYPE_TKILL_X] = {
		"tkill", 2,
		{
			{"pid", LINX_FIELD_TYPE_INT32},
			{"sig", LINX_FIELD_TYPE_INT32},
		},
	},
	[LINX_EVENT_TYPE_TIME_E] = {
		"time", 0,
		{}
	},
	[LINX_EVENT_TYPE_TIME_X] = {
		"time", 1,
		{
			{"tloc", LINX_FIELD_TYPE_UNKNOWN},
		},
	},
	[LINX_EVENT_TYPE_FUTEX_E] = {
		"futex", 0,
		{}
	},
	[LINX_EVENT_TYPE_FUTEX_X] = {
		"futex", 6,
		{
			{"uaddr", LINX_FIELD_TYPE_UNKNOWN},
			{"op", LINX_FIELD_TYPE_INT32},
			{"val", LINX_FIELD_TYPE_UINT32},
			{"utime", LINX_FIELD_TYPE_UNKNOWN},
			{"uaddr2", LINX_FIELD_TYPE_UNKNOWN},
			{"val3", LINX_FIELD_TYPE_UINT32},
		},
	},
	[LINX_EVENT_TYPE_SCHED_SETAFFINITY_E] = {
		"sched_setaffinity", 0,
		{}
	},
	[LINX_EVENT_TYPE_SCHED_SETAFFINITY_X] = {
		"sched_setaffinity", 3,
		{
			{"pid", LINX_FIELD_TYPE_INT32},
			{"len", LINX_FIELD_TYPE_UINT32},
			{"user_mask_ptr", LINX_FIELD_TYPE_UNKNOWN},
		},
	},
	[LINX_EVENT_TYPE_SCHED_GETAFFINITY_E] = {
		"sched_getaffinity", 0,
		{}
	},
	[LINX_EVENT_TYPE_SCHED_GETAFFINITY_X] = {
		"sched_getaffinity", 3,
		{
			{"pid", LINX_FIELD_TYPE_INT32},
			{"len", LINX_FIELD_TYPE_UINT32},
			{"user_mask_ptr", LINX_FIELD_TYPE_UNKNOWN},
		},
	},
	[LINX_EVENT_TYPE_SET_THREAD_AREA_E] = {
		"set_thread_area", 0,
		{}
	},
	[LINX_EVENT_TYPE_SET_THREAD_AREA_X] = {
		"set_thread_area", 1,
		{
			{"u_info", LINX_FIELD_TYPE_UNKNOWN},
		},
	},
	[LINX_EVENT_TYPE_IO_SETUP_E] = {
		"io_setup", 0,
		{}
	},
	[LINX_EVENT_TYPE_IO_SETUP_X] = {
		"io_setup", 2,
		{
			{"nr_events", LINX_FIELD_TYPE_UINT32},
			{"ctxp", LINX_FIELD_TYPE_UNKNOWN},
		},
	},
	[LINX_EVENT_TYPE_IO_DESTROY_E] = {
		"io_destroy", 0,
		{}
	},
	[LINX_EVENT_TYPE_IO_DESTROY_X] = {
		"io_destroy", 1,
		{
			{"ctx", LINX_FIELD_TYPE_UINT64},
		},
	},
	[LINX_EVENT_TYPE_IO_GETEVENTS_E] = {
		"io_getevents", 0,
		{}
	},
	[LINX_EVENT_TYPE_IO_GETEVENTS_X] = {
		"io_getevents", 5,
		{
			{"ctx_id", LINX_FIELD_TYPE_UINT64},
			{"min_nr", LINX_FIELD_TYPE_INT64},
			{"nr", LINX_FIELD_TYPE_INT64},
			{"events", LINX_FIELD_TYPE_UNKNOWN},
			{"timeout", LINX_FIELD_TYPE_UNKNOWN},
		},
	},
	[LINX_EVENT_TYPE_IO_SUBMIT_E] = {
		"io_submit", 0,
		{}
	},
	[LINX_EVENT_TYPE_IO_SUBMIT_X] = {
		"io_submit", 3,
		{
			{"ctx_id", LINX_FIELD_TYPE_UINT64},
			{"nr", LINX_FIELD_TYPE_INT64},
			{"iocbpp", LINX_FIELD_TYPE_UNKNOWN},
		},
	},
	[LINX_EVENT_TYPE_IO_CANCEL_E] = {
		"io_cancel", 0,
		{}
	},
	[LINX_EVENT_TYPE_IO_CANCEL_X] = {
		"io_cancel", 3,
		{
			{"ctx_id", LINX_FIELD_TYPE_UINT64},
			{"iocb", LINX_FIELD_TYPE_UNKNOWN},
			{"result", LINX_FIELD_TYPE_UNKNOWN},
		},
	},
	[LINX_EVENT_TYPE_GET_THREAD_AREA_E] = {
		"get_thread_area", 0,
		{}
	},
	[LINX_EVENT_TYPE_GET_THREAD_AREA_X] = {
		"get_thread_area", 1,
		{
			{"u_info", LINX_FIELD_TYPE_UNKNOWN},
		},
	},
	[LINX_EVENT_TYPE_LOOKUP_DCOOKIE_E] = {
		"lookup_dcookie", 0,
		{}
	},
	[LINX_EVENT_TYPE_LOOKUP_DCOOKIE_X] = {
		"lookup_dcookie", 3,
		{
			{"cookie", LINX_FIELD_TYPE_UINT64},
			{"buffer", LINX_FIELD_TYPE_CHARBUF},
			{"len", LINX_FIELD_TYPE_UINT64},
		},
	},
	[LINX_EVENT_TYPE_EPOLL_CREATE_E] = {
		"epoll_create", 0,
		{}
	},
	[LINX_EVENT_TYPE_EPOLL_CREATE_X] = {
		"epoll_create", 1,
		{
			{"size", LINX_FIELD_TYPE_INT32},
		},
	},
	[LINX_EVENT_TYPE_REMAP_FILE_PAGES_E] = {
		"remap_file_pages", 0,
		{}
	},
	[LINX_EVENT_TYPE_REMAP_FILE_PAGES_X] = {
		"remap_file_pages", 5,
		{
			{"start", LINX_FIELD_TYPE_UINT64},
			{"size", LINX_FIELD_TYPE_UINT64},
			{"prot", LINX_FIELD_TYPE_UINT64},
			{"pgoff", LINX_FIELD_TYPE_UINT64},
			{"flags", LINX_FIELD_TYPE_UINT64},
		},
	},
	[LINX_EVENT_TYPE_GETDENTS64_E] = {
		"getdents64", 0,
		{}
	},
	[LINX_EVENT_TYPE_GETDENTS64_X] = {
		"getdents64", 3,
		{
			{"fd", LINX_FIELD_TYPE_UINT32},
			{"dirent", LINX_FIELD_TYPE_UNKNOWN},
			{"count", LINX_FIELD_TYPE_UINT32},
		},
	},
	[LINX_EVENT_TYPE_SET_TID_ADDRESS_E] = {
		"set_tid_address", 0,
		{}
	},
	[LINX_EVENT_TYPE_SET_TID_ADDRESS_X] = {
		"set_tid_address", 1,
		{
			{"tidptr", LINX_FIELD_TYPE_UNKNOWN},
		},
	},
	[LINX_EVENT_TYPE_RESTART_SYSCALL_E] = {
		"restart_syscall", 0,
		{}
	},
	[LINX_EVENT_TYPE_RESTART_SYSCALL_X] = {
		"restart_syscall", 0,
		{
		},
	},
	[LINX_EVENT_TYPE_SEMTIMEDOP_E] = {
		"semtimedop", 0,
		{}
	},
	[LINX_EVENT_TYPE_SEMTIMEDOP_X] = {
		"semtimedop", 4,
		{
			{"semid", LINX_FIELD_TYPE_INT32},
			{"tsops", LINX_FIELD_TYPE_UNKNOWN},
			{"nsops", LINX_FIELD_TYPE_UINT32},
			{"timeout", LINX_FIELD_TYPE_UNKNOWN},
		},
	},
	[LINX_EVENT_TYPE_FADVISE64_E] = {
		"fadvise64", 0,
		{}
	},
	[LINX_EVENT_TYPE_FADVISE64_X] = {
		"fadvise64", 4,
		{
			{"fd", LINX_FIELD_TYPE_INT32},
			{"offset", LINX_FIELD_TYPE_INT64},
			{"len", LINX_FIELD_TYPE_UINT64},
			{"advice", LINX_FIELD_TYPE_INT32},
		},
	},
	[LINX_EVENT_TYPE_TIMER_CREATE_E] = {
		"timer_create", 0,
		{}
	},
	[LINX_EVENT_TYPE_TIMER_CREATE_X] = {
		"timer_create", 3,
		{
			{"which_clock", LINX_FIELD_TYPE_INT32},
			{"timer_event_spec", LINX_FIELD_TYPE_UNKNOWN},
			{"created_timer_id", LINX_FIELD_TYPE_UNKNOWN},
		},
	},
	[LINX_EVENT_TYPE_TIMER_SETTIME_E] = {
		"timer_settime", 0,
		{}
	},
	[LINX_EVENT_TYPE_TIMER_SETTIME_X] = {
		"timer_settime", 4,
		{
			{"timer_id", LINX_FIELD_TYPE_INT32},
			{"flags", LINX_FIELD_TYPE_INT32},
			{"new_setting", LINX_FIELD_TYPE_UNKNOWN},
			{"old_setting", LINX_FIELD_TYPE_UNKNOWN},
		},
	},
	[LINX_EVENT_TYPE_TIMER_GETTIME_E] = {
		"timer_gettime", 0,
		{}
	},
	[LINX_EVENT_TYPE_TIMER_GETTIME_X] = {
		"timer_gettime", 2,
		{
			{"timer_id", LINX_FIELD_TYPE_INT32},
			{"setting", LINX_FIELD_TYPE_UNKNOWN},
		},
	},
	[LINX_EVENT_TYPE_TIMER_GETOVERRUN_E] = {
		"timer_getoverrun", 0,
		{}
	},
	[LINX_EVENT_TYPE_TIMER_GETOVERRUN_X] = {
		"timer_getoverrun", 1,
		{
			{"timer_id", LINX_FIELD_TYPE_INT32},
		},
	},
	[LINX_EVENT_TYPE_TIMER_DELETE_E] = {
		"timer_delete", 0,
		{}
	},
	[LINX_EVENT_TYPE_TIMER_DELETE_X] = {
		"timer_delete", 1,
		{
			{"timer_id", LINX_FIELD_TYPE_INT32},
		},
	},
	[LINX_EVENT_TYPE_CLOCK_SETTIME_E] = {
		"clock_settime", 0,
		{}
	},
	[LINX_EVENT_TYPE_CLOCK_SETTIME_X] = {
		"clock_settime", 2,
		{
			{"which_clock", LINX_FIELD_TYPE_INT32},
			{"tp", LINX_FIELD_TYPE_UNKNOWN},
		},
	},
	[LINX_EVENT_TYPE_CLOCK_GETTIME_E] = {
		"clock_gettime", 0,
		{}
	},
	[LINX_EVENT_TYPE_CLOCK_GETTIME_X] = {
		"clock_gettime", 2,
		{
			{"which_clock", LINX_FIELD_TYPE_INT32},
			{"tp", LINX_FIELD_TYPE_UNKNOWN},
		},
	},
	[LINX_EVENT_TYPE_CLOCK_GETRES_E] = {
		"clock_getres", 0,
		{}
	},
	[LINX_EVENT_TYPE_CLOCK_GETRES_X] = {
		"clock_getres", 2,
		{
			{"which_clock", LINX_FIELD_TYPE_INT32},
			{"tp", LINX_FIELD_TYPE_UNKNOWN},
		},
	},
	[LINX_EVENT_TYPE_CLOCK_NANOSLEEP_E] = {
		"clock_nanosleep", 0,
		{}
	},
	[LINX_EVENT_TYPE_CLOCK_NANOSLEEP_X] = {
		"clock_nanosleep", 4,
		{
			{"which_clock", LINX_FIELD_TYPE_INT32},
			{"flags", LINX_FIELD_TYPE_INT32},
			{"rqtp", LINX_FIELD_TYPE_UNKNOWN},
			{"rmtp", LINX_FIELD_TYPE_UNKNOWN},
		},
	},
	[LINX_EVENT_TYPE_EXIT_GROUP_E] = {
		"exit_group", 0,
		{}
	},
	[LINX_EVENT_TYPE_EXIT_GROUP_X] = {
		"exit_group", 1,
		{
			{"error_code", LINX_FIELD_TYPE_INT32},
		},
	},
	[LINX_EVENT_TYPE_EPOLL_WAIT_E] = {
		"epoll_wait", 0,
		{}
	},
	[LINX_EVENT_TYPE_EPOLL_WAIT_X] = {
		"epoll_wait", 4,
		{
			{"epfd", LINX_FIELD_TYPE_INT32},
			{"events", LINX_FIELD_TYPE_UNKNOWN},
			{"maxevents", LINX_FIELD_TYPE_INT32},
			{"timeout", LINX_FIELD_TYPE_INT32},
		},
	},
	[LINX_EVENT_TYPE_EPOLL_CTL_E] = {
		"epoll_ctl", 0,
		{}
	},
	[LINX_EVENT_TYPE_EPOLL_CTL_X] = {
		"epoll_ctl", 4,
		{
			{"epfd", LINX_FIELD_TYPE_INT32},
			{"op", LINX_FIELD_TYPE_INT32},
			{"fd", LINX_FIELD_TYPE_INT32},
			{"event", LINX_FIELD_TYPE_UNKNOWN},
		},
	},
	[LINX_EVENT_TYPE_TGKILL_E] = {
		"tgkill", 0,
		{}
	},
	[LINX_EVENT_TYPE_TGKILL_X] = {
		"tgkill", 3,
		{
			{"tgid", LINX_FIELD_TYPE_INT32},
			{"pid", LINX_FIELD_TYPE_INT32},
			{"sig", LINX_FIELD_TYPE_INT32},
		},
	},
	[LINX_EVENT_TYPE_UTIMES_E] = {
		"utimes", 0,
		{}
	},
	[LINX_EVENT_TYPE_UTIMES_X] = {
		"utimes", 2,
		{
			{"filename", LINX_FIELD_TYPE_CHARBUF},
			{"utimes", LINX_FIELD_TYPE_UNKNOWN},
		},
	},
	[LINX_EVENT_TYPE_MBIND_E] = {
		"mbind", 0,
		{}
	},
	[LINX_EVENT_TYPE_MBIND_X] = {
		"mbind", 6,
		{
			{"start", LINX_FIELD_TYPE_UINT64},
			{"len", LINX_FIELD_TYPE_UINT64},
			{"mode", LINX_FIELD_TYPE_UINT64},
			{"nmask", LINX_FIELD_TYPE_UNKNOWN},
			{"maxnode", LINX_FIELD_TYPE_UINT64},
			{"flags", LINX_FIELD_TYPE_UINT32},
		},
	},
	[LINX_EVENT_TYPE_SET_MEMPOLICY_E] = {
		"set_mempolicy", 0,
		{}
	},
	[LINX_EVENT_TYPE_SET_MEMPOLICY_X] = {
		"set_mempolicy", 3,
		{
			{"mode", LINX_FIELD_TYPE_INT32},
			{"nmask", LINX_FIELD_TYPE_UNKNOWN},
			{"maxnode", LINX_FIELD_TYPE_UINT64},
		},
	},
	[LINX_EVENT_TYPE_GET_MEMPOLICY_E] = {
		"get_mempolicy", 0,
		{}
	},
	[LINX_EVENT_TYPE_GET_MEMPOLICY_X] = {
		"get_mempolicy", 5,
		{
			{"policy", LINX_FIELD_TYPE_UNKNOWN},
			{"nmask", LINX_FIELD_TYPE_UNKNOWN},
			{"maxnode", LINX_FIELD_TYPE_UINT64},
			{"addr", LINX_FIELD_TYPE_UINT64},
			{"flags", LINX_FIELD_TYPE_UINT64},
		},
	},
	[LINX_EVENT_TYPE_MQ_OPEN_E] = {
		"mq_open", 0,
		{}
	},
	[LINX_EVENT_TYPE_MQ_OPEN_X] = {
		"mq_open", 4,
		{
			{"u_name", LINX_FIELD_TYPE_CHARBUF},
			{"oflag", LINX_FIELD_TYPE_INT32},
			{"mode", LINX_FIELD_TYPE_UINT16},
			{"u_attr", LINX_FIELD_TYPE_UNKNOWN},
		},
	},
	[LINX_EVENT_TYPE_MQ_UNLINK_E] = {
		"mq_unlink", 0,
		{}
	},
	[LINX_EVENT_TYPE_MQ_UNLINK_X] = {
		"mq_unlink", 1,
		{
			{"u_name", LINX_FIELD_TYPE_CHARBUF},
		},
	},
	[LINX_EVENT_TYPE_MQ_TIMEDSEND_E] = {
		"mq_timedsend", 0,
		{}
	},
	[LINX_EVENT_TYPE_MQ_TIMEDSEND_X] = {
		"mq_timedsend", 5,
		{
			{"mqdes", LINX_FIELD_TYPE_INT32},
			{"u_msg_ptr", LINX_FIELD_TYPE_CHARBUF},
			{"msg_len", LINX_FIELD_TYPE_UINT64},
			{"msg_prio", LINX_FIELD_TYPE_UINT32},
			{"u_abs_timeout", LINX_FIELD_TYPE_UNKNOWN},
		},
	},
	[LINX_EVENT_TYPE_MQ_TIMEDRECEIVE_E] = {
		"mq_timedreceive", 0,
		{}
	},
	[LINX_EVENT_TYPE_MQ_TIMEDRECEIVE_X] = {
		"mq_timedreceive", 5,
		{
			{"mqdes", LINX_FIELD_TYPE_INT32},
			{"u_msg_ptr", LINX_FIELD_TYPE_CHARBUF},
			{"msg_len", LINX_FIELD_TYPE_UINT64},
			{"u_msg_prio", LINX_FIELD_TYPE_UNKNOWN},
			{"u_abs_timeout", LINX_FIELD_TYPE_UNKNOWN},
		},
	},
	[LINX_EVENT_TYPE_MQ_NOTIFY_E] = {
		"mq_notify", 0,
		{}
	},
	[LINX_EVENT_TYPE_MQ_NOTIFY_X] = {
		"mq_notify", 2,
		{
			{"mqdes", LINX_FIELD_TYPE_INT32},
			{"u_notification", LINX_FIELD_TYPE_UNKNOWN},
		},
	},
	[LINX_EVENT_TYPE_MQ_GETSETATTR_E] = {
		"mq_getsetattr", 0,
		{}
	},
	[LINX_EVENT_TYPE_MQ_GETSETATTR_X] = {
		"mq_getsetattr", 3,
		{
			{"mqdes", LINX_FIELD_TYPE_INT32},
			{"u_mqstat", LINX_FIELD_TYPE_UNKNOWN},
			{"u_omqstat", LINX_FIELD_TYPE_UNKNOWN},
		},
	},
	[LINX_EVENT_TYPE_KEXEC_LOAD_E] = {
		"kexec_load", 0,
		{}
	},
	[LINX_EVENT_TYPE_KEXEC_LOAD_X] = {
		"kexec_load", 4,
		{
			{"entry", LINX_FIELD_TYPE_UINT64},
			{"nr_segments", LINX_FIELD_TYPE_UINT64},
			{"segments", LINX_FIELD_TYPE_UNKNOWN},
			{"flags", LINX_FIELD_TYPE_UINT64},
		},
	},
	[LINX_EVENT_TYPE_WAITID_E] = {
		"waitid", 0,
		{}
	},
	[LINX_EVENT_TYPE_WAITID_X] = {
		"waitid", 5,
		{
			{"which", LINX_FIELD_TYPE_INT32},
			{"upid", LINX_FIELD_TYPE_INT32},
			{"infop", LINX_FIELD_TYPE_UNKNOWN},
			{"options", LINX_FIELD_TYPE_INT32},
			{"ru", LINX_FIELD_TYPE_UNKNOWN},
		},
	},
	[LINX_EVENT_TYPE_ADD_KEY_E] = {
		"add_key", 0,
		{}
	},
	[LINX_EVENT_TYPE_ADD_KEY_X] = {
		"add_key", 5,
		{
			{"_type", LINX_FIELD_TYPE_CHARBUF},
			{"_description", LINX_FIELD_TYPE_CHARBUF},
			{"_payload", LINX_FIELD_TYPE_UNKNOWN},
			{"plen", LINX_FIELD_TYPE_UINT64},
			{"ringid", LINX_FIELD_TYPE_INT32},
		},
	},
	[LINX_EVENT_TYPE_REQUEST_KEY_E] = {
		"request_key", 0,
		{}
	},
	[LINX_EVENT_TYPE_REQUEST_KEY_X] = {
		"request_key", 4,
		{
			{"_type", LINX_FIELD_TYPE_CHARBUF},
			{"_description", LINX_FIELD_TYPE_CHARBUF},
			{"_callout_info", LINX_FIELD_TYPE_CHARBUF},
			{"destringid", LINX_FIELD_TYPE_INT32},
		},
	},
	[LINX_EVENT_TYPE_KEYCTL_E] = {
		"keyctl", 0,
		{}
	},
	[LINX_EVENT_TYPE_KEYCTL_X] = {
		"keyctl", 5,
		{
			{"option", LINX_FIELD_TYPE_INT32},
			{"arg2", LINX_FIELD_TYPE_UINT64},
			{"arg3", LINX_FIELD_TYPE_UINT64},
			{"arg4", LINX_FIELD_TYPE_UINT64},
			{"arg5", LINX_FIELD_TYPE_UINT64},
		},
	},
	[LINX_EVENT_TYPE_IOPRIO_SET_E] = {
		"ioprio_set", 0,
		{}
	},
	[LINX_EVENT_TYPE_IOPRIO_SET_X] = {
		"ioprio_set", 3,
		{
			{"which", LINX_FIELD_TYPE_INT32},
			{"who", LINX_FIELD_TYPE_INT32},
			{"ioprio", LINX_FIELD_TYPE_INT32},
		},
	},
	[LINX_EVENT_TYPE_IOPRIO_GET_E] = {
		"ioprio_get", 0,
		{}
	},
	[LINX_EVENT_TYPE_IOPRIO_GET_X] = {
		"ioprio_get", 2,
		{
			{"which", LINX_FIELD_TYPE_INT32},
			{"who", LINX_FIELD_TYPE_INT32},
		},
	},
	[LINX_EVENT_TYPE_INOTIFY_INIT_E] = {
		"inotify_init", 0,
		{}
	},
	[LINX_EVENT_TYPE_INOTIFY_INIT_X] = {
		"inotify_init", 0,
		{
		},
	},
	[LINX_EVENT_TYPE_INOTIFY_ADD_WATCH_E] = {
		"inotify_add_watch", 0,
		{}
	},
	[LINX_EVENT_TYPE_INOTIFY_ADD_WATCH_X] = {
		"inotify_add_watch", 3,
		{
			{"fd", LINX_FIELD_TYPE_INT32},
			{"pathname", LINX_FIELD_TYPE_CHARBUF},
			{"mask", LINX_FIELD_TYPE_UINT32},
		},
	},
	[LINX_EVENT_TYPE_INOTIFY_RM_WATCH_E] = {
		"inotify_rm_watch", 0,
		{}
	},
	[LINX_EVENT_TYPE_INOTIFY_RM_WATCH_X] = {
		"inotify_rm_watch", 2,
		{
			{"fd", LINX_FIELD_TYPE_INT32},
			{"wd", LINX_FIELD_TYPE_INT32},
		},
	},
	[LINX_EVENT_TYPE_MIGRATE_PAGES_E] = {
		"migrate_pages", 0,
		{}
	},
	[LINX_EVENT_TYPE_MIGRATE_PAGES_X] = {
		"migrate_pages", 4,
		{
			{"pid", LINX_FIELD_TYPE_INT32},
			{"maxnode", LINX_FIELD_TYPE_UINT64},
			{"old_nodes", LINX_FIELD_TYPE_UNKNOWN},
			{"new_nodes", LINX_FIELD_TYPE_UNKNOWN},
		},
	},
	[LINX_EVENT_TYPE_OPENAT_E] = {
		"openat", 4,
		{
			{"dirfd", LINX_FIELD_TYPE_INT64},
			{"name", LINX_FIELD_TYPE_CHARBUF},
			{"flags", LINX_FIELD_TYPE_INT32},
			{"mode", LINX_FIELD_TYPE_UINT32},
		}
	},
	[LINX_EVENT_TYPE_OPENAT_X] = {
		"openat", 7,
		{
			{"fd", LINX_FIELD_TYPE_INT64},
			{"dirfd", LINX_FIELD_TYPE_INT64},
			{"name", LINX_FIELD_TYPE_CHARBUF},
			{"flags", LINX_FIELD_TYPE_INT32},
			{"mode", LINX_FIELD_TYPE_UINT32},
			{"dev", LINX_FIELD_TYPE_UINT32},
			{"ino", LINX_FIELD_TYPE_UINT64},
		},
	},
	[LINX_EVENT_TYPE_MKDIRAT_E] = {
		"mkdirat", 0,
		{}
	},
	[LINX_EVENT_TYPE_MKDIRAT_X] = {
		"mkdirat", 3,
		{
			{"dfd", LINX_FIELD_TYPE_INT32},
			{"pathname", LINX_FIELD_TYPE_CHARBUF},
			{"mode", LINX_FIELD_TYPE_UINT16},
		},
	},
	[LINX_EVENT_TYPE_MKNODAT_E] = {
		"mknodat", 0,
		{}
	},
	[LINX_EVENT_TYPE_MKNODAT_X] = {
		"mknodat", 4,
		{
			{"dfd", LINX_FIELD_TYPE_INT32},
			{"filename", LINX_FIELD_TYPE_CHARBUF},
			{"mode", LINX_FIELD_TYPE_UINT16},
			{"dev", LINX_FIELD_TYPE_UINT32},
		},
	},
	[LINX_EVENT_TYPE_FCHOWNAT_E] = {
		"fchownat", 0,
		{}
	},
	[LINX_EVENT_TYPE_FCHOWNAT_X] = {
		"fchownat", 5,
		{
			{"dfd", LINX_FIELD_TYPE_INT32},
			{"filename", LINX_FIELD_TYPE_CHARBUF},
			{"user", LINX_FIELD_TYPE_UINT32},
			{"group", LINX_FIELD_TYPE_UINT32},
			{"flag", LINX_FIELD_TYPE_INT32},
		},
	},
	[LINX_EVENT_TYPE_FUTIMESAT_E] = {
		"futimesat", 0,
		{}
	},
	[LINX_EVENT_TYPE_FUTIMESAT_X] = {
		"futimesat", 3,
		{
			{"dfd", LINX_FIELD_TYPE_INT32},
			{"filename", LINX_FIELD_TYPE_CHARBUF},
			{"utimes", LINX_FIELD_TYPE_UNKNOWN},
		},
	},
	[LINX_EVENT_TYPE_NEWFSTATAT_E] = {
		"newfstatat", 0,
		{}
	},
	[LINX_EVENT_TYPE_NEWFSTATAT_X] = {
		"newfstatat", 4,
		{
			{"dfd", LINX_FIELD_TYPE_INT32},
			{"filename", LINX_FIELD_TYPE_CHARBUF},
			{"statbuf", LINX_FIELD_TYPE_UNKNOWN},
			{"flag", LINX_FIELD_TYPE_INT32},
		},
	},
	[LINX_EVENT_TYPE_UNLINKAT_E] = {
		"unlinkat", 0,
		{}
	},
	[LINX_EVENT_TYPE_UNLINKAT_X] = {
		"unlinkat", 3,
		{
			{"dfd", LINX_FIELD_TYPE_INT32},
			{"pathname", LINX_FIELD_TYPE_CHARBUF},
			{"flag", LINX_FIELD_TYPE_INT32},
		},
	},
	[LINX_EVENT_TYPE_RENAMEAT_E] = {
		"renameat", 0,
		{}
	},
	[LINX_EVENT_TYPE_RENAMEAT_X] = {
		"renameat", 4,
		{
			{"olddfd", LINX_FIELD_TYPE_INT32},
			{"oldname", LINX_FIELD_TYPE_CHARBUF},
			{"newdfd", LINX_FIELD_TYPE_INT32},
			{"newname", LINX_FIELD_TYPE_CHARBUF},
		},
	},
	[LINX_EVENT_TYPE_LINKAT_E] = {
		"linkat", 0,
		{}
	},
	[LINX_EVENT_TYPE_LINKAT_X] = {
		"linkat", 5,
		{
			{"olddfd", LINX_FIELD_TYPE_INT32},
			{"oldname", LINX_FIELD_TYPE_CHARBUF},
			{"newdfd", LINX_FIELD_TYPE_INT32},
			{"newname", LINX_FIELD_TYPE_CHARBUF},
			{"flags", LINX_FIELD_TYPE_INT32},
		},
	},
	[LINX_EVENT_TYPE_SYMLINKAT_E] = {
		"symlinkat", 0,
		{}
	},
	[LINX_EVENT_TYPE_SYMLINKAT_X] = {
		"symlinkat", 3,
		{
			{"oldname", LINX_FIELD_TYPE_CHARBUF},
			{"newdfd", LINX_FIELD_TYPE_INT32},
			{"newname", LINX_FIELD_TYPE_CHARBUF},
		},
	},
	[LINX_EVENT_TYPE_READLINKAT_E] = {
		"readlinkat", 0,
		{}
	},
	[LINX_EVENT_TYPE_READLINKAT_X] = {
		"readlinkat", 4,
		{
			{"dfd", LINX_FIELD_TYPE_INT32},
			{"pathname", LINX_FIELD_TYPE_CHARBUF},
			{"buf", LINX_FIELD_TYPE_CHARBUF},
			{"bufsiz", LINX_FIELD_TYPE_INT32},
		},
	},
	[LINX_EVENT_TYPE_FCHMODAT_E] = {
		"fchmodat", 0,
		{}
	},
	[LINX_EVENT_TYPE_FCHMODAT_X] = {
		"fchmodat", 3,
		{
			{"dfd", LINX_FIELD_TYPE_INT32},
			{"filename", LINX_FIELD_TYPE_CHARBUF},
			{"mode", LINX_FIELD_TYPE_UINT16},
		},
	},
	[LINX_EVENT_TYPE_FACCESSAT_E] = {
		"faccessat", 0,
		{}
	},
	[LINX_EVENT_TYPE_FACCESSAT_X] = {
		"faccessat", 3,
		{
			{"dfd", LINX_FIELD_TYPE_INT32},
			{"filename", LINX_FIELD_TYPE_CHARBUF},
			{"mode", LINX_FIELD_TYPE_INT32},
		},
	},
	[LINX_EVENT_TYPE_PSELECT6_E] = {
		"pselect6", 0,
		{}
	},
	[LINX_EVENT_TYPE_PSELECT6_X] = {
		"pselect6", 6,
		{
			{"n", LINX_FIELD_TYPE_INT32},
			{"inp", LINX_FIELD_TYPE_UNKNOWN},
			{"outp", LINX_FIELD_TYPE_UNKNOWN},
			{"exp", LINX_FIELD_TYPE_UNKNOWN},
			{"tsp", LINX_FIELD_TYPE_UNKNOWN},
			{"sig", LINX_FIELD_TYPE_UNKNOWN},
		},
	},
	[LINX_EVENT_TYPE_PPOLL_E] = {
		"ppoll", 0,
		{}
	},
	[LINX_EVENT_TYPE_PPOLL_X] = {
		"ppoll", 5,
		{
			{"ufds", LINX_FIELD_TYPE_UNKNOWN},
			{"nfds", LINX_FIELD_TYPE_UINT32},
			{"tsp", LINX_FIELD_TYPE_UNKNOWN},
			{"sigmask", LINX_FIELD_TYPE_UNKNOWN},
			{"sigsetsize", LINX_FIELD_TYPE_UINT64},
		},
	},
	[LINX_EVENT_TYPE_UNSHARE_E] = {
		"unshare", 0,
		{}
	},
	[LINX_EVENT_TYPE_UNSHARE_X] = {
		"unshare", 1,
		{
			{"unshare_flags", LINX_FIELD_TYPE_UINT64},
		},
	},
	[LINX_EVENT_TYPE_SET_ROBUST_LIST_E] = {
		"set_robust_list", 0,
		{}
	},
	[LINX_EVENT_TYPE_SET_ROBUST_LIST_X] = {
		"set_robust_list", 2,
		{
			{"head", LINX_FIELD_TYPE_UNKNOWN},
			{"len", LINX_FIELD_TYPE_UINT64},
		},
	},
	[LINX_EVENT_TYPE_GET_ROBUST_LIST_E] = {
		"get_robust_list", 0,
		{}
	},
	[LINX_EVENT_TYPE_GET_ROBUST_LIST_X] = {
		"get_robust_list", 3,
		{
			{"pid", LINX_FIELD_TYPE_INT32},
			{"head_ptr", LINX_FIELD_TYPE_UNKNOWN},
			{"len_ptr", LINX_FIELD_TYPE_UNKNOWN},
		},
	},
	[LINX_EVENT_TYPE_SPLICE_E] = {
		"splice", 0,
		{}
	},
	[LINX_EVENT_TYPE_SPLICE_X] = {
		"splice", 6,
		{
			{"fd_in", LINX_FIELD_TYPE_INT32},
			{"off_in", LINX_FIELD_TYPE_UNKNOWN},
			{"fd_out", LINX_FIELD_TYPE_INT32},
			{"off_out", LINX_FIELD_TYPE_UNKNOWN},
			{"len", LINX_FIELD_TYPE_UINT64},
			{"flags", LINX_FIELD_TYPE_UINT32},
		},
	},
	[LINX_EVENT_TYPE_TEE_E] = {
		"tee", 0,
		{}
	},
	[LINX_EVENT_TYPE_TEE_X] = {
		"tee", 4,
		{
			{"fdin", LINX_FIELD_TYPE_INT32},
			{"fdout", LINX_FIELD_TYPE_INT32},
			{"len", LINX_FIELD_TYPE_UINT64},
			{"flags", LINX_FIELD_TYPE_UINT32},
		},
	},
	[LINX_EVENT_TYPE_SYNC_FILE_RANGE_E] = {
		"sync_file_range", 0,
		{}
	},
	[LINX_EVENT_TYPE_SYNC_FILE_RANGE_X] = {
		"sync_file_range", 4,
		{
			{"fd", LINX_FIELD_TYPE_INT32},
			{"offset", LINX_FIELD_TYPE_INT64},
			{"nbytes", LINX_FIELD_TYPE_INT64},
			{"flags", LINX_FIELD_TYPE_UINT32},
		},
	},
	[LINX_EVENT_TYPE_VMSPLICE_E] = {
		"vmsplice", 0,
		{}
	},
	[LINX_EVENT_TYPE_VMSPLICE_X] = {
		"vmsplice", 4,
		{
			{"fd", LINX_FIELD_TYPE_INT32},
			{"uiov", LINX_FIELD_TYPE_UNKNOWN},
			{"nr_segs", LINX_FIELD_TYPE_UINT64},
			{"flags", LINX_FIELD_TYPE_UINT32},
		},
	},
	[LINX_EVENT_TYPE_MOVE_PAGES_E] = {
		"move_pages", 0,
		{}
	},
	[LINX_EVENT_TYPE_MOVE_PAGES_X] = {
		"move_pages", 6,
		{
			{"pid", LINX_FIELD_TYPE_INT32},
			{"nr_pages", LINX_FIELD_TYPE_UINT64},
			{"pages", LINX_FIELD_TYPE_UNKNOWN},
			{"nodes", LINX_FIELD_TYPE_UNKNOWN},
			{"status", LINX_FIELD_TYPE_UNKNOWN},
			{"flags", LINX_FIELD_TYPE_INT32},
		},
	},
	[LINX_EVENT_TYPE_UTIMENSAT_E] = {
		"utimensat", 0,
		{}
	},
	[LINX_EVENT_TYPE_UTIMENSAT_X] = {
		"utimensat", 4,
		{
			{"dfd", LINX_FIELD_TYPE_INT32},
			{"filename", LINX_FIELD_TYPE_CHARBUF},
			{"utimes", LINX_FIELD_TYPE_UNKNOWN},
			{"flags", LINX_FIELD_TYPE_INT32},
		},
	},
	[LINX_EVENT_TYPE_EPOLL_PWAIT_E] = {
		"epoll_pwait", 0,
		{}
	},
	[LINX_EVENT_TYPE_EPOLL_PWAIT_X] = {
		"epoll_pwait", 6,
		{
			{"epfd", LINX_FIELD_TYPE_INT32},
			{"events", LINX_FIELD_TYPE_UNKNOWN},
			{"maxevents", LINX_FIELD_TYPE_INT32},
			{"timeout", LINX_FIELD_TYPE_INT32},
			{"sigmask", LINX_FIELD_TYPE_UNKNOWN},
			{"sigsetsize", LINX_FIELD_TYPE_UINT64},
		},
	},
	[LINX_EVENT_TYPE_SIGNALFD_E] = {
		"signalfd", 0,
		{}
	},
	[LINX_EVENT_TYPE_SIGNALFD_X] = {
		"signalfd", 3,
		{
			{"ufd", LINX_FIELD_TYPE_INT32},
			{"user_mask", LINX_FIELD_TYPE_UNKNOWN},
			{"sizemask", LINX_FIELD_TYPE_UINT64},
		},
	},
	[LINX_EVENT_TYPE_TIMERFD_CREATE_E] = {
		"timerfd_create", 0,
		{}
	},
	[LINX_EVENT_TYPE_TIMERFD_CREATE_X] = {
		"timerfd_create", 2,
		{
			{"clockid", LINX_FIELD_TYPE_INT32},
			{"flags", LINX_FIELD_TYPE_INT32},
		},
	},
	[LINX_EVENT_TYPE_EVENTFD_E] = {
		"eventfd", 0,
		{}
	},
	[LINX_EVENT_TYPE_EVENTFD_X] = {
		"eventfd", 1,
		{
			{"count", LINX_FIELD_TYPE_UINT32},
		},
	},
	[LINX_EVENT_TYPE_FALLOCATE_E] = {
		"fallocate", 0,
		{}
	},
	[LINX_EVENT_TYPE_FALLOCATE_X] = {
		"fallocate", 4,
		{
			{"fd", LINX_FIELD_TYPE_INT32},
			{"mode", LINX_FIELD_TYPE_INT32},
			{"offset", LINX_FIELD_TYPE_INT64},
			{"len", LINX_FIELD_TYPE_INT64},
		},
	},
	[LINX_EVENT_TYPE_TIMERFD_SETTIME_E] = {
		"timerfd_settime", 0,
		{}
	},
	[LINX_EVENT_TYPE_TIMERFD_SETTIME_X] = {
		"timerfd_settime", 4,
		{
			{"ufd", LINX_FIELD_TYPE_INT32},
			{"flags", LINX_FIELD_TYPE_INT32},
			{"utmr", LINX_FIELD_TYPE_UNKNOWN},
			{"otmr", LINX_FIELD_TYPE_UNKNOWN},
		},
	},
	[LINX_EVENT_TYPE_TIMERFD_GETTIME_E] = {
		"timerfd_gettime", 0,
		{}
	},
	[LINX_EVENT_TYPE_TIMERFD_GETTIME_X] = {
		"timerfd_gettime", 2,
		{
			{"ufd", LINX_FIELD_TYPE_INT32},
			{"otmr", LINX_FIELD_TYPE_UNKNOWN},
		},
	},
	[LINX_EVENT_TYPE_ACCEPT4_E] = {
		"accept4", 0,
		{}
	},
	[LINX_EVENT_TYPE_ACCEPT4_X] = {
		"accept4", 4,
		{
			{"fd", LINX_FIELD_TYPE_INT32},
			{"upeer_sockaddr", LINX_FIELD_TYPE_UNKNOWN},
			{"upeer_addrlen", LINX_FIELD_TYPE_UNKNOWN},
			{"flags", LINX_FIELD_TYPE_INT32},
		},
	},
	[LINX_EVENT_TYPE_SIGNALFD4_E] = {
		"signalfd4", 0,
		{}
	},
	[LINX_EVENT_TYPE_SIGNALFD4_X] = {
		"signalfd4", 4,
		{
			{"ufd", LINX_FIELD_TYPE_INT32},
			{"user_mask", LINX_FIELD_TYPE_UNKNOWN},
			{"sizemask", LINX_FIELD_TYPE_UINT64},
			{"flags", LINX_FIELD_TYPE_INT32},
		},
	},
	[LINX_EVENT_TYPE_EVENTFD2_E] = {
		"eventfd2", 0,
		{}
	},
	[LINX_EVENT_TYPE_EVENTFD2_X] = {
		"eventfd2", 2,
		{
			{"count", LINX_FIELD_TYPE_UINT32},
			{"flags", LINX_FIELD_TYPE_INT32},
		},
	},
	[LINX_EVENT_TYPE_EPOLL_CREATE1_E] = {
		"epoll_create1", 0,
		{}
	},
	[LINX_EVENT_TYPE_EPOLL_CREATE1_X] = {
		"epoll_create1", 1,
		{
			{"flags", LINX_FIELD_TYPE_INT32},
		},
	},
	[LINX_EVENT_TYPE_DUP3_E] = {
		"dup3", 1,
		{
			{"fd", LINX_FIELD_TYPE_INT64}
		}
	},
	[LINX_EVENT_TYPE_DUP3_X] = {
		"dup3", 4,
		{
			{"res", LINX_FIELD_TYPE_INT64},
			{"oldfd", LINX_FIELD_TYPE_INT64},
			{"newfd", LINX_FIELD_TYPE_INT64},
			{"flags", LINX_FIELD_TYPE_INT32},
		},
	},
	[LINX_EVENT_TYPE_PIPE2_E] = {
		"pipe2", 0,
		{}
	},
	[LINX_EVENT_TYPE_PIPE2_X] = {
		"pipe2", 2,
		{
			{"fildes", LINX_FIELD_TYPE_UNKNOWN},
			{"flags", LINX_FIELD_TYPE_INT32},
		},
	},
	[LINX_EVENT_TYPE_INOTIFY_INIT1_E] = {
		"inotify_init1", 0,
		{}
	},
	[LINX_EVENT_TYPE_INOTIFY_INIT1_X] = {
		"inotify_init1", 1,
		{
			{"flags", LINX_FIELD_TYPE_INT32},
		},
	},
	[LINX_EVENT_TYPE_PREADV_E] = {
		"preadv", 0,
		{}
	},
	[LINX_EVENT_TYPE_PREADV_X] = {
		"preadv", 5,
		{
			{"fd", LINX_FIELD_TYPE_UINT64},
			{"vec", LINX_FIELD_TYPE_UNKNOWN},
			{"vlen", LINX_FIELD_TYPE_UINT64},
			{"pos_l", LINX_FIELD_TYPE_UINT64},
			{"pos_h", LINX_FIELD_TYPE_UINT64},
		},
	},
	[LINX_EVENT_TYPE_PWRITEV_E] = {
		"pwritev", 0,
		{}
	},
	[LINX_EVENT_TYPE_PWRITEV_X] = {
		"pwritev", 5,
		{
			{"fd", LINX_FIELD_TYPE_UINT64},
			{"vec", LINX_FIELD_TYPE_UNKNOWN},
			{"vlen", LINX_FIELD_TYPE_UINT64},
			{"pos_l", LINX_FIELD_TYPE_UINT64},
			{"pos_h", LINX_FIELD_TYPE_UINT64},
		},
	},
	[LINX_EVENT_TYPE_RT_TGSIGQUEUEINFO_E] = {
		"rt_tgsigqueueinfo", 0,
		{}
	},
	[LINX_EVENT_TYPE_RT_TGSIGQUEUEINFO_X] = {
		"rt_tgsigqueueinfo", 4,
		{
			{"tgid", LINX_FIELD_TYPE_INT32},
			{"pid", LINX_FIELD_TYPE_INT32},
			{"sig", LINX_FIELD_TYPE_INT32},
			{"uinfo", LINX_FIELD_TYPE_UNKNOWN},
		},
	},
	[LINX_EVENT_TYPE_PERF_EVENT_OPEN_E] = {
		"perf_event_open", 0,
		{}
	},
	[LINX_EVENT_TYPE_PERF_EVENT_OPEN_X] = {
		"perf_event_open", 5,
		{
			{"attr_uptr", LINX_FIELD_TYPE_UNKNOWN},
			{"pid", LINX_FIELD_TYPE_INT32},
			{"cpu", LINX_FIELD_TYPE_INT32},
			{"group_fd", LINX_FIELD_TYPE_INT32},
			{"flags", LINX_FIELD_TYPE_UINT64},
		},
	},
	[LINX_EVENT_TYPE_RECVMMSG_E] = {
		"recvmmsg", 0,
		{}
	},
	[LINX_EVENT_TYPE_RECVMMSG_X] = {
		"recvmmsg", 5,
		{
			{"fd", LINX_FIELD_TYPE_INT32},
			{"mmsg", LINX_FIELD_TYPE_UNKNOWN},
			{"vlen", LINX_FIELD_TYPE_UINT32},
			{"flags", LINX_FIELD_TYPE_UINT32},
			{"timeout", LINX_FIELD_TYPE_UNKNOWN},
		},
	},
	[LINX_EVENT_TYPE_FANOTIFY_INIT_E] = {
		"fanotify_init", 0,
		{}
	},
	[LINX_EVENT_TYPE_FANOTIFY_INIT_X] = {
		"fanotify_init", 2,
		{
			{"flags", LINX_FIELD_TYPE_UINT32},
			{"event_f_flags", LINX_FIELD_TYPE_UINT32},
		},
	},
	[LINX_EVENT_TYPE_FANOTIFY_MARK_E] = {
		"fanotify_mark", 0,
		{}
	},
	[LINX_EVENT_TYPE_FANOTIFY_MARK_X] = {
		"fanotify_mark", 5,
		{
			{"fanotify_fd", LINX_FIELD_TYPE_INT32},
			{"flags", LINX_FIELD_TYPE_UINT32},
			{"mask", LINX_FIELD_TYPE_UINT64},
			{"dfd", LINX_FIELD_TYPE_INT32},
			{"pathname", LINX_FIELD_TYPE_CHARBUF},
		},
	},
	[LINX_EVENT_TYPE_PRLIMIT64_E] = {
		"prlimit64", 0,
		{}
	},
	[LINX_EVENT_TYPE_PRLIMIT64_X] = {
		"prlimit64", 4,
		{
			{"pid", LINX_FIELD_TYPE_INT32},
			{"resource", LINX_FIELD_TYPE_UINT32},
			{"new_rlim", LINX_FIELD_TYPE_UNKNOWN},
			{"old_rlim", LINX_FIELD_TYPE_UNKNOWN},
		},
	},
	[LINX_EVENT_TYPE_NAME_TO_HANDLE_AT_E] = {
		"name_to_handle_at", 0,
		{}
	},
	[LINX_EVENT_TYPE_NAME_TO_HANDLE_AT_X] = {
		"name_to_handle_at", 5,
		{
			{"dfd", LINX_FIELD_TYPE_INT32},
			{"name", LINX_FIELD_TYPE_CHARBUF},
			{"handle", LINX_FIELD_TYPE_UNKNOWN},
			{"mnt_id", LINX_FIELD_TYPE_UNKNOWN},
			{"flag", LINX_FIELD_TYPE_INT32},
		},
	},
	[LINX_EVENT_TYPE_OPEN_BY_HANDLE_AT_E] = {
		"open_by_handle_at", 0,
		{}
	},
	[LINX_EVENT_TYPE_OPEN_BY_HANDLE_AT_X] = {
		"open_by_handle_at", 3,
		{
			{"mountdirfd", LINX_FIELD_TYPE_INT32},
			{"handle", LINX_FIELD_TYPE_UNKNOWN},
			{"flags", LINX_FIELD_TYPE_INT32},
		},
	},
	[LINX_EVENT_TYPE_CLOCK_ADJTIME_E] = {
		"clock_adjtime", 0,
		{}
	},
	[LINX_EVENT_TYPE_CLOCK_ADJTIME_X] = {
		"clock_adjtime", 2,
		{
			{"which_clock", LINX_FIELD_TYPE_INT32},
			{"utx", LINX_FIELD_TYPE_UNKNOWN},
		},
	},
	[LINX_EVENT_TYPE_SYNCFS_E] = {
		"syncfs", 0,
		{}
	},
	[LINX_EVENT_TYPE_SYNCFS_X] = {
		"syncfs", 1,
		{
			{"fd", LINX_FIELD_TYPE_INT32},
		},
	},
	[LINX_EVENT_TYPE_SENDMMSG_E] = {
		"sendmmsg", 0,
		{}
	},
	[LINX_EVENT_TYPE_SENDMMSG_X] = {
		"sendmmsg", 4,
		{
			{"fd", LINX_FIELD_TYPE_INT32},
			{"mmsg", LINX_FIELD_TYPE_UNKNOWN},
			{"vlen", LINX_FIELD_TYPE_UINT32},
			{"flags", LINX_FIELD_TYPE_UINT32},
		},
	},
	[LINX_EVENT_TYPE_SETNS_E] = {
		"setns", 0,
		{}
	},
	[LINX_EVENT_TYPE_SETNS_X] = {
		"setns", 2,
		{
			{"fd", LINX_FIELD_TYPE_INT32},
			{"flags", LINX_FIELD_TYPE_INT32},
		},
	},
	[LINX_EVENT_TYPE_GETCPU_E] = {
		"getcpu", 0,
		{}
	},
	[LINX_EVENT_TYPE_GETCPU_X] = {
		"getcpu", 3,
		{
			{"cpup", LINX_FIELD_TYPE_UNKNOWN},
			{"nodep", LINX_FIELD_TYPE_UNKNOWN},
			{"unused", LINX_FIELD_TYPE_UNKNOWN},
		},
	},
	[LINX_EVENT_TYPE_PROCESS_VM_READV_E] = {
		"process_vm_readv", 0,
		{}
	},
	[LINX_EVENT_TYPE_PROCESS_VM_READV_X] = {
		"process_vm_readv", 6,
		{
			{"pid", LINX_FIELD_TYPE_INT32},
			{"lvec", LINX_FIELD_TYPE_UNKNOWN},
			{"liovcnt", LINX_FIELD_TYPE_UINT64},
			{"rvec", LINX_FIELD_TYPE_UNKNOWN},
			{"riovcnt", LINX_FIELD_TYPE_UINT64},
			{"flags", LINX_FIELD_TYPE_UINT64},
		},
	},
	[LINX_EVENT_TYPE_PROCESS_VM_WRITEV_E] = {
		"process_vm_writev", 0,
		{}
	},
	[LINX_EVENT_TYPE_PROCESS_VM_WRITEV_X] = {
		"process_vm_writev", 6,
		{
			{"pid", LINX_FIELD_TYPE_INT32},
			{"lvec", LINX_FIELD_TYPE_UNKNOWN},
			{"liovcnt", LINX_FIELD_TYPE_UINT64},
			{"rvec", LINX_FIELD_TYPE_UNKNOWN},
			{"riovcnt", LINX_FIELD_TYPE_UINT64},
			{"flags", LINX_FIELD_TYPE_UINT64},
		},
	},
	[LINX_EVENT_TYPE_KCMP_E] = {
		"kcmp", 0,
		{}
	},
	[LINX_EVENT_TYPE_KCMP_X] = {
		"kcmp", 5,
		{
			{"pid1", LINX_FIELD_TYPE_INT32},
			{"pid2", LINX_FIELD_TYPE_INT32},
			{"type", LINX_FIELD_TYPE_INT32},
			{"idx1", LINX_FIELD_TYPE_UINT64},
			{"idx2", LINX_FIELD_TYPE_UINT64},
		},
	},
	[LINX_EVENT_TYPE_FINIT_MODULE_E] = {
		"finit_module", 0,
		{}
	},
	[LINX_EVENT_TYPE_FINIT_MODULE_X] = {
		"finit_module", 3,
		{
			{"fd", LINX_FIELD_TYPE_INT32},
			{"uargs", LINX_FIELD_TYPE_CHARBUF},
			{"flags", LINX_FIELD_TYPE_INT32},
		},
	},
	[LINX_EVENT_TYPE_SCHED_SETATTR_E] = {
		"sched_setattr", 0,
		{}
	},
	[LINX_EVENT_TYPE_SCHED_SETATTR_X] = {
		"sched_setattr", 3,
		{
			{"pid", LINX_FIELD_TYPE_INT32},
			{"uattr", LINX_FIELD_TYPE_UNKNOWN},
			{"flags", LINX_FIELD_TYPE_UINT32},
		},
	},
	[LINX_EVENT_TYPE_SCHED_GETATTR_E] = {
		"sched_getattr", 0,
		{}
	},
	[LINX_EVENT_TYPE_SCHED_GETATTR_X] = {
		"sched_getattr", 4,
		{
			{"pid", LINX_FIELD_TYPE_INT32},
			{"uattr", LINX_FIELD_TYPE_UNKNOWN},
			{"usize", LINX_FIELD_TYPE_UINT32},
			{"flags", LINX_FIELD_TYPE_UINT32},
		},
	},
	[LINX_EVENT_TYPE_RENAMEAT2_E] = {
		"renameat2", 0,
		{}
	},
	[LINX_EVENT_TYPE_RENAMEAT2_X] = {
		"renameat2", 5,
		{
			{"olddfd", LINX_FIELD_TYPE_INT32},
			{"oldname", LINX_FIELD_TYPE_CHARBUF},
			{"newdfd", LINX_FIELD_TYPE_INT32},
			{"newname", LINX_FIELD_TYPE_CHARBUF},
			{"flags", LINX_FIELD_TYPE_UINT32},
		},
	},
	[LINX_EVENT_TYPE_SECCOMP_E] = {
		"seccomp", 0,
		{}
	},
	[LINX_EVENT_TYPE_SECCOMP_X] = {
		"seccomp", 3,
		{
			{"op", LINX_FIELD_TYPE_UINT32},
			{"flags", LINX_FIELD_TYPE_UINT32},
			{"uargs", LINX_FIELD_TYPE_UNKNOWN},
		},
	},
	[LINX_EVENT_TYPE_GETRANDOM_E] = {
		"getrandom", 0,
		{}
	},
	[LINX_EVENT_TYPE_GETRANDOM_X] = {
		"getrandom", 3,
		{
			{"ubuf", LINX_FIELD_TYPE_CHARBUF},
			{"len", LINX_FIELD_TYPE_UINT64},
			{"flags", LINX_FIELD_TYPE_UINT32},
		},
	},
	[LINX_EVENT_TYPE_MEMFD_CREATE_E] = {
		"memfd_create", 0,
		{}
	},
	[LINX_EVENT_TYPE_MEMFD_CREATE_X] = {
		"memfd_create", 2,
		{
			{"uname", LINX_FIELD_TYPE_CHARBUF},
			{"flags", LINX_FIELD_TYPE_UINT32},
		},
	},
	[LINX_EVENT_TYPE_KEXEC_FILE_LOAD_E] = {
		"kexec_file_load", 0,
		{}
	},
	[LINX_EVENT_TYPE_KEXEC_FILE_LOAD_X] = {
		"kexec_file_load", 5,
		{
			{"kernel_fd", LINX_FIELD_TYPE_INT32},
			{"initrd_fd", LINX_FIELD_TYPE_INT32},
			{"cmdline_len", LINX_FIELD_TYPE_UINT64},
			{"cmdline_ptr", LINX_FIELD_TYPE_CHARBUF},
			{"flags", LINX_FIELD_TYPE_UINT64},
		},
	},
	[LINX_EVENT_TYPE_BPF_E] = {
		"bpf", 0,
		{}
	},
	[LINX_EVENT_TYPE_BPF_X] = {
		"bpf", 3,
		{
			{"cmd", LINX_FIELD_TYPE_INT32},
			{"uattr", LINX_FIELD_TYPE_UNKNOWN},
			{"size", LINX_FIELD_TYPE_UINT32},
		},
	},
	[LINX_EVENT_TYPE_EXECVEAT_E] = {
		"execveat", 0,
		{}
	},
	[LINX_EVENT_TYPE_EXECVEAT_X] = {
		"execveat", 5,
		{
			{"fd", LINX_FIELD_TYPE_INT32},
			{"filename", LINX_FIELD_TYPE_CHARBUF},
			{"argv", LINX_FIELD_TYPE_UNKNOWN},
			{"envp", LINX_FIELD_TYPE_UNKNOWN},
			{"flags", LINX_FIELD_TYPE_INT32},
		},
	},
	[LINX_EVENT_TYPE_USERFAULTFD_E] = {
		"userfaultfd", 0,
		{}
	},
	[LINX_EVENT_TYPE_USERFAULTFD_X] = {
		"userfaultfd", 1,
		{
			{"flags", LINX_FIELD_TYPE_INT32},
		},
	},
	[LINX_EVENT_TYPE_MEMBARRIER_E] = {
		"membarrier", 0,
		{}
	},
	[LINX_EVENT_TYPE_MEMBARRIER_X] = {
		"membarrier", 3,
		{
			{"cmd", LINX_FIELD_TYPE_INT32},
			{"flags", LINX_FIELD_TYPE_UINT32},
			{"cpu_id", LINX_FIELD_TYPE_INT32},
		},
	},
	[LINX_EVENT_TYPE_MLOCK2_E] = {
		"mlock2", 0,
		{}
	},
	[LINX_EVENT_TYPE_MLOCK2_X] = {
		"mlock2", 3,
		{
			{"start", LINX_FIELD_TYPE_UINT64},
			{"len", LINX_FIELD_TYPE_UINT64},
			{"flags", LINX_FIELD_TYPE_INT32},
		},
	},
	[LINX_EVENT_TYPE_COPY_FILE_RANGE_E] = {
		"copy_file_range", 0,
		{}
	},
	[LINX_EVENT_TYPE_COPY_FILE_RANGE_X] = {
		"copy_file_range", 6,
		{
			{"fd_in", LINX_FIELD_TYPE_INT32},
			{"off_in", LINX_FIELD_TYPE_UNKNOWN},
			{"fd_out", LINX_FIELD_TYPE_INT32},
			{"off_out", LINX_FIELD_TYPE_UNKNOWN},
			{"len", LINX_FIELD_TYPE_UINT64},
			{"flags", LINX_FIELD_TYPE_UINT32},
		},
	},
	[LINX_EVENT_TYPE_PREADV2_E] = {
		"preadv2", 0,
		{}
	},
	[LINX_EVENT_TYPE_PREADV2_X] = {
		"preadv2", 6,
		{
			{"fd", LINX_FIELD_TYPE_UINT64},
			{"vec", LINX_FIELD_TYPE_UNKNOWN},
			{"vlen", LINX_FIELD_TYPE_UINT64},
			{"pos_l", LINX_FIELD_TYPE_UINT64},
			{"pos_h", LINX_FIELD_TYPE_UINT64},
			{"flags", LINX_FIELD_TYPE_INT32},
		},
	},
	[LINX_EVENT_TYPE_PWRITEV2_E] = {
		"pwritev2", 0,
		{}
	},
	[LINX_EVENT_TYPE_PWRITEV2_X] = {
		"pwritev2", 6,
		{
			{"fd", LINX_FIELD_TYPE_UINT64},
			{"vec", LINX_FIELD_TYPE_UNKNOWN},
			{"vlen", LINX_FIELD_TYPE_UINT64},
			{"pos_l", LINX_FIELD_TYPE_UINT64},
			{"pos_h", LINX_FIELD_TYPE_UINT64},
			{"flags", LINX_FIELD_TYPE_INT32},
		},
	},
	[LINX_EVENT_TYPE_PKEY_MPROTECT_E] = {
		"pkey_mprotect", 0,
		{}
	},
	[LINX_EVENT_TYPE_PKEY_MPROTECT_X] = {
		"pkey_mprotect", 4,
		{
			{"start", LINX_FIELD_TYPE_UINT64},
			{"len", LINX_FIELD_TYPE_UINT64},
			{"prot", LINX_FIELD_TYPE_UINT64},
			{"pkey", LINX_FIELD_TYPE_INT32},
		},
	},
	[LINX_EVENT_TYPE_PKEY_ALLOC_E] = {
		"pkey_alloc", 0,
		{}
	},
	[LINX_EVENT_TYPE_PKEY_ALLOC_X] = {
		"pkey_alloc", 2,
		{
			{"flags", LINX_FIELD_TYPE_UINT64},
			{"init_val", LINX_FIELD_TYPE_UINT64},
		},
	},
	[LINX_EVENT_TYPE_PKEY_FREE_E] = {
		"pkey_free", 0,
		{}
	},
	[LINX_EVENT_TYPE_PKEY_FREE_X] = {
		"pkey_free", 1,
		{
			{"pkey", LINX_FIELD_TYPE_INT32},
		},
	},
	[LINX_EVENT_TYPE_STATX_E] = {
		"statx", 0,
		{}
	},
	[LINX_EVENT_TYPE_STATX_X] = {
		"statx", 5,
		{
			{"dfd", LINX_FIELD_TYPE_INT32},
			{"filename", LINX_FIELD_TYPE_CHARBUF},
			{"flags", LINX_FIELD_TYPE_UINT32},
			{"mask", LINX_FIELD_TYPE_UINT32},
			{"buffer", LINX_FIELD_TYPE_UNKNOWN},
		},
	},
	[LINX_EVENT_TYPE_IO_PGETEVENTS_E] = {
		"io_pgetevents", 0,
		{}
	},
	[LINX_EVENT_TYPE_IO_PGETEVENTS_X] = {
		"io_pgetevents", 6,
		{
			{"ctx_id", LINX_FIELD_TYPE_UINT64},
			{"min_nr", LINX_FIELD_TYPE_INT64},
			{"nr", LINX_FIELD_TYPE_INT64},
			{"events", LINX_FIELD_TYPE_UNKNOWN},
			{"timeout", LINX_FIELD_TYPE_UNKNOWN},
			{"usig", LINX_FIELD_TYPE_UNKNOWN},
		},
	},
	[LINX_EVENT_TYPE_RSEQ_E] = {
		"rseq", 0,
		{}
	},
	[LINX_EVENT_TYPE_RSEQ_X] = {
		"rseq", 4,
		{
			{"rseq", LINX_FIELD_TYPE_UNKNOWN},
			{"rseq_len", LINX_FIELD_TYPE_UINT32},
			{"flags", LINX_FIELD_TYPE_INT32},
			{"sig", LINX_FIELD_TYPE_UINT32},
		},
	},
	[LINX_EVENT_TYPE_PIDFD_SEND_SIGNAL_E] = {
		"pidfd_send_signal", 0,
		{}
	},
	[LINX_EVENT_TYPE_PIDFD_SEND_SIGNAL_X] = {
		"pidfd_send_signal", 4,
		{
			{"pidfd", LINX_FIELD_TYPE_INT32},
			{"sig", LINX_FIELD_TYPE_INT32},
			{"info", LINX_FIELD_TYPE_UNKNOWN},
			{"flags", LINX_FIELD_TYPE_UINT32},
		},
	},
	[LINX_EVENT_TYPE_IO_URING_SETUP_E] = {
		"io_uring_setup", 0,
		{}
	},
	[LINX_EVENT_TYPE_IO_URING_SETUP_X] = {
		"io_uring_setup", 2,
		{
			{"entries", LINX_FIELD_TYPE_UINT32},
			{"params", LINX_FIELD_TYPE_UNKNOWN},
		},
	},
	[LINX_EVENT_TYPE_IO_URING_ENTER_E] = {
		"io_uring_enter", 0,
		{}
	},
	[LINX_EVENT_TYPE_IO_URING_ENTER_X] = {
		"io_uring_enter", 6,
		{
			{"fd", LINX_FIELD_TYPE_UINT32},
			{"to_submit", LINX_FIELD_TYPE_UINT32},
			{"min_complete", LINX_FIELD_TYPE_UINT32},
			{"flags", LINX_FIELD_TYPE_UINT32},
			{"argp", LINX_FIELD_TYPE_UNKNOWN},
			{"argsz", LINX_FIELD_TYPE_UINT64},
		},
	},
	[LINX_EVENT_TYPE_IO_URING_REGISTER_E] = {
		"io_uring_register", 0,
		{}
	},
	[LINX_EVENT_TYPE_IO_URING_REGISTER_X] = {
		"io_uring_register", 4,
		{
			{"fd", LINX_FIELD_TYPE_UINT32},
			{"opcode", LINX_FIELD_TYPE_UINT32},
			{"arg", LINX_FIELD_TYPE_UNKNOWN},
			{"nr_args", LINX_FIELD_TYPE_UINT32},
		},
	},
	[LINX_EVENT_TYPE_OPEN_TREE_E] = {
		"open_tree", 0,
		{}
	},
	[LINX_EVENT_TYPE_OPEN_TREE_X] = {
		"open_tree", 3,
		{
			{"dfd", LINX_FIELD_TYPE_INT32},
			{"filename", LINX_FIELD_TYPE_CHARBUF},
			{"flags", LINX_FIELD_TYPE_UINT32},
		},
	},
	[LINX_EVENT_TYPE_MOVE_MOUNT_E] = {
		"move_mount", 0,
		{}
	},
	[LINX_EVENT_TYPE_MOVE_MOUNT_X] = {
		"move_mount", 5,
		{
			{"from_dfd", LINX_FIELD_TYPE_INT32},
			{"from_pathname", LINX_FIELD_TYPE_CHARBUF},
			{"to_dfd", LINX_FIELD_TYPE_INT32},
			{"to_pathname", LINX_FIELD_TYPE_CHARBUF},
			{"flags", LINX_FIELD_TYPE_UINT32},
		},
	},
	[LINX_EVENT_TYPE_FSOPEN_E] = {
		"fsopen", 0,
		{}
	},
	[LINX_EVENT_TYPE_FSOPEN_X] = {
		"fsopen", 2,
		{
			{"_fs_name", LINX_FIELD_TYPE_CHARBUF},
			{"flags", LINX_FIELD_TYPE_UINT32},
		},
	},
	[LINX_EVENT_TYPE_FSCONFIG_E] = {
		"fsconfig", 0,
		{}
	},
	[LINX_EVENT_TYPE_FSCONFIG_X] = {
		"fsconfig", 5,
		{
			{"fd", LINX_FIELD_TYPE_INT32},
			{"cmd", LINX_FIELD_TYPE_UINT32},
			{"_key", LINX_FIELD_TYPE_CHARBUF},
			{"_value", LINX_FIELD_TYPE_UNKNOWN},
			{"aux", LINX_FIELD_TYPE_INT32},
		},
	},
	[LINX_EVENT_TYPE_FSMOUNT_E] = {
		"fsmount", 0,
		{}
	},
	[LINX_EVENT_TYPE_FSMOUNT_X] = {
		"fsmount", 3,
		{
			{"fs_fd", LINX_FIELD_TYPE_INT32},
			{"flags", LINX_FIELD_TYPE_UINT32},
			{"attr_flags", LINX_FIELD_TYPE_UINT32},
		},
	},
	[LINX_EVENT_TYPE_FSPICK_E] = {
		"fspick", 0,
		{}
	},
	[LINX_EVENT_TYPE_FSPICK_X] = {
		"fspick", 3,
		{
			{"dfd", LINX_FIELD_TYPE_INT32},
			{"path", LINX_FIELD_TYPE_CHARBUF},
			{"flags", LINX_FIELD_TYPE_UINT32},
		},
	},
	[LINX_EVENT_TYPE_PIDFD_OPEN_E] = {
		"pidfd_open", 0,
		{}
	},
	[LINX_EVENT_TYPE_PIDFD_OPEN_X] = {
		"pidfd_open", 2,
		{
			{"pid", LINX_FIELD_TYPE_INT32},
			{"flags", LINX_FIELD_TYPE_UINT32},
		},
	},
	[LINX_EVENT_TYPE_CLONE3_E] = {
		"clone3", 0,
		{}
	},
	[LINX_EVENT_TYPE_CLONE3_X] = {
		"clone3", 2,
		{
			{"uargs", LINX_FIELD_TYPE_UNKNOWN},
			{"size", LINX_FIELD_TYPE_UINT64},
		},
	},
	[LINX_EVENT_TYPE_CLOSE_RANGE_E] = {
		"close_range", 0,
		{}
	},
	[LINX_EVENT_TYPE_CLOSE_RANGE_X] = {
		"close_range", 3,
		{
			{"fd", LINX_FIELD_TYPE_UINT32},
			{"max_fd", LINX_FIELD_TYPE_UINT32},
			{"flags", LINX_FIELD_TYPE_UINT32},
		},
	},
	[LINX_EVENT_TYPE_OPENAT2_E] = {
		"openat2", 0,
		{}
	},
	[LINX_EVENT_TYPE_OPENAT2_X] = {
		"openat2", 4,
		{
			{"dfd", LINX_FIELD_TYPE_INT32},
			{"filename", LINX_FIELD_TYPE_CHARBUF},
			{"how", LINX_FIELD_TYPE_UNKNOWN},
			{"usize", LINX_FIELD_TYPE_UINT64},
		},
	},
	[LINX_EVENT_TYPE_PIDFD_GETFD_E] = {
		"pidfd_getfd", 0,
		{}
	},
	[LINX_EVENT_TYPE_PIDFD_GETFD_X] = {
		"pidfd_getfd", 3,
		{
			{"pidfd", LINX_FIELD_TYPE_INT32},
			{"fd", LINX_FIELD_TYPE_INT32},
			{"flags", LINX_FIELD_TYPE_UINT32},
		},
	},
	[LINX_EVENT_TYPE_FACCESSAT2_E] = {
		"faccessat2", 0,
		{}
	},
	[LINX_EVENT_TYPE_FACCESSAT2_X] = {
		"faccessat2", 4,
		{
			{"dfd", LINX_FIELD_TYPE_INT32},
			{"filename", LINX_FIELD_TYPE_CHARBUF},
			{"mode", LINX_FIELD_TYPE_INT32},
			{"flags", LINX_FIELD_TYPE_INT32},
		},
	},
	[LINX_EVENT_TYPE_PROCESS_MADVISE_E] = {
		"process_madvise", 0,
		{}
	},
	[LINX_EVENT_TYPE_PROCESS_MADVISE_X] = {
		"process_madvise", 5,
		{
			{"pidfd", LINX_FIELD_TYPE_INT32},
			{"vec", LINX_FIELD_TYPE_UNKNOWN},
			{"vlen", LINX_FIELD_TYPE_UINT64},
			{"behavior", LINX_FIELD_TYPE_INT32},
			{"flags", LINX_FIELD_TYPE_UINT32},
		},
	},
	[LINX_EVENT_TYPE_EPOLL_PWAIT2_E] = {
		"epoll_pwait2", 0,
		{}
	},
	[LINX_EVENT_TYPE_EPOLL_PWAIT2_X] = {
		"epoll_pwait2", 6,
		{
			{"epfd", LINX_FIELD_TYPE_INT32},
			{"events", LINX_FIELD_TYPE_UNKNOWN},
			{"maxevents", LINX_FIELD_TYPE_INT32},
			{"timeout", LINX_FIELD_TYPE_UNKNOWN},
			{"sigmask", LINX_FIELD_TYPE_UNKNOWN},
			{"sigsetsize", LINX_FIELD_TYPE_UINT64},
		},
	},
	[LINX_EVENT_TYPE_MOUNT_SETATTR_E] = {
		"mount_setattr", 0,
		{}
	},
	[LINX_EVENT_TYPE_MOUNT_SETATTR_X] = {
		"mount_setattr", 5,
		{
			{"dfd", LINX_FIELD_TYPE_INT32},
			{"path", LINX_FIELD_TYPE_CHARBUF},
			{"flags", LINX_FIELD_TYPE_UINT32},
			{"uattr", LINX_FIELD_TYPE_UNKNOWN},
			{"usize", LINX_FIELD_TYPE_UINT64},
		},
	},
	[LINX_EVENT_TYPE_QUOTACTL_FD_E] = {
		"quotactl_fd", 0,
		{}
	},
	[LINX_EVENT_TYPE_QUOTACTL_FD_X] = {
		"quotactl_fd", 4,
		{
			{"fd", LINX_FIELD_TYPE_UINT32},
			{"cmd", LINX_FIELD_TYPE_UINT32},
			{"id", LINX_FIELD_TYPE_UINT32},
			{"addr", LINX_FIELD_TYPE_UNKNOWN},
		},
	},
	[LINX_EVENT_TYPE_LANDLOCK_CREATE_RULESET_E] = {
		"landlock_create_ruleset", 0,
		{}
	},
	[LINX_EVENT_TYPE_LANDLOCK_CREATE_RULESET_X] = {
		"landlock_create_ruleset", 3,
		{
			{"attr", LINX_FIELD_TYPE_UNKNOWN},
			{"size", LINX_FIELD_TYPE_UINT64},
			{"flags", LINX_FIELD_TYPE_UINT32},
		},
	},
	[LINX_EVENT_TYPE_LANDLOCK_ADD_RULE_E] = {
		"landlock_add_rule", 0,
		{}
	},
	[LINX_EVENT_TYPE_LANDLOCK_ADD_RULE_X] = {
		"landlock_add_rule", 4,
		{
			{"ruleset_fd", LINX_FIELD_TYPE_INT32},
			{"rule_type", LINX_FIELD_TYPE_UINT32},
			{"rule_attr", LINX_FIELD_TYPE_UNKNOWN},
			{"flags", LINX_FIELD_TYPE_UINT32},
		},
	},
	[LINX_EVENT_TYPE_LANDLOCK_RESTRICT_SELF_E] = {
		"landlock_restrict_self", 0,
		{}
	},
	[LINX_EVENT_TYPE_LANDLOCK_RESTRICT_SELF_X] = {
		"landlock_restrict_self", 2,
		{
			{"ruleset_fd", LINX_FIELD_TYPE_INT32},
			{"flags", LINX_FIELD_TYPE_UINT32},
		},
	},
	[LINX_EVENT_TYPE_MEMFD_SECRET_E] = {
		"memfd_secret", 0,
		{}
	},
	[LINX_EVENT_TYPE_MEMFD_SECRET_X] = {
		"memfd_secret", 1,
		{
			{"flags", LINX_FIELD_TYPE_UINT32},
		},
	},
	[LINX_EVENT_TYPE_PROCESS_MRELEASE_E] = {
		"process_mrelease", 0,
		{}
	},
	[LINX_EVENT_TYPE_PROCESS_MRELEASE_X] = {
		"process_mrelease", 2,
		{
			{"pidfd", LINX_FIELD_TYPE_INT32},
			{"flags", LINX_FIELD_TYPE_UINT32},
		},
	},
	[LINX_EVENT_TYPE_FUTEX_WAITV_E] = {
		"futex_waitv", 0,
		{}
	},
	[LINX_EVENT_TYPE_FUTEX_WAITV_X] = {
		"futex_waitv", 5,
		{
			{"waiters", LINX_FIELD_TYPE_UNKNOWN},
			{"nr_futexes", LINX_FIELD_TYPE_UINT32},
			{"flags", LINX_FIELD_TYPE_UINT32},
			{"timeout", LINX_FIELD_TYPE_UNKNOWN},
			{"clockid", LINX_FIELD_TYPE_INT32},
		},
	},
	[LINX_EVENT_TYPE_SET_MEMPOLICY_HOME_NODE_E] = {
		"set_mempolicy_home_node", 0,
		{}
	},
	[LINX_EVENT_TYPE_SET_MEMPOLICY_HOME_NODE_X] = {
		"set_mempolicy_home_node", 4,
		{
			{"start", LINX_FIELD_TYPE_UINT64},
			{"len", LINX_FIELD_TYPE_UINT64},
			{"home_node", LINX_FIELD_TYPE_UINT64},
			{"flags", LINX_FIELD_TYPE_UINT64},
		},
	},
	[LINX_EVENT_TYPE_CACHESTAT_E] = {
		"cachestat", 0,
		{}
	},
	[LINX_EVENT_TYPE_CACHESTAT_X] = {
		"cachestat", 4,
		{
			{"fd", LINX_FIELD_TYPE_UINT32},
			{"cstat_range", LINX_FIELD_TYPE_UNKNOWN},
			{"cstat", LINX_FIELD_TYPE_UNKNOWN},
			{"flags", LINX_FIELD_TYPE_UINT32},
		},
	},
	[LINX_EVENT_TYPE_FCHMODAT2_E] = {
		"fchmodat2", 0,
		{}
	},
	[LINX_EVENT_TYPE_FCHMODAT2_X] = {
		"fchmodat2", 4,
		{
			{"dfd", LINX_FIELD_TYPE_INT32},
			{"filename", LINX_FIELD_TYPE_CHARBUF},
			{"mode", LINX_FIELD_TYPE_UINT16},
			{"flags", LINX_FIELD_TYPE_UINT32},
		},
	},
};

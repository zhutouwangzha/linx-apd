#ifndef __UTHASH_EXT_H__
#define __UTHASH_EXT_H__

#include "uthash.h"

#define HASH_FIND_INT32(head, findint, out) HASH_FIND(hh, head, findint, sizeof(uint32_t), out)
#define HASH_ADD_INT32(head, intfield, add) HASH_ADD(hh, head, intfield, sizeof(uint32_t), add)
#define HASH_FIND_INT64(head, findint, out) HASH_FIND(hh, head, findint, sizeof(uint64_t), out)
#define HASH_ADD_INT64(head, intfield, add) HASH_ADD(hh, head, intfield, sizeof(uint64_t), add)

#endif /* __UTHASH_EXT_H__ */

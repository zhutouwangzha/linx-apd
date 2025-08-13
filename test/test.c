#define HASH_ADD_INT(head, intfield, add) HASH_ADD(hh, head, intfield, sizeof(int), add)
扩展到 :

do {
    unsigned _ha_hashv;
    do {
        do {
            unsigned _hj_i, _hj_j, _hj_k;
            unsigned const char *_hj_key = (unsigned const char *)(&((fdi)->num));
            _ha_hashv = 0xfeedbeefu;
            _hj_i = _hj_j = 0x9e3779b9u;
            _hj_k = (unsigned)(sizeof(int));
            while (_hj_k >= 12U)
            {
                _hj_i += (_hj_key[0] + ((unsigned)_hj_key[1] << 8) + ((unsigned)_hj_key[2] << 16) + ((unsigned)_hj_key[3] << 24));
                _hj_j += (_hj_key[4] + ((unsigned)_hj_key[5] << 8) + ((unsigned)_hj_key[6] << 16) + ((unsigned)_hj_key[7] << 24));
                _ha_hashv += (_hj_key[8] + ((unsigned)_hj_key[9] << 8) + ((unsigned)_hj_key[10] << 16) + ((unsigned)_hj_key[11] << 24));
                do
                {
                    _hj_i -= _hj_j;
                    _hj_i -= _ha_hashv;
                    _hj_i ^= (_ha_hashv >> 13);
                    _hj_j -= _ha_hashv;
                    _hj_j -= _hj_i;
                    _hj_j ^= (_hj_i << 8);
                    _ha_hashv -= _hj_i;
                    _ha_hashv -= _hj_j;
                    _ha_hashv ^= (_hj_j >> 13);
                    _hj_i -= _hj_j;
                    _hj_i -= _ha_hashv;
                    _hj_i ^= (_ha_hashv >> 12);
                    _hj_j -= _ha_hashv;
                    _hj_j -= _hj_i;
                    _hj_j ^= (_hj_i << 16);
                    _ha_hashv -= _hj_i;
                    _ha_hashv -= _hj_j;
                    _ha_hashv ^= (_hj_j >> 5);
                    _hj_i -= _hj_j;
                    _hj_i -= _ha_hashv;
                    _hj_i ^= (_ha_hashv >> 3);
                    _hj_j -= _ha_hashv;
                    _hj_j -= _hj_i;
                    _hj_j ^= (_hj_i << 10);
                    _ha_hashv -= _hj_i;
                    _ha_hashv -= _hj_j;
                    _ha_hashv ^= (_hj_j >> 15);
                } while (0);
                _hj_key += 12;
                _hj_k -= 12U;
            }
            _ha_hashv += (unsigned)(sizeof(int));
            switch (_hj_k)
            {
            case 11:
                _ha_hashv += ((unsigned)_hj_key[10] << 24);
            case 10:
                _ha_hashv += ((unsigned)_hj_key[9] << 16);
            case 9:
                _ha_hashv += ((unsigned)_hj_key[8] << 8);
            case 8:
                _hj_j += ((unsigned)_hj_key[7] << 24);
            case 7:
                _hj_j += ((unsigned)_hj_key[6] << 16);
            case 6:
                _hj_j += ((unsigned)_hj_key[5] << 8);
            case 5:
                _hj_j += _hj_key[4];
            case 4:
                _hj_i += ((unsigned)_hj_key[3] << 24);
            case 3:
                _hj_i += ((unsigned)_hj_key[2] << 16);
            case 2:
                _hj_i += ((unsigned)_hj_key[1] << 8);
            case 1:
                _hj_i += _hj_key[0];
            default:;
            }
            do
            {
                _hj_i -= _hj_j;
                _hj_i -= _ha_hashv;
                _hj_i ^= (_ha_hashv >> 13);
                _hj_j -= _ha_hashv;
                _hj_j -= _hj_i;
                _hj_j ^= (_hj_i << 8);
                _ha_hashv -= _hj_i;
                _ha_hashv -= _hj_j;
                _ha_hashv ^= (_hj_j >> 13);
                _hj_i -= _hj_j;
                _hj_i -= _ha_hashv;
                _hj_i ^= (_ha_hashv >> 12);
                _hj_j -= _ha_hashv;
                _hj_j -= _hj_i;
                _hj_j ^= (_hj_i << 16);
                _ha_hashv -= _hj_i;
                _ha_hashv -= _hj_j;
                _ha_hashv ^= (_hj_j >> 5);
                _hj_i -= _hj_j;
                _hj_i -= _ha_hashv;
                _hj_i ^= (_ha_hashv >> 3);
                _hj_j -= _ha_hashv;
                _hj_j -= _hj_i;
                _hj_j ^= (_hj_i << 10);
                _ha_hashv -= _hj_i;
                _ha_hashv -= _hj_j;
                _ha_hashv ^= (_hj_j >> 15);
            } while (0);
        } while (0);
    } while (0);
    do
    {
        (fdi)->hh.hashv = (_ha_hashv);
        (fdi)->hh.key = (const void *)(&((fdi)->num));
        (fdi)->hh.keylen = (unsigned)(sizeof(int));
        if (!(info->fdlist))
        {
            (fdi)->hh.next = ((void *)0);
            (fdi)->hh.prev = ((void *)0);
            do
            {
                (fdi)->hh.tbl = (UT_hash_table *)malloc(sizeof(UT_hash_table));
                if (!(fdi)->hh.tbl)
                {
                    exit(-1);
                }
                else
                {
                    memset((fdi)->hh.tbl, '\0', sizeof(UT_hash_table));
                    (fdi)->hh.tbl->tail = &((fdi)->hh);
                    (fdi)->hh.tbl->num_buckets = 32U;
                    (fdi)->hh.tbl->log2_num_buckets = 5U;
                    (fdi)->hh.tbl->hho = (char *)(&(fdi)->hh) - (char *)(fdi);
                    (fdi)->hh.tbl->buckets = (UT_hash_bucket *)malloc(32U * sizeof(struct UT_hash_bucket));
                    (fdi)->hh.tbl->signature = 0xa0111fe1u;
                    if (!(fdi)->hh.tbl->buckets)
                    {
                        exit(-1);
                        free((fdi)->hh.tbl);
                    }
                    else
                    {
                        memset((fdi)->hh.tbl->buckets, '\0', 32U * sizeof(struct UT_hash_bucket));
                        ;
                    }
                }
            } while (0);
            (info->fdlist) = (fdi);
        }
        else
        {
            (fdi)->hh.tbl = (info->fdlist)->hh.tbl;
            do
            {
                (fdi)->hh.next = ((void *)0);
                (fdi)->hh.prev = ((void *)(((char *)((info->fdlist)->hh.tbl->tail)) - (((info->fdlist)->hh.tbl)->hho)));
                (info->fdlist)->hh.tbl->tail->next = (fdi);
                (info->fdlist)->hh.tbl->tail = &((fdi)->hh);
            } while (0);
        }
        do
        {
            unsigned _ha_bkt;
            (info->fdlist)->hh.tbl->num_items++;
            do
            {
                _ha_bkt = ((_ha_hashv) & (((info->fdlist)->hh.tbl->num_buckets) - 1U));
            } while (0);
            do
            {
                UT_hash_bucket *_ha_head = &((info->fdlist)->hh.tbl->buckets[_ha_bkt]);
                _ha_head->count++;
                (&(fdi)->hh)->hh_next = _ha_head->hh_head;
                (&(fdi)->hh)->hh_prev = ((void *)0);
                if (_ha_head->hh_head != ((void *)0))
                {
                    _ha_head->hh_head->hh_prev = (&(fdi)->hh);
                }
                _ha_head->hh_head = (&(fdi)->hh);
                if ((_ha_head->count >= ((_ha_head->expand_mult + 1U) * 10U)) && !(&(fdi)->hh)->tbl->noexpand)
                {
                    do
                    {
                        unsigned _he_bkt;
                        unsigned _he_bkt_i;
                        struct UT_hash_handle *_he_thh, *_he_hh_nxt;
                        UT_hash_bucket *_he_new_buckets, *_he_newbkt;
                        _he_new_buckets = (UT_hash_bucket *)malloc(sizeof(struct UT_hash_bucket) * ((&(fdi)->hh)->tbl)->num_buckets * 2U);
                        if (!_he_new_buckets)
                        {
                            exit(-1);
                        }
                        else
                        {
                            memset(_he_new_buckets, '\0', sizeof(struct UT_hash_bucket) * ((&(fdi)->hh)->tbl)->num_buckets * 2U);
                            ((&(fdi)->hh)->tbl)->ideal_chain_maxlen = (((&(fdi)->hh)->tbl)->num_items >> (((&(fdi)->hh)->tbl)->log2_num_buckets + 1U)) + (((((&(fdi)->hh)->tbl)->num_items & ((((&(fdi)->hh)->tbl)->num_buckets * 2U) - 1U)) != 0U) ? 1U : 0U);
                            ((&(fdi)->hh)->tbl)->nonideal_items = 0;
                            for (_he_bkt_i = 0; _he_bkt_i < ((&(fdi)->hh)->tbl)->num_buckets; _he_bkt_i++)
                            {
                                _he_thh = ((&(fdi)->hh)->tbl)->buckets[_he_bkt_i].hh_head;
                                while (_he_thh != ((void *)0))
                                {
                                    _he_hh_nxt = _he_thh->hh_next;
                                    do
                                    {
                                        _he_bkt = ((_he_thh->hashv) & ((((&(fdi)->hh)->tbl)->num_buckets * 2U) - 1U));
                                    } while (0);
                                    _he_newbkt = &(_he_new_buckets[_he_bkt]);
                                    if (++(_he_newbkt->count) > ((&(fdi)->hh)->tbl)->ideal_chain_maxlen)
                                    {
                                        ((&(fdi)->hh)->tbl)->nonideal_items++;
                                        if (_he_newbkt->count > _he_newbkt->expand_mult * ((&(fdi)->hh)->tbl)->ideal_chain_maxlen)
                                        {
                                            _he_newbkt->expand_mult++;
                                        }
                                    }
                                    _he_thh->hh_prev = ((void *)0);
                                    _he_thh->hh_next = _he_newbkt->hh_head;
                                    if (_he_newbkt->hh_head != ((void *)0))
                                    {
                                        _he_newbkt->hh_head->hh_prev = _he_thh;
                                    }
                                    _he_newbkt->hh_head = _he_thh;
                                    _he_thh = _he_hh_nxt;
                                }
                            }
                            free(((&(fdi)->hh)->tbl)->buckets);
                            ((&(fdi)->hh)->tbl)->num_buckets *= 2U;
                            ((&(fdi)->hh)->tbl)->log2_num_buckets++;
                            ((&(fdi)->hh)->tbl)->buckets = _he_new_buckets;
                            ((&(fdi)->hh)->tbl)->ineff_expands = (((&(fdi)->hh)->tbl)->nonideal_items > (((&(fdi)->hh)->tbl)->num_items >> 1)) ? (((&(fdi)->hh)->tbl)->ineff_expands + 1U) : 0U;
                            if (((&(fdi)->hh)->tbl)->ineff_expands > 1U)
                            {
                                ((&(fdi)->hh)->tbl)->noexpand = 1;
                                ;
                            };
                        }
                    } while (0);
                }
            } while (0);
            ;
            ;
        } while (0);
        ;
    } while (0);
}
while (0)
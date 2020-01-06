#include "rbtree.h"
#include "pool.h"
#include "timestamp.h"
#include "hash.h"

#define LOOP	    32
#define MIN_COUNT   64
#define MAX_COUNT   (LOOP * MIN_COUNT)
#define STRSIZE	    16


typedef struct node_key
{
    uint32_t hash;
    char str[STRSIZE];
} node_key;

static int compare_node(const node_key *k1, const node_key *k2)
{
    if (k1->hash == k2->hash) 
		return strcmp(k1->str, k2->str);
    else 
		return k1->hash	< k2->hash ? -1 : 1;
}

void randomize_string(char *str, int len)
{
    int i;
    for (i=0; i<len-1; ++i)
		str[i] = (char)('a' + rand() % 26);
    str[len-1] = '\0';
}

static int rbtree_test(void)
{
    T_RBTree rb;
    node_key *key;
    T_RBTreeNode *node;
    int err = 0;
    int count = MIN_COUNT;
    int i;
    unsigned size;

    RBTREE_Init(&rb, (RBTREE_COMP*)&compare_node);
    size = MAX_COUNT * (sizeof(*key) + RBTREE_NODE_SIZE) + RBTREE_SIZE + sizeof(T_PoolInfo);
	
	THREAD_Init();
    T_PoolInfo *pool;
	T_PoolCaching cp;
	POOL_CachingInit(&cp, NULL, 0);	
    pool = POOL_Create(&cp.in_pFactory, "rbtree", size, 0, NULL);	

    key = (node_key *)POOL_Alloc(pool, MAX_COUNT * sizeof(*key));
    if (!key) return -20;

    node = (T_RBTreeNode*)POOL_Alloc(pool, MAX_COUNT*sizeof(*node));
    if (!node) return -30;

    for (i = 0; i < LOOP; ++i){
		int j;
		T_RBTreeNode *prev, *it;
		T_Timestamp t1, t2, t_setup, t_insert, t_search, t_erase;

		assert(rb.size == 0);

		t_setup.u32.lo = t_insert.u32.lo = t_search.u32.lo = t_erase.u32.lo = 0;

		for (j = 0; j < count; j++){
			randomize_string(key[j].str, STRSIZE);

			TIME_GetTimestamp(&t1);
			node[j].key = &key[j];
			node[j].user_data = key[j].str;
			key[j].hash = HASH_Calc(0, key[j].str, HASH_KEY_TYPE_STRING);
			TIME_GetTimestamp(&t2);
			t_setup.u32.lo += (t2.u32.lo - t1.u32.lo);

			TIME_GetTimestamp(&t1);
			RBTREE_Insert(&rb, &node[j]);
			TIME_GetTimestamp(&t2);
			t_insert.u32.lo += (t2.u32.lo - t1.u32.lo);
		}

		assert(rb.size == (unsigned)count);

		// Iterate key, make sure they're sorted.
		prev = NULL;
		it = RBTREE_First(&rb);
		while(it){
			if (prev){
				if (compare_node((node_key*)prev->key, (node_key*)it->key)>=0){
					++err;
					printf("Error: %s >= %s\r\n", (char*)prev->user_data, (char*)it->user_data);
				}
			}
			prev = it;
			it = RBTREE_Next(&rb, it);
		}

		// Search.
		for (j = 0; j < count; j++){
			TIME_GetTimestamp(&t1);
			it = RBTREE_Find(&rb, &key[j]);
			TIME_GetTimestamp(&t2);
			t_search.u32.lo += (t2.u32.lo - t1.u32.lo);

			assert(it != NULL);
			if (it == NULL)
				++err;
		}

		// Erase node.
		for (j = 0; j < count; j++){
			TIME_GetTimestamp(&t1);
			it = RBTREE_Erase(&rb, &node[j]);
			TIME_GetTimestamp(&t2);
			t_erase.u32.lo += (t2.u32.lo - t1.u32.lo);
		}

		printf("count:%d, setup:%d, insert:%d, search:%d, erase:%d\r\n",
				count,
				t_setup.u32.lo, t_insert.u32.lo,
				t_search.u32.lo, t_erase.u32.lo);

		count = 2 * count;
		if (count > MAX_COUNT)
			break;
    }

    POOL_Destroy(pool);
    return err;
}




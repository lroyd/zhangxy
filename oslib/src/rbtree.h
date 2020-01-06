#ifndef __ZXY_RBTREE_H__
#define __ZXY_RBTREE_H__

#include "typedefs.h"

enum
{
    RBCOLOR_BLACK,
    RBCOLOR_RED
};

typedef struct _tagRBTreeNode 
{
    struct _tagRBTreeNode *parent, *left, *right;
    const void	*key;
    void		*user_data;
    int			color;
}T_RBTreeNode;

typedef int RBTREE_COMP(const void *key1, const void *key2);

typedef struct _tagRBTree
{
    T_RBTreeNode null_node;   /**< Constant to indicate NULL node.    */
    T_RBTreeNode *null;       /**< Constant to indicate NULL node.    */
    T_RBTreeNode *root;       /**< Root tree node.                    */
    unsigned size;              /**< Number of elements in the tree.    */
    RBTREE_COMP *comp;       /**< Key comparison function.           */
}T_RBTree;

#define RBTREE_NODE_SIZE		(sizeof(T_RBTreeNode))
#define RBTREE_SIZE				(sizeof(T_RBTree))

API_DEF(void)			RBTREE_Init(T_RBTree *tree, RBTREE_COMP *comp);
API_DEF(T_RBTreeNode*)	RBTREE_First(T_RBTree *tree);
API_DEF(T_RBTreeNode*)	RBTREE_Last(T_RBTree *tree);
API_DEF(T_RBTreeNode*)	RBTREE_Next(T_RBTree *tree, T_RBTreeNode *node);
API_DEF(T_RBTreeNode*)	RBTREE_Prev(T_RBTree *tree, T_RBTreeNode *node);
API_DEF(int)			RBTREE_Insert(T_RBTree *tree, T_RBTreeNode *node);
API_DEF(T_RBTreeNode*)	RBTREE_Find(T_RBTree *tree, const void *key);
API_DEF(T_RBTreeNode*)	RBTREE_Erase(T_RBTree *tree, T_RBTreeNode *node);
API_DEF(unsigned)		RBTREE_HeightMax(T_RBTree *tree,T_RBTreeNode *node);
API_DEF(unsigned)		RBTREE_HeightMin(T_RBTree *tree, T_RBTreeNode *node);


#endif	/* __ZXY_RBTREE_H__ */


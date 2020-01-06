#include "rbtree.h"

#define CHECK_STACK()

static void left_rotate(T_RBTree *tree, T_RBTreeNode *node) 
{
    T_RBTreeNode *rnode, *parent;
    CHECK_STACK();
    rnode = node->right;
    if (rnode == tree->null) return;
    node->right = rnode->left;
    if (rnode->left != tree->null)
        rnode->left->parent = node;
    parent = node->parent;
    rnode->parent = parent;
    if (parent != tree->null){
		if (parent->left == node)
			parent->left = rnode;
		else
			parent->right = rnode;
    }
	else
        tree->root = rnode;
    rnode->left = node;
    node->parent = rnode;
}

static void right_rotate(T_RBTree *tree, T_RBTreeNode *node) 
{
    T_RBTreeNode *lnode, *parent;
    CHECK_STACK();
    lnode = node->left;
    if (lnode == tree->null) return;
    node->left = lnode->right;
    if (lnode->right != tree->null)
		lnode->right->parent = node;
    parent = node->parent;
    lnode->parent = parent;

    if (parent != tree->null){
        if (parent->left == node)
			parent->left = lnode;
		else
			parent->right = lnode;
    }
	else
        tree->root = lnode;
    lnode->right = node;
    node->parent = lnode;
}

static void insert_fixup(T_RBTree *tree, T_RBTreeNode *node) 
{
    T_RBTreeNode *temp, *parent;
    CHECK_STACK();
    while (node != tree->root && node->parent->color == RBCOLOR_RED){
        parent = node->parent;
        if (parent == parent->parent->left){
			temp = parent->parent->right;
			if (temp->color == RBCOLOR_RED){
				temp->color = RBCOLOR_BLACK;
				node = parent;
				node->color = RBCOLOR_BLACK;
				node = node->parent;
				node->color = RBCOLOR_RED;
			} 
			else{
				if (node == parent->right){
					node = parent;
					left_rotate(tree, node);
				}
				temp = node->parent;
				temp->color = RBCOLOR_BLACK;
				temp = temp->parent;
				temp->color = RBCOLOR_RED;
				right_rotate( tree, temp);
			}
        } 
		else{
			temp = parent->parent->left;
			if (temp->color == RBCOLOR_RED){
				temp->color = RBCOLOR_BLACK;
				node = parent;
				node->color = RBCOLOR_BLACK;
				node = node->parent;
				node->color = RBCOLOR_RED;
			} 
			else{
				if (node == parent->left){
					node = parent;
					right_rotate(tree, node);
				}
				temp = node->parent;
				temp->color = RBCOLOR_BLACK;
				temp = temp->parent;
				temp->color = RBCOLOR_RED;
				left_rotate(tree, temp);
			}
        }
    }
    tree->root->color = RBCOLOR_BLACK;
}


static void delete_fixup(T_RBTree *tree, T_RBTreeNode *node)
{
    T_RBTreeNode *temp;
    CHECK_STACK();
    while (node != tree->root && node->color == RBCOLOR_BLACK){
        if (node->parent->left == node){
			temp = node->parent->right;
			if (temp->color == RBCOLOR_RED){
				temp->color = RBCOLOR_BLACK;
				node->parent->color = RBCOLOR_RED;
				left_rotate(tree, node->parent);
				temp = node->parent->right;
			}
			if (temp->left->color == RBCOLOR_BLACK && temp->right->color == RBCOLOR_BLACK){
				temp->color = RBCOLOR_RED;
				node = node->parent;
			} 
			else{
				if (temp->right->color == RBCOLOR_BLACK){
					temp->left->color = RBCOLOR_BLACK;
					temp->color = RBCOLOR_RED;
					right_rotate( tree, temp);
					temp = node->parent->right;
				}
				temp->color = node->parent->color;
				temp->right->color = RBCOLOR_BLACK;
				node->parent->color = RBCOLOR_BLACK;
				left_rotate(tree, node->parent);
				node = tree->root;
			}
        }
		else{
			temp = node->parent->left;
			if (temp->color == RBCOLOR_RED){
				temp->color = RBCOLOR_BLACK;
				node->parent->color = RBCOLOR_RED;
				right_rotate(tree, node->parent);
				temp = node->parent->left;
			}
			if (temp->right->color == RBCOLOR_BLACK && temp->left->color == RBCOLOR_BLACK){
				temp->color = RBCOLOR_RED;
				node = node->parent;
			} 
			else{
				if (temp->left->color == RBCOLOR_BLACK){
					temp->right->color = RBCOLOR_BLACK;
					temp->color = RBCOLOR_RED;
					left_rotate( tree, temp);
					temp = node->parent->left;
				}
				temp->color = node->parent->color;
				node->parent->color = RBCOLOR_BLACK;
				temp->left->color = RBCOLOR_BLACK;
				right_rotate(tree, node->parent);
				node = tree->root;
			}
        }
    }
    node->color = RBCOLOR_BLACK;
}


void RBTREE_Init(T_RBTree *tree, RBTREE_COMP *comp)
{
    CHECK_STACK();
    tree->null = tree->root = &tree->null_node;
    tree->null->key = NULL;
    tree->null->user_data = NULL;
    tree->size = 0;
    tree->null->left = tree->null->right = tree->null->parent = tree->null;
    tree->null->color = RBCOLOR_BLACK;
    tree->comp = comp;
}

T_RBTreeNode* RBTREE_First(T_RBTree *tree)
{
    register T_RBTreeNode *node = tree->root;
    register T_RBTreeNode *null = tree->null;
    CHECK_STACK();
    while (node->left != null) node = node->left;
    return node != null ? node : NULL;
}

T_RBTreeNode* RBTREE_Last(T_RBTree *tree)
{
    register T_RBTreeNode *node = tree->root;
    register T_RBTreeNode *null = tree->null;
    CHECK_STACK();
    while (node->right != null) node = node->right;
    return node != null ? node : NULL;
}

T_RBTreeNode* RBTREE_Next(T_RBTree *tree, register T_RBTreeNode *node)
{
    register T_RBTreeNode *null = tree->null;
    CHECK_STACK();
    if (node->right != null){
		for (node = node->right; node->left != null; node = node->left)
	    /* void */;
    } 
	else{
        register T_RBTreeNode *temp = node->parent;
        while (temp!=null && temp->right==node){
			node = temp;
			temp = temp->parent;
		}
		node = temp;
    }    
    return node != null ? node : NULL;
}

T_RBTreeNode* RBTREE_Prev(T_RBTree *tree, register T_RBTreeNode *node)
{
    register T_RBTreeNode *null = tree->null;
    CHECK_STACK();
    if (node->left != null){
        for (node=node->left; node->right!=null; node=node->right)
	   /* void */;
    } 
	else{
        register T_RBTreeNode *temp = node->parent;
        while (temp!=null && temp->left==node){
			node = temp;
			temp = temp->parent;
        }
        node = temp;
    }    
    return node != null ? node : NULL;
}

int RBTREE_Insert(T_RBTree *tree, T_RBTreeNode *element)
{
    int rv = 0;
    T_RBTreeNode *node, *parent = tree->null, 
		   *null = tree->null;
    RBTREE_COMP *comp = tree->comp;
	
    CHECK_STACK();

    node = tree->root;	
    while (node != null){
        rv = (*comp)(element->key, node->key);
        if (rv == 0){
			/* found match, i.e. entry with equal key already exist */
			return -1;
		}    
		parent = node;
        node = rv < 0 ? node->left : node->right;
    }

    element->color = RBCOLOR_RED;
    element->left = element->right = null;

    node = element;
    if (parent != null){
		node->parent = parent;
		if (rv < 0)
			parent->left = node;
		else
			parent->right = node;
		insert_fixup( tree, node);
    } 
	else{
        tree->root = node;
        node->parent = null;
        node->color = RBCOLOR_BLACK;
    }
	
    ++tree->size;
    return 0;
}


T_RBTreeNode* RBTREE_Find(T_RBTree *tree, const void *key)
{
    int rv;
    T_RBTreeNode *node = tree->root;
    T_RBTreeNode *null = tree->null;
    RBTREE_COMP *comp = tree->comp;
    
    while (node != null){
        rv = (*comp)(key, node->key);
        if (rv == 0)
			return node;
        node = rv < 0 ? node->left : node->right;
    }
    return node != null ? node : NULL;
}

T_RBTreeNode* RBTREE_Erase(T_RBTree *tree, T_RBTreeNode *node)
{
    T_RBTreeNode *succ;
    T_RBTreeNode *null = tree->null;
    T_RBTreeNode *child;
    T_RBTreeNode *parent;
    
    CHECK_STACK();
    if (node->left == null || node->right == null){
        succ = node;
    } 
	else{
        for (succ = node->right; succ->left != null; succ = succ->left)
	   /* void */;
    }

    child = succ->left != null ? succ->left : succ->right;
    parent = succ->parent;
    child->parent = parent;
    
    if (parent != null){
		if (parent->left == succ)
			parent->left = child;
		else
			parent->right = child;
    } else
        tree->root = child;

    if (succ != node){
		succ->parent = node->parent;
		succ->left = node->left;
		succ->right = node->right;
		succ->color = node->color;
		parent = node->parent;
		if (parent != null){
			if (parent->left==node)
				parent->left=succ;
			else
				parent->right=succ;
		}
		if (node->left != null)
			node->left->parent = succ;;
		if (node->right != null)
			node->right->parent = succ;
		if (tree->root == node)
			tree->root = succ;
    }

    if (succ->color == RBCOLOR_BLACK){
		if (child != null) 
			delete_fixup(tree, child);
		tree->null->color = RBCOLOR_BLACK;
    }
    --tree->size;
    return node;
}


unsigned RBTREE_HeightMax(T_RBTree *tree, T_RBTreeNode *node)
{
    unsigned l, r;
    CHECK_STACK();
    if (node==NULL) 
		node = tree->root;
    
    l = node->left != tree->null ? RBTREE_HeightMax(tree,node->left)+1 : 0;
    r = node->right != tree->null ? RBTREE_HeightMax(tree,node->right)+1 : 0;
    return l > r ? l : r;
}

unsigned RBTREE_HeightMin(T_RBTree *tree, T_RBTreeNode *node)
{
    unsigned l, r;
    CHECK_STACK();
    if (node == NULL) node = tree->root;
    l = (node->left != tree->null) ? RBTREE_HeightMax(tree,node->left)+1 : 0;
    r = (node->right != tree->null) ? RBTREE_HeightMax(tree,node->right)+1 : 0;
    return l > r ? r : l;
}



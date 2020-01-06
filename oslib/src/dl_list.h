/*******************************************************************************
	> File Name: 
	> Author: lroyd
	> Mail: htzhangxmu@163.com
	> Created Time: 
 *******************************************************************************/
#ifndef __DL_LIST_H__
#define __DL_LIST_H__

#include <stdlib.h>

typedef struct dl_list 
{
	struct dl_list *next;
	struct dl_list *prev;
}T_DoubleList;

#define DL_LIST_HEAD_INIT(l) { &(l), &(l) }

static inline void DL_ListInit(T_DoubleList *_pList)
{
	_pList->next = _pList;
	_pList->prev = _pList;
}

static inline void DL_ListAdd(T_DoubleList *_pList, T_DoubleList *_pItem)
{
	_pItem->next = _pList->next;
	_pItem->prev = _pList;
	_pList->next->prev = _pItem;
	_pList->next = _pItem;
}

static inline void DL_ListAddTail(T_DoubleList *_pList, T_DoubleList *_pItem)
{
	DL_ListAdd(_pList->prev, _pItem);
}

static inline void DL_ListAddDelete(T_DoubleList *_pItem)
{
	_pItem->next->prev = _pItem->prev;
	_pItem->prev->next = _pItem->next;
	_pItem->next = NULL;
	_pItem->prev = NULL;
}

static inline int DL_ListIsEmpty(T_DoubleList *_pList)
{
	return _pList->next == _pList;
}

static inline unsigned int DL_ListGetLen(T_DoubleList *_pList)
{
	T_DoubleList *pItem;
	int count = 0;
	for (pItem = _pList->next; pItem != _pList; pItem = pItem->next)
		count++;
	return count;
}




#ifndef offsetof
#define offsetof(type, member) ((long) &((type *) 0)->member)
#endif

#define DL_ListEntryInfo(item, type, member) \
	((type *) ((char *) item - offsetof(type, member)))

//获取当前节点的下一个 = next
#define DL_ListGetNext(list, type, member) \
	(DL_ListIsEmpty((list)) ? NULL : \
	 DL_ListEntryInfo((list)->next, type, member))

//获取当前节点的上一个 = prev
#define DL_ListGetPrev(list, type, member) \
	(DL_ListIsEmpty((list)) ? NULL : \
	 DL_ListEntryInfo((list)->prev, type, member))
 

	 
/* 顺序取出 */
//1.item:取出的元素
//2.list:链表的头
//3.type:取出元素的结构体
//4.member:取出元素的结构体中的list成员名字
#define DL_ListForeach(item, list, type, member) \
	for (item = DL_ListEntryInfo((list)->next, type, member); \
	     &item->member != (list); \
	     item = DL_ListEntryInfo(item->member.next, type, member))
//n:和item一样缓存
#define DL_ListForeachSafe(item, n, list, type, member) \
	for (item = DL_ListEntryInfo((list)->next, type, member), \
		     n = DL_ListEntryInfo(item->member.next, type, member); \
	     &item->member != (list); \
	     item = n, n = DL_ListEntryInfo(n->member.next, type, member))
/* 倒序取出 */
#define DL_ListForeachReverse(item, list, type, member) \
	for (item = DL_ListEntryInfo((list)->prev, type, member); \
	     &item->member != (list); \
	     item = DL_ListEntryInfo(item->member.prev, type, member))

//直接定义，包括初始化
#define DEFINE_DL_LIST(name) \
	struct dl_list name = { &(name), &(name) }

#endif 

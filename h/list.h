/* Copyright (C) 2013-2014, Hsiang Kao (e0e1e) <esxgx@163.com>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#ifndef __LIST_H
#define __LIST_H


// singly-linked list
//
typedef void *		_list_node_s;

#define __plist_node_s__(x)	((_list_node_s *)(x))

#define singly_linked_list_go(x)	(*__plist_node_s__(x))

#define singly_linked_list_the_head	singly_linked_list_go
#define singly_linked_list_the_end(x)	((void *)0)

#define singly_linked_list_insert_after(x, y)	do{\
	singly_linked_list_go(y) = singly_linked_list_go(x); \
	singly_linked_list_go(x) = (void *)(y);	\
}while(0)

#define singly_linked_list_remove_after(x)	\
	(singly_linked_list_go(x) = singly_linked_list_go(singly_linked_list_go(x)))



// doubly-linked list
//
#include "record.h"

_record(list_node_d)
	void *next, *prev;
_end_record(list_node_d)

#define doubly_linked_list_pre_initialise_fake(x)	{NULL, NULL}
#define doubly_linked_list_pre_initialise(x)		{&x, &x}

#define __plist_node_d__(x)	((_list_node_d *)(x))
#define doubly_linked_list_go(x)	(__plist_node_d__(x)->next)
#define doubly_linked_list_back(x)	(__plist_node_d__(x)->prev)

#define doubly_linked_list_the_head	singly_linked_list_go
#define doubly_linked_list_the_tail	doubly_linked_list_back
#define doubly_linked_list_the_end(x)	((void *)(x))

#define doubly_linked_list_insert_after(x, y) do{ \
	_list_node_d *forw = __plist_node_d__(doubly_linked_list_go(x));	\
	__plist_node_d__(y)->next = (void *)forw;	\
	__plist_node_d__(y)->prev = (void *)(x);	\
	__plist_node_d__(x)->next = forw->prev = (void *)(y);	\
}while(0)

#define doubly_linked_list_insert_before(x, y) do{ \
	_list_node_d *backw = __plist_node_d__(doubly_linked_list_back(x));	\
	__plist_node_d__(y)->prev = (void *)backw;	\
	__plist_node_d__(y)->next = (void *)(x);	\
	__plist_node_d__(x)->prev = backw->next = (void *)(y);	\
}while(0)

#define doubly_linked_list_remove(x) do {\
	_list_node_d *backw = __plist_node_d__(doubly_linked_list_back(x));	\
	_list_node_d *forw = __plist_node_d__(doubly_linked_list_go(x));	\
	backw->next = (void *)forw;	\
	forw->prev = (void *)backw;	\
}while(0)



// In addition, we introduce a c-type called "_list_node".
// _list_node is sufficiently large to store all kinds of the linked list node.
//
typedef _list_node_d	_list_node;
#define linked_list_pre_initialise_singly	doubly_linked_list_pre_initialise_fake
#define linked_list_pre_initialise_doubly	doubly_linked_list_pre_initialise

#endif

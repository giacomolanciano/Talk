/*
 * stack.h
 *
 *  Created on: 31/mar/2015
 *      Author: davide & giacomo
 */


#ifndef	__STACK__
#define __STACK__

typedef struct stack stack;

stack* 	stack_new();								//costruttore
void 	stack_delete(stack* s);						//distruttore
void 	stack_push(stack* s, const unsigned elem);
void 	stack_pop(stack* s);
int		stack_top(stack* s, unsigned* elem);
int		stack_empty(stack* s);

#endif

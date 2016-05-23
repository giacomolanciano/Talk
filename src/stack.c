/*
 * stack.c
 *
 *  Created on: 31/mar/2015
 *      Author: davide & giacomo
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>			//per utilizzare assert()

#include "stack.h"
#include "list.h"

struct nodo {
	unsigned elem;
	nodo* next;
};

//i metodi della lista NON effettuano side effect sul puntatore alla testa della lista

nodo* list_aggiungi_testa(nodo* l, unsigned elem) {

	nodo* p = malloc(sizeof(nodo));
	assert(p!= NULL);
	p->elem = elem;
	p->next = l;

	return p;
}

nodo* list_rimuovi_testa(nodo* l) {

	assert(l!=NULL);
	nodo* p = l->next;
	free(l);

	return p;
}

void list_dealloca(nodo* l) {

	while (l != NULL) {
		l = list_rimuovi_testa(l);
	}
}

struct stack {
	//scegliamo implementazione basata su lista collegata

	nodo* testa;
	unsigned dimensione;
};

stack* stack_new() {

	stack* s = malloc(sizeof(stack));
	assert(s != NULL);
	s->testa = NULL;
	s->dimensione = 0;

	return s;
}

void stack_delete(stack* s) {

	list_dealloca(s->testa);
	free(s);
}

void stack_push(stack* s, const unsigned elem) {

	s->testa = list_aggiungi_testa(s->testa, elem);
	s->dimensione++;
}

void stack_pop(stack* s) {

	assert(!stack_empty(s));
	s->testa = list_rimuovi_testa(s->testa);
	s->dimensione--;
}

int stack_top(stack* s, unsigned* elem) {
	if(stack_empty(s)) {
		return -1;
	}
	int val = s->testa->elem;
	memcpy(elem, &val, sizeof(unsigned));
	return 0;
}

int stack_empty(stack* s) {
	return s->dimensione == 0;
}





void stampa_stack(stack* s) {

	nodo* aux = s->testa;
	if(aux == NULL) {
		printf("stack vuota");
	}
	else {
		while (aux != NULL) {
			printf("%d ", aux->elem);
			aux = aux->next;
		}
	}

	printf("\n");
}




/*

void main() {

	stack* s = stack_new();

	stack_push(s, 1);
	stack_push(s, 2);
	stack_push(s, 3);
	stack_push(s, 4);

	printf("Stack: ");
	stampa_stack(s);

	printf("\n");

	stack_pop(s);
	printf("toglie il 4, la cima è 3\n");
	stack_pop(s);
	printf("toglie il 3, la cima è 2\n");
	printf("Stack: ");
	stampa_stack(s);

	printf("\n");

	int val;
	stack_top(s, &val);
	printf("il valore della cima è: %d\n", val);	//deve stampare 2

	printf("\n");

	stack_pop(s);
	printf("toglie il 2, la cima è 1\n");
	stack_pop(s);
	printf("toglie il 1, la cima è vuota\n");

	printf("\n");


	if(stack_empty(s))
		printf("la stack è vuota\n");



	stack_delete(s);

}

 */


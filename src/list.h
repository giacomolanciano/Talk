/*
 * list.h
 *
 *  Created on: 31/mar/2015
 *      Author: davide & giacomo
 */

#ifndef __LIST__
#define __LIST__

typedef struct nodo nodo;

nodo*	list_aggiungi_testa(nodo* l, unsigned elem);
nodo*	list_rimuovi_testa(nodo* l);
void	list_dealloca(nodo* l);

#endif

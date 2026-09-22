#ifndef OPERANDOS_H
#define OPERANDOS_H

#include "MV.h"

#define MAXVEC 4

typedef int (*getters)(TMV *, int);
typedef void (*setters)(TMV *, int, int);

int get(TMV *mv, int op);
void set(TMV *mv, int op, int valor);
void escribirMemoria(TMV *mv, int direLogica, int cantBytes, int valor);

#endif
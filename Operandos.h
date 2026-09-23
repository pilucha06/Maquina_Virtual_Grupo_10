#ifndef OPERANDOS_H
#define OPERANDOS_H

#include "MV.h"

#define MAXVEC 4

typedef int (*getters)(TMV *, int);
typedef void (*setters)(TMV *, int, int);

int get(TMV *mv, int op);
void set(TMV *mv, int op, int valor);

//para sys
void escribirMemoria(TMV *mv, int direLogica, int tam, int valor);
void leerMemoria(TMV *mv, int direLogica, int tam, int *valor);

#endif
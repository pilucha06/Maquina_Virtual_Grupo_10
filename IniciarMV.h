#ifndef INICIARMV_H
#define INICIARMV_H

#include "MV.h"

typedef struct {
    char *nomVMX;          // NULL si no vino
    char *nomVMI;          // NULL si no vino
    unsigned int tamMemKiB;
    int mostrarDis;        // 1 si vino -d
    char **params;         // params[0..cantParam-1] = parámetros que vinieron después de -p
    int cantParam;         // cuántos parámetros hay después de -p
} TArgumentos;

int reservoEspacioMV(TMV *MV, unsigned int tamRam);
int leerArgumentos(int argc, char *argv[], TArgumentos *args);
int inicializoMV(TMV *MV, TArgumentos *args);
void liberarMV(TMV *MV);

#endif
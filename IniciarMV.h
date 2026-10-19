#ifndef INICIARMV_H
#define INICIARMV_H

#include "MV.h"

typedef struct {
    char *nomVMX;          // NULL si no vino
    char *nomVMI;          // NULL si no vino
    unsigned int tamMemKiB;
    int mostrarDis;        // 1 si vino -d
    int posParam;          // índice en argv del primer parámetro de -p
    int cantParam;         // cuántos parámetros hay después de -p
} TArgumentos;

int reservoEspacioMV(TMV *MV, unsigned int tamRam);
int leerArgumentos(int argc, char *argv[], TArgumentos *args);
int inicializoMV(TMV *MV, char nomArch[]);
void liberarMV(TMV *MV);

#endif
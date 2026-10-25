#ifndef IMAGEN_H
#define IMAGEN_H

#include "MV.h"

#define ID_MVI  "VMI26"

void escribirImagen(TMV *MV, char nomArch[]);
void breakpoint(TMV *MV);
int cargarImagen(TMV *MV, char nomArch[]);
#endif
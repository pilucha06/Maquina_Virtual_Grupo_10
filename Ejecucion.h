#ifndef EJECUCION_H
#define EJECUCION_H

#include "MV.h"

// Tablas de nombres (para el disassembler), compartidas si otro archivo las necesita
extern const char *nombres_registros[CANT_REGS];
extern const char *nombres_mnemonicos[CANT_REGS];

// Funciones públicas de este módulo
int  direc_fisica(int direc_logica, TMV *MV, int cant_bytes);
void calcula_cant_op(int instruccion, int *tipo_A, int *tipo_B, int *cant_op);
int  get_RAM(int tipo_op, int fisica_ip, TMV *MV);
int extender_signo_16(int valor16);
void imprimir_instruccion_hex(TMV *MV, int fisica_inicio, int tamanio_total);
void formatear_operando(int operando, char *buffer);
void mostrar_desensamblado(TMV *MV, int fisica_ip, int tamanio_total);
void ejecutar(TMV *MV, int modo_disassembler);

#endif
#ifndef EJECUCION_H
#define EJECUCION_H

#include "estructuras.h" // ajustar al nombre real del .h donde está registrosM


// Tablas de nombres (para el disassembler), compartidas si otro archivo las necesita
extern const char *nombres_registros[32];
extern const char *nombres_mnemonicos[32];

// Funciones públicas de este módulo
int  direc_fisica(int direc_logica, registrosM *MV, int cant_bytes);
void calcula_cant_op(int instruccion, int *tipo_A, int *tipo_B, int *cant_op);
int  get_RAM(int tipo_op, int fisica_ip, registrosM *MV);
void imprimir_instruccion_hex(registrosM *MV, int fisica_inicio, int tamanio_total);
void formatear_operando(int operando, char *buffer);
void mostrar_desensamblado(registrosM *MV, int fisica_ip, int tamanio_total);
void ejecutar(registrosM *MV, int modo_disassembler);

#endif
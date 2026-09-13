/*Tiene que tener minimo 2 funciones (get,set), se
 puede hacer un vector de get y uno de set para cada tipo
de operando y aislar codigo (op1 | op2) 3 bits mas sig son el tipo, get\set mem, inm, reg shl 3 
get[*getinm, *getmem, *getreg](4 bytes - 3 bits)
set[*setmem, *setreg](4bytes - 3bits, int)
recibe por parametro la maquina y la linea de operacion entera, (OP)*/


//AVANCES TIZI :D -------->

/*Los 8 bits mas significativos (el byte mas alto): 
Almacenan el codigo del tipo de operando:
00 (0): Ninguno
01 (1): Registro
10 (2): Inmediato
11 (3): Memoria
Los 24 bits restantes (los 3 bytes mas bajos): Almacenan el valor o codificacion
del operando tal cual vino en la instruccion. */

 /* aca nos sacamos la duda: Es una excelente solucion usar un vector de punteros a funciones
En arquitectura de computadoras y desarrollo de compiladores/emuladores, esta tecnica se 
conoce como Dispatch Table (Tabla de Despacho). Sus principales ventajas son:
  -> Elimina bloques largos de if/else o switch: Evita tener que preguntar repetidamente si 
es registro, memoria o inmediato cada vez que se ejecuta una instruccion.
  ->Escalabilidad y legibilidad: El codigo principal (get y set) queda resuelto 
en una sola linea muy limpia.
  ->Rendimiento: El acceso mediante indice tabla[tipo](...)
se resuelve en tiempo $O(1)$, ejecutandose directo en el hardware.*/

#include <stdio.h>
#include "Operandos.h"

//MIS CONSTANTES 
#define MAXVEC 4

//GETTERS!

int getReg(tRegMV *mv, int op){
    int regNum ;
    regNum= op &  0x1F; //5 ult bits -> codreg
    return mv->registros[regNum];
}

int getInm(tRegMV *mv, int op){
    return op & 0xFFFF; //los 4 bytes
}

int getMem(tRegMV *mv, int op){
    int regNum, segmento, direLogica;
    regNum = op & 0x1F // dire->5 ult bits
    offset = (op >> 8) &  0xFFFF; // 8=dire+3 reservados
    segmento= mv->registros[regNum];
    //calculo direccion logica = 2 bytes +s segmento + 2 bytes -s offset
    direLogica = (segmento << 16) | (offset & 0xFFFF);
        return leerMemoria(mv, dirLogica); // haganla *n* ... (creo q en ejecucion.c)
}            //(funcion que hace la traduccion de la direccion 
            //logica a direccion fisica y lee los 4 bytes de la RAM)


typedef int (*getters)(tRegMV *, int);
typedef void (*setters)(tRegMV *, int, int);

getters vecGet[MAXVEC] = { NULL, getReg, getInm, getMem}
setters setVec[MAXVEC] = { NULL, setReg, setInm, setMem }


//en la llamada, debemos referenciar:  OP1-> REG[2] y OP2-> REG[3]
int get(tRegMV *mv, int op){
   int tipo;
   tipo=(op >> 24) & 0xFF; 
   if (tipo >=1 && tipo<=3 && vecGet[tipo] != NULL) 
        return tablaGet[tipo](mv, op);
    else
        return 0; //?
}

void set(tRegMV *mv, int op, int valor){
    int tipo;
    tipo = (op >> 24) && OXFF;
    if (tipo>=1 && tipo <=3 && vecSet[tipo != NULL])
        vecSet[tipo](mv, op, valor)
}


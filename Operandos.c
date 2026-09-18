#include <stdio.h>
#include "MV.h"
#include "Operandos.h"
#include "Ejecucion.h"

#define MAXVEC 4

//FUNCIONES PARTICULARES DE MEMORIA 
//cito de la especificación:
/*Cada vez que se realiza una operación en la memoria, se debe cargar en el registro LAR la dirección
lógica a la que se quiere acceder y la cantidad de bytes en la parte alta del registro MAR (los 2 bytes más
significativos). Luego de realizar la traducción a una dirección física, el resultado debe almacenarse en la
parte baja del registro MAR (los 2 bytes menos significativos). En el registro MBR debe quedar el valor con
el cual se está operando, ya sea el valor que se desea almacenar en el caso de una escritura o el que se
obtuvo después de la lectura. La lectura de la instrucción no debe modificar ninguno de estos registros*/

void leerMemoria(TMV *mv, int direLogica, int *valor){
    int i, direFisica;
    direFisica = dire_fisica(direLogica, mv, 4);
    if (direFisica == -1 )
        printf("Error: fallo de segmento al leer memoria.\n");
    else{
        mv->registros[REG_LAR] = direLogica;
        mv->registros[REG_MAR] = (4<<16) | (direFisica & 0xFFFF);
        //lectura
        *valor = 0; //x las dudas
        for(i=0; i<4; i++) 
            //voy concatenando 4 veces en "valor" los 8 bits de cada celda en RAM
            *valor = (*valor<<8) | mv->RAM[direFisica + i];
        mv->registros[REG_MBR] = *valor;
    }
}

void escribirMemoria(TMV *mv, int direLogica, int valor){
    int direFisica;
    direFisica = dire_fisica(direLogica, mv, 4);
    if (direFisica == -1 ){
        printf("Error: fallo de segmento al escribir en memoria.\n");
    }
    else{
        mv->registros[REG_LAR] = direLogica;
        mv->registros[REG_MAR] = (4<<16) | (direFisica & 0xFFFF);
        mv->registros[REG_MBR] = valor;
        //escribo 4  bytes en RAM
        mv->RAM[direFisica + 0] = (valor >> 24) & 0xFF;
        mv->RAM[direFisica + 1] = (valor >> 16) & 0xFF;
        mv->RAM[direFisica + 2] = (valor >> 8)  & 0xFF;
        mv->RAM[direFisica + 3] = valor & 0xFF; 
    }
}

// en getinm, getmem y setmem puse casteos a short para
// forzar la extension de signo en valores inm y mem...
// x  ej, si viene un offset negativo (ej [EDX - 5]), al pasarlo a la variable
// de 32 bits C con ese casteo mantiene el signo negativo rellenando con 1s a la izquierda
// en vez  de convertirlo en un entero gigante positivo.

//fijense q en los de reg no, xq los num de registro son siempre del 0 al 31 (>0), no llevan signo.


//GETTERS
int getReg(TMV *mv, int op){
    int regNum ;
    regNum= (op &  0x1F); //5 ult bits -> codreg
    return mv->registros[regNum];
}

int getInm(TMV *mv, int op){
    return (short)(op & 0xFFFF); //los 4 bytes
}

int getMem(TMV *mv, int op){
    int regNum, segmento, direLogica, offset, valor, puntero, offsetPuntero;
    offset = (short)((op >> 8) &  0xFFFF); // corro 8=dire +3 reservados, [EBX + 2] despl extra
    regNum = op & 0x1F; // dire reg->5 ult bits
    //puntero=elem en reg=> 16 izq=segmento, 16 der= offset en segmento
    puntero= mv->registros[regNum];
    segmento= (puntero >> 16) & 0xFFFF;
    offsetPuntero=puntero & 0xFFFF;
    //calculo direccion logica = 2 bytes +s segmento + 2 bytes -s offset
    direLogica = (segmento << 16) | ((offsetPuntero + offset) & 0xFFFF);
    valor = 0;
    leerMemoria(mv, direLogica, &valor);
    return valor; 
}           

//SETTERS
void setReg(TMV *mv, int op, int valor){
    int regNum = op & 0x1F; //5 ult bits -> codreg
    mv->registros[regNum] = valor;
}


void setMem(TMV *mv, int op, int valor){
    int regNum, segmento, direLogica, offset, offsetPuntero, puntero;
    offset = (short)((op >> 8) &  0xFFFF); // corro 8=dire+3 reservados
    regNum = op & 0x1F; // dire->5 ult bits
    puntero= mv->registros[regNum];
    segmento= (puntero >> 16) & 0xFFFF;
    offsetPuntero=puntero & 0xFFFF;
    //calculo direccion logica = 2 bytes +s segmento + 2 bytes -s offset
    direLogica = (segmento << 16) | ((offset + offsetPuntero) & 0xFFFF);
    escribirMemoria(mv, direLogica, valor);
}        

//FUNCIONES PPALES Y DEFINICIONES

typedef int (*getters)(TMV *, int);
typedef void (*setters)(TMV *, int, int);

getters vecGet[MAXVEC] = { NULL, getReg, getInm, getMem };
setters vecSet[MAXVEC] = { NULL, setReg, NULL, setMem };


/*Los 8 bits mas significativos (el byte mas alto): 
Almacenan el codigo del tipo de operando:
00 (0): Ninguno
01 (1): Registro
10 (2): Inmediato
11 (3): Memoria
Los 24 bits restantes (los 3 bytes mas bajos): Almacenan el valor o codificacion
del operando tal cual vino en la instruccion. */


//en la llamada, debemos referenciar:  OP1-> REG[2] y OP2-> REG[3]
int get(TMV *mv, int op){
   int tipo;
   tipo=(op >> 24) & 0xFF; 
   if (tipo >=1 && tipo<=3 && vecGet[tipo] != NULL) 
        return vecGet[tipo](mv, op);
    else
        return 0; 
}

void set(TMV *mv, int op, int valor){
    int tipo;
    tipo = (op >> 24) & 0XFF;
    if (tipo>=1 && tipo <=3 && vecSet[tipo] != NULL)
        vecSet[tipo](mv, op, valor);
}


#include <stdio.h>
#include "MV.h"
#include "Operandos.h"
#include "Ejecucion.h"

//FUNCIONES PARTICULARES DE MEMORIA 
//cito de la especificación:
/*Cada vez que se realiza una operación en la memoria, se debe cargar en el registro LAR la dirección
lógica a la que se quiere acceder y la cantidad de bytes en la parte alta del registro MAR (los 2 bytes más
significativos). Luego de realizar la traducción a una dirección física, el resultado debe almacenarse en la
parte baja del registro MAR (los 2 bytes menos significativos). En el registro MBR debe quedar el valor con
el cual se está operando, ya sea el valor que se desea almacenar en el caso de una escritura o el que se
obtuvo después de la lectura. La lectura de la instrucción no debe modificar ninguno de estos registros*/

void leerMemoria(TMV *mv, int direLogica, int tam, int *valor){
    int i, direFisica;
    direFisica = direc_fisica(direLogica, mv, tam);  
    if (direFisica == -1 )
        mv->error=3;
    else{
        mv->registros[REG_LAR] = direLogica;
        mv->registros[REG_MAR] = (tam<<16) | (direFisica & 0xFFFF);
        //lectura
        *valor = 0; //x las dudas
        for(i=0; i<tam; i++) 
            //voy concatenando 4 veces en "valor" los 8 bits de cada celda en RAM
            *valor = (*valor<<8) | mv->RAM[direFisica + i];
        mv->registros[REG_MBR] = *valor;
    }
}

void escribirMemoria(TMV *mv, int direLogica, int tam, int valor){
    int direFisica;
    direFisica = direc_fisica(direLogica, mv, tam);
    if (direFisica == -1 ){
        mv->error=3;
    }
    else{
        mv->registros[REG_LAR] = direLogica;
        mv->registros[REG_MAR] = (tam<<16) | (direFisica & 0xFFFF);
        mv->registros[REG_MBR] = valor;
        //escribo [tam] bytes en RAM
        for (int j = 0; j < tam; j++) {
            mv->RAM[direFisica + (tam - 1 - j)] = (valor >> (8 * j)) & 0xFF;
        }
    }
}

// en getinm, getmem y setmem puse casteos a short para
// forzar la extension de signo en valores inm y mem...
// x  ej, si viene un offset negativo (ej [EDX - 5]), al pasarlo a la variable
// de 32 bits C con ese casteo mantiene el signo negativo rellenando con 1s a la izquierda
// en vez  de convertirlo en un entero gigante positivo.

//fijense q en los de reg no, xq los num de registro son siempre del 0 al 31 (>0), no llevan signo.


//GETTERS

// MV2 -> getReg debe extraer el código de registro (5 bits) (como antes) 
// y ahora ademas el sector (2 bits)

// OPREG = 1 bit reservado + 2 bits sec reg + 5 bits cod reg

int getReg(TMV *mv, int op){
    int codReg, sector, valorReg;
    codReg = op & 0x1F; // 0001 1111, 5 ult bits -> codreg
    sector = (op >> 5) & 0x03;
    valorReg = mv->registros[codReg];

    switch (sector){
        case 0: // caso 00, registro de 4 bytes (EAX)
            return valorReg;
        case 1: // caso 01, 4to byte del registro (AL)
            return extenderSigno(valorReg & 0xFF, 1);
        case 2: // caso 10, 3er byte del registro (AH)
            return extenderSigno((valorReg >> 8) & 0xFF, 1); 
        case 3: // caso 11, registro de 2 bytes (AX)
            return extenderSigno(valorReg & 0xFFFF, 2);
        default: return 0; // no deberia pasar, pero por las dudas
    }
    // el retorno es siempre un int de 32 bits 
}

int getInm(TMV *mv, int op){
    return (short)(op & 0xFFFF); //los 4 bytes
}


// MV2 -> para memoria...
/* cito de especificacion: 
Operando de memoria: los tres bits más significativos indican el tamaño del valor a
leer o escribir, de acuerdo a la siguiente codificación: 
        001 byte (b) 1 byte 
        010 word (w) 2 bytes
        100 long (l) 4 bytes
*/

int getMem(TMV *mv, int op){
    int regNum, segmento, direLogica, offset, valor, puntero, offsetPuntero, tamBytes;
    tamBytes=obtenerCantBytesMem(op);
    offset = (short)((op >> 8) &  0xFFFF); // corro 8=dire +3 reservados, [EBX + 2] despl extra
    regNum = op & 0x1F; // dire reg->5 ult bits
    //puntero=elem en reg=> 16 izq=segmento, 16 der= offset en segmento
    puntero= mv->registros[regNum];
    segmento= (puntero >> 16) & 0xFFFF;
    offsetPuntero=puntero & 0xFFFF;
    //calculo direccion logica = 2 bytes +s segmento + 2 bytes -s offset
    direLogica = (segmento << 16) | ((offsetPuntero + offset) & 0xFFFF);
    valor = 0;
    leerMemoria(mv, direLogica, tamBytes, &valor);
    return extenderSigno(valor, tamBytes); // output siempre int 32 bits 
}           

//SETTERS

// MV2 -> ahora, al escribir en un registro, solo hay que modificar el
// sector indicado, conservando los bytes restantes del registro intactos

// OPREG = 1 bit reservado + 2 bits sec reg + 5 bits cod reg

void setReg(TMV *mv, int op, int valor){
    int codReg, sector, valorReg;
    codReg = op & 0x1F; //5 ult bits -> codreg
    valorReg=mv->registros[codReg]; //para algunos casos, hay q tener en cuenta el valor anterior
    sector = (op>>5) & 0x03;

    switch (sector){
        case 0: // caso 00, modificar reg entero (EAX)
        // entramos en este caso si el vmx es de la version 1
            mv->registros[codReg] = valor; 
            break;
        case 1: // caso 01, solo cambiar el menos significativo (parte baja) (AL)
            mv->registros[codReg] = (valorReg & 0xFFFFFF00) | (valor & 0xFF);
            break;
        case 2: // caso 10, cambiar solo el segundo byte (parte alta) (AH)
            mv->registros[codReg] = (valorReg & 0xFFFF00FF) | ((valor & 0xFF) <<8 ); //corro un byte el valor filtrado para que entre 
            break;
        case 3: // caso 11, 2 bytes menos significativos (AX)
            mv->registros[codReg] = (valorReg & 0xFFFF0000) | (valor & 0xFFFF);
            break;
    }
}


void setMem(TMV *mv, int op, int valor){
    int regNum, segmento, direLogica, offset, offsetPuntero, puntero, tamBytes;
    tamBytes = obtenerCantBytesMem(op);
    offset = (short)((op >> 8) &  0xFFFF); // corro 8=dire+3 reservados
    regNum = op & 0x1F; // dire->5 ult bits
    puntero= mv->registros[regNum];
    segmento= (puntero >> 16) & 0xFFFF;
    offsetPuntero=puntero & 0xFFFF;
    //calculo direccion logica = 2 bytes +s segmento + 2 bytes -s offset
    direLogica = (segmento << 16) | ((offset + offsetPuntero) & 0xFFFF);
    escribirMemoria(mv, direLogica, tamBytes, valor);
}        

//FUNCIONES PPALES 

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




//-------agregados de MV2------------

// funcion para registros y memoria
int extenderSigno(int valor, int bytes) {
    if (bytes == 1) 
        return (int)(signed char)(valor & 0xFF);
    else if (bytes == 2) 
        return (int)(short)(valor & 0xFFFF);
    else
        return valor; // para 4 bytes se mantiene igual
}

// funcion para memoria
int obtenerCantBytesMem(int op) {
    int mod = (op >> 5) & 0x07; // filtra los bits 5, 6 y 7
    if (mod == 1) 
        return 1;     // 001: byte (b)
    else if (mod == 2) 
        return 2;     // 010: word (w)
    else if (mod == 4) 
        return 4;     // 100: long (l)
    else 
        return 4;   // si es 0 (como en la MV1) devuelve 4 por default
}
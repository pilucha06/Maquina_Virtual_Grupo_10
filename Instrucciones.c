#include "MV.h"
#include "Operandos.h"
#include "Ejecucion.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "Instrucciones.h"

#define SYS_READ  1
#define SYS_WRITE 2


instrucciones vecInstr[MAXINSTR] = { SYS, JMP, JP, JN, JZ, JC, JV, JNP, JNN, JNZ, NOT, NULL, NULL, NULL, NULL, STOP, MOV, ADD, SUB, MUL, DIV, CMP, AND, OR, XOR, SWAP, SHL, SHR, SAR, LDL, LDH, RND };

//-------SECCION SYS--------
//segun el formato leo de manera distinta, devuelve valor leido
static int leerValor(int formato, TMV *mv){
    int valor = 0;
    unsigned int aux;          // para hexa y octal
    unsigned int acum = 0;     // para armar el binario
    unsigned char c;
    char bin[33];              // 32 dígitos + '\0'
    size_t i, largo;

    switch (formato) {
        case 0x01:
            scanf("%d", &valor);
            break;
            
        case 0x08:
            scanf("%x", &aux);
            valor = aux;
            break;

        case 0x04:
            scanf("%o", &aux);
            valor = aux;
            break;

        case 0x02:
            scanf(" %c", &c);
            valor = c;
            break;

        case 0x10:
            scanf("%32s", bin);
            largo = strlen(bin);
            i = 0;
            while (i < largo && mv->error == 0) { //voy pasando del string a un numero binario, construyo bit a bit
                if (bin[i] == '0' || bin[i] == '1')
                    acum = (acum << 1) | (bin[i] - '0'); 
                else
                    mv->error = 1;
                i++;
            }
            valor = acum;
            break;
    }
    return valor;
}

void sysRead(TMV *mv){
    int dir, cantCeldas, tam, formato;
    int seg, off, offCelda, dirCelda, dirFis, valor, i;

    dir        = mv->registros[REG_EDX];
    formato    = mv->registros[REG_EAX] & 0x1F;
    cantCeldas = mv->registros[REG_ECX] & 0xFFFF;
    tam        = (mv->registros[REG_ECX] >> 16) & 0xFFFF;

    if (tam < 1 || tam > 4)
        mv->error = 1;
    else if (formato == 0 || (formato & (formato - 1)) != 0)
        mv->error = 1;
    else {
        seg = (dir >> 16) & 0xFFFF;
        off = dir & 0xFFFF;
        i = 0;
        while (i < cantCeldas && mv->error == 0) {
            offCelda = off + i * tam;
            if (offCelda <= 0xFFFF)
                dirFis = direc_fisica((seg << 16) | offCelda, mv, tam);
            else
                dirFis = -1;

            if (dirFis == -1)
                mv->error = 3;
            else {
                dirCelda = (seg << 16) | offCelda;
                printf("[%04X]: ", dirFis); //En requerimientos pide esto
                valor = leerValor(formato, mv);
                if (mv->error == 0)
                    escribirMemoria(mv, dirCelda, tam, valor);
            }
            i++;
        }
    }
}

// 0x1 -> READ, 0x2 -> WRITE

void SYS(TMV *mv, int opa, int opb){
    int llamada = get(mv, opa); // según llamada: sysRead(mv) o sysWrite(mv)
    if (llamada == SYS_READ)
        sysRead(mv);
    else
        if (llamada == SYS_WRITE)
            //funcion de tizi
        else
            mv->error = 1; //op invalida
}

//-------SECCION JUMPS--------
// BITS CC (4 bits más significativos): [N] [Z] [C] [V]

void salto(TMV *mv, int opa){
    int inicioCS, opA;
    inicioCS=((mv->tablaSegmentos[REG_CS])  >> 16 ) & 0xFF;
    opA= get(mv, opa);
    mv->registros[REG_IP]=inicioCS+opA;
}


void JMP(TMV *mv, int opa, int opb){ //salta siempre
    salto(mv, opa);
}

void JP(TMV *mv, int opa, int opb){ //salta cuando es positivo
    if (!((mv->registros[REG_CC] >> 28) & 0xC)) //masc=1100 tiene q dar 0
        salto(mv, opa);
}

void JN(TMV *mv, int opa, int opb){ //salta cuando es negativo
    if ((mv->registros[REG_CC] >> 28) & 0x8) //masc=1000 tiene q dar 1
        salto(mv, opa);
}

void JZ(TMV *mv, int opa, int opb){ //salta cuando es cero
    if ((mv->registros[REG_CC] >> 28) & 0x4) //masc=0100 tiene q dar 1
        salto(mv, opa);
}

void JC(TMV *mv, int opa, int opb){ //salta cuando hay carry
    if ((mv->registros[REG_CC] >> 28) & 0x2) //masc=0010 tiene q dar 1
        salto(mv, opa);
}

void JV(TMV *mv, int opa, int opb){ //salta cuando hay overflow
    if ((mv->registros[REG_CC] >> 28) & 0x1) //masc=0001 tiene q dar 1
        salto(mv, opa);
}

void JNP(TMV *mv, int opa, int opb){ //salta cuando es negativo o cero
    if ((mv->registros[REG_CC] >> 28) & 0xC) //masc=1100 tiene q dar 1
        salto(mv, opa);
}

void JNN(TMV *mv, int opa, int opb){ //salta cuando es positivo o cero
    if (!((mv->registros[REG_CC] >> 28) & 0x8)) //masc=1000 tiene q dar 0, no es negativo
        salto(mv, opa);
}

void JNZ(TMV *mv, int opa, int opb){ //salta cuando es positivo o negativo
    if (!((mv->registros[REG_CC] >> 28) & 0x4)) //masc=0100 tiene q dar 0, no es zero
        salto(mv, opa);
}

//------------------------

void modCC(TMV *mv, int valor){
    mv->registros[REG_CC];// *`u`*
}

void NOT(TMV *mv, int opa, int opb){ //solamente invertir bits y actualiza reg CC
    int opA, res;
    opA= get(mv, opa);
    res=~opA;
    set(mv, opa, res);
    modCC(mv, res);
}

void STOP(TMV *mv, int opa, int opb){
    mv->registros[REG_IP]=-1;
}

void MOV(TMV *mv, int opa, int opb){
    int opB;
    opB=get(mv, opb);
    set(mv, opa, opB);
}

void ADD(TMV *mv, int opa, int opb){
    int opA, opB, res;
    opA=get(mv, opa);
    opB=get(mv, opb);
    res= opA+opB;
    set(mv, opa, res); 
    modCC(mv, res);
}

void SUB(TMV *mv, int opa, int opb){
    int opA, opB;
    int res;
    opA=get(mv, opa);
    opB=get(mv, opb);
    res = opA-opB;
    set(mv, opa, res);
    modCC(mv, res); 
}

void MUL(TMV *mv, int opa, int opb){
    int opA, opB;
    unsigned int res;
    opA=get(mv, opa);
    opB=get(mv, opb);
    res = opA*opB;
    set(mv, opa, res);
    modCC(mv, res); 
}

void DIV(TMV *mv, int opa, int opb){
    int opA, opB;
    unsigned int res;
    opA=get(mv, opa);
    opB=get(mv, opb);
    if (opB != 0){
        res = opA/opB;
        set(mv, opa, res);
        modCC(mv, res); }
    else
        mv->error=2; //div x cero
}

void CMP(TMV *mv, int opa, int opb){
    int opA, opB, res;
    opA=get(mv, opa);
    opB=get(mv, opb);
    res = opA-opB;
    modCC(mv, res); 
}

/*AND, OR, XOR: efectúan las operaciones lógicas básicas bit a bit entre los operandos y afectan al
registro CC. El resultado se almacena en el primer operando.*/

void AND(TMV *mv, int opa, int opb){
    int opA, opB, res;
    opA=get(mv, opa);
    opB=get(mv, opb);
    res = opA & opB;
    set(mv, opa, res);
    modCC(mv, res);
}

void OR(TMV *mv, int opa, int opb){
    int opA, opB, res;
    opA=get(mv, opa);
    opB=get(mv, opb);
    res = opA | opB;
    set(mv, opa, res);
    modCC(mv, res);
}

void XOR(TMV *mv, int opa, int opb){
    int opA, opB;
    int res;
    opA=get(mv, opa);
    opB=get(mv, opb);
    res = opA ^ opB;
    set(mv, opa, res);
    modCC(mv, res);
}

/*SWAP: intercambia los valores de los operandos (ambos deben ser registros y/o celdas de memoria).
Equivale a realizar las siguientes operaciones:
XOR OPN_A, OPN_B
XOR OPN_B, OPN_A
XOR OPN_A, OPN_B
Por lo tanto, afecta al registro CC del mismo modo que el último XOR.*/

void SWAP(TMV *mv, int opa, int opb){
    XOR(mv, opa, opb);
    XOR(mv, opb, opa);
    XOR(mv, opa, opb);
}

/*SHL, SHR, SAR: realizan desplazamientos de los bits almacenados en un registro o una posición de
memoria y afectan al registro CC. SHL y SHR efectuan corrimientos a la izquierda y a la derecha
(respectivamente) y los bits que quedan libres se completan con ceros. */


void SHL(TMV *mv, int opa, int opb){
    int opA, opB, res;
    opA=get(mv, opa);
    opB=get(mv, opb);
    res=opA;
    res=res << opB;
    set(mv, opa, res);
    modCC(mv, res);
}

void SHR(TMV *mv, int opa, int opb){
    int opA, opB, res;
    opA=get(mv, opa);
    opB=get(mv, opb);
    res=opA;
    res = (unsigned int)res >> opB;
    set(mv, opa, res);
    modCC(mv, res);
}

/*SAR también desplaza a la
derecha, pero los bits de la izquierda propagan el bit anterior. Es decir, si el contenido es un número
negativo, el resultado también lo será, porque agrega unos. Si es un número positivo, agrega ceros.*/

//ni idea como programar esto por los 1s a la izq
void SAR(TMV *mv, int opa, int opb){
    int opA, opB, res;
    opA=get(mv, opa);
    opB=get(mv, opb);
    res=opA >> opB;
    set(mv, opa, res);
    modCC(mv, res); 
}

/*LDL: carga los 2 bytes menos significativos del primer operando, con los 2 bytes menos significativos
del segundo operando. Esta instrucción está especialmente pensada para poder cargar un inmediato de
16 bits, aunque también se puede utilizar con otro tipo de operando.*/

void LDL(TMV *mv, int opa, int opb){
    int opA, opB, res;
    opA=get(mv, opa);
    opB=get(mv, opb);
    res = (opA & 0xFFFF0000) | (opB & 0xFFFF);
    //no modifica cc
    set(mv, opa, res);
}

/*LDH: carga los 2 bytes más significativos del primer operando, con los 2 bytes menos significativos del
segundo operando. Esta instrucción está especialmente pensada para poder cargar un inmediato de 16
bits, aunque también se puede utilizar con otro tipo de operando.*/

void LDH(TMV *mv, int opa, int opb){
    int opA, opB, res;
    opA=get(mv, opa);
    opB=get(mv, opb);
    res = ((opB & 0xFFFF) << 16) | (opA & 0x0000FFFF);
    //no modifica cc
    set(mv, opa, res);
}

/*RND: carga en el primer operando un número aleatorio entre 0 y el valor del segundo operando*/

//random esta en stdlib!
void RND(TMV *mv, int opa, int opb){
    int opB, aleatorio;
    opB=get(mv,opb);
    if (opB > 0){
        aleatorio=rand() % (opB + 1);
        set(mv, opa, aleatorio);
    } else
        mv->error=2; // o podemos guardar 0 :)
    // no modifica CC
}






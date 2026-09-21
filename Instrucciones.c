#include "MV.h"
#include "Operandos.h"
#include "Ejecucion.h"
#include <stdlib.h>
#include "Instrucciones.h"


instrucciones vecInstr[MAXINSTR] = { SYS, JMP, JP, JN, JZ, JC, JV, JNP, JNN, JNZ, NOT, NULL, NULL, NULL, NULL, STOP, MOV, ADD, SUB, MUL, DIV, CMP, AND, OR, XOR, SWAP, SHL, SHR, SAR, LDL, LDH, RND };

//llamado a sistema
// 0x1 -> READ, 0x2 -> WRITE
void SYS(TMV *mv, int opa, int opb){
    
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






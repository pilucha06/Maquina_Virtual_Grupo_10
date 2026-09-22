#include "MV.h"
#include "Operandos.h"
#include "Ejecucion.h"
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "Instrucciones.h"

#define SYS_READ  1
#define SYS_WRITE 2


//llamado a sistema
// 0x1 -> READ, 0x2 -> WRITE
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
        if (llamada == SYS_WRITE){
            //funcion de tizi
        }
        else
            mv->error = 1; //op invalida
}

//-------SECCION JUMPS--------
// BITS CC (4 bits más significativos): [N] [Z] [C] [V]

void salto(TMV *mv, int opa){
    int inicioCS, opA;
    inicioCS=(mv->registros[REG_CS]) & 0xFFFF0000;
    opA= get(mv, opa);
    mv->registros[REG_IP]=inicioCS | (opA & 0xFFFF);
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

// Arma el registro CC completo a partir del resultado y de carry/overflow
// ya calculados por la operación que llama (cada familia de instrucciones
// calcula carry/overflow distinto, ver funciones auxiliares mas abajo).
void modCC(TMV *mv, int valor, int carry, int overflow){
    int cc = 0;
    if (valor < 0)  
        cc |= (1 << 31); // N
    if (valor == 0) 
        cc |= (1 << 30); // Z
    if (carry)       
        cc |= (1 << 29); // C
    if (overflow)    
        cc |= (1 << 28); // V
    mv->registros[REG_CC] = cc;
}

// Funciones auxiliares de carry/overflow
// Para ADD/SUB: suma o resta con signo de 32 bits
void carryOverflowSuma(int opA, int opB, int res, int *carry, int *overflow){
    long long suma64 = (long long)(unsigned int)opA + (unsigned int)opB;
    *carry = (suma64 >> 32) & 1; //corre 32 bits de la cadena de 64, si el bit 33 = 1 hay carry
    *overflow = ((opA >= 0 && opB >= 0 && res < 0) || (opA < 0  && opB < 0  && res >= 0));
    // 2 neg = pos | 2 pos = neg|0 (regla de overflow en suma)
}

void carryOverflowResta(int opA, int opB, int res, int *carry, int *overflow){
    // resta como la hace el hardware: opA + ~opB + 1
    unsigned long long suma64 = (unsigned long long)(unsigned int)opA + (unsigned int)~opB + 1;
    *carry = (suma64 >> 32) & 1;
    *overflow = ((opA < 0) != (opB < 0)) && ((res < 0) != (opA < 0));
}

// Para MUL
void carryOverflowMul(int opA, int opB, int *carry, int *overflow){
    long long prod64 = (long long)opA * (long long)opB;
    *overflow = (prod64 < INT32_MIN || prod64 > INT32_MAX);
    // Calculamos el carry como producto sin signo para ver si excede 32 bits
    *carry = (((unsigned long long)(unsigned int)opA * (unsigned int)opB) >> 32) != 0;
}

// Para SHL: bits que se "caen" por la izquierda
void carryOverflowShl(int opA, int cant, int *carry, int *overflow){
    unsigned int bits_perdidos;
    int top;
    if (cant <= 0) {
        *carry = 0;
        *overflow = 0;
    } else if (cant >= 32) {
        *carry = (opA != 0);
        *overflow = (opA != 0);
    } else {
        bits_perdidos = (unsigned int)opA >> (32 - cant);
        *carry = (bits_perdidos != 0);
        // entra en 32 bits solo si los cant+1 bits más altos son todos iguales al signo
        top = opA >> (31 - cant);
        *overflow = (top != 0 && top != -1);
    }
}

// Para SHR/SAR: bits que se "caen" por la derecha (mismo criterio para ambas)
void carryOverflowShr(int opA, int cant, int *carry, int *overflow){
    unsigned int bits_perdidos;
    if (cant <= 0)
        *carry = 0;
    else if (cant >= 32)
        *carry = (opA != 0);
    else {
        bits_perdidos = (unsigned int)opA & ((1u << cant) - 1);
        *carry = (bits_perdidos != 0);
    }
    *overflow = 0;
}

void NOT(TMV *mv, int opa, int opb){ //solamente invertir bits y actualiza reg CC
    int opA, res;
    opA= get(mv, opa);
    res=~opA;
    set(mv, opa, res);
    modCC(mv, res, 0, 0);
}

void STOP(TMV *mv, int opa, int opb){
    mv->registros[REG_IP]=-1;
}

void MOV(TMV *mv, int opa, int opb){
    int opB;
    opB=get(mv, opb);
    set(mv, opa, opB);
    modCC(mv, opB, 0, 0);
}

void ADD(TMV *mv, int opa, int opb){
    int opA, opB, res, carry, overflow;
    opA=get(mv, opa);
    opB=get(mv, opb);
    res= opA+opB;
    carryOverflowSuma(opA, opB, res, &carry, &overflow);
    set(mv, opa, res); 
    modCC(mv, res, carry, overflow);
}

void SUB(TMV *mv, int opa, int opb){
    int opA, opB, res, carry, overflow;
    opA=get(mv, opa);
    opB=get(mv, opb);
    res = opA-opB;
    carryOverflowResta(opA, opB, res, &carry, &overflow);
    set(mv, opa, res);
    modCC(mv, res, carry, overflow); 
}
void MUL(TMV *mv, int opa, int opb){
    int opA, opB, res, carry, overflow;
    opA=get(mv, opa);
    opB=get(mv, opb);
    res = opA*opB;
    carryOverflowMul(opA, opB, &carry, &overflow);
    set(mv, opa, res);
    modCC(mv, res, carry, overflow); 
}
void DIV(TMV *mv, int opa, int opb){
    int opA, opB, res, overflow;
    opA = get(mv, opa);
    opB = get(mv, opb);
    
    if (opB != 0) {
        overflow = (opA == INT32_MIN && opB == -1); // Detección previa
        
        if (overflow) {
            res = opA; // Evitamos la división que hace crashear
            mv->registros[REG_AC] = 0; 
        } else {
            res = opA / opB;
            mv->registros[REG_AC] = opA % opB; // Guardamos el resto[cite: 2]
        }
        
        set(mv, opa, res);
        modCC(mv, res, 0, overflow); // División nunca da carry
    } else {
        mv->error = 2; // División por cero
    }
}
void CMP(TMV *mv, int opa, int opb){
    int opA, opB, res, carry, overflow;
    opA=get(mv, opa);
    opB=get(mv, opb);
    res = opA-opB;
    carryOverflowResta(opA, opB, res, &carry, &overflow);
    modCC(mv, res, carry, overflow); 
}

/*AND, OR, XOR: efectúan las operaciones lógicas básicas bit a bit entre los operandos y afectan al
registro CC. El resultado se almacena en el primer operando.*/

void AND(TMV *mv, int opa, int opb){
    int opA, opB, res;
    opA=get(mv, opa);
    opB=get(mv, opb);
    res = opA & opB;
    set(mv, opa, res);
    modCC(mv, res, 0, 0); // logica: sin carry ni overflow
}

void OR(TMV *mv, int opa, int opb){
    int opA, opB, res;
    opA=get(mv, opa);
    opB=get(mv, opb);
    res = opA | opB;
    set(mv, opa, res);
    modCC(mv, res, 0, 0);
}

void XOR(TMV *mv, int opa, int opb){
    int opA, opB, res;
    opA=get(mv, opa);
    opB=get(mv, opb);
    res = opA ^ opB;
    set(mv, opa, res);
    modCC(mv, res, 0, 0);
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
    int opA, opB, res, carry, overflow;
    opA = get(mv, opa);
    opB = get(mv, opb);
    if (opB <= 0)
        res = opA;                                  // no desplaza
    else if (opB >= 32)
        res = 0;                                    // se cae todo
    else
        res = (int)((unsigned int)opA << opB);
    carryOverflowShl(opA, opB, &carry, &overflow);
    set(mv, opa, res);
    modCC(mv, res, carry, overflow);
}

void SHR(TMV *mv, int opa, int opb){
    int opA, opB, res, carry, overflow;
    opA = get(mv, opa);
    opB = get(mv, opb);
    if (opB <= 0)
        res = opA;
    else if (opB >= 32)
        res = 0;
    else
        res = (int)((unsigned int)opA >> opB);     // entran ceros por la izquierda
    carryOverflowShr(opA, opB, &carry, &overflow);
    set(mv, opa, res);
    modCC(mv, res, carry, overflow);
}

/*SAR también desplaza a la
derecha, pero los bits de la izquierda propagan el bit anterior. Es decir, si el contenido es un número
negativo, el resultado también lo será, porque agrega unos. Si es un número positivo, agrega ceros.*/

void SAR(TMV *mv, int opa, int opb){
    int opA, opB, res, carry, overflow;
    opA = get(mv, opa);
    opB = get(mv, opb);
    if (opB <= 0)
        res = opA;
    else if (opB >= 32)
        res = (opA < 0) ? -1 : 0;                   // queda solo el signo propagado
    else
        res = opA >> opB;                           // gcc propaga el bit de signo
    carryOverflowShr(opA, opB, &carry, &overflow);
    set(mv, opa, res);
    modCC(mv, res, carry, overflow);
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
    opB = get(mv, opb);
    if (opB > 0)             // <- 0 también es válido
        aleatorio = rand() % (opB + 1);
    else 
        aleatorio=0;
    set(mv, opa, aleatorio);

    // si opB es negativo, el documento no dice qué hacer
}
instrucciones vecInstr[MAXINSTR] = { SYS, JMP, JP, JN, JZ, JC, JV, JNP, JNN, JNZ, NOT, NULL, NULL, NULL, NULL, STOP, MOV, ADD, SUB, MUL, DIV, CMP, AND, OR, XOR, SWAP, SHL, SHR, SAR, LDL, LDH, RND };





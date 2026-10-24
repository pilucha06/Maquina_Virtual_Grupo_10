
#include "MV.h"
#include "Operandos.h"
#include "Ejecucion.h"
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "Instrucciones.h"
#include "Imagen.h"
 
#define SYS_CLR   0  // limpiar pantalla
#define SYS_READ  1  // leer valores formateados 
#define SYS_WRITE 2  // escribir "
#define SYS_RSTR  3  // leer string
#define SYS_WSTR  4  // escribir "
#define SYS_BREAK 0xF // breakpoint
 

//------READ------
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
 
//------WRITE--------
 
void imprimirBinario(unsigned int valor) {
    int b = 31;
    while (b > 0 && ((valor >> b) & 1) == 0)   // salteo los ceros de adelante
        b--;
    printf("0b");
    while (b >= 0) {
        printf("%u", (valor >> b) & 1);
        b--;
    }
}
 
/* imprime una celda de "tam" bytes en todos los formatos prendidos en "formato",
   del bit mas alto al mas bajo, separados por un espacio */
void imprimirValor(int formato, int valor, int tam) {
    unsigned int sinSigno;
    int conSigno, bits, primero = 1;
 
    bits = tam * 8;
    if (tam < 4)
        sinSigno = (unsigned int)valor & ((1u << bits) - 1);  // me quedo solo con los bytes de la celda
    else
        sinSigno = (unsigned int)valor;
    conSigno = (int)(sinSigno << (32 - bits)) >> (32 - bits); // extiendo el signo segun el tamanio
 
    if (formato & 0x10) {           // binario
        imprimirBinario(sinSigno);
        primero = 0;
    }
    if (formato & 0x08) {           // hexadecimal
        if (!primero) printf(" ");
        printf("0x%X", sinSigno);
        primero = 0;
    }
    if (formato & 0x04) {           // octal
        if (!primero) printf(" ");
        printf("0o%o", sinSigno);
        primero = 0;
    }
    if (formato & 0x02) {           // caracter
        if (!primero) printf(" ");
        if (sinSigno >= 32 && sinSigno <= 126)
            printf("%c", (char)sinSigno);
        else
            printf(".");
        primero = 0;
    }
    if (formato & 0x01) {           // decimal
        if (!primero) printf(" ");
        printf("%d", conSigno);
    }
    printf("\n");
}
 
void sysWrite(TMV *mv) {
    int dir, cantCeldas, tam, formato;
    int seg, off, offCelda, dirCelda, dirFis, valor, i;
 
    dir        = mv->registros[REG_EDX];
    formato    = mv->registros[REG_EAX] & 0x1F;
    cantCeldas = mv->registros[REG_ECX] & 0xFFFF;
    tam        = (mv->registros[REG_ECX] >> 16) & 0xFFFF;
 
    if (tam < 1 || tam > 4 || formato == 0) {
        mv->error = 1; // error de instrucc invalida
    } else {
        seg = (dir >> 16) & 0xFFFF;
        off = dir & 0xFFFF;
        i = 0;
 
        while (i < cantCeldas && mv->error == 0) {
            offCelda = off + i * tam;
            if (offCelda <= 0xFFFF)
                dirFis = direc_fisica((seg << 16) | offCelda, mv, tam);
            else
                dirFis = -1;
 
            if (dirFis == -1) {
                mv->error = 3; // Fallo de segmento
            } else {
                dirCelda = (seg << 16) | offCelda;
                leerMemoria(mv, dirCelda, tam, &valor);
                if (mv->error == 0) {
                    printf("[%04X]: ", dirFis); // prompt que exige requerimientos
                    imprimirValor(formato, valor, tam);
                }
            }
            i++;
        }
    }
}

void sysReadString(TMV *mv) {
    int dirDestino, maxChars, c, i;
    dirDestino = mv->registros[REG_EDX];
    maxChars = mv->registros[REG_ECX];
    i = 0;
    c = getchar();

    //mientras: no sea fin de línea \n o eof, mv->error == 0 y no hayamos alcanzado el max de caracteres 
    while (c != '\n' && c != EOF && mv->error == 0 && (maxChars == -1 || i < maxChars)) {
        escribirMemoria(mv, dirDestino + i, 1, c);
        i++;
        // si no alcanzamos el limite -> otro pido caract
        if (mv->error == 0 && (maxChars == -1 || i < maxChars))
            c = getchar();
    }
    //caracter nulo \0 al final
    if (mv->error == 0)
        escribirMemoria(mv, dirDestino + i, 1, 0);
}

void sysWriteString(TMV *mv) {
    int dirOrigen, c, offset;
    dirOrigen = mv->registros[REG_EDX];
    c = 0;
    offset = 0;
    leerMemoria(mv, dirOrigen + offset, 1, &c);
    // mientras: no haya error de memoria y no sea el car nulo \0
    while (mv->error == 0 && (c & 0xFF) != 0) {
        printf("%c", c);
        offset++;
        leerMemoria(mv, dirOrigen + offset, 1, &c);
    }
}


/*clear ejecuta una limpieza de pantalla. No requiere ningún registro configurado y
tampoco modifica ninguno.
Llama al comando propio del sistema operativo donde se esté corriendo tu Máquina Virtual
para vaciar el texto de la terminal y dejar la pantalla limpia.*/

void sysClear(TMV *mv) { // no tengo ni idea q hace esto
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}
 
void SYS(TMV *mv, int opa, int opb) {
    int llamada = get(mv, opa);

    switch (llamada) {
        case SYS_CLR:
            sysClear(mv);
            break;
        case SYS_READ:
            sysRead(mv);
            break;
        case SYS_WRITE:
            sysWrite(mv);
            break;
        case SYS_RSTR:
            sysReadString(mv);
            break;
        case SYS_WSTR:
            sysWriteString(mv);
            break;
        case SYS_BREAK:
            breakpoint(mv);
            break;
        default:
            // cualquier otro numero de llamada no hace nada
            break;
    }
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


// ---------- MV2-> INSTRUCCIONES DE PILA------------
void PUSH(TMV *mv, int opa, int opb) {
    int valor, sp, segSS, segSP;
    int offsetSP, tamanioSS, nuevoOffset;

    if (mv->registros[REG_SS] == -1) {
        mv->error = 3;
        return;
    }

    sp = mv->registros[REG_SP];

    segSS = (mv->registros[REG_SS] >> 16) & MASCARA_16_BITS;
    segSP = (sp >> 16) & MASCARA_16_BITS;
    offsetSP = sp & MASCARA_16_BITS;

    if (segSS >= CANT_SEGS || segSP != segSS ||
        mv->tablaSegmentos[segSS] == ENTRADA_LIBRE) {
        mv->error = 3;
        return;
    }

    tamanioSS = mv->tablaSegmentos[segSS] & MASCARA_16_BITS;

    if (offsetSP > tamanioSS || offsetSP < 4) {
        mv->error = 4;
        return;
    }

    nuevoOffset = offsetSP - 4;
    valor = get(mv, opa);

    if (mv->error != 0)
        return;

    escribirMemoria(mv, armarPunteroLogico(segSS, nuevoOffset),
                    4, valor);

    if (mv->error == 0)
        mv->registros[REG_SP] = armarPunteroLogico(segSS, nuevoOffset);
}
void POP(TMV *mv, int opa, int opb) {
    int valorTope, sp, segSS, segSP;
    int offsetSP, tamanioSS, nuevoOffset;

    if (mv->registros[REG_SS] == -1) {
        mv->error = 3;
        return;
    }

    sp = mv->registros[REG_SP];

    segSS = (mv->registros[REG_SS] >> 16) & MASCARA_16_BITS;
    segSP = (sp >> 16) & MASCARA_16_BITS;
    offsetSP = sp & MASCARA_16_BITS;

    if (segSS >= CANT_SEGS || segSP != segSS ||
        mv->tablaSegmentos[segSS] == ENTRADA_LIBRE) {
        mv->error = 3;
        return;
    }

    tamanioSS = mv->tablaSegmentos[segSS] & MASCARA_16_BITS;

    if (offsetSP > tamanioSS || tamanioSS - offsetSP < 4) {
        mv->error = 5;
        return;
    }

    leerMemoria(mv, sp, 4, &valorTope);

    if (mv->error != 0)
        return;

    set(mv, opa, valorTope);

    if (mv->error != 0)
        return;

    nuevoOffset = offsetSP + 4;
    mv->registros[REG_SP] = armarPunteroLogico(segSS, nuevoOffset);
}
void CALL(TMV *mv, int opa, int opb) {
    int opIP;

    /* Tipo 1: registro; sector 0: registro completo; REG_IP = 0. */
    opIP = (1 << 24) | REG_IP;

    PUSH(mv, opIP, 0);

    if (mv->error == 0)
        salto(mv, opa);
}

void RET(TMV *mv, int opa, int opb) {
    int opIP;

    /* Tipo 1: registro; sector 0: registro completo; REG_IP = 0. */
    opIP = (1 << 24) | REG_IP;

    POP(mv, opIP, 0);
}

/*
void PUSH(TMV *mv, int opa, int opb){
    int opA, sp;
    sp=mv->registros[REG_SP]-4;
    if (sp < mv->registros[REG_SS])
        mv->error=4;
    else {
        mv->registros[REG_SP]=sp;
        opA=get(mv, opa); // ya operandos cumple con la parte de devolver int de 32 bits
        escribirMemoria(mv, sp, 4, opA); // escribe en nuevo sp (tope de la pila) el valor
    }

}



void POP(TMV *mv, int opa, int opb){
    int valorTope, sp, segSS, offsetSP, tamanioSS;
    sp = mv->registros[REG_SP];
    segSS = (sp >> 16) & 0xFFFF; // segm-> 2 bytes mas sign
    offsetSP = sp & 0xFFFF; // 2 bytes menos sign
    // checkeo en la tabla de segmentos el tamanio de ss
    tamanioSS = mv->tablaSegmentos[segSS] & 0xFFFF;
    if (offsetSP + 4 > tamanioSS) // pila vacía? -> stack underflow (error 5)
        mv->error = 5; 
    else {
        leerMemoria(mv, sp, 4, &valorTope);
        set(mv, opa, valorTope); // reg q pasaste := valor del tope (algo asi como sacaP)
        mv->registros[REG_SP] = sp+4 ;
    }
}


void CALL(TMV *mv, int opa, int opb){
    int ip;
    ip=mv->registros[REG_IP];
    PUSH(mv, ip, 0);
    salto(mv, opa);
}



void RET(TMV *mv, int opa, int opb){
    POP(mv, REG_IP, 0); //pop->set->setReg y hace mv->registros[REG_IP] = valorTope
}


//--------------------------------------
*/

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
    if (opB > 0)          
        aleatorio = rand() % (opB + 1);
    else 
        aleatorio=0;
    set(mv, opa, aleatorio);
 
    // si opB es negativo, el documento no dice qué hacer
}
 
 
instrucciones vecInstr[MAXINSTR] = { SYS, JMP, JP, JN, JZ, JC, JV, JNP, JNN, JNZ, NOT, PUSH, POP, CALL, RET, STOP, MOV, ADD, SUB, MUL, DIV, CMP, AND, OR, XOR, SWAP, SHL, SHR, SAR, LDL, LDH, RND };
 
 
 
 
 

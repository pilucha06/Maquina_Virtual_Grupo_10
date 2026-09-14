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
se resuelve en tiempo O(1), ejecutandose directo en el hardware.*/
 
#include <stdio.h>
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

void leerMemoria(tRegMV *mv, int direLogica, int *valor){
    int i, direFisica;
    direFisica = dire_fisica(direLogica, mv, 4);
    if (direFisica == -1 ){
        printf("Error: fallo de segmento al leer memoria.\n");
    }
    else{
        //LAR = REG[4] y MAR = REG[5]
        mv->registros[4] = direLogica;
        mv->registros[5] = (4<<16) | (direFisica & 0xFFFF);
        //lectura
        *valor = 0; //x las dudas
        for(i=0; i<4; i++) 
            //voy concatenando 4 veces en "valor" los 8 bits de cada celda en RAM
            *valor = (*valor<<8) | mv->RAM[direFisica + i];
        //MBR = REG[6]
        mv->registros[6] = *valor;
    }
}

void escribirMemoria(tRegMV *mv, int direLogica, int valor){
    int direFisica;
    direFisica = dire_fisica(direLogica, mv, 4);
    if (direFisica == -1 ){
        printf("Error: fallo de segmento al escribir en memoria.\n");
    }
    else{
        //LAR = REG[4] y MAR = REG[5] y MBR = REG[6]
        mv->registros[4] = direLogica;
        mv->registros[5] = (4<<16) | (direFisica & 0xFFFF);
        mv->registros[6] = valor;
        //escribo 4  bytes en RAM
        mv->RAM[direFisica + 0] = (valor >> 24) & 0xFF;
        mv->RAM[direFisica + 1] = (valor >> 16) & 0xFF;
        mv->RAM[direFisica + 2] = (valor >> 8)  & 0xFF;
        mv->RAM[direFisica + 3] = valor & 0xFF; 
    }
}

//GETTERS

int getReg(tRegMV *mv, int op){
    int regNum ;
    regNum= op &  0x1F; //5 ult bits -> codreg
    return mv->registros[regNum];
}

int getInm(tRegMV *mv, int op){
    return op & 0xFFFF; //los 4 bytes
}

int getMem(tRegMV *mv, int op){
    int regNum, segmento, direLogica, offset, valor;
    regNum = op & 0x1F; // dire->5 ult bits
    offset = (op >> 8) &  0xFFFF; // 8=dire+3 reservados
    segmento= mv->registros[regNum];
    //calculo direccion logica = 2 bytes +s segmento + 2 bytes -s offset
    direLogica = (segmento << 16) | (offset & 0xFFFF);
    valor = 0;
    leerMemoria(mv, direLogica, &valor);
    return valor; 
}           

//SETTERS
void setReg(tRegMV *mv, int op, int valor){
    int regNum = op & 0x1F; //5 ult bits -> codreg
    mv->registros[regNum] = valor;
}

void setMem(tRegMV *mv, int op, int valor){
    int regNum, segmento, direLogica, offset;
    regNum = op & 0x1F; // dire->5 ult bits
    offset = (op >> 8) &  0xFFFF; // 8=dire+3 reservados
    segmento= mv->registros[regNum];
    //calculo direccion logica = 2 bytes +s segmento + 2 bytes -s offset
    direLogica = (segmento << 16) | (offset & 0xFFFF);
    escribirMemoria(mv, direLogica, valor);
}        

void setInm(tRegMV *mv, int op, int valor) {
    printf("Error: Operando inmediato no escribible.\n");
}

//FUNCIONES PPALES Y DEFINICIONES

typedef int (*getters)(tRegMV *, int);
typedef void (*setters)(tRegMV *, int, int);

getters vecGet[MAXVEC] = { NULL, getReg, getInm, getMem };
setters vecSet[MAXVEC] = { NULL, setReg, setInm, setMem };


//en la llamada, debemos referenciar:  OP1-> REG[2] y OP2-> REG[3]
int get(tRegMV *mv, int op){
   int tipo;
   tipo=(op >> 24) & 0xFF; 
   if (tipo >=1 && tipo<=3 && vecGet[tipo] != NULL) 
        return vecGet[tipo](mv, op);
    else
        return 0; //?
}

void set(tRegMV *mv, int op, int valor){
    int tipo;
    tipo = (op >> 24) & 0XFF;
    if (tipo>=1 && tipo <=3 && vecSet[tipo] != NULL)
        vecSet[tipo](mv, op, valor);
}


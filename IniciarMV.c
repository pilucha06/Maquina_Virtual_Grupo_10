#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define TAM_RAM 16384
#define CANT_REGS 32
#define CANT_SEGS 8
#define ENTRADA_LIBRE 0xFFFFFFFF
#define ID_MV  "VMX26"
#define TAM_ID 5
#define VER_MV 1

//SEGMENTOS DEFINIDOS DE LA TABLA PARA CS Y DS
#define SEG_COD 0
#define SEG_DAT 1

//CONST DE ERRORES Y SUS TIPOS EN INICIALIZACION
#define OK 0
#define ERR_ARCH 1
#define ERR_ID 2
#define ERR_VER 3
#define ERR_TAM_COD 4

typedef enum {
    REG_IP = 0, REG_OPC, REG_OP1, REG_OP2,
    REG_LAR, REG_MAR, REG_MBR,
    REG_EAX = 10, REG_EBX, REG_ECX, REG_EDX, REG_EEX, REG_EFX,
    REG_AC = 16, REG_CC,
    REG_CS = 26, REG_DS
} CodigoRegistro;

typedef struct {
    unsigned char  *RAM;          // 16384 celdas de 1 byte
    int  *registros;        // vector de 32 registros de 4 bytes
    unsigned int *tablaSegmentos;   // 8 entradas, 2 bytes base + 2 bytes tamaño

    char     id[TAM_ID + 1];  // "VMX26" + '\0'
    unsigned char  version;
} TMV;

int reservoEspacioMV(TMV *MV){
    MV->RAM = calloc(TAM_RAM, sizeof(unsigned char)); //reserva 16384 bytes en el heap todos en 0 y devuelve puntero a 1er posicion
    if(MV->RAM == NULL)
        return 0;

    MV->registros = calloc(CANT_REGS, sizeof(int));
    if (MV->registros == NULL){
        free(MV->RAM);
        return 0;
    }

    MV->tablaSegmentos = calloc(CANT_SEGS, sizeof(unsigned int));
    if (MV->tablaSegmentos == NULL){
        free(MV->RAM);
        free(MV->registros);
        return 0;
    } 

    return 1;
}

unsigned int armarEntrada(unsigned short base, unsigned short tam) {
    return ((unsigned int)base << 16) | tam;
}

unsigned int armarPunteroLogico(unsigned short segmento, unsigned short offset){
    return ((unsigned int)segmento << 16) | offset;
}

int inicializoMV(TMV *MV, char nomArch[]){
    FILE *arch;
    unsigned char aux[2]; //sirve para ayudar a leer el header
    size_t num; //verificador de que los reads den bien
    int i, baseCod = 0;
    unsigned short tamCod;

    arch = fopen(nomArch, "rb");
    if (arch == NULL)
        return ERR_ARCH;

    //LEO ID
    num = fread(MV->id, sizeof(char), TAM_ID, arch);
    MV->id[TAM_ID] = '\0'; //pongo fin de string
    if (num < TAM_ID){
        fclose(arch);
        return ERR_ARCH;
    }
    else if (strcmp(MV->id, ID_MV) != 0){
            fclose(arch);
            return ERR_ID;
         }
    
    //LEO VERSION
    num = fread(&MV->version, sizeof(char), 1, arch);
    if (num < 1){
        fclose(arch);
        return ERR_ARCH;
    }
    else if(MV->version != VER_MV){
            fclose(arch);
            return ERR_VER;
         }

    //LEO SIZE
    num = fread(aux, sizeof(char), 2, arch);
    if (num < 2){
        fclose(arch);
        return ERR_ARCH;
    }
    else{
        tamCod = (aux[0] << 8) | aux[1];
        if (tamCod > TAM_RAM){
            fclose(arch);
            return ERR_TAM_COD;
        }
    }

    //TABLA DE SEGMENTOS
    MV->tablaSegmentos[SEG_COD] = armarEntrada(baseCod, tamCod);
    MV->tablaSegmentos[SEG_DAT] = armarEntrada(baseCod + tamCod, TAM_RAM - tamCod);
    for (i = 2; i < CANT_SEGS; i++)
        MV->tablaSegmentos[i] = ENTRADA_LIBRE;

    //REGISTROS
    MV->registros[REG_CS] = armarPunteroLogico(SEG_COD, 0);
    MV->registros[REG_DS] = armarPunteroLogico(SEG_DAT, 0);
    MV->registros[REG_IP] = MV->registros[REG_CS];

    num = fread(MV->RAM + baseCod, sizeof(char), tamCod, arch);
    if (num < tamCod){
        fclose(arch);
        return ERR_ARCH;
    }

    fclose(arch);
    return OK;
}

void liberarMV(TMV *MV) {
    free(MV->RAM);
    MV->RAM = NULL;

    free(MV->registros);
    MV->registros = NULL;

    free(MV->tablaSegmentos);
    MV->tablaSegmentos = NULL;
}

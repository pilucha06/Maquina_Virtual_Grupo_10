#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "IniciarMV.h"

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

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "IniciarMV.h"

// En la v1 la tabla es fija: codigo en la entrada 0 y datos en la 1
#define V1_IDX_COD 0
#define V1_IDX_DAT 1

int reservoEspacioMV(TMV *MV, unsigned int tamRam){
    MV->tam_ram = tamRam;
    MV->RAM = calloc(tamRam, sizeof(unsigned char)); //reserva tamRam bytes en el heap, todos en 0
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
    char id[TAM_ID + 1];
    unsigned char version;

    arch = fopen(nomArch, "rb");
    if (arch == NULL)
        return ERR_ARCH;

    //LEO ID
    num = fread(id, sizeof(char), TAM_ID, arch);
    id[TAM_ID] = '\0'; //pongo fin de string
    if (num < TAM_ID){
        fclose(arch);
        return ERR_ARCH;
    }
    else if (strcmp(id, ID_MV) != 0){
            fclose(arch);
            return ERR_ID;
         }
    
    //LEO VERSION
    num = fread(&version, sizeof(char), 1, arch);
    if (num < 1){
        fclose(arch);
        return ERR_ARCH;
    }
    else if(version != VER_1){ // PENDIENTE: aceptar VER_2 cuando este la carga v2
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
        if (tamCod > MV->tam_ram){
            fclose(arch);
            return ERR_TAM_COD;
        }
    }

    //TABLA DE SEGMENTOS
    MV->tablaSegmentos[V1_IDX_COD] = armarEntrada(baseCod, tamCod);
    MV->tablaSegmentos[V1_IDX_DAT] = armarEntrada(baseCod + tamCod, MV->tam_ram - tamCod);
    for (i = 2; i < CANT_SEGS; i++)
        MV->tablaSegmentos[i] = ENTRADA_LIBRE;

    //REGISTROS
    MV->registros[REG_CS] = armarPunteroLogico(V1_IDX_COD, 0);
    MV->registros[REG_DS] = armarPunteroLogico(V1_IDX_DAT, 0);
    MV->registros[REG_ES] = -1;   // segmentos que no existen en la v1
    MV->registros[REG_SS] = -1;
    MV->registros[REG_KS] = -1;
    MV->registros[REG_PS] = -1;
    MV->registros[REG_IP] = MV->registros[REG_CS];

    num = fread(MV->RAM + baseCod, sizeof(char), tamCod, arch);
    if (num < tamCod){
        fclose(arch);
        return ERR_ARCH;
    }
    MV->error=0;
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
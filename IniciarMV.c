#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "IniciarMV.h"

// En la v1 la tabla es fija: codigo en la entrada 0 y datos en la 1
#define V1_IDX_COD 0
#define V1_IDX_DAT 1

int reservoEspacioMV(TMV *MV, unsigned int tamRam){
    MV->tam_ram = tamRam;
    MV->nombreVMI = NULL;
    MV->error = 0;
    MV->estadoDebug = DBG_NORMAL;
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

// Devuelve 1 si str termina exactamente en ext (ej: ".vmx"), 0 si no
static int terminaEn(const char *str, const char *ext) {
    size_t largoStr = strlen(str);
    size_t largoExt = strlen(ext);
    int resultado = 0;

    if (largoStr >= largoExt)
        resultado = (strcmp(str + largoStr - largoExt, ext) == 0);
    return resultado;
}

int leerArgumentos(int argc, char *argv[], TArgumentos *args) {
    int i = 1;            // argv[0] es el nombre del ejecutable, se saltea
    int finFlags = 0;     // se pone en 1 cuando aparece -p
    int result = OK;
    long posibleM;
    char *inicioM, *finPointer;

    // Valores por defecto
    args->nomVMX = NULL;
    args->nomVMI = NULL;
    args->tamMemKiB = TAM_RAM_DEFAULT / 1024;   // 16 KiB
    args->mostrarDis = 0;
    args->posParam = 0;
    args->cantParam = 0;

    while (i < argc && !finFlags && result == OK) {
        if (terminaEn(argv[i], ".vmx"))
            args->nomVMX = argv[i];

        else if (terminaEn(argv[i], ".vmi"))
            args->nomVMI = argv[i];

        else if (strncmp(argv[i], "m=", 2) == 0) {
            inicioM = argv[i] + 2;              // apunta a lo que sigue a "m="
            posibleM = strtol(inicioM, &finPointer, 10);
            if (finPointer == inicioM || *finPointer != '\0' || posibleM <= 0 || posibleM > MAX_MEM_KIB)
                result = ERR_ARG_M;          // vacío, con basura atrás, o fuera de rango
            else
                args->tamMemKiB = (unsigned int)posibleM;
        }

        else if (strcmp(argv[i], "-d") == 0)
            args->mostrarDis = 1;

        else if (strcmp(argv[i], "-p") == 0) {
            finFlags = 1;
            args->posParam = i + 1;            // el primer parámetro está justo después de -p
            args->cantParam = argc - (i + 1);  // todo lo que queda hasta el final
        }
        // cualquier otro argumento se ignora
        i++;
    }

    if (result == OK && args->nomVMX == NULL && args->nomVMI == NULL)
        result = ERR_SIN_ARCH;               // el PDF exige al menos uno de los dos

    if (args->nomVMX == NULL)
        args->cantParam = 0;                 // sin .vmx, los parámetros de -p se ignoran

    return result;
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
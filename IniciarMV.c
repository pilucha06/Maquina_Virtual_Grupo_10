#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "IniciarMV.h"

typedef enum {TS_PARAM = 0, TS_CONST, TS_CODE, TS_DATA, TS_EXTRA, TS_STACK, CANT_SEG_PROC } TipoSegmento;

typedef struct {
    unsigned short tam[CANT_SEG_PROC];   // tamaño en bytes de cada segmento, 0 = no existe
    unsigned short entryPoint;           // offset dentro del Code Segment
} TTamanios;

// Orden en que el header v2 trae los tamaños de los segmentos (bytes 6 a 15).
// No coincide con el orden en memoria (TipoSegmento), por eso cada lectura
// se guarda en tam->tam[ordenHeaderV2[i]].
static const TipoSegmento ordenHeaderV2[] = { TS_CODE, TS_DATA, TS_EXTRA, TS_STACK, TS_CONST };
#define CANT_TAM_HEADER_V2 5

// Registro de segmento que corresponde a cada TipoSegmento (mismo orden que el enum)
static const CodigoRegistro regDeSegmento[CANT_SEG_PROC] = { REG_PS, REG_KS, REG_CS, REG_DS, REG_ES, REG_SS };

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

// Lee 2 bytes en big endian del archivo y los deja en *valor.
// Devuelve OK, o ERR_ARCH si el archivo se cortó antes.
static int leer16(FILE *arch, unsigned short *valor) {
    unsigned char aux[2];
    int result = OK;

    if (fread(aux, sizeof(unsigned char), 2, arch) < 2)
        result = ERR_ARCH;
    else
        *valor = (unsigned short)((aux[0] << 8) | aux[1]);
    return result;
}

// Header v1: solo trae el tamaño del código (bytes 6-7).
// En la v1 el código va en el Code Segment y TODA la memoria que sobra es el Data Segment.
// No hay Param, Const, Extra ni Stack, y la ejecución arranca en el offset 0.
static int leerHeaderV1(FILE *arch, unsigned int tamRam, TTamanios *tam) {
    unsigned short tamCod;
    int i, result;

    for (i = 0; i < CANT_SEG_PROC; i++)
        tam->tam[i] = 0;
    tam->entryPoint = 0;

    result = leer16(arch, &tamCod);
    if (result == OK) {
        if (tamCod > tamRam)
            result = ERR_TAM_COD;
        else {
            tam->tam[TS_CODE] = tamCod;
            tam->tam[TS_DATA] = (unsigned short)(tamRam - tamCod);
        }
    }
    return result;
}

// Header v2: trae los tamaños de Code, Data, Extra, Stack y Const (en ese orden)
// y el offset del entry point. El Param Segment no viene en el header: lo calcula
// otra función a partir de los argumentos.
static int leerHeaderV2(FILE *arch, TTamanios *tam) {
    int i, result = OK;

    for (i = 0; i < CANT_SEG_PROC; i++)
        tam->tam[i] = 0;
    tam->entryPoint = 0;

    i = 0;
    while (i < CANT_TAM_HEADER_V2 && result == OK) {
        result = leer16(arch, &tam->tam[ordenHeaderV2[i]]);
        i++;
    }

    if (result == OK)
        result = leer16(arch, &tam->entryPoint);

    if (result == OK) {
        if (tam->tam[TS_CODE] == 0 || tam->tam[TS_STACK] == 0 || tam->entryPoint >= tam->tam[TS_CODE])
            result = ERR_HEADER;
    }

    return result;
}

// Arma el proceso en memoria a partir de los tamaños del header:
// tabla de segmentos (contigua, sin huecos), registros de segmento, IP y SP,
// y carga el código y las constantes desde el archivo.
static int armarProceso(TMV *MV, TTamanios *tam, FILE *arch) {
    unsigned int total = 0;               // suma de todos los tamaños
    unsigned int base = 0;                // dónde empieza el próximo segmento en la RAM
    unsigned int baseSeg[CANT_SEG_PROC];  // base física de cada segmento (para cargar código y constantes)
    int i, idx = 0;                       // idx = próxima entrada libre de la tabla
    int result = OK;
    size_t leidos;

    // 1. ¿Entra el proceso?
    for (i = 0; i < CANT_SEG_PROC; i++)
        total += tam->tam[i];
    if (total > MV->tam_ram)
        result = ERR_MEM_INSUF;

    if (result == OK) {
        // 2. Tabla y registros de segmento, en orden Param -> Stack
        for (i = 0; i < CANT_SEG_PROC; i++) {
            baseSeg[i] = base;
            if (tam->tam[i] > 0) {
                MV->tablaSegmentos[idx] = armarEntrada(base, tam->tam[i]);
                MV->registros[regDeSegmento[i]] = armarPunteroLogico(idx, 0);
                base += tam->tam[i];    // el próximo segmento empieza donde termina este
                idx++;                  // y va en la siguiente entrada de la tabla
            }
            else
                MV->registros[regDeSegmento[i]] = -1;
        }
        for (i = idx; i < CANT_SEGS; i++)        // las entradas que sobran quedan libres
            MV->tablaSegmentos[i] = ENTRADA_LIBRE;

        // 3. IP y SP
        // IP: segmento del CS (parte alta) + offset del entry point (parte baja)
        MV->registros[REG_IP] = (MV->registros[REG_CS] & 0xFFFF0000) | tam->entryPoint;
        // SP: pila vacía = apunta justo después del final del Stack Segment
        if (MV->registros[REG_SS] == -1)
            MV->registros[REG_SP] = -1;            // sin pila (v1)
        else
            MV->registros[REG_SP] = (MV->registros[REG_SS] & 0xFFFF0000) | tam->tam[TS_STACK];

        // 4. Cargar código y constantes (en el archivo vienen en ese orden)
        leidos = fread(MV->RAM + baseSeg[TS_CODE], sizeof(unsigned char), tam->tam[TS_CODE], arch);
        if (leidos < tam->tam[TS_CODE])
            result = ERR_ARCH;
        else {
            leidos = fread(MV->RAM + baseSeg[TS_CONST], sizeof(unsigned char), tam->tam[TS_CONST], arch);
            if (leidos < tam->tam[TS_CONST])
                result = ERR_ARCH;
        }
    }

    return result;
}

int inicializoMV(TMV *MV, char nomArch[]){
    FILE *arch;
    size_t num; //verificador de que los reads den bien
    int result;
    char id[TAM_ID + 1];
    unsigned char version;
    TTamanios tam;

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

    //LEO VERSION Y EL RESTO DEL HEADER (segun la version)
    num = fread(&version, sizeof(char), 1, arch);
    if (num < 1){
        fclose(arch);
        return ERR_ARCH;
    }

    switch (version) {
        case VER_1:
            result = leerHeaderV1(arch, MV->tam_ram, &tam);
            break;
        case VER_2:
            result = leerHeaderV2(arch, &tam);
            break;
        default:
            result = ERR_VER;
            break;
    }
    if (result != OK){
        fclose(arch);
        return result;
    }
    
    //ARMO EL PROCESO (tabla, registros, IP, SP) Y CARGO CODIGO Y CONSTANTES
    result = armarProceso(MV, &tam, arch);
    if (result != OK){
        fclose(arch);
        return result;
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
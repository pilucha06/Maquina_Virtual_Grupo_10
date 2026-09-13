#ifndef MV_H
#define MV_H

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

//ENUMERACION DE REGISTROS
typedef enum {
    REG_IP = 0, REG_OPC, REG_OP1, REG_OP2, 
    REG_LAR, REG_MAR, REG_MBR,
    REG_EAX = 10, REG_EBX, REG_ECX, REG_EDX, REG_EEX, REG_EFX,
    REG_AC = 16, REG_CC,
    REG_CS = 26, REG_DS
} CodigoRegistro;

//STRUCT - DEFINICION DE LA MAQUINA VIRTUAL
typedef struct {
    unsigned char  *RAM;          // 16384 celdas de 1 byte
    int  *registros;        // vector de 32 registros de 4 bytes
    unsigned int *tablaSegmentos;   // 8 entradas, 2 bytes base + 2 bytes tamaño

    char     id[TAM_ID + 1];  // "VMX26" + '\0'
    unsigned char  version;
} TMV;

//UTILIDADES PARA ARMAR Y LEER VALORES EMPAQUETADOS
unsigned int armarEntrada(unsigned short base, unsigned short tam);
unsigned int armarPunteroLogico(unsigned short segmento, unsigned short offset);

#endif
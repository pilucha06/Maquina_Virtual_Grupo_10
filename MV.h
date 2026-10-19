#ifndef MV_H
#define MV_H

#define TAM_RAM_DEFAULT 16384
#define CANT_REGS 32
#define CANT_SEGS 8
#define ENTRADA_LIBRE 0xFFFFFFFF
#define ID_MV  "VMX26"
#define TAM_ID 5

//CONST DE ERRORES Y SUS TIPOS EN INICIALIZACION
#define OK 0
#define ERR_ARCH 1
#define ERR_ID 2
#define ERR_VER 3
#define ERR_TAM_COD 4

typedef enum {
    VER_1 = 1, VER_2 = 2
} VersionesMV;

//ENUMERACION DE REGISTROS
typedef enum {
    REG_IP = 0, REG_OPC, REG_OP1, REG_OP2,
    REG_LAR, REG_MAR, REG_MBR,
    REG_SP = 7, REG_BP = 8,
    REG_EAX = 10, REG_EBX, REG_ECX, REG_EDX, REG_EEX, REG_EFX,
    REG_AC = 16, REG_CC,
    REG_CS = 26, REG_DS, REG_ES, REG_SS, REG_KS, REG_PS
} CodigoRegistro;
typedef enum {
    DBG_NORMAL = 0, DBG_PASO, DBG_SALIR
} EstadoDebug;
//STRUCT - DEFINICION DE LA MAQUINA VIRTUAL
typedef struct {
    unsigned char *RAM;             // memoria principal, celdas de 1 byte
    unsigned int  tam_ram;          // tamaño de la memoria EN BYTES (default TAM_RAM_DEFAULT o m=M * 1024)
    int           *registros;       // vector de 32 registros de 4 bytes
    unsigned int  *tablaSegmentos;  // 8 entradas: 2 bytes base + 2 bytes tamaño
    short int     error;
    char       *nombreVMI;    // nombre del .vmi, NULL si no vino por parametro
    EstadoDebug estadoDebug;  // NORMAL / PASO / SALIR            
} TMV;

// error de ejecucion: 0 ok, 1 instr invalida, 
//2 div cero, 3 segmento, 
//4 stack overflow, 5 stack underflow

//UTILIDADES PARA ARMAR Y LEER VALORES EMPAQUETADOS
unsigned int armarEntrada(unsigned short base, unsigned short tam);
unsigned int armarPunteroLogico(unsigned short segmento, unsigned short offset);

#endif
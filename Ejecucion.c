//Acá se va a realizar la ejecución de la máquina (el ciclo while básicamente)
/*Arranca con la maquina ya inicializada, el primer llamado desde el main sería ejecutar(MV, modo_disassembler)
después de inicializar todas las estructuras*/

#include <stdio.h>
#include "Ejecucion.h"

// Máscaras y corrimientos propios de la codificación de instrucciones/operandos
// (no están en MV.h porque son de formato, no de la estructura de la máquina)
#define MASCARA_16_BITS         0xFFFF
#define BITS_SEGMENTO           16
#define BITS_TIPO_OPERANDO      24
#define MASCARA_VALOR_OPERANDO  0x00FFFFFF
#define MASCARA_BYTE            0xFF
#define MASCARA_OPCODE          0x1F
#define OPCODE_STOP             0x0F
#define TAM_BUFFER_TEXTO        32
#define BIT_SIGNO_16            0x8000  // bit más alto de un valor de 16 bits (indica negativo)
#define COMPLEMENTO_16          0x10000 // para restaurar el signo real de un valor de 16 bits

const char *nombres_registros[CANT_REGS] = {
    [REG_IP]  = "IP",  [REG_OPC] = "OPC", [REG_OP1] = "OP1", [REG_OP2] = "OP2",
    [REG_LAR] = "LAR", [REG_MAR] = "MAR", [REG_MBR] = "MBR",
    [REG_EAX] = "EAX", [REG_EBX] = "EBX", [REG_ECX] = "ECX", [REG_EDX] = "EDX",
    [REG_EEX] = "EEX", [REG_EFX] = "EFX",
    [REG_AC]  = "AC",  [REG_CC]  = "CC",
    [REG_CS]  = "CS",  [REG_DS]  = "DS"
    // el resto (7,8,9,18-25,28-31) quedan en NULL: reservados/no usados
};

const char *nombres_mnemonicos[CANT_REGS] = {
    [0x00] = "SYS",  [0x01] = "JMP", [0x02] = "JP",  [0x03] = "JN",
    [0x04] = "JZ",   [0x05] = "JC",  [0x06] = "JV",  [0x07] = "JNP",
    [0x08] = "JNN",  [0x09] = "JNZ", [0x0A] = "NOT", [OPCODE_STOP] = "STOP",
    [0x10] = "MOV",  [0x11] = "ADD", [0x12] = "SUB", [0x13] = "MUL",
    [0x14] = "DIV",  [0x15] = "CMP", [0x16] = "AND", [0x17] = "OR",
    [0x18] = "XOR",  [0x19] = "SWAP",[0x1A] = "SHL", [0x1B] = "SHR",
    [0x1C] = "SAR",  [0x1D] = "LDL", [0x1E] = "LDH", [0x1F] = "RND"
    // el resto queda en NULL: opcode inválido
};

/* direc_fisica: a partir de una dirección lógica y una cantidad de bytes a leer,
   valida que la posición inicial sea correcta y que se puedan leer esos bytes
   sin salirse del segmento. Si es válida, devuelve la posición física inicial. */
int direc_fisica(int direc_logica, TMV *MV, int cant_bytes) {
    int seg    = (direc_logica >> BITS_SEGMENTO) & MASCARA_16_BITS;
    int offset = direc_logica & MASCARA_16_BITS;

    if (seg < 0 || seg >= CANT_SEGS || MV->tablaSegmentos[seg] == ENTRADA_LIBRE)
        return -1; // índice inválido en la tabla o entrada no usada

    int base        = (MV->tablaSegmentos[seg] >> BITS_SEGMENTO) & MASCARA_16_BITS;
    int tamanio_seg = MV->tablaSegmentos[seg] & MASCARA_16_BITS;

    if (offset < 0 || offset + cant_bytes > tamanio_seg)
        return -1; // se cae del segmento

    return base + offset;
}

/* calcula_cant_op: a partir del primer byte de la instrucción, decide si es de
   0, 1 o 2 operandos, y carga los tipos correspondientes por puntero. */
void calcula_cant_op(int instruccion, int *tipo_A, int *tipo_B, int *cant_op) {
    if (((instruccion >> 4) & 0x01) != 0) {
        // dos operandos
        *cant_op = 2;
        *tipo_A = (instruccion >> 4) & 0x03;
        *tipo_B = (instruccion >> 6) & 0x03;
    } else if ((instruccion & MASCARA_OPCODE) != OPCODE_STOP) {
        // un operando
        *cant_op = 1;
        *tipo_A = (instruccion >> 6) & 0x03;
        *tipo_B = 0;
    } else {
        // cero operandos (STOP)
        *cant_op = 0;
        *tipo_A = 0;
        *tipo_B = 0;
    }
}

/* get_RAM: concatena "tipo_op" bytes de la RAM a partir de fisica_ip,
   armando el valor completo (byte más significativo primero). */
int get_RAM(int tipo_op, int fisica_ip, TMV *MV) {
    int valor = 0;
    for (int i = 0; i < tipo_op; i++) {
        valor = (valor << 8) | MV->RAM[fisica_ip + i]; // RAM ya es unsigned char*, no hace falta castear
    }
    return valor;
}

/* extender_signo_16: interpreta un valor de 16 bits (offset o inmediato) como
   número con signo (complemento a 2), según pide el documento (-32768..32767).
   Sin esto, cualquier valor negativo se leería como un número positivo grande. */
int extender_signo_16(int valor16) {
    if (valor16 & BIT_SIGNO_16)
        return valor16 - COMPLEMENTO_16;
    return valor16;
}

void imprimir_instruccion_hex(TMV *MV, int fisica_inicio, int tamanio_total) {
    for (int i = 0; i < tamanio_total; i++) {
        printf("%02X ", MV->RAM[fisica_inicio + i]);
    }
}

void formatear_operando(int operando, char *buffer) {
    int tipo  = (operando >> BITS_TIPO_OPERANDO) & MASCARA_BYTE;
    int valor = operando & MASCARA_VALOR_OPERANDO;

    switch (tipo) {
        case 0: // no existe
            buffer[0] = '\0';
            break;
        case 1: // registro
            snprintf(buffer, TAM_BUFFER_TEXTO, "%s", nombres_registros[valor & MASCARA_OPCODE]);
            break;
        case 2: { // inmediato (con signo)
            int inmediato = extender_signo_16(valor & MASCARA_16_BITS);
            snprintf(buffer, TAM_BUFFER_TEXTO, "%d", inmediato);
            break;
        }
        case 3: { // memoria: [REG+offset] o [REG-offset]
            int codigo_reg = valor & MASCARA_OPCODE;
            int offset     = extender_signo_16((valor >> 8) & MASCARA_16_BITS);
            if (offset >= 0)
                snprintf(buffer, TAM_BUFFER_TEXTO, "[%s+%d]", nombres_registros[codigo_reg], offset);
            else
                snprintf(buffer, TAM_BUFFER_TEXTO, "[%s%d]", nombres_registros[codigo_reg], offset);
            break;
        }
    }
}

void mostrar_desensamblado(TMV *MV, int fisica_ip, int tamanio_total) {
    char op_A_str[TAM_BUFFER_TEXTO], op_B_str[TAM_BUFFER_TEXTO];
    formatear_operando(MV->registros[REG_OP1], op_A_str);
    formatear_operando(MV->registros[REG_OP2], op_B_str);

    printf("[%04X] ", fisica_ip);
    imprimir_instruccion_hex(MV, fisica_ip, tamanio_total);
    printf("| %s", nombres_mnemonicos[MV->registros[REG_OPC]]);

    if (op_A_str[0] != '\0' && op_B_str[0] != '\0')
        printf(" %s, %s\n", op_A_str, op_B_str);
    else if (op_A_str[0] != '\0')
        printf(" %s\n", op_A_str);
    else
        printf("\n");
}
void ejecutar(TMV *MV, int modo_disassembler) {
    int corriendo = 1;
    while (corriendo) {

        // Paso 1: validar que exista al menos el primer byte de la instrucción
        int fisica_ip = direc_fisica(MV->registros[REG_IP], MV, 1);
        if (fisica_ip == -1) {
            printf("Error: fallo de segmento\n");
            corriendo = 0;
            break;
        }

        // Paso 2: distingo la operación y los tipos de operandos
        int instruccion = MV->RAM[fisica_ip];
        int cant_op, tipo_A, tipo_B;
        calcula_cant_op(instruccion, &tipo_A, &tipo_B, &cant_op);

        // Paso 3: valido poder leer la instrucción completa (opcode + operandos)
        int tamanio_op = tipo_A + tipo_B + 1;
        if (cant_op != 0) {
            if (direc_fisica(MV->registros[REG_IP], MV, tamanio_op) == -1) {
                printf("Error: fallo de segmento\n");
                corriendo = 0;
                break;
            }
        }

        // Paso 4: cargo OPC, OP1 y OP2
        MV->registros[REG_OPC] = instruccion & MASCARA_OPCODE;
        int inicio_lec = fisica_ip + 1;

        switch (cant_op) {
            case 1:
                MV->registros[REG_OP1] = (tipo_A << BITS_TIPO_OPERANDO) | get_RAM(tipo_A, inicio_lec, MV);
                MV->registros[REG_OP2] = 0;
                break;
            case 2:
                // se codifican en orden inverso: primero B, después A
                MV->registros[REG_OP2] = (tipo_B << BITS_TIPO_OPERANDO) | get_RAM(tipo_B, inicio_lec, MV);
                MV->registros[REG_OP1] = (tipo_A << BITS_TIPO_OPERANDO) | get_RAM(tipo_A, inicio_lec + tipo_B, MV);
                break;
            default:
                MV->registros[REG_OP1] = MV->registros[REG_OP2] = 0;
        }

        // Paso 5: si está activo el modo -d, muestro el desensamblado
        if (modo_disassembler) {
            mostrar_desensamblado(MV, fisica_ip, tamanio_op);
        }

        // Paso 6: avanzo el IP (solo el offset, preservando el segmento)
        int offset_actual = MV->registros[REG_IP] & MASCARA_16_BITS;
        offset_actual += tamanio_op;
        MV->registros[REG_IP] = (MV->registros[REG_IP] & ~MASCARA_16_BITS) | (offset_actual & MASCARA_16_BITS);

        // FALTA: acá va el switch/dispatch que ejecuta la operación según MV->registros[REG_OPC]
        // (MOV, ADD, SUB, JMP, STOP, etc.) — sin esto, el programa nunca hace nada
        // con la instrucción ni corta la ejecución salvo por error de segmento.
    }
}
//Acá se va a realizar la ejecución de la máquina (el ciclo while básicamente)
/*Arranca con la maquina ya inicializada, el primer llamado desde el main sería ejecutar(MV, modo_disassembler)
después de inicializar todas las estructuras*/

#include <stdlib.h>
#include <stdio.h>
#include "Ejecucion.h"
#include "Instrucciones.h"
#include "Imagen.h"
#include "Operandos.h"

// Máscaras y corrimientos propios de la codificación de instrucciones/operandos
// (no están en MV.h porque son de formato, no de la estructura de la máquina)
#define BITS_SEGMENTO           16
#define BITS_TIPO_OPERANDO      24
#define MASCARA_VALOR_OPERANDO  0x00FFFFFF
#define MASCARA_BYTE            0xFF
#define MASCARA_OPCODE          0x1F
#define OPCODE_STOP             0x0F
#define TAM_BUFFER_TEXTO        32

const char *nombres_registros[CANT_REGS] = {
    [REG_IP]  = "IP",  [REG_OPC] = "OPC", [REG_OP1] = "OP1", [REG_OP2] = "OP2",
    [REG_LAR] = "LAR", [REG_MAR] = "MAR", [REG_MBR] = "MBR", [REG_SP]  = "SP",[REG_BP]  = "BP",
    [REG_EAX] = "EAX", [REG_EBX] = "EBX", [REG_ECX] = "ECX", [REG_EDX] = "EDX",
    [REG_EEX] = "EEX", [REG_EFX] = "EFX",
    [REG_AC]  = "AC",  [REG_CC]  = "CC",
    [REG_CS]  = "CS",  [REG_DS]  = "DS", [REG_ES]  = "ES",  [REG_SS]  = "SS",
    [REG_KS]  = "KS",  [REG_PS]  = "PS",
    // El resto (9, 18-25) queda en NULL: reservados/no usados.
};

const char *nombres_mnemonicos[CANT_REGS] = {
    [0x00] = "SYS",  [0x01] = "JMP", [0x02] = "JP",  [0x03] = "JN",
    [0x04] = "JZ",   [0x05] = "JC",  [0x06] = "JV",  [0x07] = "JNP",
    [0x08] = "JNN",  [0x09] = "JNZ", [0x0A] = "NOT",[0x0B] = "PUSH", 
    [0x0C] = "POP", [0x0D] = "CALL", [0x0E] = "RET",[OPCODE_STOP] = "STOP",
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
    int seg        = (direc_logica >> BITS_SEGMENTO) & MASCARA_16_BITS;
    int offset_raw = direc_logica & MASCARA_16_BITS;
   
    if (seg >= CANT_SEGS || MV->tablaSegmentos[seg] == ENTRADA_LIBRE)
        return -1; // índice inválido en la tabla o entrada no usada

    int base        = (MV->tablaSegmentos[seg] >> BITS_SEGMENTO) & MASCARA_16_BITS;
    int tamanio_seg = MV->tablaSegmentos[seg] & MASCARA_16_BITS;
    if (cant_bytes < 0 ||offset_raw > tamanio_seg ||cant_bytes > tamanio_seg - offset_raw)
        return -1;
    int fisica = base + offset_raw;
    if (fisica > MV->tam_ram ||cant_bytes > MV->tam_ram - fisica)
        return -1;

    return fisica;
}
/* calcula_cant_op: a partir del primer byte de la instrucción, decide si es de
   0, 1 o 2 operandos, y carga los tipos correspondientes por puntero. */
void calcula_cant_op(int instruccion, int *tipo_A, int *tipo_B, int *cant_op) {
    if (((instruccion >> 4) & 0x01) != 0) {
        // dos operandos
        *cant_op = 2;
        *tipo_A = (instruccion >> 4) & 0x03;
        *tipo_B = (instruccion >> 6) & 0x03;
    } else if ((instruccion & MASCARA_OPCODE) != OPCODE_STOP && (instruccion & MASCARA_OPCODE) != 0x0E) {
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
void imprimir_instruccion_hex(TMV *MV, int fisica_inicio, int tamanio_total) {
    for (int i = 0; i < tamanio_total; i++) {
        printf("%02X ", MV->RAM[fisica_inicio + i]);
    }
}

void formatear_operando(int operando, char *buffer) {
    int tipo  = (operando >> BITS_TIPO_OPERANDO) & MASCARA_BYTE;
    int valor = operando & MASCARA_VALOR_OPERANDO;

    switch (tipo) {
        case 0: // Sin operando
            buffer[0] = '\0';
            break;

        case 1: { // Registro
            int codigo_reg = valor & MASCARA_OPCODE;
            int sector = (valor >> 5) & 0x03;
            const char *nombre = nombres_registros[codigo_reg];

            // Solo EAX, EBX, ECX, EDX, EEX y EFX tienen sectores
            if (codigo_reg >= REG_EAX && codigo_reg <= REG_EFX) {
                char letra = nombre[1];

                switch (sector) {
                    case 1:
                        snprintf(buffer, TAM_BUFFER_TEXTO, "%cL", letra);
                        break;
                    case 2:
                        snprintf(buffer, TAM_BUFFER_TEXTO, "%cH", letra);
                        break;
                    case 3:
                        snprintf(buffer, TAM_BUFFER_TEXTO, "%cX", letra);
                        break;
                    default:
                        snprintf(buffer, TAM_BUFFER_TEXTO, "%s", nombre);
                        break;
                }
            } else {
                // Los demás registros no tienen pseudónimos de sector
                snprintf(buffer, TAM_BUFFER_TEXTO, "%s", nombre);
            }
            break;
        }

        case 2: { // Inmediato con signo
            int inmediato = extenderSigno(valor & MASCARA_16_BITS, 2);
            snprintf(buffer, TAM_BUFFER_TEXTO, "%d", inmediato);
            break;
        }

        case 3: { // Memoria
            int codigo_reg = valor & MASCARA_OPCODE;
            int modificador = (valor >> 5) & 0x07;
            int offset = extenderSigno(
                (valor >> 8) & MASCARA_16_BITS, 2
            );

            const char *nombre = nombres_registros[codigo_reg];
            char prefijo = '\0';

            switch (modificador) {
                case 1:
                    prefijo = 'b';
                    break;
                case 2:
                    prefijo = 'w';
                    break;
                case 4:
                    prefijo = 'l';
                    break;
                default:
                    // Modificador 0: formato de la MV1, sin prefijo
                    break;
            }

            if (offset >= 0) {
                if (prefijo != '\0')
                    snprintf(buffer, TAM_BUFFER_TEXTO,
                             "%c[%s+%d]", prefijo, nombre, offset);
                else
                    snprintf(buffer, TAM_BUFFER_TEXTO,
                             "[%s+%d]", nombre, offset);
            } else {
                if (prefijo != '\0')
                    snprintf(buffer, TAM_BUFFER_TEXTO,
                             "%c[%s%d]", prefijo, nombre, offset);
                else
                    snprintf(buffer, TAM_BUFFER_TEXTO,
                             "[%s%d]", nombre, offset);
            }
            break;
        }
    }
}
/*FUNCIONES DESENSAMBLADO*/

void mostrar_desensamblado(int fisica_ip, TMV *MV, int opc, int op1, int op2, int tamanio_total, int es_entry_point){
    char op_A_str[TAM_BUFFER_TEXTO], op_B_str[TAM_BUFFER_TEXTO];
    formatear_operando(op1, op_A_str);
    formatear_operando(op2, op_B_str);
    printf("%c[%04X] ", es_entry_point ? '>' : ' ', fisica_ip);
    imprimir_instruccion_hex(MV, fisica_ip, tamanio_total);
    printf("| %s", nombres_mnemonicos[opc]);

    if (op_A_str[0] != '\0' && op_B_str[0] != '\0')
        printf(" %s, %s\n", op_A_str, op_B_str);
    else if (op_A_str[0] != '\0')
        printf(" %s\n", op_A_str);
    else
        printf("\n");
}

void desensamblar(TMV *MV) {
    int inicioCS = MV->registros[REG_CS] & 0xFFFF0000;   // parte alta de CS: índice del segmento de código
    int offsetAux = 0;
    int fisica = direc_fisica(inicioCS | offsetAux, MV, 1);
    int instruccion, cant_op, tipo_A, tipo_B, tamanio_op;
    int opc, inicio_lec, op1, op2;
    int entry_point = direc_fisica(MV->registros[REG_IP], MV, 1);

    while (fisica != -1) {
        instruccion = MV->RAM[fisica];
        calcula_cant_op(instruccion, &tipo_A, &tipo_B, &cant_op);
        tamanio_op = tipo_A + tipo_B + 1;

        if (direc_fisica(inicioCS | offsetAux, MV, tamanio_op) == -1)
            fisica = -1; // instruccion truncada al final del segmento: corta el ciclo
        else {
            opc = instruccion & MASCARA_OPCODE;
            inicio_lec = fisica + 1;
            op1 = 0;
            op2 = 0;

            switch (cant_op) {
                case 1:
                    op1 = (tipo_A << BITS_TIPO_OPERANDO) | get_RAM(tipo_A, inicio_lec, MV);
                    break;
                case 2:
                    op2 = (tipo_B << BITS_TIPO_OPERANDO) | get_RAM(tipo_B, inicio_lec, MV);
                    op1 = (tipo_A << BITS_TIPO_OPERANDO) | get_RAM(tipo_A, inicio_lec + tipo_B, MV);
                    break;
            }

            mostrar_desensamblado(fisica, MV, opc, op1, op2,tamanio_op,entry_point != -1 && fisica == entry_point);

            offsetAux += tamanio_op;
            fisica = direc_fisica(inicioCS | offsetAux, MV, 1);
        }
    }
}
/*FIN DE FUNCIONES DESENSAMBLADO*/

void ejecutar(TMV *MV) {
    int fisica_ip, instruccion, cant_op, tipo_A, tipo_B, tamanio_op, inicio_lec, offset_actual;

    // Paso 1: la ejecucion sigue mientras IP apunte dentro del segmento de codigo
    fisica_ip = direc_fisica(MV->registros[REG_IP], MV, 1);
    while (fisica_ip != -1 && MV->error == 0 && MV->estadoDebug != DBG_SALIR) {

        // Paso 2: distingo la operacion y los tipos de operandos
        instruccion = MV->RAM[fisica_ip];
        calcula_cant_op(instruccion, &tipo_A, &tipo_B, &cant_op);
        tamanio_op = tipo_A + tipo_B + 1;

        // Paso 3: valido poder leer la instruccion completa (opcode + operandos)
        if (direc_fisica(MV->registros[REG_IP], MV, tamanio_op) == -1)
            MV->error = 3;
        else {
            // Paso 4: cargo OPC, OP1 y OP2
            MV->registros[REG_OPC] = instruccion & MASCARA_OPCODE;
            inicio_lec = fisica_ip + 1;
            switch (cant_op) {
                case 1:
                    MV->registros[REG_OP1] = (tipo_A << BITS_TIPO_OPERANDO) | get_RAM(tipo_A, inicio_lec, MV);
                    MV->registros[REG_OP2] = 0;
                    break;
                case 2:
                    // se codifican en orden inverso: primero B, despues A
                    MV->registros[REG_OP2] = (tipo_B << BITS_TIPO_OPERANDO) | get_RAM(tipo_B, inicio_lec, MV);
                    MV->registros[REG_OP1] = (tipo_A << BITS_TIPO_OPERANDO) | get_RAM(tipo_A, inicio_lec + tipo_B, MV);
                    break;
                default:
                    MV->registros[REG_OP1] = MV->registros[REG_OP2] = 0;
            }

            // Paso 5: valido que el opcode exista
            if (vecInstr[MV->registros[REG_OPC]] == NULL)
                MV->error = 1;
            else {
                // Paso 6: avanzo el IP ANTES de ejecutar (asi los saltos lo pueden pisar)
                offset_actual = (MV->registros[REG_IP] & MASCARA_16_BITS) + tamanio_op;
                MV->registros[REG_IP] = (MV->registros[REG_IP] & ~MASCARA_16_BITS) | (offset_actual & MASCARA_16_BITS);

                // Paso 7: ejecuto la instruccion
                vecInstr[MV->registros[REG_OPC]](MV, MV->registros[REG_OP1], MV->registros[REG_OP2]);
                if (MV->estadoDebug == DBG_PASO && MV->error == 0 && !(MV->registros[REG_OPC] == 0 && (MV->registros[REG_OP1] & 0xFFFF) == 0xF)) {
                    breakpoint(MV); 
                }
            }
        }

        // Paso 9: busco la proxima instruccion (si hubo error, el while corta igual)
        fisica_ip = direc_fisica(MV->registros[REG_IP], MV, 1);
    }

    switch (MV->error){
        case 0: printf("Ejecucion exitosa\n");
                break;
        case 1: printf("Error: Instruccion invalida\n");
                break;
        case 2: printf("Error: Division por cero\n");
                break;
        case 3: printf("Error: Falla de segmento\n");
                break;
        case 4: printf("Error: Stack Overflow\n");
                break;
        case 5: printf("Error: Stack Underflow\n");
                break;
    }
}
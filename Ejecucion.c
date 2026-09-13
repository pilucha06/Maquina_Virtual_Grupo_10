//Acá se va a realizar la ejecución de la máquina (el ciclo while básicamente)
//Carpetas a incluir: Instrucciones.c / stdio.h
/*Arranca con la maquina ya inicializada, el primer llamado desde el main sería ejecutar(MV, modo_disassembler)
después de inicializar todas las estructuras*/

#include <stdio.h>

//constantes
#define CS 26
#define DS 27
#define IP 0
#define OP1 2
#define OP2 3
#define OPC 1

const char *nombres_registros[32] = {
    [0]  = "IP",  [1]  = "OPC", [2]  = "OP1", [3]  = "OP2",
    [4]  = "LAR", [5]  = "MAR", [6]  = "MBR",
    [10] = "EAX", [11] = "EBX", [12] = "ECX", [13] = "EDX",
    [14] = "EEX", [15] = "EFX",
    [16] = "AC",  [17] = "CC",
    [26] = "CS",  [27] = "DS"
    // el resto (7,8,9,18-25,28-31) quedan en NULL: reservados/no usados
};

const char *nombres_mnemonicos[32] = {
    [0x00] = "SYS",  [0x01] = "JMP", [0x02] = "JP",  [0x03] = "JN",
    [0x04] = "JZ",   [0x05] = "JC",  [0x06] = "JV",  [0x07] = "JNP",
    [0x08] = "JNN",  [0x09] = "JNZ", [0x0A] = "NOT", [0x0F] = "STOP",
    [0x10] = "MOV",  [0x11] = "ADD", [0x12] = "SUB", [0x13] = "MUL",
    [0x14] = "DIV",  [0x15] = "CMP", [0x16] = "AND", [0x17] = "OR",
    [0x18] = "XOR",  [0x19] = "SWAP",[0x1A] = "SHL", [0x1B] = "SHR",
    [0x1C] = "SAR",  [0x1D] = "LDL", [0x1E] = "LDH", [0x1F] = "RND"
    // el resto queda en NULL: opcode inválido
};

/* direc_fisica: a partir de una dirección lógica y una cantidad de bytes a leer,
   valida que la posición inicial sea correcta y que se puedan leer esos bytes
   sin salirse del segmento. Si es válida, devuelve la posición física inicial. */
int direc_fisica(int direc_logica, registrosM *MV, int cant_bytes) {
    int seg    = (direc_logica >> 16) & 0xFFFF;
    int offset = direc_logica & 0xFFFF;

    if (seg < 0 || seg >= 8 || MV->tabla_segmentos[seg] == -1)
        return -1; // índice inválido en la tabla o entrada no usada

    int base = (MV->tabla_segmentos[seg] >> 16) & 0xFFFF;
    int tamanio_seg = MV->tabla_segmentos[seg] & 0xFFFF;

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
    } else if ((instruccion & 0x1F) != 0x0F) {
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
int get_RAM(int tipo_op, int fisica_ip, registrosM *MV) {
    int valor = 0;
    for (int i = 0; i < tipo_op; i++) {
        valor = (valor << 8) | (unsigned char)MV->RAM[fisica_ip + i];
    }
    return valor;
}

void imprimir_instruccion_hex(registrosM *MV, int fisica_inicio, int tamanio_total) {
    for (int i = 0; i < tamanio_total; i++) {
        printf("%02X ", (unsigned char)MV->RAM[fisica_inicio + i]);
    }
}

void formatear_operando(int operando, char *buffer) {
    int tipo  = (operando >> 24) & 0xFF;
    int valor = operando & 0x00FFFFFF;

    switch (tipo) {
        case 0: // no existe
            buffer[0] = '\0';
            break;
        case 1: // registrosistro
            snprintf(buffer, 32, "%s", nombres_registrosistros[valor & 0x1F]);
            break;
        case 2: // inmediato
            snprintf(buffer, 32, "%d", valor);
            break;
        case 3: { // memoria: [registros+offset]
            int codigo_registros = valor & 0x1F;
            int offset     = (valor >> 8) & 0xFFFF;
            snprintf(buffer, 32, "[%s+%d]", nombres_registrosistros[codigo_registros], offset);
            break;
        }
    }
}

void mostrar_desensamblado(registrosM *MV, int fisica_ip, int tamanio_total) {
    char op_A_str[32], op_B_str[32];
    formatear_operando(MV->registros[OP1], op_A_str);
    formatear_operando(MV->registros[OP2], op_B_str);

    printf("[%04X] ", fisica_ip);
    imprimir_instruccion_hex(MV, fisica_ip, tamanio_total);
    printf("| %s ", nombres_mnemonicos[MV->registros[OPC]]);

    if (op_A_str[0] != '\0' && op_B_str[0] != '\0')
        printf("%s, %s\n", op_A_str, op_B_str);
    else if (op_A_str[0] != '\0')
        printf("%s\n", op_A_str);
    else
        printf("\n");
}

void ejecutar(registrosM *MV, int modo_disassembler) {
    int corriendo = 1;

    MV->registros[DS] = (1 << 16) | 0;
    MV->registros[CS] = 0;
    MV->registros[IP] = MV->registros[CS];

    while (corriendo) {

        // Paso 1: validar que exista al menos el primer byte de la instrucción
        int fisica_ip = direc_fisica(MV->registros[IP], MV, 1); 
        if (fisica_ip == -1) {
            printf("Error: fallo de segmento\n");
            corriendo = 0;
            break;
        }

        // Paso 2: distingo la operación y los tipos de operandos
        int instruccion = (unsigned char)MV->RAM[fisica_ip];
        int cant_op, tipo_A, tipo_B;
        calcula_cant_op(instruccion, &tipo_A, &tipo_B, &cant_op);

        // Paso 3: valido poder leer la instrucción completa (opcode + operandos)
        int tamanio_op = tipo_A + tipo_B + 1; 
        if (cant_op != 0) { 
            if (direc_fisica(MV->registros[IP], MV, tamanio_op) == -1) {
                printf("Error: fallo de segmento\n");
                corriendo = 0;
                break;
            }
        }

        // Paso 4: cargo OPC, OP1 y OP2
        MV->registros[OPC] = instruccion & 0x1F;
        int inicio_lec = fisica_ip + 1;

        switch (cant_op) {
            case 1:
                MV->registros[OP1] = (tipo_A << 24) | get_RAM(tipo_A, inicio_lec, MV);
                MV->registros[OP2] = 0;
                break;
            case 2:
                // se codifican en orden inverso: primero B, después A
                MV->registros[OP2] = (tipo_B << 24) | get_RAM(tipo_B, inicio_lec, MV);
                MV->registros[OP1] = (tipo_A << 24) | get_RAM(tipo_A, inicio_lec + tipo_B, MV);
                break;
            default:
                MV->registros[OP1] = MV->registros[OP2] = 0;
        }

        // Paso 5: si está activo el modo -d, muestro el desensamblado
        if (modo_disassembler) { // de esto depende si escribe o no en terminal
            mostrar_desensamblado(MV, fisica_ip, tamanio_op);
        }

        // Paso 6: avanzo el IP (solo el offset, preservando el segmento)
        int offset_actual = MV->registros[IP] & 0xFFFF;
        offset_actual += tamanio_op;
        MV->registros[IP] = (MV->registros[IP] & 0xFFFF0000) | (offset_actual & 0xFFFF);
        // FALTA: acá va el switch/dispatch que ejecuta la operación según MV->registros[OPC]
        // (MOV, ADD, SUB, JMP, STOP, etc.) — sin esto, el programa nunca hace nada
        // con la instrucción ni corta la ejecución salvo por error de segmento.
    }
}
//Acá se va a realizar la ejecución de la máquina (el ciclo while básicamente)
//Carpetas a incluir: Instrucciones.c
/*Arranca con la maquina ya inicializada, el primer llamado desde el main seria (ejecutar(MV))
después de inicializar todas las estructuras*/
/* inicializamos los registros [reg DS 1 y reg CS 0 y reg IP [CS]]
            mientras IP!=-1 || no se puedan extraer los operandos
            validar los datos de la tabla de segmentos(que la dirección logica sea mayor o igual a la dirección 
            base y que con la suma de los bytes a leer no se pase del límite establecido, tamaño)
            Almacena en OPC la operación, en OP1 A, en OP2 B(3 bits mas sig, el tipo de op, en los demas el valor)
            nos desplazamos en el IP (IP+tamaño(OP+A+B)) 
            A partir de la operación hacer los procesos correspondientes
            En cada operación entra por parametro el archivo de salida, ya previamente con [] y la lecutura de la memor
            ahi, si se ejecuta correctamente, guardar el mnemonico y los operando
            PREGUNTAR, si por salidas seria print o el archivo.txt
        cuando corta la ejecución a IP=-1 (STOP)*/
#define CS 26
#define DS 27
#define IP 0
#define OP1 2
#define OP2 3
#define OPC 1
int direc_fisica(int direc_logica, RegM *MV, int cant_bytes){
    int seg = (direc_logica>>16) & 0xFFFF;// 16 y máscara constantes.h
    int offset = direc_logica & 0xFFFF;
    if(seg<0 || seg>=8 || MV->tabla_segmentos[seg]==-1)
        return 0;//indice invalido en tabla o registro invalido por falta de información.
    int base = (MV->tabla_segmentos[seg] >> 16) & 0xFFFF;
    int tamanio_seg = MV->tabla_segmentos[seg] & 0xFFFF;
    if (offset<0 || offset + cant_bytes > tamanio_seg)
        return 0;//la posición logica que apunta ip es invalida (mayor o menor al CS)
    return 1;
}
void ejecutar(RegM *MV){
    int corriendo=1;
    MV->reg[DS] = (1 << 16) | 0;
    MV->reg[CS] = 0;
    MV->reg[IP] = MV.reg[CS];
    
    while (corriendo){
            //Paso 1: validar IP
            int fisica_ip = direc_fisica(MV->reg[IP],MV,1);
            if (fisica_ip == -1){
                printf("Error: falla en el segmento\n");
                corriendo = 0;
                break;
            }
            int instruccion = MV->RAM[fisica_ip];
            int tamanio_ops = 0;
            int cant_op = 0;
            if  (((instruccion >> 4) & 0x01) != 0) {
                cant_op = 2;
                int tipo_A = (instruccion >> 4) & 0x03;
                int tipo_B = (instruccion >> 6) & 0x03;
                tamanio_ops = tipo_A + tipo_B;
            } else {
                if ((instruccion & 0x1F) != 0x0F)){
                    cant_op = 1;
                    int tipo_A = (instruccion >> 6) & 0x03;
                    tamanio_ops = tipo_A;
                } 
            }
            fisica_ip = direc_fisica(MV->reg[IP],MV,tamanio_ops+1);
            if (fisica_ip == -1) {
                printf("Error: fallo de segmento\n");
                corriendo = 0;
                break;
            }
            MV->reg[OPC] = MV->RAM[fisica_ip] & 0x1F;
            switch (cant_op){
                case 1: MV->reg[OP1] =(tipoA << 24 ) | //hacer función para leer el valor en ram
                        break;
                case 2: //OP1 y OP2
                        break;
                default: MV->reg[OP1] = MV->reg[OP2] = 0;
            }

            
    }
}
#include <stdio.h>
#include "Imagen.h"
//Función auxiliar para escribir en big endian
static void auxwrite(FILE* Arch, unsigned int dato, int cantbytes){
    int i=cantbytes-1;
    for(;i>=0;i--)
        fputc((dato >> (8 * i)) & 0xFF, Arch);//fputc escribe un solo byte en un archivo abierto
}

/*Verifique con compilación que reconociera y escribiera bien en el archivo los
numeros negativos, resultado: -1 se escribe como 0xFFFFFFFF (00000000  12 34 56 78 AB CD FF FF FF FF)*/
void escribirImagen(TMV *MV, char nomArch[]){
    FILE* Arch=fopen(nomArch,"wb");
    if(Arch==NULL){
        printf("Error al abrir el archivo %s\n",nomArch);
        return;
    }
    fwrite("VMI26", 1, 5, Arch);
    fputc(1, Arch);
    auxwrite(Arch, MV->tam_ram / 1024, 2);
    for (int i = 0; i < CANT_REGS; i++)
        auxwrite(Arch, MV->registros[i], 4);
    for (int i = 0; i < CANT_SEGS; i++)
        auxwrite(Arch, MV->tablaSegmentos[i], 4);
    fwrite(MV->RAM, 1, MV->tam_ram, Arch);
    fclose(Arch);
}
void breakpoint(TMV *MV){
    if(MV->nombreVMI!=NULL){ 
        char linea[8];
        int valida = 0;
        escribirImagen(MV, MV->nombreVMI);  
        while (!valida){
            fgets(linea, sizeof(linea), stdin);
            switch(linea[0]){
                case'g': MV->estadoDebug=DBG_NORMAL;
                        valida=1;
                        break;
                case'q': MV->estadoDebug=DBG_SALIR;
                        valida=1;
                        break;
                case'\n': MV->estadoDebug=DBG_PASO;
                        valida=1;
                        break;
                default:printf("Ingrese g para continuar, q para salir o ENTER para paso a paso\n");
                        valida=0;
                        break;
            }
        }
    }
}
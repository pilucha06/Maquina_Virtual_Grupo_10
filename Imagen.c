#include <stdio.h>
#include <string.h>
#include "Imagen.h"
#include "MV.h"
#include "IniciarMV.h"

//Función auxiliar para escribir en big endian
static void auxwrite(FILE* Arch, unsigned int dato, int cantbytes){
    int i=cantbytes-1;
    for(;i>=0;i--)
        fputc((dato >> (8 * i)) & 0xFF, Arch);//fputc escribe un solo byte en un archivo abierto
}
static unsigned int auxread(FILE* Arch, int cantbytes){
    int i=cantbytes-1;
    unsigned int dato=0;
    for(;i>=0;i--){
        unsigned char c=fgetc(Arch);
        dato |= ((unsigned int)c << (8 * i));
    }
    return dato;
}
/*Verifique con compilación que reconociera y escribiera bien en el archivo los
numeros negativos, resultado: -1 se escribe como 0xFFFFFFFF (00000000  12 34 56 78 AB CD FF FF FF FF)*/
void escribirImagen(TMV *MV, char nomArch[]){
    FILE* Arch=fopen(nomArch,"wb");
    if(Arch==NULL){
        printf("Error al abrir el archivo %s\n",nomArch);
        return;
    }
    fwrite(ID_MVI, sizeof(char), TAM_ID, Arch);
    fputc(1, Arch);
    auxwrite(Arch, MV->tam_ram / 1024, 2);
    for (int i = 0; i < CANT_REGS; i++)
        auxwrite(Arch, MV->registros[i], 4);
    for (int i = 0; i < CANT_SEGS; i++)
        auxwrite(Arch, MV->tablaSegmentos[i], 4);
    fwrite(MV->RAM, 1, MV->tam_ram, Arch);
    fclose(Arch);
}
int cargarImagen(TMV*MV,char nomArch[]){
    FILE*Arch=fopen(nomArch,"rb");
    if(Arch==NULL){
        return ERR_ARCH;
    }
    char id[TAM_ID + 1];
    int num;
    num = fread(id, sizeof(char), TAM_ID, Arch);
    id[TAM_ID] = '\0';
    if(num != TAM_ID) {
        fclose(Arch);
        return ERR_ARCH;
    }
    if (strcmp(id, ID_MVI) != 0) {
        fclose(Arch);
        return ERR_ID;
    }
    int ver = fgetc(Arch);
    if (ver != 1) {
        fclose(Arch);
        return ERR_VER;
    }
    unsigned int tamRam = auxread(Arch, 2) * 1024;
    if (!reservoEspacioMV(MV, tamRam)){
        fclose(Arch);
        return ERR_MEM;
    }
    for (int i = 0; i < CANT_REGS; i++)
        MV->registros[i] = auxread(Arch, 4);
    for (int i = 0; i < CANT_SEGS; i++) 
        MV->tablaSegmentos[i] = auxread(Arch, 4);
    size_t leidos = fread(MV->RAM, 1, MV->tam_ram, Arch);
    if (leidos != MV->tam_ram) {
        fclose(Arch);
        return ERR_ARCH;
    }
    MV->error = 0;
    MV->estadoDebug = DBG_NORMAL;
    MV->nombreVMI = nomArch;
    fclose(Arch);
    return OK;
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
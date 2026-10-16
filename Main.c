#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "IniciarMV.h"
#include "Ejecucion.h"   // <-- agregar este include

void mostrarResult(int result){
    switch (result){
        case OK:
            printf("Programa cargado correctamente\n");
            break;
        case ERR_ARCH:
            printf("ERROR: No se pudo abrir o leer el archivo\n");
            break;
        case ERR_ID:
            printf("ERROR: El archivo no es un programa VMX26\n");
            break;
        case ERR_VER:
            printf("ERROR: Version del programa no soportada\n");
            break;
        case ERR_TAM_COD:
            printf("ERROR: El codigo no entra en la memoria\n");
            break;
        default:
            printf("ERROR: Codigo de resultado desconocido (%d)\n", result);
    }
}

int main(int argc, char *argv[]){
    TMV MV;
    int i, mostrarDis = 0, result;

    if (argc >= 2)
        if (reservoEspacioMV(&MV, TAM_RAM_DEFAULT)){
            result = inicializoMV(&MV, argv[1]);
            mostrarResult(result);
            if (result == OK){
                for (i = 2; i < argc; i++)
                    if (strcmp(argv[i], "-d") == 0)
                        mostrarDis = 1;
                if (mostrarDis)
                    desensamblar(&MV);
                ejecutar(&MV);

                liberarMV(&MV);
                return 0;
            }
            else{
                liberarMV(&MV);
                return 1;
            }
        }
        else{
            printf("ERROR: No se pudo reservar espacio para componentes de la MV");
            return 1;
        }
    else{
        printf("ERROR: Parametros incorrectos o insuficientes");
        return 1;
    }
}


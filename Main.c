#include <stdlib.h>
#include <stdio.h>
#include "Imagen.h"
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
        case ERR_MEM:
            printf("ERROR: No se pudo reservar memoria para la MV\n");
            break;
        case ERR_ARG_M:
            printf("ERROR: Tamanio de memoria invalido (m=M, con M entre 1 y %d)\n", MAX_MEM_KIB);
            break;
        case ERR_SIN_ARCH:
            printf("ERROR: Debe indicarse al menos un archivo .vmx o .vmi\n");
            break;
        case ERR_HEADER:
            printf("ERROR: Header espantoso, boludas! ERROR DE HEADERRRR\n");
            break;
        case ERR_MEM_INSUF:
            printf("ERROR: Memoria insuficiente: el proceso no entra en la memoria\n");
            break;
        default:
            printf("ERROR: Codigo de resultado desconocido (%d)\n", result);
    }
}

int main(int argc, char *argv[]){
    TMV MV;
    TArgumentos args;
    int result;

    // Punteros en NULL: así liberarMV es seguro aunque no se haya reservado nada
    MV.RAM = NULL;
    MV.registros = NULL;
    MV.tablaSegmentos = NULL;

    result = leerArgumentos(argc, argv, &args);

    if (result == OK) {
        if (args.nomVMX != NULL) {
            if (!reservoEspacioMV(&MV, args.tamMemKiB * 1024)) //el usuario dio KiB, y reservoEspacioMV pide bytes
                result = ERR_MEM;
            else {
                result = inicializoMV(&MV, &args);
                if (result == OK)
                    MV.nombreVMI = args.nomVMI;   // NULL si no vino .vmi -> breakpoints ignorados
            }
        }
        else {
            // Camino solo .vmi: cargarImagen reserva la memoria por dentro
            result = cargarImagen(&MV, args.nomVMI);
        }
    }

    mostrarResult(result);

    if (result == OK) {
        if (args.mostrarDis)
            desensamblar(&MV);
        ejecutar(&MV);
    }

    liberarMV(&MV);
    return (result == OK) ? 0 : 1;
}

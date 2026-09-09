//definir las estructuras

int main(int argc, int args[]){
    //argc el tamaño del vec
    //args nombre del archivo ejecutable .vmx [0]
    /*
    paso 1: crear la máquina
        crear las estructuras de hardware
        crear el registro máquina
        inicializar los punteros a las estructuras
    paso 2: inicializarla/ejecutarla
        a partir del archivo.vmx (FILE* open)
        guardar el identificador y la versión en la máquina
        guardar el tamaño del código en una variable (sirve para inicializar el CS)
        ciclo de lectura donde guardamos el .vmx en RAM hasta que se termine el archivo
        inicializar la tabla de segmentos ([0]CS 0, tamaño, [1]DS tamaño, 16KiB-tamaño)
        completamos el resto de la tabla con -1(0xFFFF)
        inicializamos los registros [reg DS 1 y reg CS 0 y reg IP [CS]]
            mientras IP!=-1
            validar los datos de la tabla de segmentos(que la dirección logica sea mayor o igual a la dirección base y que con la suma de los bytes a leer no se pase del límite establecido, tamaño)
            Almacena en OPC la operación, en OP1 A, en OP2 B(3 bits mas sig, el tipo de op, en los demas el valor)
            nos desplazamos en el IP (IP+tamaño(OP+A+B)) 
            A partir de la operación hacer los procesos correspondientes
        cuando corta la ejecución a IP=-1 (STOP)
        cargar en la máquina la versión
        cargar las instrucciones en la memoria RAM
        CS apunta al inicio de las instrucciones, DS aputa a el final de las instrucciones(pos CS + tamaño)
    */
}
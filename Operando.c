/*
Tiene que tener minimo 2 funciones (get,set), se puede hacer un vector de get y uno de set para cada tipo
de operando y aislar codigo (op1 | op2) 3 bits mas sig son el tipo, get\set mem, inm, reg
shl 3 
get[*getinm, *getmem, *getreg](4 bytes - 3 bits)
set[*setmem, *setreg](4bytes - 3bits, int)
recibe por parametro la maquina y la linea de operación entera*/
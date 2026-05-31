#!/bin/bash

# Si hay argumento, filtrar por ese prefijo. Si no, contar todos.
if [ $# -eq 1 ]; then
    PREFIJO=$1
else
    PREFIJO=""
fi

# ls -p añade / a los directorios, grep -v "/" los elimina
# grep "^$PREFIJO" filtra los que empiezan por el prefijo
N=$(ls -pA | grep -v "/" | grep "^$PREFIJO" | wc -l)

echo "Número de ficheros encontrados: $N"

sleep 5  # espera para poder monitorizarlo

if [ $N -gt 0 ]; then
    exit 0
else
    exit 1
fi
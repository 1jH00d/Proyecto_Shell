#!/bin/bash
# =============================================================================
# GUÍA COMPLETA DE BASH PARA EL EXAMEN
# =============================================================================


# =============================================================================
# 1. VARIABLES
# =============================================================================
N=0                    # asignar (sin espacios alrededor del =)
NOMBRE="hola mundo"    # string con espacios → comillas
echo $N                # usar variable con $
echo ${N}              # forma explícita, útil en: echo "${N}mundo"
readonly PI=3.14       # variable de solo lectura
unset N                # eliminar variable

# Variables especiales:
# $0    → nombre del script
# $1 $2 → argumentos del script
# $#    → número de argumentos
# $@    → todos los argumentos como lista
# $*    → todos los argumentos como string
# $?    → código de retorno del último comando
# $$    → PID del script actual
# $!    → PID del último proceso en background


# =============================================================================
# 2. ARITMÉTICA
# =============================================================================
N=$((N+1))             # suma
N=$((N-1))             # resta
N=$((N*2))             # multiplicación
N=$((N/2))             # división entera
N=$((N%3))             # módulo
((N++))                # incrementar
((N--))                # decrementar


# =============================================================================
# 3. CONDICIONES - if
# =============================================================================
if [ condición ]; then
    echo "rama then"
elif [ condición ]; then
    echo "rama elif"
else
    echo "rama else"
fi

# Comparación de NÚMEROS:
# -eq   igual          [ $A -eq $B ]
# -ne   distinto       [ $A -ne $B ]
# -gt   mayor que      [ $A -gt $B ]
# -lt   menor que      [ $A -lt $B ]
# -ge   mayor o igual  [ $A -ge $B ]
# -le   menor o igual  [ $A -le $B ]

# Comparación de STRINGS:
# =     igual          [ "$A" = "$B" ]
# !=    distinto       [ "$A" != "$B" ]
# -z    string vacío   [ -z "$A" ]
# -n    string no vacío [ -n "$A" ]

# Operadores de FICHEROS:
# -f    es fichero regular
# -d    es directorio
# -e    existe
# -r    tiene permisos de lectura
# -w    tiene permisos de escritura
# -x    tiene permisos de ejecución
# -s    fichero no vacío (size > 0)

# Operadores LÓGICOS:
# !     NOT:                  [ ! -f fichero ]
# -a    AND dentro de [ ]:    [ $A -gt 0 -a $B -gt 0 ]
# -o    OR dentro de [ ]:     [ $A -eq 0 -o $B -eq 0 ]
# &&    AND entre comandos:   cmd1 && cmd2
# ||    OR entre comandos:    cmd1 || cmd2


# =============================================================================
# 4. BUCLES
# =============================================================================

# for con lista de valores:
for i in 1 2 3 4 5; do
    echo $i
done

# for con rango (seq):
for i in $(seq 1 10); do
    echo $i
done

# for recorriendo ficheros del directorio actual:
for f in *; do
    if [ -f "$f" ]; then
        echo "Fichero: $f"
    fi
done

# for recorriendo argumentos del script:
for arg in "$@"; do
    echo "Argumento: $arg"
done

# while:
N=0
while [ $N -lt 10 ]; do
    N=$((N+1))
    echo $N
done

# until (hasta que la condición sea verdadera):
until [ $N -eq 0 ]; do
    N=$((N-1))
done

# break y continue:
for i in 1 2 3 4 5; do
    if [ $i -eq 2 ]; then continue; fi   # saltar iteración
    if [ $i -eq 4 ]; then break; fi      # salir del bucle
    echo $i
done


# =============================================================================
# 5. CAPTURAR SALIDA DE COMANDOS
# =============================================================================
N=$(ls | wc -l)              # contar ficheros
FECHA=$(date)                # capturar fecha
LINEAS=$(wc -l < fichero)    # contar líneas de un fichero
RESULTADO=$(comando)         # guardar cualquier salida


# =============================================================================
# 6. PIPES Y FILTROS ÚTILES
# =============================================================================

# Contar ficheros (no directorios) en directorio actual:
N=$(ls -pA | grep -v "/" | wc -l)

# Contar ficheros con prefijo:
N=$(ls -pA | grep -v "/" | grep "^PREFIJO" | wc -l)

# Contar ficheros con extensión:
N=$(ls -pA | grep -v "/" | grep "\.ext$" | wc -l)

# Contar directorios:
N=$(ls -pA | grep "/" | wc -l)

# Buscar texto en ficheros:
N=$(grep -rl "texto" . 2>/dev/null | wc -l)

# Contar procesos de un usuario:
N=$(ps -u $USER --no-headers 2>/dev/null | wc -l)

# Notas sobre ls:
# ls -p  → añade / al final de directorios
# ls -A  → incluye ficheros ocultos (excepto . y ..)
# ls -1  → un fichero por línea
# ls -pA → combinación: incluye ocultos y marca directorios


# =============================================================================
# 7. CASE
# =============================================================================
case $VAR in
    "hola")
        echo "saludo"
        ;;
    "adios")
        echo "despedida"
        ;;
    [0-9])
        echo "es un dígito"
        ;;
    *)
        echo "otro caso"
        ;;
esac


# =============================================================================
# 8. FUNCIONES
# =============================================================================
mi_funcion() {
    local resultado=$1    # local: variable local a la función
    echo "Argumento: $resultado"
    return 0              # valor de retorno (0-255)
}

mi_funcion "hola"
echo "Retornó: $?"


# =============================================================================
# 9. STRINGS - OPERACIONES ÚTILES
# =============================================================================
STR="hola mundo"
echo ${#STR}              # longitud: 10
echo ${STR:0:4}           # subcadena: "hola"
echo ${STR/hola/adios}    # reemplazar primera ocurrencia
echo ${STR^^}             # HOLA MUNDO (mayúsculas)
echo ${STR,,}             # hola mundo (minúsculas)

# Valor por defecto si variable no existe:
PREFIJO=${1:-""}          # si $1 no está, usar ""
USER=${1:-$(whoami)}      # si $1 no está, usar el usuario actual


# =============================================================================
# 10. ARRAYS
# =============================================================================
arr=(1 2 3 4 5)           # declarar array
echo ${arr[0]}            # primer elemento: 1
echo ${arr[-1]}           # último elemento: 5
echo ${arr[@]}            # todos los elementos
echo ${#arr[@]}           # número de elementos: 5
arr+=(6)                  # añadir elemento al final

# Recorrer array:
for elem in "${arr[@]}"; do
    echo $elem
done


# =============================================================================
# 11. ENTRADA/SALIDA Y REDIRECCIONES
# =============================================================================
echo "texto"              # imprimir con salto de línea
printf "texto\n"          # más control del formato
read VAR                  # leer línea del teclado
read -p "Nombre: " VAR    # leer con prompt

# Redirecciones:
comando > fichero         # sobreescribir
comando >> fichero        # añadir al final
comando < fichero         # leer desde fichero
comando 2> errores.txt    # redirigir stderr
comando &> todo.txt       # redirigir stdout y stderr
comando 2>/dev/null       # descartar errores


# =============================================================================
# 12. MISCELÁNEA ÚTIL
# =============================================================================
sleep 5                   # esperar 5 segundos (para el evaluador)
exit 0                    # salir con éxito
exit 1                    # salir con error
chmod +x script.sh        # dar permisos de ejecución

# Comprobar si un proceso existe:
if kill -0 $PID 2>/dev/null; then
    echo "El proceso $PID existe"
fi

# Comprobar si un comando existe:
if command -v gcc &>/dev/null; then
    echo "gcc existe"
fi

# Ejecutar comando y capturar código de retorno:
ls /no/existe 2>/dev/null
if [ $? -eq 0 ]; then
    echo "OK"
else
    echo "Error"
fi

# Equivalente más corto:
if ls /no/existe 2>/dev/null; then
    echo "OK"
else
    echo "Error"
fi


# =============================================================================
# ESTRUCTURA TÍPICA DE UN SCRIPT PARA EL EXAMEN
# =============================================================================
#!/bin/bash

# 1. Procesar argumentos
if [ $# -eq 1 ]; then
    PARAM=$1
else
    PARAM=""              # valor por defecto
fi

# 2. Hacer el trabajo
N=$(ls -pA | grep -v "/" | grep "^$PARAM" | wc -l)

# 3. Mostrar resultado (formato EXACTO según enunciado)
echo "Número de ficheros encontrados: $N"

# 4. Esperar para que el evaluador pueda monitorizar
sleep 5

# 5. Retornar código significativo
if [ $N -gt 0 ]; then
    exit 0
else
    exit 1
fi

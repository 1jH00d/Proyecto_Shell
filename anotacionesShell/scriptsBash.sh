#!/bin/bash
# =============================================================================
# SCRIPTS BASH PROBABLES PARA EL EXAMEN
# =============================================================================
# Cada sección es un script completo listo para adaptar.
# Recordar siempre: chmod +x nombre_script.sh
# =============================================================================


# =============================================================================
# SCRIPT 1: cuentafich.sh - contar ficheros con prefijo (ya implementado)
# =============================================================================
# Uso: ./cuentafich.sh [prefijo]

cat << 'EOF' > cuentafich.sh
#!/bin/bash
if [ $# -eq 1 ]; then
    PREFIJO=$1
else
    PREFIJO=""
fi
N=$(ls -pA | grep -v "/" | grep "^$PREFIJO" | wc -l)
echo "Número de ficheros encontrados: $N"
sleep 5
if [ $N -gt 0 ]; then exit 0; else exit 1; fi
EOF


# =============================================================================
# SCRIPT 2: cuentadir.sh - contar directorios con prefijo
# =============================================================================
# Uso: ./cuentadir.sh [prefijo]
# Similar a cuentafich pero cuenta directorios (terminan en /)

cat << 'EOF' > cuentadir.sh
#!/bin/bash
if [ $# -eq 1 ]; then
    PREFIJO=$1
else
    PREFIJO=""
fi
# ls -p pone / al final de directorios, grep "/" los selecciona
N=$(ls -pA | grep "/" | grep "^$PREFIJO" | wc -l)
echo "Número de directorios encontrados: $N"
sleep 5
if [ $N -gt 0 ]; then exit 0; else exit 1; fi
EOF


# =============================================================================
# SCRIPT 3: cuentaext.sh - contar ficheros por extensión
# =============================================================================
# Uso: ./cuentaext.sh <extension>
# Ejemplo: ./cuentaext.sh txt  → cuenta ficheros .txt

cat << 'EOF' > cuentaext.sh
#!/bin/bash
if [ $# -eq 0 ]; then
    echo "Uso: cuentaext.sh <extension>"
    exit 1
fi
EXT=$1
N=$(ls -pA | grep -v "/" | grep "\.${EXT}$" | wc -l)
echo "Ficheros .${EXT} encontrados: $N"
sleep 5
if [ $N -gt 0 ]; then exit 0; else exit 1; fi
EOF


# =============================================================================
# SCRIPT 4: cuentalineas.sh - contar líneas totales de ficheros con prefijo
# =============================================================================
# Uso: ./cuentalineas.sh [prefijo]

cat << 'EOF' > cuentalineas.sh
#!/bin/bash
PREFIJO=${1:-""}
TOTAL=0
for f in $(ls -pA | grep -v "/" | grep "^$PREFIJO"); do
    if [ -f "$f" ]; then
        N=$(wc -l < "$f")
        TOTAL=$((TOTAL + N))
    fi
done
echo "Total de líneas: $TOTAL"
sleep 5
if [ $TOTAL -gt 0 ]; then exit 0; else exit 1; fi
EOF


# =============================================================================
# SCRIPT 5: busca.sh - buscar texto en ficheros del directorio actual
# =============================================================================
# Uso: ./busca.sh <texto>

cat << 'EOF' > busca.sh
#!/bin/bash
if [ $# -eq 0 ]; then
    echo "Uso: busca.sh <texto>"
    exit 1
fi
TEXTO=$1
N=$(grep -rl "$TEXTO" . 2>/dev/null | wc -l)
echo "Ficheros que contienen '$TEXTO': $N"
sleep 5
if [ $N -gt 0 ]; then exit 0; else exit 1; fi
EOF


# =============================================================================
# SCRIPT 6: tamfich.sh - listar ficheros mayores de N bytes
# =============================================================================
# Uso: ./tamfich.sh <tamanio_minimo_bytes>

cat << 'EOF' > tamfich.sh
#!/bin/bash
MIN=${1:-0}
N=0
for f in $(ls -pA | grep -v "/"); do
    if [ -f "$f" ]; then
        TAM=$(wc -c < "$f")
        if [ $TAM -gt $MIN ]; then
            echo "$f: $TAM bytes"
            N=$((N + 1))
        fi
    fi
done
echo "Total ficheros mayores de $MIN bytes: $N"
sleep 5
if [ $N -gt 0 ]; then exit 0; else exit 1; fi
EOF


# =============================================================================
# SCRIPT 7: cuentaproc.sh - contar procesos de un usuario
# =============================================================================
# Uso: ./cuentaproc.sh [usuario]

cat << 'EOF' > cuentaproc.sh
#!/bin/bash
USER=${1:-$(whoami)}
N=$(ps -u "$USER" --no-headers 2>/dev/null | wc -l)
echo "Procesos de $USER: $N"
sleep 5
if [ $N -gt 0 ]; then exit 0; else exit 1; fi
EOF


# =============================================================================
# SCRIPT 8: checkpid.sh - comprobar si un proceso existe
# =============================================================================
# Uso: ./checkpid.sh <pid>
# Retorna 0 si existe, 1 si no

cat << 'EOF' > checkpid.sh
#!/bin/bash
if [ $# -eq 0 ]; then
    echo "Uso: checkpid.sh <pid>"
    exit 1
fi
PID=$1
if kill -0 $PID 2>/dev/null; then
    echo "El proceso $PID existe"
    exit 0
else
    echo "El proceso $PID no existe"
    exit 1
fi
EOF


# =============================================================================
# NOTAS IMPORTANTES PARA SCRIPTS BASH EN EL EXAMEN
# =============================================================================
# 
# CONTAR FICHEROS (no directorios):
#   ls -pA | grep -v "/"             → lista sin directorios
#   ls -pA | grep -v "/" | wc -l     → cuenta
#
# CONTAR DIRECTORIOS:
#   ls -pA | grep "/"                → solo directorios (tienen / al final)
#
# FILTRAR POR PREFIJO:
#   grep "^PREFIJO"                  → líneas que empiezan por PREFIJO
#   Si PREFIJO está vacío, "^" coincide con todo
#
# FILTRAR POR EXTENSIÓN:
#   grep "\.ext$"                    → líneas que terminan en .ext
#
# VARIABLES ÚTILES:
#   $#    → número de argumentos
#   $1    → primer argumento
#   $?    → código de retorno del último comando
#   $(comando)  → captura la salida de un comando
#
# COMPARACIONES EN IF:
#   -eq   igual
#   -ne   distinto
#   -gt   mayor que
#   -lt   menor que
#   -ge   mayor o igual
#   -le   menor o igual
#   -f    es un fichero regular
#   -d    es un directorio
#   -z    cadena vacía
#   -n    cadena no vacía
#
# ARITMÉTICA:
#   N=$((N + 1))    → incrementar
#   N=$((A * B))    → multiplicar
#
# SIEMPRE AÑADIR AL FINAL:
#   sleep 5          → para que el evaluador pueda monitorizar
#   exit 0 / exit 1  → código de retorno significativo
#   chmod +x script.sh → dar permisos de ejecución

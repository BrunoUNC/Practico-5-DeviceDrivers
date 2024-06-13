#!/bin/bash

# Nombre del módulo y ruta del dispositivo
MODULE_NAME="signal_input"
DEVICE_PATH="/dev/signal_input"
MAJOR_NUMBER=""

echo "  -> Limpiando compilaciones anteriores..."
make clean

echo "  -> Compilando el módulo..."
make

# Verificar si la compilación falló
if [ $? -ne 0 ]; then
    echo "  -> La compilación falló. Saliendo."
    exit 1
fi

echo "  -> Desinstalando el módulo (si está cargado)..."
sudo rmmod $MODULE_NAME 2>/dev/null

echo "  -> Cargando el nuevo módulo..."
sudo insmod ./${MODULE_NAME}.ko gpio_pin1=22 gpio_pin2=27

# Verificar si la carga del módulo falló
if [ $? -ne 0 ]; then
    echo "  -> Falló la carga del módulo. Saliendo."
    exit 1
fi

# Obtener el número mayor del dispositivo
MAJOR_NUMBER=$(awk "\$2==\"$MODULE_NAME\" {print \$1}" /proc/devices)

# Verificar si se pudo obtener el número mayor
if [ -z "$MAJOR_NUMBER" ]; then
    echo "  -> No se pudo obtener el número mayor. Saliendo."
    exit 1
fi

echo "  -> Creando nodo de dispositivo..."
sudo mknod $DEVICE_PATH c $MAJOR_NUMBER 0

echo "  -> Estableciendo permisos en el nodo de dispositivo..."
sudo chmod 666 $DEVICE_PATH

echo "Compilación completada."
#!/bin/bash

echo "  -> Desinstalando el módulo (si está cargado)..."
sudo rmmod signal_input 2>/dev/null

echo " -> Eliminando el nodo de dispositivo..."
sudo rm /dev/signal_input 2>/dev/null

echo " -> Limpiando archivos de compilación..."
make clean

echo "Limpieza completada."

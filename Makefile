CC = gcc
LD = gcc

# Flags para versión secuencial (sin pthread)
CFLAGS_SEQ = -g -O3 -Wall -pedantic -c
LDFLAGS_SEQ = -g -O3 -o
LIBS_SEQ = -L. -lm

# Flags para versión concurrente (con pthread)
CFLAGS_CONC = -g -O3 -Wall -pedantic -pthread -c
LDFLAGS_CONC = -g -O3 -pthread -o
LIBS_CONC = -L. -lm -pthread

INCLUDES = -I.

# Treat NT and non-NT windows the same
ifeq ($(OS),Windows_NT)
	OS = Windows
endif

ifeq ($(OS),Windows)
	EXE = .exe
	DEL = del
else	#assume Linux/Unix
	EXE =
	DEL = rm -f
endif

# Compilar ambas versiones por defecto
all:	CalcArboles$(EXE) CalcArbolesConcurrent$(EXE)
	@echo "==================================================="
	@echo "Compilación completada exitosamente!"
	@echo "==================================================="
	@echo "Versión SECUENCIAL:   ./CalcArboles"
	@echo "Versión CONCURRENTE:  ./CalcArbolesConcurrent"
	@echo "==================================================="

# Versión secuencial original
CalcArboles$(EXE):	CalcArboles.o 
		@echo "Enlazando versión SECUENCIAL..."
		$(LD) $^ $(LIBS_SEQ) $(LDFLAGS_SEQ) $@

CalcArboles.o:	CalcArboles.c ConvexHull.h
		@echo "Compilando versión SECUENCIAL..."
		$(CC) $(INCLUDES) $(CFLAGS_SEQ) $<

# Versión concurrente con pthreads
CalcArbolesConcurrent$(EXE):	CalcArbolesConcurrent.o 
		@echo "Enlazando versión CONCURRENTE..."
		$(LD) $^ $(LIBS_CONC) $(LDFLAGS_CONC) $@

CalcArbolesConcurrent.o:	CalcArbolesConcurrent.c ConvexHull.h
		@echo "Compilando versión CONCURRENTE..."
		$(CC) $(INCLUDES) $(CFLAGS_CONC) $<

# Compilar solo versión secuencial
seq: CalcArboles$(EXE)
	@echo "Versión SECUENCIAL compilada."

# Compilar solo versión concurrente
conc: CalcArbolesConcurrent$(EXE)
	@echo "Versión CONCURRENTE compilada."

# Ejecutar pruebas de comparación
test: all
	@echo ""
	@echo "=========================================="
	@echo "PRUEBA CON EJEMPLO 1 (6 árboles)"
	@echo "=========================================="
	@echo ""
	@echo "--- VERSIÓN SECUENCIAL ---"
	time ./CalcArboles$(EXE) ./ConjuntoPruebas/Ejemplo1.dat Ejemplo1_seq.res
	@echo ""
	@echo "--- VERSIÓN CONCURRENTE (4 hilos) ---"
	time ./CalcArbolesConcurrent$(EXE) ./ConjuntoPruebas/Ejemplo1.dat 4 Ejemplo1_conc.res
	@echo ""
	@echo "=========================================="
	@echo "Comparando resultados..."
	@diff -s Ejemplo1_seq.res Ejemplo1_conc.res || echo "¡ATENCIÓN! Los resultados difieren"
	@echo "=========================================="

# Ejecutar pruebas con ejemplo pequeño
test-small: all
	@echo ""
	@echo "=========================================="
	@echo "PRUEBA CON EJEMPLO 2 (3 árboles)"
	@echo "=========================================="
	@echo ""
	@echo "--- VERSIÓN SECUENCIAL ---"
	time ./CalcArboles$(EXE) ./ConjuntoPruebas/Ejemplo2.dat Ejemplo2_seq.res
	@echo ""
	@echo "--- VERSIÓN CONCURRENTE (2 hilos) ---"
	time ./CalcArbolesConcurrent$(EXE) ./ConjuntoPruebas/Ejemplo2.dat 2 Ejemplo2_conc.res
	@echo ""
	@echo "=========================================="
	@echo "Comparando resultados..."
	@diff -s Ejemplo2_seq.res Ejemplo2_conc.res || echo "¡ATENCIÓN! Los resultados difieren"
	@echo "=========================================="

# Ejecutar pruebas con ejemplo grande (para análisis de rendimiento)
test-big: all
	@echo ""
	@echo "=========================================="
	@echo "PRUEBA CON EJEMPLO GORDO (25 árboles)"
	@echo "=========================================="
	@echo ""
	@echo "--- VERSIÓN SECUENCIAL ---"
	time ./CalcArboles$(EXE) ./ConjuntoPruebas/EjemploGordo.dat EjemploGordo_seq.res
	@echo ""
	@echo "--- VERSIÓN CONCURRENTE (8 hilos) ---"
	time ./CalcArbolesConcurrent$(EXE) ./ConjuntoPruebas/EjemploGordo.dat 8 EjemploGordo_conc.res
	@echo ""
	@echo "=========================================="
	@echo "Comparando resultados..."
	@diff -s EjemploGordo_seq.res EjemploGordo_conc.res || echo "¡ATENCIÓN! Los resultados difieren"
	@echo "=========================================="

# Análisis de escalabilidad (prueba con diferentes números de hilos)
benchmark: all
	@echo ""
	@echo "=========================================="
	@echo "ANÁLISIS DE ESCALABILIDAD"
	@echo "=========================================="
	@echo ""
	@echo "Ejecutando versión secuencial..."
	@time ./CalcArboles$(EXE) ./ConjuntoPruebas/EjemploGordo.dat EjemploGordo_seq.res 2>&1 | grep real
	@echo ""
	@for n in 1 2 4 8 16; do \
		echo "Probando con $n hilo(s)..."; \
		time ./CalcArbolesConcurrent$(EXE) ./ConjuntoPruebas/EjemploGordo.dat $n EjemploGordo_$n.res 2>&1 | grep real; \
		echo ""; \
	done
	@echo "=========================================="
	@echo "Benchmark completado. Revisa los tiempos."
	@echo "=========================================="

# Limpiar archivos compilados
clean:
	@echo "Limpiando archivos objeto y ejecutables..."
	$(DEL) *.o
	$(DEL) CalcArboles$(EXE)
	$(DEL) CalcArbolesConcurrent$(EXE)
	@echo "Limpieza completada."

# Limpiar también archivos de resultados
cleanall: clean
	@echo "Limpiando archivos de resultados..."
	$(DEL) *.res
	@echo "Limpieza total completada."

# Ayuda
help:
	@echo "=========================================="
	@echo "Makefile para Práctica 3 - SCP"
	@echo "=========================================="
	@echo ""
	@echo "Comandos disponibles:"
	@echo "  make              - Compila ambas versiones"
	@echo "  make seq          - Compila solo versión secuencial"
	@echo "  make conc         - Compila solo versión concurrente"
	@echo "  make test         - Prueba con Ejemplo1 y compara resultados"
	@echo "  make test-small   - Prueba con Ejemplo2 (rápido)"
	@echo "  make test-big     - Prueba con EjemploGordo"
	@echo "  make benchmark    - Análisis de escalabilidad (1,2,4,8,16 hilos)"
	@echo "  make clean        - Elimina ejecutables y objetos"
	@echo "  make cleanall     - Elimina todo (incluye .res)"
	@echo "  make help         - Muestra esta ayuda"
	@echo ""
	@echo "=========================================="

.PHONY: all seq conc test test-small test-big benchmark clean cleanall help
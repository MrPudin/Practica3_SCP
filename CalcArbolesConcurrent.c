#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <pthread.h>
#include <semaphore.h>
#include <string.h>
#include <time.h>

#include "ConvexHull.h"

#define DMaxArboles 25
#define DMaximoCoste 999999
#define S 1000000
#define DDebug 0

// Estructuras de datos
struct Arbol {
    int IdArbol;
    Point Coord;
    int Valor;
    int Longitud;
};
typedef struct Arbol TArbol, *PtrArbol;

struct Bosque {
    int NumArboles;
    TArbol Arboles[DMaxArboles];
};
typedef struct Bosque TBosque, *PtrBosque;

struct ListaArboles {
    int NumArboles;
    float Coste;
    float CosteArbolesCortados;
    float CosteArbolesRestantes;
    float LongitudCerca;
    float MaderaSobrante;
    int Arboles[DMaxArboles];
};
typedef struct ListaArboles TListaArboles, *PtrListaArboles;

typedef struct {
    int CombinacionesEvaluadas;
    int CombinacionesValidas;
    int CombinacionesNoValidas;
    int MejorCombinacionCoste;
    int PeorCombinacionCoste;
    int MejorCombinacionArboles;
    int PeorCombinacionArboles;
} TEstadisticas, *PtrEstadisticas;

typedef Point TVectorCoordenadas[DMaxArboles], *PtrVectorCoordenadas;
typedef enum {false, true} bool;

// Estructura para pasar datos a cada hilo
typedef struct {
    int id_hilo;
    int primera_combinacion;
    int ultima_combinacion;
    TEstadisticas estadisticas_locales;
} DatosHilo;

// Variables Globales
TBosque ArbolesEntrada;
TListaArboles OptimoGlobal;
TEstadisticas EstadisticasGlobales;
int num_threads_total;
int hilos_terminados = 0;
double elapsed_sec;

// Mecanismos de sincronización
pthread_mutex_t mutex_optimo;
pthread_mutex_t mutex_estadisticas;
pthread_mutex_t mutex_terminado;
pthread_cond_t cond_terminado;
sem_t sem_print;

// Prototipos
bool LeerFicheroEntrada(char *PathFicIn);
bool GenerarFicheroSalida(TListaArboles optimo, char *PathFicOut);
void PrintResultado(TListaArboles Optimo);
bool GenerarFicheroSalidaFile(TListaArboles Optimo, FILE *FicOut);
bool CalcularCercaOptimaConcurrente(PtrListaArboles Optimo, int num_threads);
void* HiloTrabajador(void* arg);
void OrdenarArboles();
int EvaluarCombinacionListaArboles(int Combinacion);
int ConvertirCombinacionToArboles(int Combinacion, PtrListaArboles CombinacionArboles);
int ConvertirCombinacionToArbolesTalados(int Combinacion, PtrListaArboles CombinacionArbolesTalados);
void ObtenerListaCoordenadasArboles(TListaArboles CombinacionArboles, TVectorCoordenadas Coordenadas);
float CalcularLongitudCerca(TVectorCoordenadas CoordenadasCerca, int SizeCerca);
float CalcularDistancia(int x1, int y1, int x2, int y2);
int CalcularMaderaArbolesTalados(TListaArboles CombinacionArboles);
int CalcularCosteCombinacion(TListaArboles CombinacionArboles);
void MostrarArboles(TListaArboles CombinacionArboles);
void ResetEstadisticas(PtrEstadisticas std);
void PrintEstadisticas(TEstadisticas estadisticas, char *tipo);
void ActualizarEstadisticasGlobales(TEstadisticas *locales);
void CalcularEstadisticasFinalesOptimo(PtrListaArboles Optimo);

int main(int argc, char *argv[]) {
    TListaArboles Optimo;
    
    if (argc < 3 || argc > 4) {
        printf("Error Argumentos. Usage: CalcArboles <Fichero_Entrada> <Max_Threads> [<Fichero_Salida>]\n");
        exit(1);
    }

    if (!LeerFicheroEntrada(argv[1])) {
        printf("Error lectura fichero entrada.\n");
        exit(1);
    }

    num_threads_total = atoi(argv[2]);
    if (num_threads_total < 1) {
        printf("Error: Número de threads debe ser mayor que 0.\n");
        exit(1);
    }

    if (!CalcularCercaOptimaConcurrente(&Optimo, num_threads_total)) {
        printf("Error CalcularCercaOptima.\n");
        exit(1);
    }

    printf("\n[CONCURRENTE] Calculo cerca optima %d arboles con %d hilos: Tiempo: %05.6f\n", 
           ArbolesEntrada.NumArboles, num_threads_total, elapsed_sec);
    PrintEstadisticas(EstadisticasGlobales, "Estadisticas Finales Globales");
    printf("Solucion:\n");
    PrintResultado(Optimo);

    if (argc == 3) {
        if (!GenerarFicheroSalida(Optimo, "./Valla.res")) {
            printf("Error GenerarFicheroSalida.\n");
            exit(1);
        }
    } else {
        if (!GenerarFicheroSalida(Optimo, argv[3])) {
            printf("Error GenerarFicheroSalida.\n");
            exit(1);
        }
    }

    // Liberar recursos
    pthread_mutex_destroy(&mutex_optimo);
    pthread_mutex_destroy(&mutex_estadisticas);
    pthread_mutex_destroy(&mutex_terminado);
    pthread_cond_destroy(&cond_terminado);
    sem_destroy(&sem_print);

    exit(0);
}

bool LeerFicheroEntrada(char *PathFicIn) {
    FILE *FicIn;
    int a;

    FicIn = fopen(PathFicIn, "r");
    if (FicIn == NULL) {
        perror("Lectura Fichero entrada.");
        return false;
    }
    printf("Datos Entrada:\n");

    if (fscanf(FicIn, "%d", &(ArbolesEntrada.NumArboles)) < 1) {
        perror("Lectura arboles entrada");
        fclose(FicIn);
        return false;
    }
    printf("\tArboles: %d.\n", ArbolesEntrada.NumArboles);

    for (a = 0; a < ArbolesEntrada.NumArboles; a++) {
        ArbolesEntrada.Arboles[a].IdArbol = a + 1;
        if (fscanf(FicIn, "%d %d %d %d", 
                   &(ArbolesEntrada.Arboles[a].Coord.x),
                   &(ArbolesEntrada.Arboles[a].Coord.y),
                   &(ArbolesEntrada.Arboles[a].Valor),
                   &(ArbolesEntrada.Arboles[a].Longitud)) < 4) {
            perror("Lectura datos arbol.");
            fclose(FicIn);
            return false;
        }
        printf("\tArbol %d-> (%d,%d) Coste:%d, Longitud:%d.\n", 
               a + 1, ArbolesEntrada.Arboles[a].Coord.x,
               ArbolesEntrada.Arboles[a].Coord.y,
               ArbolesEntrada.Arboles[a].Valor,
               ArbolesEntrada.Arboles[a].Longitud);
    }

    fclose(FicIn);
    return true;
}

bool GenerarFicheroSalida(TListaArboles Optimo, char *PathFicOut) {
    FILE *FicOut;

    FicOut = fopen(PathFicOut, "w+");
    if (FicOut == NULL) {
        perror("Escritura fichero salida.");
        return false;
    }

    bool result = GenerarFicheroSalidaFile(Optimo, FicOut);
    fclose(FicOut);
    return result;
}

void PrintResultado(TListaArboles Optimo) {
    GenerarFicheroSalidaFile(Optimo, stdout);
}

bool GenerarFicheroSalidaFile(TListaArboles Optimo, FILE *FicOut) {
    int a;

    if (fprintf(FicOut, "Cortar %d arbol/es: ", Optimo.NumArboles) < 1) {
        perror("Escribir numero de arboles a talar");
        return false;
    }

    for (a = 0; a < Optimo.NumArboles; a++) {
        if (fprintf(FicOut, "%d ", ArbolesEntrada.Arboles[Optimo.Arboles[a]].IdArbol) < 1) {
            perror("Escritura numero arbol.");
            return false;
        }
    }

    if (fprintf(FicOut, "\nMadera Sobrante: %4.2f (%4.2f)", 
                Optimo.MaderaSobrante, Optimo.LongitudCerca) < 1) {
        perror("Escribir madera sobrante.");
        return false;
    }

    if (fprintf(FicOut, "\nValor arboles cortados: %4.2f.", 
                Optimo.CosteArbolesCortados) < 1) {
        perror("Escribir coste arboles a talar.");
        return false;
    }

    if (fprintf(FicOut, "\nValor arboles restantes: %4.2f\n", 
                Optimo.CosteArbolesRestantes) < 1) {
        perror("Escribir coste arboles restantes.");
        return false;
    }

    return true;
}

bool CalcularCercaOptimaConcurrente(PtrListaArboles Optimo, int num_threads) {
    struct timespec start, finish;
    int MaxCombinaciones;
    pthread_t threads[num_threads];
    DatosHilo datos_hilos[num_threads];
    int combinaciones_por_hilo;

    MaxCombinaciones = (int)pow(2.0, ArbolesEntrada.NumArboles) - 1;
    OrdenarArboles();

    // Inicializar mecanismos de sincronización
    pthread_mutex_init(&mutex_optimo, NULL);
    pthread_mutex_init(&mutex_estadisticas, NULL);
    pthread_mutex_init(&mutex_terminado, NULL);
    pthread_cond_init(&cond_terminado, NULL);
    sem_init(&sem_print, 0, 1);

    // Inicializar óptimo global
    OptimoGlobal.NumArboles = 0;
    OptimoGlobal.Coste = DMaximoCoste;
    ResetEstadisticas(&EstadisticasGlobales);
    hilos_terminados = 0;

    clock_gettime(CLOCK_MONOTONIC, &start);

    // Dividir trabajo entre hilos
    combinaciones_por_hilo = MaxCombinaciones / num_threads;

    printf("\nEvaluacion Combinaciones posibles con %d hilos:\n", num_threads);

    // Crear hilos
    for (int i = 0; i < num_threads; i++) {
        datos_hilos[i].id_hilo = i;
        datos_hilos[i].primera_combinacion = 1 + i * combinaciones_por_hilo;
        
        if (i == num_threads - 1) {
            datos_hilos[i].ultima_combinacion = MaxCombinaciones + 1;
        } else {
            datos_hilos[i].ultima_combinacion = 1 + (i + 1) * combinaciones_por_hilo;
        }
        
        ResetEstadisticas(&datos_hilos[i].estadisticas_locales);
        
        if (pthread_create(&threads[i], NULL, HiloTrabajador, &datos_hilos[i]) != 0) {
            perror("Error al crear hilo");
            return false;
        }
    }

    // Esperar a que todos los hilos terminen usando variable de condición
    pthread_mutex_lock(&mutex_terminado);
    while (hilos_terminados < num_threads) {
        pthread_cond_wait(&cond_terminado, &mutex_terminado);
    }
    pthread_mutex_unlock(&mutex_terminado);

    clock_gettime(CLOCK_MONOTONIC, &finish);
    elapsed_sec = (finish.tv_sec - start.tv_sec);
    elapsed_sec += (finish.tv_nsec - start.tv_nsec) / 1000000000.0;

    // Join de hilos para limpiar recursos
    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
        printf("\n");
        char titulo[100];
        sprintf(titulo, "Estadisticas Hilo %d", i);
        PrintEstadisticas(datos_hilos[i].estadisticas_locales, titulo);
    }

    // Copiar resultado final
    *Optimo = OptimoGlobal;
    CalcularEstadisticasFinalesOptimo(Optimo);

    return true;
}

void* HiloTrabajador(void* arg) {
    DatosHilo* datos = (DatosHilo*)arg;
    int Combinacion, MejorCombinacion = 0, CosteMejorCombinacion;
    int Coste;
    TListaArboles OptimoParcial;

    CosteMejorCombinacion = DMaximoCoste;

    for (Combinacion = datos->primera_combinacion; 
         Combinacion < datos->ultima_combinacion; 
         Combinacion++) {
        
        Coste = EvaluarCombinacionListaArboles(Combinacion);
        
        // Actualizar estadísticas locales
        datos->estadisticas_locales.CombinacionesEvaluadas++;
        
        if (Coste != DMaximoCoste) {
            datos->estadisticas_locales.CombinacionesValidas++;
            
            TListaArboles CombTalados;
            int NumTalados = ConvertirCombinacionToArbolesTalados(Combinacion, &CombTalados);
            
            if (datos->estadisticas_locales.MejorCombinacionCoste > Coste)
                datos->estadisticas_locales.MejorCombinacionCoste = Coste;
            if (datos->estadisticas_locales.PeorCombinacionCoste < Coste)
                datos->estadisticas_locales.PeorCombinacionCoste = Coste;
            if (datos->estadisticas_locales.MejorCombinacionArboles > NumTalados)
                datos->estadisticas_locales.MejorCombinacionArboles = NumTalados;
            if (datos->estadisticas_locales.PeorCombinacionArboles < NumTalados)
                datos->estadisticas_locales.PeorCombinacionArboles = NumTalados;
        } else {
            datos->estadisticas_locales.CombinacionesNoValidas++;
        }

        // Actualizar mejor local
        if (Coste < CosteMejorCombinacion) {
            CosteMejorCombinacion = Coste;
            MejorCombinacion = Combinacion;
        }

        // Actualizar óptimo global si encontramos algo mejor
        pthread_mutex_lock(&mutex_optimo);
        if (Coste < OptimoGlobal.Coste) {
            ConvertirCombinacionToArbolesTalados(Combinacion, &OptimoGlobal);
            OptimoGlobal.Coste = Coste;
        }
        pthread_mutex_unlock(&mutex_optimo);

        // Cada S combinaciones, actualizar estadísticas globales y mostrar progreso
        if ((Combinacion % S) == 0) {
            ActualizarEstadisticasGlobales(&datos->estadisticas_locales);
            
            sem_wait(&sem_print);
            ConvertirCombinacionToArbolesTalados(MejorCombinacion, &OptimoParcial);
            printf("\r\t[Hilo %d][%d] OptimoParcial %d-> Coste %d, %d Arboles talados: ",
                   datos->id_hilo, Combinacion, MejorCombinacion, 
                   CosteMejorCombinacion, OptimoParcial.NumArboles);
            MostrarArboles(OptimoParcial);
            fflush(stdout);
            sem_post(&sem_print);
        }
    }

    // Actualizar estadísticas globales finales
    ActualizarEstadisticasGlobales(&datos->estadisticas_locales);

    // Señalar que este hilo terminó
    pthread_mutex_lock(&mutex_terminado);
    hilos_terminados++;
    if (hilos_terminados == num_threads_total) {
        pthread_cond_signal(&cond_terminado);
    }
    pthread_mutex_unlock(&mutex_terminado);

    return NULL;
}

void ActualizarEstadisticasGlobales(TEstadisticas *locales) {
    pthread_mutex_lock(&mutex_estadisticas);
    
    EstadisticasGlobales.CombinacionesEvaluadas += locales->CombinacionesEvaluadas;
    EstadisticasGlobales.CombinacionesValidas += locales->CombinacionesValidas;
    EstadisticasGlobales.CombinacionesNoValidas += locales->CombinacionesNoValidas;
    
    if (locales->MejorCombinacionCoste < EstadisticasGlobales.MejorCombinacionCoste)
        EstadisticasGlobales.MejorCombinacionCoste = locales->MejorCombinacionCoste;
    if (locales->PeorCombinacionCoste > EstadisticasGlobales.PeorCombinacionCoste)
        EstadisticasGlobales.PeorCombinacionCoste = locales->PeorCombinacionCoste;
    if (locales->MejorCombinacionArboles < EstadisticasGlobales.MejorCombinacionArboles)
        EstadisticasGlobales.MejorCombinacionArboles = locales->MejorCombinacionArboles;
    if (locales->PeorCombinacionArboles > EstadisticasGlobales.PeorCombinacionArboles)
        EstadisticasGlobales.PeorCombinacionArboles = locales->PeorCombinacionArboles;
    
    pthread_mutex_unlock(&mutex_estadisticas);
}

void CalcularEstadisticasFinalesOptimo(PtrListaArboles Optimo) {
    TListaArboles CombinacionArboles;
    TVectorCoordenadas CoordArboles, CercaArboles;
    int NumArboles, PuntosCerca;
    float MaderaArbolesTalados;

    // Buscar la combinación que dio el óptimo
    int MaxCombinaciones = (int)pow(2.0, ArbolesEntrada.NumArboles) - 1;
    for (int comb = 1; comb <= MaxCombinaciones; comb++) {
        int coste = EvaluarCombinacionListaArboles(comb);
        if (coste == Optimo->Coste) {
            NumArboles = ConvertirCombinacionToArboles(comb, &CombinacionArboles);
            ObtenerListaCoordenadasArboles(CombinacionArboles, CoordArboles);
            PuntosCerca = chainHull_2D(CoordArboles, NumArboles, CercaArboles);
            
            Optimo->LongitudCerca = CalcularLongitudCerca(CercaArboles, PuntosCerca);
            MaderaArbolesTalados = CalcularMaderaArbolesTalados(*Optimo);
            Optimo->MaderaSobrante = MaderaArbolesTalados - Optimo->LongitudCerca;
            Optimo->CosteArbolesCortados = Optimo->Coste;
            Optimo->CosteArbolesRestantes = CalcularCosteCombinacion(CombinacionArboles);
            break;
        }
    }
}

void OrdenarArboles() {
    int a, b;
    
    for (a = 0; a < (ArbolesEntrada.NumArboles - 1); a++) {
        for (b = a + 1; b < ArbolesEntrada.NumArboles; b++) {
            if (ArbolesEntrada.Arboles[b].Coord.x < ArbolesEntrada.Arboles[a].Coord.x ||
                (ArbolesEntrada.Arboles[b].Coord.x == ArbolesEntrada.Arboles[a].Coord.x && 
                 ArbolesEntrada.Arboles[b].Coord.y < ArbolesEntrada.Arboles[a].Coord.y)) {
                TArbol aux = ArbolesEntrada.Arboles[a];
                ArbolesEntrada.Arboles[a] = ArbolesEntrada.Arboles[b];
                ArbolesEntrada.Arboles[b] = aux;
            }
        }
    }
}

int EvaluarCombinacionListaArboles(int Combinacion) {
    TVectorCoordenadas CoordArboles, CercaArboles;
    TListaArboles CombinacionArboles, CombinacionArbolesTalados;
    int NumArboles, NumArbolesTalados, PuntosCerca, CosteCombinacion;
    float LongitudCerca, MaderaArbolesTalados;

    NumArboles = ConvertirCombinacionToArboles(Combinacion, &CombinacionArboles);
    ObtenerListaCoordenadasArboles(CombinacionArboles, CoordArboles);
    PuntosCerca = chainHull_2D(CoordArboles, NumArboles, CercaArboles);
    LongitudCerca = CalcularLongitudCerca(CercaArboles, PuntosCerca);

    NumArbolesTalados = ConvertirCombinacionToArbolesTalados(Combinacion, &CombinacionArbolesTalados);
    MaderaArbolesTalados = CalcularMaderaArbolesTalados(CombinacionArbolesTalados);
    
    if (LongitudCerca > MaderaArbolesTalados) {
        return DMaximoCoste;
    }

    CosteCombinacion = CalcularCosteCombinacion(CombinacionArbolesTalados);
    return CosteCombinacion;
}

int ConvertirCombinacionToArboles(int Combinacion, PtrListaArboles CombinacionArboles) {
    int arbol = 0;

    CombinacionArboles->NumArboles = 0;
    CombinacionArboles->Coste = 0;

    while (arbol < ArbolesEntrada.NumArboles) {
        if ((Combinacion % 2) == 0) {
            CombinacionArboles->Arboles[CombinacionArboles->NumArboles] = arbol;
            CombinacionArboles->NumArboles++;
            CombinacionArboles->Coste += ArbolesEntrada.Arboles[arbol].Valor;
        }
        arbol++;
        Combinacion = Combinacion >> 1;
    }

    return CombinacionArboles->NumArboles;
}

int ConvertirCombinacionToArbolesTalados(int Combinacion, PtrListaArboles CombinacionArbolesTalados) {
    int arbol = 0;

    CombinacionArbolesTalados->NumArboles = 0;
    CombinacionArbolesTalados->Coste = 0;

    while (arbol < ArbolesEntrada.NumArboles) {
        if ((Combinacion % 2) == 1) {
            CombinacionArbolesTalados->Arboles[CombinacionArbolesTalados->NumArboles] = arbol;
            CombinacionArbolesTalados->NumArboles++;
            CombinacionArbolesTalados->Coste += ArbolesEntrada.Arboles[arbol].Valor;
        }
        arbol++;
        Combinacion = Combinacion >> 1;
    }

    return CombinacionArbolesTalados->NumArboles;
}

void ObtenerListaCoordenadasArboles(TListaArboles CombinacionArboles, TVectorCoordenadas Coordenadas) {
    int c, arbol;

    for (c = 0; c < CombinacionArboles.NumArboles; c++) {
        arbol = CombinacionArboles.Arboles[c];
        Coordenadas[c].x = ArbolesEntrada.Arboles[arbol].Coord.x;
        Coordenadas[c].y = ArbolesEntrada.Arboles[arbol].Coord.y;
    }
}

float CalcularLongitudCerca(TVectorCoordenadas CoordenadasCerca, int SizeCerca) {
    int x;
    float coste = 0;
    
    for (x = 0; x < (SizeCerca - 1); x++) {
        coste += CalcularDistancia(CoordenadasCerca[x].x, CoordenadasCerca[x].y,
                                   CoordenadasCerca[x + 1].x, CoordenadasCerca[x + 1].y);
    }
    
    return coste;
}

float CalcularDistancia(int x1, int y1, int x2, int y2) {
    return sqrt(pow((double)abs(x2 - x1), 2.0) + pow((double)abs(y2 - y1), 2.0));
}

int CalcularMaderaArbolesTalados(TListaArboles CombinacionArboles) {
    int a;
    int LongitudTotal = 0;
    
    for (a = 0; a < CombinacionArboles.NumArboles; a++) {
        LongitudTotal += ArbolesEntrada.Arboles[CombinacionArboles.Arboles[a]].Longitud;
    }
    
    return LongitudTotal;
}

int CalcularCosteCombinacion(TListaArboles CombinacionArboles) {
    int a;
    int CosteTotal = 0;
    
    for (a = 0; a < CombinacionArboles.NumArboles; a++) {
        CosteTotal += ArbolesEntrada.Arboles[CombinacionArboles.Arboles[a]].Valor;
    }
    
    return CosteTotal;
}

void MostrarArboles(TListaArboles CombinacionArboles) {
    int a;

    for (a = 0; a < CombinacionArboles.NumArboles; a++)
        printf("%d ", ArbolesEntrada.Arboles[CombinacionArboles.Arboles[a]].IdArbol);

    for (; a < ArbolesEntrada.NumArboles; a++)
        printf("  ");
}

void ResetEstadisticas(PtrEstadisticas std) {
    memset(std, 0, sizeof(TEstadisticas));
    std->MejorCombinacionCoste = DMaximoCoste;
    std->MejorCombinacionArboles = DMaxArboles;
}

void PrintEstadisticas(TEstadisticas estadisticas, char *tipo) {
    printf("\n++ %s ++++++++++++++++++++++++++++++++++++++++\n", tipo);
    printf("++ Comb Evaluadas: %d \tComb Validas: %d \tComb Invalidas: %d\n",
           estadisticas.CombinacionesEvaluadas, estadisticas.CombinacionesValidas, 
           estadisticas.CombinacionesNoValidas);
    printf("++ Mejor Coste: %d \t\tPeor Coste: %d\n",
           estadisticas.MejorCombinacionCoste, estadisticas.PeorCombinacionCoste);
    printf("++ Mejor num arboles: %d \tPeor num arboles: %d\n",
           estadisticas.MejorCombinacionArboles, estadisticas.PeorCombinacionArboles);
    printf("++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n");
}
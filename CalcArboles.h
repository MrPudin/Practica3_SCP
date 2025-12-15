//
// Created by Fernando Cores Prado on 12/12/25.
//

#ifndef CALCARBOLESSEC_CALCARBOLES_H
#define CALCARBOLESSEC_CALCARBOLES_H

#include <stdio.h>
#include <stdbool.h>

#include "ConvexHull.h"


////////////////
// Constantes //
////////////////

#define DMaxArboles 	25
#define DMaximoCoste 999999
#define S 1000000
#define DDebug 0


//////////////////////////
// Estructuras de datos //
//////////////////////////

// Definicin estructura arbol entrada (Conjunto �boles).
struct  Arbol
{
    int	  IdArbol;
    Point Coord;			// Posicin �bol
    int Valor;				// Valor / Coste �bol.
    int Longitud;			// Cantidad madera �bol
};
typedef struct Arbol TArbol, *PtrArbol;


// Definicin estructura Bosque entrada (Conjunto �boles).
struct Bosque
{
    int 		NumArboles;
    TArbol 	Arboles[DMaxArboles];
};
typedef struct Bosque TBosque, *PtrBosque;


// Combinacin .
struct ListaArboles
{
    int 		NumArboles;
    float		Coste;
    float		CosteArbolesCortados;
    float		CosteArbolesRestantes;
    float		LongitudCerca;
    float		MaderaSobrante;
    int 		Arboles[DMaxArboles];
};
typedef struct ListaArboles TListaArboles, *PtrListaArboles;


// Estadisticas
typedef struct
{
    int CombinacionesEvaluadas;
    int CombinacionesValidas;
    int CombinacionesNoValidas;
    int MejorCombinacionCoste;
    int PeorCombinacionCoste;
    int MejorCombinacionArboles;
    int PeorCombinacionArboles;
} TEstadisticas, *PtrEstadisticas;


// Vector est�ico Coordenadas.
typedef Point TVectorCoordenadas[DMaxArboles], *PtrVectorCoordenadas;


////////////////////////
// Variables Globales //
////////////////////////

TBosque ArbolesEntrada;
double elapsed_sec;

// Estadisticas
TEstadisticas TotalEstadisticas={0,0,0,DMaximoCoste,0,DMaxArboles,0};


//////////////////////////
// Prototipos funciones //
//////////////////////////

bool LeerFicheroEntrada(char *PathFicIn);
bool GenerarFicheroSalida(TListaArboles optimo, char *PathFicOut);
void PrintResultado(TListaArboles Optimo);
bool GenerarFicheroSalidaFile(TListaArboles Optimo, FILE *FicOut);
bool CalcularCercaOptima(PtrListaArboles Optimo);
void OrdenarArboles(void);
bool CalcularCombinacionOptima(int PrimeraCombinacion, int UltimaCombinacion, PtrListaArboles Optimo);
int EvaluarCombinacionListaArboles(int Combinacion);
int ConvertirCombinacionToArboles(int Combinacion, PtrListaArboles CombinacionArboles);
int ConvertirCombinacionToArbolesTalados(int Combinacion, PtrListaArboles CombinacionArbolesTalados);
void ObtenerListaCoordenadasArboles(TListaArboles CombinacionArboles, TVectorCoordenadas Coordenadas);
float CalcularLongitudCerca(TVectorCoordenadas CoordenadasCerca, int SizeCerca);
float CalcularDistancia(int x1, int y1, int x2, int y2);
int CalcularMaderaArbolesTalados(TListaArboles CombinacionArboles);
int CalcularCosteCombinacion(TListaArboles CombinacionArboles);
void MostrarBosque(void);
void PrintArbol(TArbol arbol);
void MostrarArboles(TListaArboles CombinacionArboles);
void ResetEstadidisticas(PtrEstadisticas std);
void PrintEstadisticas(TEstadisticas estadisticas, char *tipo);


#endif //CALCARBOLESSEC_CALCARBOLES_H

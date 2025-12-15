#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <string.h>

#include "CalcArboles.h"


int main(int argc, char *argv[])
{
	TListaArboles Optimo;
	
	if (argc<2 || argc>3)
		printf("Error Argumentos. Usage: CalcArboles <Fichero_Entrada> [<Fichero_Salida>]");

	if (!LeerFicheroEntrada(argv[1]))
	{
		printf("Error lectura fichero entrada.\n");
		exit(1);
	}

	if (!CalcularCercaOptima(&Optimo))
	{
		printf("Error CalcularCercaOptima.\n");
		exit(1);
	}

	printf("\n[SECUENCIAL] Calculo cerca optima %d arboles con %d hilos: Tiempo: %05.6f. \n",ArbolesEntrada.NumArboles, 1, elapsed_sec);
	PrintEstadisticas(TotalEstadisticas, "Estadisticas Finales");
	printf("Solucion:\n");
	PrintResultado(Optimo);

	if (argc==2)
	{
		if (!GenerarFicheroSalida(Optimo, "./Valla.res"))
		{
			printf("Error GenerarFicheroSalida.\n");
			exit(1);
		}
	}
	else
	{
		if (!GenerarFicheroSalida(Optimo, argv[2]))
		{
			printf("Error GenerarFicheroSalida.\n");
			exit(1);
		}
	}
	exit(0);
}



bool LeerFicheroEntrada(char *PathFicIn)
{
	FILE *FicIn;
	int a;

	FicIn=fopen(PathFicIn,"r");
	if (FicIn==NULL)
	{
		perror("Lectura Fichero entrada.");
		return false;
	}
	printf("Datos Entrada:\n");

	// Leemos el nmero de arboles del bosque de entrada.
	if (fscanf(FicIn, "%d", &(ArbolesEntrada.NumArboles))<1)
	{
		perror("Lectura arboles entrada");
		return false;
	}
	printf("\tArboles: %d.\n",ArbolesEntrada.NumArboles);

	// Leer atributos arboles.
	for(a=0;a<ArbolesEntrada.NumArboles;a++)
	{
		ArbolesEntrada.Arboles[a].IdArbol=a+1;
		// Leer x, y, Coste, Longitud.
		if (fscanf(FicIn, "%d %d %d %d",&(ArbolesEntrada.Arboles[a].Coord.x), &(ArbolesEntrada.Arboles[a].Coord.y), &(ArbolesEntrada.Arboles[a].Valor), &(ArbolesEntrada.Arboles[a].Longitud))<4)
		{
			perror("Lectura datos arbol.");
			return false;
		}
		printf("\tArbol %d-> (%d,%d) Coste:%d, Longitud:%d.\n",a+1,ArbolesEntrada.Arboles[a].Coord.x, ArbolesEntrada.Arboles[a].Coord.y, ArbolesEntrada.Arboles[a].Valor, ArbolesEntrada.Arboles[a].Longitud);
	}

	return true;
}





bool GenerarFicheroSalida(TListaArboles Optimo, char *PathFicOut)
{
	FILE *FicOut;

	FicOut=fopen(PathFicOut,"w+");
	if (FicOut==NULL)
	{
		perror("Escritura fichero salida.");
		return false;
	}

	return GenerarFicheroSalidaFile(Optimo, FicOut);
}


void PrintResultado(TListaArboles Optimo)
{
	GenerarFicheroSalidaFile(Optimo, stdout);
}


bool GenerarFicheroSalidaFile(TListaArboles Optimo, FILE *FicOut)
{
	int a;

	// Escribir arboles a talartalado.
	// Escribimos nmero de arboles a talar.
	if (fprintf(FicOut, "Cortar %d arbol/es: ", Optimo.NumArboles)<1)
	{
		perror("Escribir numero de arboles a talar");
		return false;
	}

	for(a=0;a<Optimo.NumArboles;a++)
	{
		// Escribir nmero arbol.
		if (fprintf(FicOut, "%d ",ArbolesEntrada.Arboles[Optimo.Arboles[a]].IdArbol)<1)
		{
			perror("Escritura numero arbol.");
			return false;
		}
	}

	// Escribimos coste arboles a talar.
	if (fprintf(FicOut, "\nMadera Sobrante: %4.2f (%4.2f)", Optimo.MaderaSobrante,  Optimo.LongitudCerca)<1)
	{
		perror("Escribir coste arboles a talar.");
		return false;
	}

	// Escribimos coste arboles a talar.
	if (fprintf(FicOut, "\nValor arboles cortados: %4.2f.", Optimo.CosteArbolesCortados)<1)
	{
		perror("Escribir coste arboles a talar.");
		return false;
	}

	// Escribimos coste arboles a talar.
	if (fprintf(FicOut, "\nValor arboles restantes: %4.2f\n", Optimo.CosteArbolesRestantes)<1)
	{
		perror("Escribir coste arboles a talar.");
		return false;
	}

	return true;
}



bool CalcularCercaOptima(PtrListaArboles Optimo)
{
	struct timespec start, finish;
	int MaxCombinaciones;

	/* C�culo Máximo Combinaciones */
	MaxCombinaciones = (int) pow(2.0,ArbolesEntrada.NumArboles)-1;

	// Ordenar Arboles por segun coordenadas crecientes de x,y
	OrdenarArboles();
    MostrarBosque();

    //Test();

	/* C�culo �timo */
	clock_gettime(CLOCK_MONOTONIC, &start);
	ResetEstadidisticas(&TotalEstadisticas);
	Optimo->NumArboles = 0;
	Optimo->Coste = DMaximoCoste;
	CalcularCombinacionOptima(1, MaxCombinaciones+1, Optimo);

	clock_gettime(CLOCK_MONOTONIC, &finish);
	elapsed_sec = (finish.tv_sec - start.tv_sec);
	elapsed_sec += (finish.tv_nsec - start.tv_nsec) / 1000000000.0;

	return true;
}



void OrdenarArboles(void)
{
  int a,b;
  
	for(a=0; a<(ArbolesEntrada.NumArboles-1); a++)
	{
		for(b=a+1; b<ArbolesEntrada.NumArboles; b++)
		{
			if ( ArbolesEntrada.Arboles[b].Coord.x < ArbolesEntrada.Arboles[a].Coord.x ||
				 (ArbolesEntrada.Arboles[b].Coord.x == ArbolesEntrada.Arboles[a].Coord.x && ArbolesEntrada.Arboles[b].Coord.y < ArbolesEntrada.Arboles[a].Coord.y) )
			{
				TArbol aux;

				// aux=a
				aux.Coord.x = ArbolesEntrada.Arboles[a].Coord.x;
				aux.Coord.y = ArbolesEntrada.Arboles[a].Coord.y;
				aux.IdArbol = ArbolesEntrada.Arboles[a].IdArbol;
				aux.Valor = ArbolesEntrada.Arboles[a].Valor;
				aux.Longitud = ArbolesEntrada.Arboles[a].Longitud;

				// a=b
				ArbolesEntrada.Arboles[a].Coord.x = ArbolesEntrada.Arboles[b].Coord.x;
				ArbolesEntrada.Arboles[a].Coord.y = ArbolesEntrada.Arboles[b].Coord.y;
				ArbolesEntrada.Arboles[a].IdArbol = ArbolesEntrada.Arboles[b].IdArbol;
				ArbolesEntrada.Arboles[a].Valor = ArbolesEntrada.Arboles[b].Valor;
				ArbolesEntrada.Arboles[a].Longitud = ArbolesEntrada.Arboles[b].Longitud;

				// b=aux
				ArbolesEntrada.Arboles[b].Coord.x = aux.Coord.x;
				ArbolesEntrada.Arboles[b].Coord.y = aux.Coord.y;
				ArbolesEntrada.Arboles[b].IdArbol = aux.IdArbol;
				ArbolesEntrada.Arboles[b].Valor = aux.Valor;
				ArbolesEntrada.Arboles[b].Longitud = aux.Longitud;
			}
		}
	}
}




// Calcula la combinacin ptima entre el rango de combinaciones PrimeraCombinacion-UltimaCombinacion.

bool CalcularCombinacionOptima(int PrimeraCombinacion, int UltimaCombinacion, PtrListaArboles Optimo)
{
	int Combinacion, MejorCombinacion=0, CosteMejorCombinacion;
	int Coste;
	TListaArboles OptimoParcial;

	TListaArboles CombinacionArboles;
	TVectorCoordenadas CoordArboles, CercaArboles;
	int NumArboles, PuntosCerca;
	float MaderaArbolesTalados;

  	printf("\nEvaluacion Combinaciones posibles:");
	CosteMejorCombinacion = Optimo->Coste;
	for (Combinacion=PrimeraCombinacion; Combinacion<UltimaCombinacion; Combinacion++)
	{
        if (Combinacion==7)
    	    printf("\tC%d -> \t",Combinacion);
		Coste = EvaluarCombinacionListaArboles(Combinacion);
		if ( Coste < CosteMejorCombinacion )
		{
			CosteMejorCombinacion = Coste;
			MejorCombinacion = Combinacion;
	      	//printf("***");
		}
		TotalEstadisticas.CombinacionesEvaluadas++;
		if ((Combinacion%S)==0)
		{
			ConvertirCombinacionToArbolesTalados(MejorCombinacion, &OptimoParcial);
			printf("\r\t[%d] OptimoParcial %d-> Coste %d, %d Arboles talados:", Combinacion, MejorCombinacion, CosteMejorCombinacion, OptimoParcial.NumArboles);
			MostrarArboles(OptimoParcial);

			// Mostrar Estadisticas
			PrintEstadisticas(TotalEstadisticas,"Estadisticas Globales");
		}
			
//    printf("\n");
	}

	printf("\n");
	
	ConvertirCombinacionToArbolesTalados(MejorCombinacion, &OptimoParcial);
	printf("\r\tOptimo %d-> Coste %d, %d Arboles talados:", MejorCombinacion, CosteMejorCombinacion, OptimoParcial.NumArboles);
	MostrarArboles(OptimoParcial);
	printf("\n");

	if (CosteMejorCombinacion == Optimo->Coste)
		return false;  // No se ha encontrado una combinacin mejor.

	// Asignar combinacin encontrada.
	ConvertirCombinacionToArbolesTalados(MejorCombinacion, Optimo);
	Optimo->Coste = CosteMejorCombinacion;
	// Calcular estadisticas óptimo.
	NumArboles = ConvertirCombinacionToArboles(MejorCombinacion, &CombinacionArboles);
	ObtenerListaCoordenadasArboles(CombinacionArboles, CoordArboles);
	PuntosCerca = chainHull_2D( CoordArboles, NumArboles, CercaArboles );

	Optimo->LongitudCerca = CalcularLongitudCerca(CercaArboles, PuntosCerca);
	MaderaArbolesTalados = CalcularMaderaArbolesTalados(*Optimo);
	Optimo->MaderaSobrante = MaderaArbolesTalados - Optimo->LongitudCerca;
	Optimo->CosteArbolesCortados = CosteMejorCombinacion;
	Optimo->CosteArbolesRestantes = CalcularCosteCombinacion(CombinacionArboles);

	return true;
}



int EvaluarCombinacionListaArboles(int Combinacion)
{
	TVectorCoordenadas CoordArboles, CercaArboles;
	TListaArboles CombinacionArboles, CombinacionArbolesTalados;
	int NumArboles, NumArbolesTalados, PuntosCerca, CosteCombinacion;
	float LongitudCerca, MaderaArbolesTalados;

	// Convertimos la combinacin al vector de arboles no talados.
	NumArboles = ConvertirCombinacionToArboles(Combinacion, &CombinacionArboles);

	// Obtener el vector de coordenadas de arboles no talados.
	ObtenerListaCoordenadasArboles(CombinacionArboles, CoordArboles);

	// Calcular la cerca
	PuntosCerca = chainHull_2D( CoordArboles, NumArboles, CercaArboles );

	/* Evaluar si obtenemos suficientes �boles para construir la cerca */
	LongitudCerca = CalcularLongitudCerca(CercaArboles, PuntosCerca);

	// Evaluar la madera obtenida mediante los arboles talados.
	// Convertimos la combinacin al vector de arboles no talados.
	NumArbolesTalados = ConvertirCombinacionToArbolesTalados(Combinacion, &CombinacionArbolesTalados);
	if (DDebug) printf(" %d arboles cortados: ",NumArbolesTalados);
	if (DDebug) MostrarArboles(CombinacionArbolesTalados);
    MaderaArbolesTalados = CalcularMaderaArbolesTalados(CombinacionArbolesTalados);
	if (DDebug) printf("  Madera:%4.2f  \tCerca:%4.2f ",MaderaArbolesTalados, LongitudCerca);
	if (LongitudCerca > MaderaArbolesTalados)
	{	// Los arboles cortados no tienen suficiente madera para construir la cerca.
		TotalEstadisticas.CombinacionesNoValidas++;
		return DMaximoCoste;
	}

	// Evaluar el coste de los arboles talados.
	CosteCombinacion = CalcularCosteCombinacion(CombinacionArbolesTalados);

	// Actualizar estadisticas parciales.
	TotalEstadisticas.CombinacionesValidas++;
	if (TotalEstadisticas.MejorCombinacionCoste>CosteCombinacion)
		TotalEstadisticas.MejorCombinacionCoste=CosteCombinacion;
	if (TotalEstadisticas.PeorCombinacionCoste<CosteCombinacion)
		TotalEstadisticas.PeorCombinacionCoste=CosteCombinacion;
	if (TotalEstadisticas.MejorCombinacionArboles>NumArbolesTalados)
		TotalEstadisticas.MejorCombinacionArboles=NumArbolesTalados;
	if (TotalEstadisticas.PeorCombinacionArboles<NumArbolesTalados)
		TotalEstadisticas.PeorCombinacionArboles=NumArbolesTalados;

if (DDebug) printf("\tCoste:%d",CosteCombinacion);
  
	return CosteCombinacion;
}


int ConvertirCombinacionToArboles(int Combinacion, PtrListaArboles CombinacionArboles)
{
	int arbol=0;

	CombinacionArboles->NumArboles=0;
	CombinacionArboles->Coste=0;

	while (arbol<ArbolesEntrada.NumArboles)
	{
		if ((Combinacion%2)==0)
		{
			CombinacionArboles->Arboles[CombinacionArboles->NumArboles]=arbol;
			CombinacionArboles->NumArboles++;
			CombinacionArboles->Coste+= ArbolesEntrada.Arboles[arbol].Valor;
		}
		arbol++;
		Combinacion = Combinacion>>1;
	}

	return CombinacionArboles->NumArboles;
}


int ConvertirCombinacionToArbolesTalados(int Combinacion, PtrListaArboles CombinacionArbolesTalados)
{
	int arbol=0;

	CombinacionArbolesTalados->NumArboles=0;
	CombinacionArbolesTalados->Coste=0;

	while (arbol<ArbolesEntrada.NumArboles)
	{
		if ((Combinacion%2)==1)
		{
			CombinacionArbolesTalados->Arboles[CombinacionArbolesTalados->NumArboles]=arbol;
			CombinacionArbolesTalados->NumArboles++;
			CombinacionArbolesTalados->Coste+= ArbolesEntrada.Arboles[arbol].Valor;
		}
		arbol++;
		Combinacion = Combinacion>>1;
	}

	return CombinacionArbolesTalados->NumArboles;
}



void ObtenerListaCoordenadasArboles(TListaArboles CombinacionArboles, TVectorCoordenadas Coordenadas)
{
	int c, arbol;

	for (c=0;c<CombinacionArboles.NumArboles;c++)
	{
        arbol=CombinacionArboles.Arboles[c];
		Coordenadas[c].x = ArbolesEntrada.Arboles[arbol].Coord.x;
		Coordenadas[c].y = ArbolesEntrada.Arboles[arbol].Coord.y;
	}
}


	
float CalcularLongitudCerca(TVectorCoordenadas CoordenadasCerca, int SizeCerca)
{
	int x;
	float coste=0;
	
	for (x=0;x<(SizeCerca-1);x++)
	{
		coste+= CalcularDistancia(CoordenadasCerca[x].x, CoordenadasCerca[x].y, CoordenadasCerca[x+1].x, CoordenadasCerca[x+1].y);
	}
	
	return coste;
}



float CalcularDistancia(int x1, int y1, int x2, int y2)
{
	return(sqrt(pow((double)abs(x2-x1),2.0)+pow((double)abs(y2-y1),2.0)));
}



int 
CalcularMaderaArbolesTalados(TListaArboles CombinacionArboles)
{
	int a;
	int LongitudTotal=0;
	
	for (a=0;a<CombinacionArboles.NumArboles;a++)
	{
		LongitudTotal += ArbolesEntrada.Arboles[CombinacionArboles.Arboles[a]].Longitud;
	}
	
	return(LongitudTotal);
}


int 
CalcularCosteCombinacion(TListaArboles CombinacionArboles)
{
	int a;
	int CosteTotal=0;
	
	for (a=0;a<CombinacionArboles.NumArboles;a++)
	{
		CosteTotal += ArbolesEntrada.Arboles[CombinacionArboles.Arboles[a]].Valor;
	}
	
	return(CosteTotal);
}


void
MostrarBosque(void)
{
    int a;

    for (a=0;a<ArbolesEntrada.NumArboles;a++)
        PrintArbol(ArbolesEntrada.Arboles[a]);
}

void
PrintArbol(TArbol arbol)
{
    printf("Arbol Id:%d --> x:%d,y:%d longitud:%d valor:%d.\n",arbol.IdArbol, arbol.Coord.x, arbol.Coord.y, arbol.Longitud, arbol.Valor);
}


void
MostrarArboles(TListaArboles CombinacionArboles)
{
	int a;

	for (a=0;a<CombinacionArboles.NumArboles;a++)
		printf("%d ",ArbolesEntrada.Arboles[CombinacionArboles.Arboles[a]].IdArbol);

  for (;a<ArbolesEntrada.NumArboles;a++)
    printf("  ");  
}

void ResetEstadidisticas(PtrEstadisticas std)
{
	memset(std, 0, sizeof(TEstadisticas));
	std->MejorCombinacionCoste = DMaximoCoste;
	std->MejorCombinacionArboles = DMaxArboles;
}

void PrintEstadisticas(TEstadisticas estadisticas, char *tipo)
{
	printf("\n++ %s +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n",tipo);
	printf("++ Comb Evaluadas: %d \tComb Validas: %d \tComb Invalidas:    %d\n",
						estadisticas.CombinacionesEvaluadas, estadisticas.CombinacionesValidas, estadisticas.CombinacionesNoValidas);
	printf("++ Mejor Coste:    %d \t\tPeor Coste:   %d  \tMejor num arboles: %d \tPeor num arboles: %d\n",
					estadisticas.MejorCombinacionCoste, estadisticas.PeorCombinacionCoste,
					estadisticas.MejorCombinacionArboles, estadisticas.PeorCombinacionArboles);
	printf("++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n");

}
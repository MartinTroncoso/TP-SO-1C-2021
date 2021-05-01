/*
 * Discordiador.h
 *
 *  Created on: 25 abr. 2021
 *      Author: utnso
 */

#ifndef DISCORDIADOR_H_
#define DISCORDIADOR_H_

#include <utils.h>

char* IP_MI_RAM;
char* PUERTO_MI_RAM;
char* IP_I_MONGO_STORE;
char* PUERTO_I_MONGO_STORE;
int GRADO_MULTITAREA;
char* ALGORITMO;
int QUANTUM;
int DURACION_SABOTAJE;
int RETARDO_CICLO_CPU;

typedef struct
{
	int cantidadTripulantes;
	char* rutaDeTareas;
	t_list* coordenadasTripulantes;
} comandoIniciarPatota;

typedef struct
{
	int coordenadaX;
	int coordenadaY;
} coordenadasTripulante;

void leerConfiguracion(t_config*);
void logearConfiguracion(t_log*);
void leer_consola(t_dictionary*,t_log*);
void paquete(int);
void terminar_programa(int, t_log*, t_config*);

void partirCadena(char*);

void crearDiccionarioComandos(t_dictionary*);

#endif /* DISCORDIADOR_H_ */

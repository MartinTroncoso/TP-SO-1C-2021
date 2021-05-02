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
}t_iniciar_patota;

typedef struct
{
	int coordenadaX;
	int coordenadaY;
}coordenadasTripulante;

typedef struct{
	uint32_t pid;
	void* tareas;
}patota;

typedef struct{
	uint32_t tid;
	char estado;
	coordenadasTripulante posicion;
	uint32_t proxInstruccion;
}tripulante;

t_config* configuracionDiscordiador;
t_log* loggerDiscordiador;
t_dictionary* diccionarioDiscordiador;

int socket_cliente_miRam;
int socket_cliente_iMongo;

int id_tripulante; //ID GLOBAL QUE SE VA INCREMENTANDO CADA VEZ QUE SE CREA UN TRIPULANTE.

t_list* tripulantes;

void inicializarVariables();

void iniciarPatota(t_iniciar_patota);
void listarTripulantes();
void expulsarTripulante(int);
void iniciarPlanificacion();
void pausarPlanificacion();
void obtenerBitacora(int);

void sumarIdTripulante();
void leer_consola(t_dictionary*,t_log*);
void paquete(int,int);
void terminar_programa();
void partirCadena(char*);
void crearDiccionarioComandos(t_dictionary*);

#endif /* DISCORDIADOR_H_ */

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

typedef struct{
	uint32_t cantidadTripulantes;
	char* rutaDeTareas;
	t_list* coordenadasTripulantes;
}t_iniciar_patota;

typedef struct{
	uint32_t pid;
	char* archivoTareas;
	t_list* tripulantes;
}t_patota;

typedef struct{
	uint32_t tid;
	char estado; //N-R-E-B
	posicion* posicion;
	tarea proxTarea;
	uint32_t idPatota;
}t_tripulante;

t_config* configuracionDiscordiador;
t_log* loggerDiscordiador;
t_dictionary* diccionarioDiscordiador;

int socket_escucha_iMongo;

uint32_t idTripulante;
uint32_t idPatota;

t_list* patotas;
t_list* tripulantes;

void inicializarVariables();

void iniciarPatota(t_iniciar_patota*);
void listarTripulantes();
void expulsarTripulante(int);
void iniciarPlanificacion();
void pausarPlanificacion();
void obtenerBitacora(uint32_t);

void* serializar_tripulante(t_tripulante*);
void gestionarTripulante(t_tripulante*);
void ingresar_comandos();
void terminar_programa();
void partirCadena(char**);
char* obtenerTareasComoCadena(char*);
void crearDiccionarioComandos(t_dictionary*);
t_iniciar_patota* obtenerDatosPatota(char**);

#endif /* DISCORDIADOR_H_ */

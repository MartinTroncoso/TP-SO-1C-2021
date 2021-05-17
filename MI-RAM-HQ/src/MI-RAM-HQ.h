/*
 * MI-RAM-HQ.h
 *
 *  Created on: 25 abr. 2021
 *      Author: utnso
 */

#ifndef MI_RAM_HQ_H_
#define MI_RAM_HQ_H_

#include <utils.h>

int TAMANIO_MEMORIA;
char* ESQUEMA_MEMORIA;
int TAMANIO_PAGINA;
int TAMANIO_SWAP;
char* PATH_SWAP;
char* ALGORITMO_REEMPLAZO;
char* IP_MI_RAM;
char* PUERTO_MI_RAM;

typedef struct{
	uint32_t pid;
	uint32_t direccionTareas;
	char* tareas; //hasta pasarlas a la memoria
}PCB;

typedef struct{
	uint32_t tid;
	uint32_t pid;
	char estado;
	posicion* posicion;
	uint32_t proxInstruccion;
	uint32_t direccionPCB;
}TCB;

t_config* configuracionMiRam;
t_log* loggerMiRam;

t_list* tripulantes; //capaz esta lista sea al pedo cuando guardemos todo en memoria
t_list* patotas; //idem

void inicializarVariables();

void* receptor_conexion(void*);
void recibir_datos_patota(int);
void recibir_datos_tripulante(int);

void terminar_programa();

#endif /* MI_RAM_HQ_H_ */

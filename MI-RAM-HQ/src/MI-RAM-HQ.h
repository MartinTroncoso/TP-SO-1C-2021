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
	char estado;
	uint32_t posX;
	uint32_t posY;
	char* proxInstruccion; //hasta pasarlas a memoria, luego debe ser uint32_t
	PCB* direccionPCB;
}TCB;

t_config* configuracionMiRam;
t_log* loggerMiRam;

t_list* tripulantes; //capaz esta lista sea al pedo cuando guardemos todo en memoria
t_list* patotas; //idem

pthread_mutex_t mutexTripulantes;
pthread_mutex_t mutexPatotas;

void inicializarVariables();

void atenderTripulante(void*);
void recibir_datos_patota(void*);
TCB* recibir_datos_tripulante(int);

PCB* buscar_patota(uint32_t);

void recibir_movimiento_tripulante(TCB*, int);
void enviar_proxima_tarea(TCB*, int);
void avanzar_proxima_instruccion(TCB*);

void terminar_programa();

#endif /* MI_RAM_HQ_H_ */

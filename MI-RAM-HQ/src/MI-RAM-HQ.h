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
char* PUERTO;

typedef struct{
	int coordenadaX;
	int coordenadaY;
}coordenadasTripulante;

typedef struct{
	uint32_t pid;
	uint32_t direccionTareas;
}PCB;

typedef struct{
	uint32_t tid;
	char estado;
	coordenadasTripulante posicion;
	uint32_t proxInstruccion;
	uint32_t direccionPCB;
}TCB;

typedef enum{
	INICIAR_PATOTA=1,
	EXPULSAR_TRIPULANTE=2
}tipo_mensaje;

t_config* configuracionMiRam;
t_log* loggerMiRam;

int socket_servidor;
int socket_discordiador;

void inicializarVariables();

#endif /* MI_RAM_HQ_H_ */

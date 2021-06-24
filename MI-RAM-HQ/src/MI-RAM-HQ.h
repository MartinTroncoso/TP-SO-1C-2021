/*
 * MI-RAM-HQ.h
 *
 *  Created on: 25 abr. 2021
 *      Author: utnso
 */

#ifndef MI_RAM_HQ_H_
#define MI_RAM_HQ_H_

#include <utils.h>
#include "Mapa.h"
#include "admin_memoria.h"
#include "memoria_basica.h"
#include "segmentacion.h"

int TAMANIO_MEMORIA;
char* ESQUEMA_MEMORIA;
int TAMANIO_PAGINA;
int TAMANIO_SWAP;
char* PATH_SWAP;
char* ALGORITMO_REEMPLAZO;
char* IP_MI_RAM;
char* PUERTO_MI_RAM;

void inicializarVariables();
void inicializarMapa();

void atenderTripulante(void* _cliente);
void recibir_datos_patota(void* _cliente);
uint32_t recibir_datos_tripulante(int socket_tripulante);

void recibir_movimiento_tripulante(int socket_tripulante, uint32_t tid);
char recibir_cambio_estado(int socket_tripulante, uint32_t tid);
void enviar_proxima_tarea(int socket_tripulante, uint32_t tid);
void finalizar_tripulante(int socket_tripulante, uint32_t tid);

void iniciar_dump_memoria();
void realizar_dump();
void terminar_programa();

#endif /* MI_RAM_HQ_H_ */

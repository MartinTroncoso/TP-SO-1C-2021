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

int TAMANIO_MEMORIA;
char* ESQUEMA_MEMORIA;
int TAMANIO_PAGINA;
int TAMANIO_SWAP;
char* PATH_SWAP;
char* ALGORITMO_REEMPLAZO;
char* IP_MI_RAM;
char* PUERTO_MI_RAM;

t_config* configuracionMiRam;
t_log* loggerPrincipal;
t_log* loggerSecundario;

NIVEL* nivel;

void inicializarVariables();
void inicializarMapa();

void atenderTripulante(void*);
void recibir_datos_patota(void*);
void recibir_datos_tripulante(int, uint32_t*, uint32_t*);
void recibir_movimiento_tripulante(uint32_t, int);
void enviar_proxima_tarea(uint32_t, int);
void finalizar_tripulante(uint32_t, int);

void terminar_programa();

#endif /* MI_RAM_HQ_H_ */

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
int PUERTO;

void leerConfiguracion(t_config*);
void logearConfiguracion(t_log*);
#endif /* MI_RAM_HQ_H_ */

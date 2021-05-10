/*
 * I-MONGO-STORE.h
 *
 *  Created on: 25 abr. 2021
 *      Author: utnso
 */

#ifndef I_MONGO_STORE_H_
#define I_MONGO_STORE_H_

#include <utils.h>

char* PUNTO_MONTAJE;
char* PUERTO;
int TIEMPO_SINCRONIZACION;

t_config* configuracionMongo;
t_log* loggerMongo;

int socket_servidor;
int socket_discordiador;

void inicializarVariables();

#endif /* I_MONGO_STORE_H_ */

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
int PUERTO;
int TIEMPO_SINCRONIZACION;

void leerConfiguracion(t_config*);
void logearConfiguracion(t_log*);

#endif /* I_MONGO_STORE_H_ */

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
int PUERTO_MI_RAM;
char* IP_I_MONGO_STORE;
int PUERTO_I_MONGO_STORE;
int GRADO_MULTITAREA;
char* ALGORITMO;
int QUANTUM;
int DURACION_SABOTAJE;
int RETARDO_CICLO_CPU;
void leerConfiguracion(t_config*);
void logearConfiguracion(t_log*);

#endif /* DISCORDIADOR_H_ */

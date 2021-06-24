/*
 * memoria_simple.h
 *
 *  Created on: 18 jun. 2021
 *      Author: utnso
 */

#ifndef MEMORIA_BASICA_H_
#define MEMORIA_BASICA_H_

#include <utils.h>
#include "admin_memoria.h"

typedef struct{
	uint32_t pid;
	uint32_t cant_tripulantes;
	uint32_t tripulantes_activos;
	char* tareas;
	pthread_mutex_t mutex_patota;
} bas_patota;

typedef struct{
	uint32_t tid;
	char estado;
	uint32_t posX;
	uint32_t posY;
	char* proxInstruccion; //Apunta a la posicion char*, dentro de la cadena original
	bas_patota* patota;
} bas_tripulante;

void bas_inicializacion();
void bas_guardar_nueva_patota(datos_patota*);
void bas_guardar_nuevo_tripulante(datos_tripulante*);
char bas_obtener_estado_tripulante(uint32_t);
char* bas_obtener_prox_instruccion_tripulante(uint32_t);
void bas_actualizar_estado_tripulante(uint32_t, char);
void bas_actualizar_posicion_tripulante(uint32_t, uint32_t, uint32_t);
void bas_actualizar_instruccion_tripulante(uint32_t);
void bas_generar_dump_memoria(FILE*);
void bas_liberar_tripulante(uint32_t);

#endif /* MEMORIA_BASICA_H_ */

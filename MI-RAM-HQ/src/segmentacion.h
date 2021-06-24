/*
 * segmentacion.h
 *
 *  Created on: 14 may. 2021
 *      Author: utnso
 */

#ifndef SEGMENTACION_H_
#define SEGMENTACION_H_

#include <utils.h>
#include "admin_memoria.h"

typedef struct {
	uint32_t inicio; //Direccion fisica de memoria
	uint32_t tamanio;
} segmento;

typedef struct {
	segmento* segmento;
	bool inicializado;
} seg_tabla_registro;

typedef struct {
	t_list* registros;
	uint32_t c_tripulantes;
	uint32_t pid;
} seg_tabla;

void seg_inicializacion();
void seg_guardar_nueva_patota(datos_patota*);
void seg_guardar_nuevo_tripulante(datos_tripulante*);
char seg_obtener_estado_tripulante(uint32_t);
char* seg_obtener_prox_instruccion_tripulante(uint32_t);
void seg_actualizar_estado_tripulante(uint32_t, char);
void seg_actualizar_posicion_tripulante(uint32_t, uint32_t, uint32_t);
void seg_actualizar_instruccion_tripulante(uint32_t);
void seg_liberar_tripulante(uint32_t);

#endif /* SEGMENTACION_H_ */

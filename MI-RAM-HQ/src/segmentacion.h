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
} reg_tabla_segmentos;

typedef struct {
	t_list* registros;
	uint32_t c_tripulantes;
	uint32_t pid;
} tabla_segmentos;


//CONSTANTES <NO TOCAR>
uint32_t TAMANIO_PCB = 8;
uint32_t TAMANIO_TCB = 21;

void crear_tabla_segmentos(datos_patota*);
void guardar_tripulante_a_memoria(datos_tripulante*);

segmento* buscar_reservar_memoria_first_fit(uint32_t);
segmento* reservar_memoria(uint32_t, uint32_t);
int guardar_buffer_en_segmento(segmento*, void*, uint32_t);

void seg_inicializacion();
void seg_guardar_nueva_patota(datos_patota*);
void seg_guardar_nuevo_tripulante(datos_tripulante*);
datos_patota* seg_obtener_patota(uint32_t);
datos_tripulante* seg_obtener_tripulante(uint32_t);
void seg_actualizar_estado_tripulante(uint32_t, char);
void seg_actualizar_posicion_tripulante(uint32_t, uint32_t, uint32_t);
void seg_actualizar_instruccion_tripulante(uint32_t);
void seg_liberar_tripulante(uint32_t);

#endif /* SEGMENTACION_H_ */

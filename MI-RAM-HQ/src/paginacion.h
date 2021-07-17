/*
 * paginacion.h
 *
 *  Created on: 14 may. 2021
 *      Author: utnso
 */

#ifndef PAGINACION_H_
#define PAGINACION_H_

#include <utils.h>
#include "admin_memoria.h"

typedef struct {
	uint32_t n_pagina;
	uint32_t n_frame;
	bool valido;
	bool usada_solo_trip;
	t_list* index_trip_ocupantes;
} pagina;

typedef struct{
	uint32_t pid;
	uint32_t n_pagina;
	bool libre;
} marco;

typedef struct {
	t_list* paginas;
	uint32_t pid;
	uint32_t tamanio_tareas;
	uint32_t c_tripulantes;
	t_list* tid_tripulantes;
	uint32_t tripulantes_finalizados;
} pag_tabla;


void pag_inicializacion();
void pag_guardar_nueva_patota(datos_patota*);
void pag_guardar_nuevo_tripulante(datos_tripulante*);
char pag_obtener_estado_tripulante(uint32_t);
char* pag_obtener_prox_instruccion_tripulante(uint32_t);
void pag_actualizar_estado_tripulante(uint32_t, char);
void pag_actualizar_posicion_tripulante(uint32_t, uint32_t, uint32_t);
void pag_actualizar_instruccion_tripulante(uint32_t);
void pag_generar_dump_memoria(FILE*);
void pag_liberar_tripulante(uint32_t);

#endif /* PAGINACION_H_ */

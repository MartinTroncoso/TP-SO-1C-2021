/*
 * memoria.h
 *
 *  Created on: 24 may. 2021
 *      Author: utnso
 */

#ifndef ADMIN_MEMORIA_H_
#define ADMIN_MEMORIA_H_

#include <utils.h>

typedef struct {
	uint32_t pid;
	uint32_t tripulantes;
	char* tareas;
} datos_patota;

typedef struct {
	uint32_t tid;
	uint32_t pid;
	char estado;
	char* proxInstruccion;
	uint32_t posX;
	uint32_t posY;
} datos_tripulante;


uint32_t obtener_direccion_logica (uint32_t, uint32_t);
void inicializar_administrador(uint32_t,
		void (*f_inicializacion)(),
		void (*f_g_n_patota)(datos_patota*),
		void (*f_g_n_tripulante)(datos_tripulante*),
		datos_patota* (*f_obt_patota)(uint32_t),
		datos_tripulante* (*f_obt_tripulante)(uint32_t),
		void (*f_act_est_tripulante)(uint32_t, char),
		void (*f_act_pos_tripulante)(uint32_t, uint32_t, uint32_t),
		void (*f_act_instr_tripulante)(uint32_t),
		void (*f_liberar_tripulante)(uint32_t));

void liberar_datos_tripulante(datos_tripulante*);
void liberar_datos_patota(datos_patota*);

void (*inicializacion)();
void (*guardar_nueva_patota)(datos_patota*);
void (*guardar_nuevo_tripulante)(datos_tripulante*);
datos_patota* (*obtener_patota)(uint32_t);
datos_tripulante* (*obtener_tripulante)(uint32_t);
void (*actualizar_estado_tripulante)(uint32_t, char);
void (*actualizar_posicion_tripulante)(uint32_t, uint32_t, uint32_t);
void (*actualizar_instruccion_tripulante)(uint32_t);
void (*liberar_tripulante)(uint32_t);

void* memoria_principal;

#endif /* ADMIN_MEMORIA_H_ */

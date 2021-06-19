/*
 * memoria.c
 *
 *  Created on: 24 may. 2021
 *      Author: utnso
 */

#include "admin_memoria.h"

static void reservar_memoria_principal(uint32_t);


void inicializar_administrador(uint32_t mem_reservar,
		void (*f_inicializacion)(),
		void (*f_g_n_patota)(datos_patota*),
		void (*f_g_n_tripulante)(datos_tripulante*),
		datos_patota* (*f_obt_patota)(uint32_t),
		datos_tripulante* (*f_obt_tripulante)(uint32_t),
		void (*f_act_est_tripulante)(uint32_t, char),
		void (*f_act_pos_tripulante)(uint32_t, uint32_t, uint32_t),
		void (*f_act_instr_tripulante)(uint32_t),
		void (*f_liberar_tripulante)(uint32_t)
		){

	if(mem_reservar > 0) {
		reservar_memoria_principal(mem_reservar);
	}
	inicializacion = f_inicializacion;
	guardar_nueva_patota = f_g_n_patota;
	guardar_nuevo_tripulante = f_g_n_tripulante;
	obtener_patota = f_obt_patota;
	obtener_tripulante = f_obt_tripulante;
	actualizar_estado_tripulante = f_act_est_tripulante;
	actualizar_posicion_tripulante = f_act_pos_tripulante;
	actualizar_instruccion_tripulante = f_act_instr_tripulante;
	liberar_tripulante = f_liberar_tripulante;

	inicializacion();
}

//Primer byte para el segmento
uint32_t obtener_direccion_logica (uint32_t n_segmento, uint32_t desplazamiento) {

	uint32_t nueva_direccion = 0;
	nueva_direccion |= desplazamiento;
	nueva_direccion |= n_segmento << 12;
	return nueva_direccion;
}

uint32_t obtener_n_segmento (uint32_t direccion) {
	return direccion >> 12;
}

uint32_t obtener_desplazamiento (uint32_t direccion) {
	return direccion & 0x0000ffff;
}


void liberar_datos_patota(datos_patota* d_patota) {
	free(d_patota->tareas);
	free(d_patota);
}

void liberar_datos_tripulante(datos_tripulante* d_tripulante) {
	if(d_tripulante->proxInstruccion != NULL) free(d_tripulante->proxInstruccion);
	free(d_tripulante);
}

static void reservar_memoria_principal(uint32_t tam_memoria) {
	memoria_principal = malloc(tam_memoria);
}


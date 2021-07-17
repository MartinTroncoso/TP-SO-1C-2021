/*
 * memoria.c
 *
 *  Created on: 24 may. 2021
 *      Author: utnso
 */

#include "admin_memoria.h"

static void reservar_memoria_principal();
static void def_receptor_sigusr2();

const uint32_t TAMANIO_PCB = 8;
const uint32_t TAMANIO_TCB = 21;

const uint32_t TCB_POS_TID = 0;
const uint32_t TCB_POS_ESTADO = 4;
const uint32_t TCB_POS_POSX = 5;
const uint32_t TCB_POS_POSY = 9;
const uint32_t TCB_POS_PROX_T = 13;
const uint32_t TCB_POS_PUNT_PCB = 17;

void inicializar_administrador(t_log* logger,
		void (*f_inicializacion)(),
		void (*f_g_n_patota)(datos_patota*),
		void (*f_g_n_tripulante)(datos_tripulante*),
		char (*f_obt_est_tripulante)(uint32_t),
		char* (*f_obt_prox_instr_tripulante)(uint32_t),
		void (*f_act_est_tripulante)(uint32_t, char),
		void (*f_act_pos_tripulante)(uint32_t, uint32_t, uint32_t),
		void (*f_act_instr_tripulante)(uint32_t),
		void (*f_generar_dump_memoria)(FILE*),
		void (*f_receptor_sigusr2)(),
		void (*f_liberar_tripulante)(uint32_t)
		){
	logger_admin = logger;
	reservar_memoria_principal();
	mem_inicializacion = f_inicializacion;
	mem_guardar_nueva_patota = f_g_n_patota;
	mem_guardar_nuevo_tripulante = f_g_n_tripulante;
	mem_obtener_estado_tripulante = f_obt_est_tripulante;
	mem_obtener_prox_instruccion_tripulante = f_obt_prox_instr_tripulante;
	mem_actualizar_estado_tripulante = f_act_est_tripulante;
	mem_actualizar_posicion_tripulante = f_act_pos_tripulante;
	mem_actualizar_instruccion_tripulante = f_act_instr_tripulante;
	mem_generar_dump_memoria = f_generar_dump_memoria;
	mem_receptor_sigusr2 = f_receptor_sigusr2 == NULL ? def_receptor_sigusr2 : f_receptor_sigusr2;
	mem_liberar_tripulante = f_liberar_tripulante;

	mem_inicializacion();
}

void finalizar_administrador(){
	free(mem_principal->bloque);
	free(mem_principal);
	//Luego hay que liberar lo demás que utiliza cada administrador en particular...
}

void liberar_datos_patota(datos_patota* d_patota) {
	free(d_patota->tareas);
	free(d_patota);
}

void liberar_datos_tripulante(datos_tripulante* d_tripulante) {
	if(d_tripulante->proxInstruccion != NULL) free(d_tripulante->proxInstruccion);
	free(d_tripulante);
}

void lectura_de_memoria(void* buffer, uint32_t direccion_fisica, uint32_t size) {
	memcpy(buffer, mem_principal->bloque + direccion_fisica, size);
}

void escritura_a_memoria(uint32_t direccion_fisica, uint32_t size, void* buffer) {
	memcpy(mem_principal->bloque + direccion_fisica, buffer, size);
}

static void reservar_memoria_principal() {
	mem_principal = malloc(sizeof(memoria_principal));
	mem_principal->tamanio = TAMANIO_MEMORIA;
	mem_principal->bloque = malloc(TAMANIO_MEMORIA);
}

static void def_receptor_sigusr2() {
	log_info(logger_admin, "No se ha definido comportamiento para SIGUSR2");
}

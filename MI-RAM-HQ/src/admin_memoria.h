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

typedef struct {
	uint32_t tamanio;
	void* bloque;
} memoria_principal;

//CONSTANTES <NO TOCAR>
const uint32_t TAMANIO_PCB;
const uint32_t TAMANIO_TCB;

const uint32_t TCB_POS_TID;
const uint32_t TCB_POS_ESTADO;
const uint32_t TCB_POS_POSX;
const uint32_t TCB_POS_POSY;
const uint32_t TCB_POS_PROX_T;
const uint32_t TCB_POS_PUNT_PCB;

int TAMANIO_MEMORIA;
char* ESQUEMA_MEMORIA;
int TAMANIO_PAGINA;
int TAMANIO_SWAP;
char* PATH_SWAP;
char* ALGORITMO_REEMPLAZO;
char* CRITERIO_SELECCION;

void inicializar_administrador(t_log*,
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
		void (*f_liberar_tripulante)(uint32_t));

void liberar_datos_tripulante(datos_tripulante*);
void liberar_datos_patota(datos_patota*);
void lectura_de_memoria(void* buffer, uint32_t direccion_fisica, uint32_t size);
void escritura_a_memoria(uint32_t direccion_fisica, uint32_t size, void* buffer);
void finalizar_administrador();

void (*mem_inicializacion)();
void (*mem_guardar_nueva_patota)(datos_patota*);
void (*mem_guardar_nuevo_tripulante)(datos_tripulante*);
char (*mem_obtener_estado_tripulante)(uint32_t);
char* (*mem_obtener_prox_instruccion_tripulante)(uint32_t);
void (*mem_actualizar_estado_tripulante)(uint32_t, char);
void (*mem_actualizar_posicion_tripulante)(uint32_t, uint32_t, uint32_t);
void (*mem_actualizar_instruccion_tripulante)(uint32_t);
void (*mem_generar_dump_memoria)(FILE*); //sigusr1
void (*mem_receptor_sigusr2)();
void (*mem_liberar_tripulante)(uint32_t);

memoria_principal* mem_principal; //Aca esta la memoria para aquellos (pag y seg) que la necesiten XD
t_log* logger_admin; //Logger los metodos de administracion

#endif /* ADMIN_MEMORIA_H_ */

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

void inicializar_administrador(uint32_t, t_log*,
		void (*f_inicializacion)(),
		void (*f_g_n_patota)(datos_patota*),
		void (*f_g_n_tripulante)(datos_tripulante*),
		char (*f_obt_est_tripulante)(uint32_t),
		char* (*f_obt_prox_instr_tripulante)(uint32_t),
		void (*f_act_est_tripulante)(uint32_t, char),
		void (*f_act_pos_tripulante)(uint32_t, uint32_t, uint32_t),
		void (*f_act_instr_tripulante)(uint32_t),
		void (*f_generar_dump_memoria)(FILE*),
		void (*f_liberar_tripulante)(uint32_t));

void liberar_datos_tripulante(datos_tripulante*);
void liberar_datos_patota(datos_patota*);
void lectura_de_memoria(void* buffer, uint32_t direccion_fisica, uint32_t size);
void escritura_a_memoria(uint32_t direccion_fisica, uint32_t size, void* buffer);
void finalizar_administrador();

void (*inicializacion)();
void (*guardar_nueva_patota)(datos_patota*);
void (*guardar_nuevo_tripulante)(datos_tripulante*);
char (*obtener_estado_tripulante)(uint32_t);
char* (*obtener_prox_instruccion_tripulante)(uint32_t);
void (*actualizar_estado_tripulante)(uint32_t, char);
void (*actualizar_posicion_tripulante)(uint32_t, uint32_t, uint32_t);
void (*actualizar_instruccion_tripulante)(uint32_t);
void (*generar_dump_memoria)(FILE*);
void (*liberar_tripulante)(uint32_t);

memoria_principal* mem_principal; //Aca esta la memoria para aquellos (pag y seg) que la necesiten XD
t_log* logger_admin; //Logger los metodos de administracion

#endif /* ADMIN_MEMORIA_H_ */

/*
 * memoria_simple.c
 *
 *  Created on: 18 jun. 2021
 *      Author: utnso
 */

#include "memoria_basica.h"

t_list* tripulantes;
t_list* patotas;
pthread_mutex_t mutex_patotas;
pthread_mutex_t mutex_tripulantes;

static bas_patota* buscar_patota(uint32_t);
static bas_tripulante* buscar_tripulante(uint32_t tid);


void bas_inicializacion() {
	patotas = list_create();
	tripulantes = list_create();
	pthread_mutex_init(&mutex_patotas,NULL);
	pthread_mutex_init(&mutex_tripulantes,NULL);
}

void bas_guardar_nueva_patota(datos_patota* d_nueva_patota) {
	bas_patota* nueva_patota = malloc(sizeof(bas_patota));
	nueva_patota->pid = d_nueva_patota->pid;
	nueva_patota->cant_tripulantes = d_nueva_patota->tripulantes;
	nueva_patota->tareas = string_duplicate(d_nueva_patota->tareas);
	pthread_mutex_lock(&mutex_patotas);
	list_add(patotas, nueva_patota);
	pthread_mutex_unlock(&mutex_patotas);
}

void bas_guardar_nuevo_tripulante(datos_tripulante* d_nuevo_tripulante) {

	bas_tripulante* nuevo_tripulante = malloc(sizeof(bas_tripulante));
	nuevo_tripulante->tid = d_nuevo_tripulante->tid;
	nuevo_tripulante->posX = d_nuevo_tripulante->posX;
	nuevo_tripulante->posY = d_nuevo_tripulante->posY;
	nuevo_tripulante->estado = d_nuevo_tripulante->estado;

	bas_patota* patota = buscar_patota(d_nuevo_tripulante->pid);
	nuevo_tripulante->patota = patota;
	nuevo_tripulante->proxInstruccion = patota->tareas;
	pthread_mutex_lock(&mutex_tripulantes);
	list_add(tripulantes, nuevo_tripulante);
	pthread_mutex_unlock(&mutex_tripulantes);
}

datos_patota* bas_obtener_patota(uint32_t pid) {
	datos_patota* d_patota = malloc(sizeof(datos_patota));
	bas_patota* patota = buscar_patota(pid);
	d_patota->pid = patota->pid;
	d_patota->tareas = string_duplicate(patota->tareas);
	d_patota->tripulantes = patota->cant_tripulantes;
	return d_patota;
}

datos_tripulante* bas_obtener_tripulante(uint32_t tid) {
	datos_tripulante* d_tripulante = malloc(sizeof(datos_tripulante));
	bas_tripulante* tripulante = buscar_tripulante(tid);
	char* instruccion_a_enviar;
	d_tripulante->tid = tripulante->tid;
	d_tripulante->estado = tripulante->estado;
	d_tripulante->posX = tripulante->posX;
	d_tripulante->posY = tripulante->posY;
	d_tripulante->pid = tripulante->patota->pid;

	if(tripulante->proxInstruccion != NULL && &(tripulante->proxInstruccion) != 0) {
		char *instrucciones_proximas = tripulante->proxInstruccion;
		//No puedo en enviar toda la cadena, sino hasta encontrar un delimitador
		char* pos_delimitador = strchr(instrucciones_proximas, '|');

		if(pos_delimitador != NULL) {
			int size = pos_delimitador - instrucciones_proximas;
			instruccion_a_enviar = string_substring(instrucciones_proximas, 0, size);
		}
		else {
			instruccion_a_enviar = string_duplicate(instrucciones_proximas);
		}
		d_tripulante->proxInstruccion = instruccion_a_enviar;
	}
	else
		printf("OJOOO, hay alguna operacion no valida");
	return d_tripulante;
}

void bas_actualizar_estado_tripulante(uint32_t tid, char n_estado) {
	bas_tripulante* tripulante = buscar_tripulante(tid);
	tripulante->estado = n_estado;
}

void bas_actualizar_posicion_tripulante(uint32_t tid, uint32_t posX, uint32_t posY) {
	bas_tripulante* tripulante = buscar_tripulante(tid);
	tripulante->posX = posX;
	tripulante->posY = posY;
}

void bas_actualizar_instruccion_tripulante(uint32_t tid) {
	bas_tripulante* tripulante = buscar_tripulante(tid);
	char* instruccion_actual = tripulante->proxInstruccion;
	char* pos_prox_delimitador = strchr(instruccion_actual, '|');

	if(&(tripulante->proxInstruccion) == 0) {
		printf("OJOOOO, ya estaba en el finallll\n");
		return;
	}

	if(pos_prox_delimitador != NULL) {
		tripulante->proxInstruccion = pos_prox_delimitador + 1;
	}
	else {
		// Voy al final de la cadena
		tripulante->proxInstruccion = instruccion_actual + strlen(instruccion_actual);
	}
}

void bas_liberar_tripulante(uint32_t tid) {
	bas_tripulante* trip_a_eliminar = buscar_tripulante(tid);
	bool es_el_tripulante(void* trip) {
		return trip == trip_a_eliminar;
	}
	pthread_mutex_lock(&mutex_tripulantes);
	list_remove_by_condition(tripulantes, es_el_tripulante);
	pthread_mutex_unlock(&mutex_tripulantes);
	free(trip_a_eliminar);
}


static bas_patota* buscar_patota(uint32_t pid) {
	bool patota_tiene_el_pid(void* patota) {
		return ((bas_patota*) patota)->pid == pid;
	}
	pthread_mutex_lock(&mutex_patotas);
	bas_patota* patota_enc = list_find(patotas, patota_tiene_el_pid);
	pthread_mutex_unlock(&mutex_patotas);
	return patota_enc;
}

static bas_tripulante* buscar_tripulante(uint32_t tid) {
	bool tripulante_tiene_el_tid(void* tripulante) {
		return ((bas_tripulante*) tripulante)->tid == tid;
	}
	pthread_mutex_lock(&mutex_tripulantes);
	bas_tripulante* tripulante_enc = list_find(tripulantes, tripulante_tiene_el_tid);
	pthread_mutex_unlock(&mutex_tripulantes);
	return tripulante_enc;
}

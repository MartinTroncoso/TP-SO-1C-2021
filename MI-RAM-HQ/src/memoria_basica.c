/*
 * memoria_simple.c
 *
 *  Created on: 18 jun. 2021
 *      Author: utnso
 */

#include "memoria_basica.h"

static t_list* tripulantes;
static t_list* patotas;
static pthread_mutex_t mutex_lista_patotas;
static pthread_mutex_t mutex_lista_tripulantes;

static bas_patota* buscar_patota(uint32_t);
static bas_tripulante* buscar_tripulante(uint32_t tid);
static void _bas_liberar_patota(bas_patota* patota);

void bas_inicializacion() {
	patotas = list_create();
	tripulantes = list_create();
	pthread_mutex_init(&mutex_lista_patotas,NULL);
	pthread_mutex_init(&mutex_lista_tripulantes,NULL);
	log_info(logger_admin, "Se inicia la administracion de memoria: BASICA");
}

void bas_guardar_nueva_patota(datos_patota* d_nueva_patota) {
	bas_patota* nueva_patota = malloc(sizeof(bas_patota));
	nueva_patota->pid = d_nueva_patota->pid;
	nueva_patota->cant_tripulantes = d_nueva_patota->tripulantes;
	nueva_patota->tareas = string_duplicate(d_nueva_patota->tareas);
	nueva_patota->tripulantes_activos = nueva_patota->cant_tripulantes;
	pthread_mutex_init(&(nueva_patota->mutex_patota), NULL);
	pthread_mutex_lock(&mutex_lista_patotas);
	list_add(patotas, nueva_patota);
	pthread_mutex_unlock(&mutex_lista_patotas);
	log_info(logger_admin, "Se guarda la patota %d", d_nueva_patota->pid);
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
	pthread_mutex_lock(&mutex_lista_tripulantes);
	list_add(tripulantes, nuevo_tripulante);
	pthread_mutex_unlock(&mutex_lista_tripulantes);
	log_info(logger_admin, "Se guarda la patota %d", d_nuevo_tripulante->tid);
}

/*datos_patota* bas_obtener_patota(uint32_t pid) {
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
}*/

char bas_obtener_estado_tripulante(uint32_t tid) {
	bas_tripulante* tripulante = buscar_tripulante(tid);
	log_info(logger_admin, "Se obtiene de memoria el estado (%c) del tripulante %d", tripulante->estado, tid);
	return tripulante->estado;
}

char* bas_obtener_prox_instruccion_tripulante(uint32_t tid) {
	bas_tripulante* tripulante = buscar_tripulante(tid);
	char* instruccion_a_enviar;
	if(tripulante->proxInstruccion != NULL && *(tripulante->proxInstruccion) != 0) {
		instruccion_a_enviar = string_substring_until_char(tripulante->proxInstruccion, '|');
	}
	else {
		log_error(logger_admin, "Ocurrio un error al intentar devolver la prox instruccion del tripulante %d", tid);
	}
	log_info(logger_admin, "Se obtiene de memoria la prox instruccion (%s) del tripulante %d", instruccion_a_enviar, tid);
	return instruccion_a_enviar;
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

	if(*instruccion_actual == 0) {
		log_error(logger_admin, "Ocurrio un error al intentar avanzar la instruccion del tripulante %d, ya habia llegado al final", tid);
		return;
	}

	char* pos_prox_delimitador = strchr(instruccion_actual, '|');

	if(pos_prox_delimitador != NULL) {
		tripulante->proxInstruccion = pos_prox_delimitador + 1;
	}
	else {
		// Voy al final de la cadena
		tripulante->proxInstruccion = instruccion_actual + strlen(instruccion_actual);
	}
	log_info(logger_admin, "Se actualizo la prox instruccion del tripulante %d", tid);
}

void bas_liberar_tripulante(uint32_t tid) {
	bas_tripulante* trip_a_eliminar = buscar_tripulante(tid);
	bas_patota* patota = trip_a_eliminar->patota;
	uint32_t trip_restantes_en_patota;

	bool es_el_tripulante(void* trip) {
		return trip == trip_a_eliminar;
	}
	//Quito de la lista al tripulante:
	pthread_mutex_lock(&mutex_lista_tripulantes);
	list_remove_by_condition(tripulantes, es_el_tripulante);
	pthread_mutex_unlock(&mutex_lista_tripulantes);

	//Disminuyo tripulantes activos de la patota:
	pthread_mutex_lock(&(patota->mutex_patota));
	trip_restantes_en_patota = --(patota->tripulantes_activos);
	pthread_mutex_unlock(&(patota->mutex_patota));

	//Se libera memoria al tripulante
	free(trip_a_eliminar);
	log_info(logger_admin, "Se libero de memoria al tripulante %d", tid);

	if(trip_restantes_en_patota == 0) {
		_bas_liberar_patota(trip_a_eliminar->patota);
	}
	else if(trip_restantes_en_patota < 0) {
		log_error(logger_admin, "Ocurrio un error, supuestamente hay %d trip en la patota %d", trip_restantes_en_patota, patota->pid);
	}
}

void bas_generar_dump_memoria(FILE* archivo) {

	uint32_t cant_patotas, cant_tripulantes;
	cant_patotas = list_size(patotas);
	cant_tripulantes = list_size(tripulantes);

	char* detalle_patotas = string_from_format("Patotas activas: %d\n", cant_patotas);
	char* detalle_tripulantes = string_from_format("Tripulantes activos: %d\n", cant_tripulantes);

	txt_write_in_file(archivo, detalle_patotas);
	txt_write_in_file(archivo, detalle_tripulantes);

	void escritura_por_patota(void* _patota) {
		bas_patota* patota = (bas_patota*) _patota;
		pthread_mutex_lock(&(patota->mutex_patota));
		char* linea = string_from_format("Patota: %d\tTripulantes activos:%d\n", patota->pid, patota->tripulantes_activos);
		txt_write_in_file(archivo, linea);
		pthread_mutex_unlock(&(patota->mutex_patota));
		free(linea);
	}
	pthread_mutex_lock(&mutex_lista_patotas);
	list_iterate(patotas, escritura_por_patota);
	pthread_mutex_unlock(&mutex_lista_patotas);
	free(detalle_patotas);
	free(detalle_tripulantes);
}


void _bas_liberar_patota(bas_patota* patota) {
	uint32_t pid = patota->pid;
	bool es_la_patota(void* p) {
		return p == patota;
	}
	//Quito de la lista
	pthread_mutex_lock(&mutex_lista_patotas);
	list_remove_by_condition(patotas, es_la_patota);
	pthread_mutex_unlock(&mutex_lista_patotas);
	//Libero todoo de la patota
	pthread_mutex_destroy(&(patota->mutex_patota));
	free(patota->tareas);
	free(patota);
	log_info(logger_admin, "Se libero de memoria a la patota %d", pid);
}


static bas_patota* buscar_patota(uint32_t pid) {
	bool patota_tiene_el_pid(void* patota) {
		return ((bas_patota*) patota)->pid == pid;
	}
	pthread_mutex_lock(&mutex_lista_patotas);
	bas_patota* patota_enc = list_find(patotas, patota_tiene_el_pid);
	pthread_mutex_unlock(&mutex_lista_patotas);
	if(patota_enc == NULL) {
		log_error(logger_admin, "No se encuentra la patota %d", pid);
	}
	return patota_enc;
}

static bas_tripulante* buscar_tripulante(uint32_t tid) {
	bool tripulante_tiene_el_tid(void* tripulante) {
		return ((bas_tripulante*) tripulante)->tid == tid;
	}
	pthread_mutex_lock(&mutex_lista_tripulantes);
	bas_tripulante* tripulante_enc = list_find(tripulantes, tripulante_tiene_el_tid);
	pthread_mutex_unlock(&mutex_lista_tripulantes);
	if(tripulante_enc == NULL) {
		log_error(logger_admin, "No se encuentra el tripulante %d", tid);
	}
	return tripulante_enc;
}

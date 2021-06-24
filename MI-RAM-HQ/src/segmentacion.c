/*
 * segmentacion.c
 *
 *  Created on: 14 may. 2021
 *      Author: utnso
 */

#include "segmentacion.h"

typedef struct {
	int n_segmento;
	seg_tabla* tabla_pid;
} result_busqueda;

//CONSTANTES <NO TOCAR>
const uint32_t TAMANIO_PCB = 8;
const uint32_t TAMANIO_TCB = 21;

const uint32_t TCB_POS_TID = 0;
const uint32_t TCB_POS_ESTADO = 4;
const uint32_t TCB_POS_POSX = 5;
const uint32_t TCB_POS_POSY = 9;
const uint32_t TCB_POS_PROX_T = 13;
const uint32_t TCB_POS_PUNT_PCB = 17;

//GLOBALES IMPORTANTE!!!
t_dictionary* dic_tid_tabla; //Sin este diccionario, para buscar la tabla, habría que buscar en cada tabla y recorrer cada registro...
t_list* tablas_segmentacion; //Aca van las tablas de segmentos
t_list* segmentos_ordenados; //Aca van los segmentos ocupados en memoria, ordenados por su posicion de memoria (inicio)

static bool segmento_esta_antes(void*, void*);
static seg_tabla* buscar_tabla(uint32_t);
static int buscar_tripulante_no_inicializado(seg_tabla*);
static seg_tabla_registro* obtener_registro_de_tabla(seg_tabla*, int);
static result_busqueda* buscar_tripulante(uint32_t tid);

static segmento* buscar_reservar_memoria_first_fit(uint32_t tamanio);
static segmento* reservar_memoria(uint32_t posicion, uint32_t tamanio);

static uint32_t obtener_direccion_logica (uint32_t, uint32_t);
static uint32_t obtener_n_segmento (uint32_t);
static uint32_t obtener_desplazamiento (uint32_t);
static uint32_t segmentacion_mmu(uint32_t pid, uint32_t direccion_logica);
static uint32_t segmentacion_mmu_mult(uint32_t pid, uint32_t direccion_logica, uint32_t size);

void seg_inicializacion() {
	tablas_segmentacion = list_create();
	segmentos_ordenados = list_create();
	dic_tid_tabla = dictionary_create();
	log_info(logger_admin, "Se inicia la administracion de memoria: SEGMENTACION");
}

void seg_guardar_nueva_patota(datos_patota* nueva_patota) {

	log_info(logger_admin, "[Almacenamiento patota %d] Se comienza la operacion", nueva_patota->pid);

	uint32_t cantidad_tripulantes = nueva_patota->tripulantes;
	uint32_t tamanio_tareas = strlen(nueva_patota->tareas) + 1;

	//RESERVO MEMORIA PRIMERO:

	segmento* segmento_pcb = buscar_reservar_memoria_first_fit(TAMANIO_PCB);
	t_list* segmentos_tcb = list_create();
	for(int i = 0; i < cantidad_tripulantes; i++) {
		list_add(segmentos_tcb, buscar_reservar_memoria_first_fit(TAMANIO_TCB));
	}
	segmento* segmento_tareas = buscar_reservar_memoria_first_fit(tamanio_tareas);

	log_info(logger_admin, "[Almacenamiento patota %d] Se reservo la memoria de todos los segmentos", nueva_patota->pid);

	// ARMO LA TABLA DE SEGMENTOS

	seg_tabla* nueva_tabla = malloc(sizeof(seg_tabla));
	nueva_tabla->c_tripulantes = nueva_patota->tripulantes;
	nueva_tabla->registros = list_create();
	nueva_tabla->pid = nueva_patota->pid;
	list_add(tablas_segmentacion, nueva_tabla);

	seg_tabla_registro* registro_pcb = malloc(sizeof(seg_tabla_registro));
	registro_pcb->inicializado = true;
	registro_pcb->segmento = segmento_pcb;
	list_add(nueva_tabla->registros, registro_pcb);

	for(int i = 0; i < cantidad_tripulantes; i++) {
		seg_tabla_registro* registro_tcb = malloc(sizeof(seg_tabla_registro));
		registro_tcb->inicializado = false;
		registro_tcb->segmento = (segmento*) list_get(segmentos_tcb, i);
		list_add(nueva_tabla->registros, registro_tcb);
	}

	seg_tabla_registro* registro_tareas = malloc(sizeof(seg_tabla_registro));
	registro_tareas->inicializado = true;
	registro_tareas->segmento = segmento_tareas;
	list_add(nueva_tabla->registros, registro_tareas);

	log_info(logger_admin, "[Almacenamiento patota %d] Se armo la tabla de segmentos", nueva_patota->pid);

	//CARGO PRIMER REGISTRO DE TABLA = PCB, Y EL ULTIMO: TAREAS

	void* buffer = malloc(TAMANIO_PCB);
	uint32_t desp_buffer = 0;
	uint32_t dir_logica_pcb = obtener_direccion_logica(0, 0); //Segmento 0, al inicio
	uint32_t dir_logica_tareas = obtener_direccion_logica(cantidad_tripulantes + 1, 0);
	uint32_t dir_fisica;

	memcpy(buffer + desp_buffer, &(nueva_patota->pid), sizeof(uint32_t));
	desp_buffer += sizeof(uint32_t);
	memcpy(buffer + desp_buffer, &dir_logica_tareas ,sizeof(uint32_t));

	dir_fisica = segmentacion_mmu(nueva_patota->pid, dir_logica_pcb);
	escritura_a_memoria(dir_fisica, TAMANIO_PCB, buffer);

	buffer = realloc(buffer, tamanio_tareas);
	memcpy(buffer, nueva_patota->tareas, tamanio_tareas);

	dir_fisica = segmentacion_mmu(nueva_patota->pid, dir_logica_tareas);
	escritura_a_memoria(dir_fisica, tamanio_tareas, buffer);

	log_info(logger_admin, "[Almacenamiento patota %d] Se cargo los segmentos PCB y Tareas", nueva_patota->pid);

	//LIBERO

	free(buffer);
	list_destroy(segmentos_tcb);

	log_info(logger_admin, "[Almacenamiento patota %d] Se finalizo la operacion", nueva_patota->pid);
}

void seg_guardar_nuevo_tripulante(datos_tripulante* nuevo_tripulante) {

	log_info(logger_admin, "[Almacenamiento tripulante %d] Se comienza la operacion", nuevo_tripulante->tid);

	seg_tabla* tabla_pid = buscar_tabla(nuevo_tripulante->pid);
	int n_registro = buscar_tripulante_no_inicializado(tabla_pid);
	uint32_t dir_logica_pcb = obtener_direccion_logica(0, 0);
	uint32_t dir_logica_tcb = obtener_direccion_logica(n_registro, 0);
	uint32_t dir_logica_tareas = obtener_direccion_logica(tabla_pid->c_tripulantes + 1, 0);

	//ARMO BUFFER

	void* buffer = malloc(TAMANIO_TCB);
	uint32_t desp_buffer = 0;
	memcpy(buffer + desp_buffer, &(nuevo_tripulante->tid), sizeof(uint32_t));
	desp_buffer += sizeof(uint32_t);
	memcpy(buffer + desp_buffer, &(nuevo_tripulante->estado), 1);
	desp_buffer += 1;
	memcpy(buffer + desp_buffer, &(nuevo_tripulante->posX), sizeof(uint32_t));
	desp_buffer += sizeof(uint32_t);
	memcpy(buffer + desp_buffer, &(nuevo_tripulante->posY), sizeof(uint32_t));
	desp_buffer += sizeof(uint32_t);
	memcpy(buffer + desp_buffer, &dir_logica_tareas, sizeof(uint32_t));
	desp_buffer += sizeof(uint32_t);
	memcpy(buffer + desp_buffer, &dir_logica_pcb, sizeof(uint32_t));

	log_info(logger_admin, "[Almacenamiento tripulante %d] Se armo buffer para escribir en memoria", nuevo_tripulante->tid);

	//BUSCO DIR FISICA Y ESCRIBO EN MEMORIA

	uint32_t dir_fisica = segmentacion_mmu(nuevo_tripulante->pid, dir_logica_tcb);
	escritura_a_memoria(dir_fisica, TAMANIO_TCB, buffer);

	log_info(logger_admin, "[Almacenamiento tripulante %d] Se guardo en memoria el TCB", nuevo_tripulante->tid);

	//MARCO SEGMENTO COMO INICIALIZADO

	obtener_registro_de_tabla(tabla_pid, n_registro)->inicializado = true;

	char* tid_string = string_itoa(nuevo_tripulante->tid);
	dictionary_put(dic_tid_tabla, tid_string, (void*)tabla_pid);

	free(tid_string);
	free(buffer);

	log_info(logger_admin, "[Almacenamiento tripulante %d] Se finalizo la operacion", nuevo_tripulante->tid);
}

char seg_obtener_estado_tripulante(uint32_t tid) {
	log_info(logger_admin, "[Obtención estado trip %d] Se inicia la operacion", tid);
	result_busqueda* busqueda = buscar_tripulante(tid);
	uint32_t dir_logica = obtener_direccion_logica(busqueda->n_segmento, TCB_POS_ESTADO);
	uint32_t dir_fisica = segmentacion_mmu(busqueda->tabla_pid->pid, dir_logica);
	char estado;
	lectura_de_memoria(&estado, dir_fisica, sizeof(char));
	free(busqueda);
	log_info(logger_admin, "[Obtención estado trip %d] Se finaliza la operacion - estado actual: %c", tid, estado);
	return estado;
}
char* seg_obtener_prox_instruccion_tripulante(uint32_t tid) {

	log_info(logger_admin, "[Obtención prox inst trip %d] Se inicia la operacion", tid);
	// Obtengo la dirección logica, donde esta la prox tarea a ejecutar
	result_busqueda* busqueda = buscar_tripulante(tid);
	uint32_t dir_logica_pos_prox_tarea = obtener_direccion_logica(busqueda->n_segmento, TCB_POS_PROX_T);
	uint32_t dir_fisica_pos_prox_tarea = segmentacion_mmu_mult(busqueda->tabla_pid->pid, dir_logica_pos_prox_tarea, sizeof(uint32_t));
	uint32_t dir_logica_prox_tarea;
	lectura_de_memoria(&dir_logica_prox_tarea, dir_fisica_pos_prox_tarea, sizeof(uint32_t));

	// Obtengo todas las tareas restantes

	int n_segmento_tareas = obtener_n_segmento(dir_logica_prox_tarea);
	uint32_t desplazamiento_tareas = obtener_desplazamiento(dir_logica_prox_tarea);
	uint32_t tam_seg_tareas = obtener_registro_de_tabla(busqueda->tabla_pid, n_segmento_tareas)->segmento->tamanio;
	uint32_t cant_caracteres_restantes = tam_seg_tareas - desplazamiento_tareas;
	char* tareas_restantes = malloc(cant_caracteres_restantes);
	uint32_t dir_fisica_prox_tarea = segmentacion_mmu_mult(busqueda->tabla_pid->pid, dir_logica_prox_tarea, cant_caracteres_restantes);
	lectura_de_memoria(tareas_restantes, dir_fisica_prox_tarea, cant_caracteres_restantes);

	// Envio solo la primera, hasta llegar a un '|' o '\0'

	char* prox_tarea = string_substring_until_char(tareas_restantes, '|');

	free(tareas_restantes);
	free(busqueda);
	log_info(logger_admin, "[Obtención prox inst trip %d] Se finaliza la operacion - prox instr: %s", tid, prox_tarea);
	return prox_tarea;
}

void seg_actualizar_estado_tripulante(uint32_t tid, char nuevo_estado) {
	result_busqueda* busqueda = buscar_tripulante(tid);
	uint32_t dir_logica = obtener_direccion_logica(busqueda->n_segmento, TCB_POS_ESTADO);
	uint32_t dir_fisica = segmentacion_mmu(busqueda->tabla_pid->pid, dir_logica);
	escritura_a_memoria(dir_fisica, sizeof(char), &nuevo_estado);
	free(busqueda);
}

void seg_actualizar_posicion_tripulante(uint32_t tid, uint32_t posX, uint32_t posY) {
	result_busqueda* busqueda = buscar_tripulante(tid);
	uint32_t dir_logica = obtener_direccion_logica(busqueda->n_segmento, TCB_POS_POSX);
	uint32_t dir_fisica = segmentacion_mmu_mult(busqueda->tabla_pid->pid, dir_logica, sizeof(uint32_t));
	escritura_a_memoria(dir_fisica, sizeof(uint32_t), &posX);
	dir_logica = obtener_direccion_logica(busqueda->n_segmento, TCB_POS_POSY);
	dir_fisica = segmentacion_mmu_mult(busqueda->tabla_pid->pid, dir_logica, sizeof(uint32_t));
	escritura_a_memoria(dir_fisica, sizeof(uint32_t), &posY);
	free(busqueda);
}

void seg_actualizar_instruccion_tripulante(uint32_t tid) {

	log_info(logger_admin, "[Actualizacion prox inst trip %d] Se inicia la operacion", tid);

	// Obtengo la dirección logica, donde esta la prox tarea a ejecutar
	result_busqueda* busqueda = buscar_tripulante(tid);
	uint32_t dir_logica_pos_prox_tarea = obtener_direccion_logica(busqueda->n_segmento, TCB_POS_PROX_T);
	uint32_t dir_fisica_pos_prox_tarea = segmentacion_mmu_mult(busqueda->tabla_pid->pid, dir_logica_pos_prox_tarea, sizeof(uint32_t));
	uint32_t dir_logica_prox_tarea;
	lectura_de_memoria(&dir_logica_prox_tarea, dir_fisica_pos_prox_tarea, sizeof(uint32_t));

	// Obtengo todas las tareas restantes

	int n_segmento_tareas = obtener_n_segmento(dir_logica_prox_tarea);
	uint32_t desplazamiento_tareas = obtener_desplazamiento(dir_logica_prox_tarea);
	uint32_t tam_seg_tareas = obtener_registro_de_tabla(busqueda->tabla_pid, n_segmento_tareas)->segmento->tamanio;
	uint32_t cant_caracteres_restantes = tam_seg_tareas - desplazamiento_tareas;
	char* tareas_restantes = malloc(cant_caracteres_restantes);
	uint32_t dir_fisica_prox_tarea = segmentacion_mmu_mult(busqueda->tabla_pid->pid, dir_logica_prox_tarea, cant_caracteres_restantes);
	lectura_de_memoria(tareas_restantes, dir_fisica_prox_tarea, cant_caracteres_restantes);
	log_info(logger_admin, "[Actualizacion prox inst trip %d] Tareas restantes: %s", tid, tareas_restantes);

	// Calculo cant posiciones a avanzar

	if(*tareas_restantes == 0) {
		log_error(logger_admin, "Ocurrio un error al intentar avanzar la instruccion del tripulante %d, ya habia llegado al final", tid);
		return;
	}

	uint32_t avance_posiciones;
	char* pos_prox_delimitador = strchr(tareas_restantes, '|');

	if(pos_prox_delimitador != NULL) {
		avance_posiciones = (uint32_t)(pos_prox_delimitador - tareas_restantes) + 1;
		log_info(logger_admin, "[Actualizacion prox inst trip %d] Se avanza hacia la sig tarea", tid);
	}
	else {
		// Voy al final de la cadena
		avance_posiciones = strlen(tareas_restantes);
		log_info(logger_admin, "[Actualizacion prox inst trip %d] Se avanza hacia el final", tid);
	}

	// Modifico el valor de prox_tarea (PCB)

	uint32_t nueva_prox_tarea = dir_logica_prox_tarea + avance_posiciones;
	log_info(logger_admin, "[Actualizacion prox inst trip %d] Se avanza: dir_log_ant(%#010X) - dir_log_nueva(%#010X)", tid, dir_logica_prox_tarea, nueva_prox_tarea);
	escritura_a_memoria(dir_fisica_pos_prox_tarea, sizeof(uint32_t), &nueva_prox_tarea);
	free(busqueda);
}

void seg_liberar_tripulante(uint32_t tid) {
	result_busqueda* busqueda = buscar_tripulante(tid);
	char* tid_string = string_itoa(busqueda->n_segmento);

	//LIBERO MEMORIA

	seg_tabla_registro* registro_a_liberar = list_remove(busqueda->tabla_pid->registros, busqueda->n_segmento); //Saco registro de la tabla

	bool es_el_segmento_trip(void* segmento) {
		return segmento == registro_a_liberar->segmento;
	}
	list_remove_by_condition(segmentos_ordenados, es_el_segmento_trip); //Saco de la lista de segmentos_ordenados, ya se puede ocupar por otro
	dictionary_remove(dic_tid_tabla, tid_string); //Saco key-value del diccionario
	free(registro_a_liberar->segmento); //Libero segmento del registro
	free(registro_a_liberar); //Libero registro
	free(busqueda);
	free(tid_string);
}


//Devuelve el segmento donde se reservo la informacion
//Devuelve null si no se encontro un lugar disponible
static segmento* buscar_reservar_memoria_first_fit(uint32_t tamanio) {

	log_info(logger_admin, "[Reserva mem FIRST FIT] Pedido %d bytes - Se comienza operacion", tamanio);

	uint32_t pos_candidata = 0;

	int cant_segmentos = list_size(segmentos_ordenados);

	//Ver si entraria al principio...
	if(cant_segmentos == 0) {
		log_info(logger_admin, "[Reserva mem FIRST FIT] Pedido %d bytes - No hay segmentos, va al inicio", tamanio);
		return reservar_memoria(pos_candidata, tamanio);
	}

	for(int i = 0; i < cant_segmentos; i++) {
		segmento* segmento_ya_reservado = (segmento*) list_get(segmentos_ordenados, i);
		if(pos_candidata + tamanio <= segmento_ya_reservado->inicio) {
			log_info(logger_admin, "[Reserva mem FIRST FIT] Pedido %d bytes - Se encontro un espacio disponible", tamanio);
			return reservar_memoria(pos_candidata, tamanio);
		}
		pos_candidata = segmento_ya_reservado->inicio + segmento_ya_reservado->tamanio;
		log_info(logger_admin, "[Reserva mem FIRST FIT] Pedido %d bytes - Busco el prox segmento", tamanio);
	}

	//Ver si entra al final...
	if(pos_candidata + tamanio <= mem_principal->tamanio) {
		log_info(logger_admin, "[Reserva mem FIRST FIT] Pedido %d bytes - Se encontro un espacio luego del ultimo segmento", tamanio);
		return reservar_memoria(pos_candidata, tamanio);
	}

	log_error(logger_admin, "[Reserva mem FIRST FIT] Pedido %d bytes - Ocurrio un error, no hay memoria", tamanio);
	return NULL;
}

static segmento* reservar_memoria(uint32_t posicion, uint32_t tamanio) {

	segmento* segmento_reservado = malloc(sizeof(segmento));
	segmento_reservado->inicio = posicion;
	segmento_reservado->tamanio = tamanio;
	list_add_sorted(segmentos_ordenados, segmento_reservado, segmento_esta_antes);
	log_info(logger_admin, "[Reserva mem] - Se reserva memoria: [%d-%d] ", posicion, posicion + tamanio - 1);
	log_info(logger_admin, "[Reserva mem] - Ahora hay: %d segmentos", list_size(segmentos_ordenados));
	return segmento_reservado;
}

//Devuelve la direccion fisica
//Si falla, devuelve UINT32_MAX
static uint32_t segmentacion_mmu(uint32_t pid, uint32_t direccion_logica) {
	return segmentacion_mmu_mult(pid, direccion_logica, 1);
}

//Devuelve la direccion fisica
//Si falla, devuelve UINT32_MAX
static uint32_t segmentacion_mmu_mult(uint32_t pid, uint32_t direccion_logica, uint32_t size) {
	seg_tabla* tabla = buscar_tabla(pid);
	uint32_t n_segmento = obtener_n_segmento(direccion_logica);
	uint32_t desplazamiento = obtener_desplazamiento(direccion_logica);
	segmento* segmento = obtener_registro_de_tabla(tabla, n_segmento)->segmento;
	//Chequeo que el inicio no supere el tam del segmento
	if(desplazamiento > segmento->tamanio - 1) {
		log_error(logger_admin, "[MMU] Error al intentar traducir la dir_log(%#010X):"
				" pid(%d) / desp(%d) / tam_seg(%d)", direccion_logica, pid, desplazamiento, segmento->tamanio);
		return UINT32_MAX;
	}
	//Chequeo que el fin no supere el tam del segmento
	if(size > 1 && desplazamiento + (size - 1) > segmento->tamanio - 1) {
		log_error(logger_admin, "[MMU] Error al intentar traducir la dir_log(%#010X) en el final:"
				" pid(%d) / desp(%d)-size(%d) / tam_seg(%d)", direccion_logica, pid, desplazamiento, size, segmento->tamanio);
		return UINT32_MAX;
	}
	log_info(logger_admin, "[MMU] - Se tradujo correctamente la dir_log(%#010X)/pid(%u) a dir_fis(%u)", direccion_logica, pid, segmento->inicio + desplazamiento);
	return segmento->inicio + desplazamiento;
}

//Primer byte para el segmento
static uint32_t obtener_direccion_logica (uint32_t n_segmento, uint32_t desplazamiento) {

	uint32_t nueva_direccion = 0;
	nueva_direccion |= desplazamiento;
	nueva_direccion |= n_segmento << 16;
	return nueva_direccion;
}

static uint32_t obtener_n_segmento (uint32_t direccion) {
	return direccion >> 16;
}

static uint32_t obtener_desplazamiento (uint32_t direccion) {
	return direccion & 0x0000ffff;
}

static bool segmento_esta_antes(void* segmento1, void* segmento2) {
	return ((segmento*) segmento1)->inicio < ((segmento*) segmento2)->inicio;
}

static seg_tabla* buscar_tabla(uint32_t pid) {
	bool tabla_es_del_pid(void* tabla) {
		return ((seg_tabla*) tabla)->pid == pid;
	}
	return (seg_tabla*) list_find(tablas_segmentacion, tabla_es_del_pid);
}

static seg_tabla_registro* obtener_registro_de_tabla(seg_tabla* tabla, int n_segmento) {
	return (seg_tabla_registro*)list_get(tabla->registros, n_segmento);
}

// Devuelve el numero de segmento donde esta el tripulante
static int buscar_tripulante_no_inicializado(seg_tabla* tabla) {
	//En la tabla, los registros:
	// 0 -> PCB
	// 1 .. c_tripulantes -> TCB's
	// c_tripulantes+1 -> tareas
	for(int i = 1; i <= tabla->c_tripulantes; i++) {
		seg_tabla_registro* registro_actual = obtener_registro_de_tabla(tabla, i);
		if(!registro_actual->inicializado) {
			return i;
		}
	}
	return -1; // En caso de error
}

//Devuelve el numero de segmento donde esta el tripulante
static result_busqueda* buscar_tripulante(uint32_t tid) {

	result_busqueda* resultado = malloc(sizeof(result_busqueda));
	int n;
	uint32_t dir_logica;
	uint32_t dir_fisica;
	char* tid_string = string_itoa(tid);
	seg_tabla* tabla_pid = (seg_tabla*) dictionary_get(dic_tid_tabla, tid_string);
	uint32_t* buffer_tid = malloc(sizeof(uint32_t));

	//Recorro registros tripulantes de la tabla..
	for(n = 1; n <= tabla_pid->c_tripulantes; n++) {
		seg_tabla_registro* registro_trip = obtener_registro_de_tabla(tabla_pid, n);
		if(registro_trip->inicializado) {  //Espero que no puedan expulsar si no lo inicializaron!
			dir_logica = obtener_direccion_logica(n, 0); //El TID esta en el inicio...
			dir_fisica = segmentacion_mmu(tabla_pid->pid, dir_logica);
			lectura_de_memoria(buffer_tid, dir_fisica, sizeof(uint32_t));
			if(tid == *buffer_tid)
				break;
		}
	}
	free(tid_string);
	free(buffer_tid);
	if(n > tabla_pid->c_tripulantes) {
		resultado->n_segmento = -1;
		resultado->tabla_pid = tabla_pid;
	}
	resultado->n_segmento = n;
	resultado->tabla_pid = tabla_pid;
	return resultado;
}

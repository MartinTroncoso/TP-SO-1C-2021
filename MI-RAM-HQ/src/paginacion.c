/*
 * paginacion.c
 *
 *  Created on: 14 jul. 2021
 *      Author: utnso
 */

#include "paginacion.h"

typedef struct {
	int index_tripulante; //index_tripulante es el orden en el cual estan los tcb =/= de tid
	pag_tabla* tabla_pid;
} result_busqueda;

typedef struct {
	bool utilizado;
	uint32_t pid;
	uint32_t n_pagina;
} pagina_swap;

typedef enum {
	MARCO = 1,
	SWAP = 2,
	FALLA = 3
} tipo_memoria;

typedef struct {
	tipo_memoria tipo;
	int index;
} respuesta_solicitud_memoria;

//Para SWAP:
void* memoria_swap;
pagina_swap* paginas_swap;
uint32_t cantidad_paginas_swap;
uint32_t cantidad_paginas_swap_libres;

//Para memoria fisica
marco* marcos_array;
uint32_t cantidad_marcos;
uint32_t cantidad_marcos_libres;
t_list* cola_marcos_usados;

t_list* tablas_paginacion;
static t_dictionary* dic_tid_tabla;

static void pag_escritura_a_memoria(uint32_t pid, uint32_t direccion_logica, uint32_t size, void* buffer);
static void pag_lectura_de_memoria(uint32_t pid, uint32_t direccion_logica, uint32_t size, void* buffer);
static void _pag_liberar_patota(pag_tabla* tabla_a_eliminar);
static uint32_t buscar_index_swap(uint32_t pid, uint32_t n_pagina);

static uint32_t segmentacion_mmu(uint32_t pid, uint32_t direccion_logica);
static respuesta_solicitud_memoria solicitar_memoria_disponible();

static pag_tabla* buscar_tabla(uint32_t pid);
static result_busqueda* buscar_tripulante(uint32_t tid);
static void liberar_pagina(pagina* pag_a_liberar, pag_tabla* tabla);

static uint32_t obtener_n_pagina(uint32_t direccion_logica);
static uint32_t obtener_offset(uint32_t direccion_logica);
static uint32_t min_number(uint32_t number1, uint32_t number2);

static uint32_t eleccion_marco_victima();


void pag_inicializacion() {
	cantidad_marcos = TAMANIO_MEMORIA / TAMANIO_PAGINA;
	cantidad_paginas_swap = TAMANIO_SWAP / TAMANIO_PAGINA;

	cantidad_marcos_libres = cantidad_marcos;
	cantidad_paginas_swap_libres = cantidad_paginas_swap;

	marcos_array = malloc(sizeof(marco) * cantidad_marcos);
	paginas_swap = malloc(sizeof(pagina_swap) * cantidad_paginas_swap);

	cola_marcos_usados = list_create();

	for(int i = 0; i < cantidad_marcos; i++) {
		(marcos_array[i]).libre = true;
	}
	tablas_paginacion = list_create();
	dic_tid_tabla = dictionary_create();

	// MEMORIA VIRTUAL
    int fd, result;

    fd = open(PATH_SWAP, O_RDWR | O_CREAT | O_TRUNC, (mode_t)0600);
    if (fd == -1) {
    	log_error(logger_admin, "[INICIALIZACION] - Error al generar archivo de swap");
    }
    result = lseek(fd, TAMANIO_SWAP - 1, SEEK_SET);
    if (result == -1) {
    	close(fd);
    	log_error(logger_admin, "[INICIALIZACION] - Error al configurar el tamanio del archivo de swap");
    }
    result = write(fd, "", 1);
    if (result != 1) {
    	close(fd);
    	log_error(logger_admin, "[INICIALIZACION] - Error al escribir sobre el archivo de swap");
    }
    memoria_swap = mmap(0, TAMANIO_SWAP, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (memoria_swap == MAP_FAILED) {
    	close(fd);
    	log_error(logger_admin, "[INICIALIZACION] - Error al mapear el archivo");
    }

    for(int i = 0; i < cantidad_paginas_swap; i++) {
    	paginas_swap[i].utilizado = false;
    }

	log_info(logger_admin, "[INICIALIZACION] - Se finalizo la administracion de memoria: PAGINACION");
}

void pag_guardar_nueva_patota(datos_patota* nueva_patota) {

	uint32_t cant_tripulantes = nueva_patota->tripulantes;
	uint32_t tamanio_tareas = strlen(nueva_patota->tareas) + 1;
	uint32_t memoria_necesaria = TAMANIO_PCB + cant_tripulantes * TAMANIO_TCB + tamanio_tareas;
	uint32_t memoria_reservada = 0;

	pag_tabla* tabla_nueva = malloc(sizeof(pag_tabla));
	tabla_nueva->pid = nueva_patota->pid;
	tabla_nueva->c_tripulantes = cant_tripulantes;
	tabla_nueva->tid_tripulantes = list_create();
	tabla_nueva->tripulantes_finalizados = 0;
	tabla_nueva->tamanio_tareas = tamanio_tareas;
	tabla_nueva->paginas = list_create();
	uint32_t index_prox_pagina = 0;

	// RESERVO MEMORIA

	while(memoria_reservada < memoria_necesaria) {

		respuesta_solicitud_memoria respuesta_pedido = solicitar_memoria_disponible();

		if(respuesta_pedido.tipo == FALLA) {
			log_error(logger_admin, "[PROBLEMMM] - No hay mas memoria, NO DEBERIA PASAR ESTO AUNN");
		}

		pagina* nueva_pagina = malloc(sizeof(pagina));
		nueva_pagina->n_pagina = index_prox_pagina;
		nueva_pagina->valido = true;
		nueva_pagina->index_trip_ocupantes = list_create();

		if(respuesta_pedido.tipo == MARCO) {
			int index_frame = respuesta_pedido.index;
			nueva_pagina->n_frame = index_frame;
			nueva_pagina->presente = true;
			marcos_array[index_frame].n_pagina = index_prox_pagina;
			marcos_array[index_frame].pid = nueva_patota->pid;
		}
		else {
			int index_swap = respuesta_pedido.index;
			nueva_pagina->presente = false;
			paginas_swap[index_swap].n_pagina = index_prox_pagina;
			paginas_swap[index_swap].pid = nueva_patota->pid;
		}

		//En terminos de direccion logica:
		uint32_t pag_inicio = memoria_reservada;
		uint32_t pag_fin = memoria_reservada + TAMANIO_PAGINA - 1;

		if(pag_inicio < TAMANIO_PCB) {
			nueva_pagina->usada_solo_trip = false; //pagina marcada como usada por PCB
			log_info(logger_admin, "[Almacenamiento patota %d] - Pagina %d - Marcada como usada por PCB", nueva_patota->pid, nueva_pagina->n_pagina);
		}
		else if(pag_fin >= TAMANIO_PCB + cant_tripulantes * TAMANIO_TCB) {
			nueva_pagina->usada_solo_trip = false; //pagina marcada como usada por tareas
			log_info(logger_admin, "[Almacenamiento patota %d] - Pagina %d - Marcada como usada por Area Tareas", nueva_patota->pid, nueva_pagina->n_pagina);
		}
		else {
			//Marco todos los tripulantes que usan esta pagina (solo_trip)
			//Empiezo con [8-28]
			nueva_pagina->usada_solo_trip = true;
			uint32_t index_trip = 0;
			uint32_t trip_inicio = TAMANIO_PCB;
			uint32_t trip_fin = TAMANIO_PCB + TAMANIO_TCB - 1;
			while(index_trip < cant_tripulantes) {
				if(pag_inicio >= trip_inicio && pag_inicio <= trip_fin) {
					list_add(nueva_pagina->index_trip_ocupantes, (void*) index_trip);
				}
				else if(pag_fin >= trip_inicio && pag_fin <= trip_fin) {
					list_add(nueva_pagina->index_trip_ocupantes, (void*) index_trip);
				}
				index_trip++;
				trip_inicio += TAMANIO_TCB;
				trip_fin += TAMANIO_TCB;
			}
			log_info(logger_admin, "[Almacenamiento patota %d] - Pagina %d - Marcada como solo usada por tripulantes", nueva_patota->pid, nueva_pagina->n_pagina);
		}

		list_add(tabla_nueva->paginas, nueva_pagina);

		index_prox_pagina ++;
		memoria_reservada += TAMANIO_PAGINA;
	}

	log_info(logger_admin, "[Almacenamiento patota %d] Se reservo memoria para la patota, paginas utilizadas: %d", nueva_patota->pid, list_size(tabla_nueva->paginas));

	//SUMO A LAS TABLAS DE PAGINACION
	list_add(tablas_paginacion, tabla_nueva);

	//CARGO PRIMER REGISTRO DE TABLA = PCB, Y EL ULTIMO: TAREAS

	void* buffer = malloc(TAMANIO_PCB);
	uint32_t desp_buffer = 0;
	uint32_t dir_logica_pcb = 0; //Al inicio
	uint32_t dir_logica_tareas = TAMANIO_PCB + cant_tripulantes * TAMANIO_TCB;

	memcpy(buffer + desp_buffer, &(nueva_patota->pid), sizeof(uint32_t));
	desp_buffer += sizeof(uint32_t);
	memcpy(buffer + desp_buffer, &dir_logica_tareas ,sizeof(uint32_t));
	pag_escritura_a_memoria(nueva_patota->pid, dir_logica_pcb, TAMANIO_PCB, buffer);

	buffer = realloc(buffer, tamanio_tareas);
	memcpy(buffer, nueva_patota->tareas, tamanio_tareas);
	pag_escritura_a_memoria(nueva_patota->pid, dir_logica_tareas, tamanio_tareas, buffer);

	free(buffer);

}

void pag_guardar_nuevo_tripulante(datos_tripulante* nuevo_tripulante) {

	log_info(logger_admin, "[Almacenamiento tripulante %d] Se comienza la operacion", nuevo_tripulante->tid);

	pag_tabla* tabla_pid = buscar_tabla(nuevo_tripulante->pid);

	//Reviso la cant tripulantes ya inicializados
	int index_tripulante = list_size(tabla_pid->tid_tripulantes);
	if(index_tripulante >= tabla_pid->c_tripulantes){
		log_error(logger_admin, "[Almacenamiento tripulante %d] YA ESTAN TODOS LOS TRIPULANTES INICIALIZADOS PARA LA PATOTA %d", nuevo_tripulante->tid, nuevo_tripulante->pid);
	}

	uint32_t dir_logica_pcb = 0; //al inicio
	uint32_t dir_logica_tcb = TAMANIO_PCB + index_tripulante * TAMANIO_TCB;
	uint32_t dir_logica_tareas = TAMANIO_PCB + tabla_pid->c_tripulantes * TAMANIO_TCB;

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

	//ESCRIBO EN MEMORIA
	pag_escritura_a_memoria(nuevo_tripulante->pid, dir_logica_tcb, TAMANIO_TCB, buffer);

	log_info(logger_admin, "[Almacenamiento tripulante %d] Se guardo en memoria el TCB", nuevo_tripulante->tid);

	//AGREGO TID TRIPULANTE INICIALIZADO
	list_add(tabla_pid->tid_tripulantes, (void*) nuevo_tripulante->tid);

	char* tid_string = string_itoa(nuevo_tripulante->tid);
	dictionary_put(dic_tid_tabla, tid_string, (void*)tabla_pid);

	free(tid_string);
	free(buffer);

	log_info(logger_admin, "[Almacenamiento tripulante %d] Se finalizo la operacion - PID: %d - Index-Trip: %d", nuevo_tripulante->tid, nuevo_tripulante->pid, index_tripulante);
}

char pag_obtener_estado_tripulante(uint32_t tid) {
	log_info(logger_admin, "[Obtención estado trip %d] Se inicia la operacion", tid);
	result_busqueda* busqueda = buscar_tripulante(tid);
	uint32_t dir_logica = TAMANIO_PCB + busqueda->index_tripulante * TAMANIO_TCB + TCB_POS_ESTADO;
	char estado;
	pag_lectura_de_memoria(busqueda->tabla_pid->pid, dir_logica, sizeof(char), &estado);
	free(busqueda);
	log_info(logger_admin, "[Obtención estado trip %d] Se finaliza la operacion - estado actual: %c", tid, estado);
	return estado;
}

char* pag_obtener_prox_instruccion_tripulante(uint32_t tid) {

	log_info(logger_admin, "[Obtención prox inst trip %d] Se inicia la operacion", tid);
	// Obtengo la dirección logica, donde esta la prox tarea a ejecutar
	result_busqueda* busqueda = buscar_tripulante(tid);
	uint32_t dir_logica_pos_prox_tarea = TAMANIO_PCB + busqueda->index_tripulante * TAMANIO_TCB + TCB_POS_PROX_T;
	uint32_t dir_logica_prox_tarea;

	pag_lectura_de_memoria(busqueda->tabla_pid->pid, dir_logica_pos_prox_tarea, sizeof(uint32_t), &dir_logica_prox_tarea);

	log_info(logger_admin, "[Obtención prox inst trip %d] La prox instruccion esta en la direccion: %d", tid, dir_logica_prox_tarea);

	// Obtengo todas las tareas restantes

	uint32_t dir_logica_inicio_tareas = TAMANIO_PCB + busqueda->tabla_pid->c_tripulantes * TAMANIO_TCB; //Direccion logica inicio de tareas
	uint32_t dir_logica_fin_tareas = dir_logica_inicio_tareas + busqueda->tabla_pid->tamanio_tareas; //Direccion logica fin tareas
	//Ej: inicia en 15 - tamanio 8 => Fin: 23 (23 ya no sería una posicion valida, la ultima fue 22)

	uint32_t cant_caracteres_restantes = dir_logica_fin_tareas - dir_logica_prox_tarea;
	char* tareas_restantes = malloc(cant_caracteres_restantes);

	log_info(logger_admin, "[Obtención prox inst trip %d] Se va a leer de memoria las instrucciones restantes, %d caracteres", tid, cant_caracteres_restantes);

	pag_lectura_de_memoria(busqueda->tabla_pid->pid, dir_logica_prox_tarea, cant_caracteres_restantes, tareas_restantes);

	log_info(logger_admin, "[Obtención prox inst trip %d] Se leyo de memoria: %s", tid, tareas_restantes);

	// Envio solo la primera, hasta llegar a un '|' o '\0'
	char* prox_tarea = string_substring_until_char(tareas_restantes, '|');

	free(tareas_restantes);
	free(busqueda);
	log_info(logger_admin, "[Obtención prox inst trip %d] Se finaliza la operacion - prox instr: %s", tid, prox_tarea);
	return prox_tarea;
}

void pag_actualizar_estado_tripulante(uint32_t tid, char nuevo_estado) {
	result_busqueda* busqueda = buscar_tripulante(tid);
	uint32_t dir_logica = TAMANIO_PCB + busqueda->index_tripulante * TAMANIO_TCB + TCB_POS_ESTADO;
	pag_escritura_a_memoria(busqueda->tabla_pid->pid, dir_logica, TCB_POS_ESTADO, &nuevo_estado);
	free(busqueda);
}

void pag_actualizar_posicion_tripulante(uint32_t tid, uint32_t posX, uint32_t posY) {
	result_busqueda* busqueda = buscar_tripulante(tid);
	uint32_t pid = busqueda->tabla_pid->pid;
	uint32_t dir_logica = TAMANIO_PCB + busqueda->index_tripulante * TAMANIO_TCB + TCB_POS_POSX;
	pag_escritura_a_memoria(pid, dir_logica, sizeof(uint32_t), &posX);
	dir_logica = TAMANIO_PCB + busqueda->index_tripulante * TAMANIO_TCB + TCB_POS_POSY;
	pag_escritura_a_memoria(pid, dir_logica, sizeof(uint32_t), &posY);
	free(busqueda);
}

void pag_actualizar_instruccion_tripulante(uint32_t tid) {

	log_info(logger_admin, "[Actualizacion prox inst trip %d] Se inicia la operacion", tid);

	// Obtengo la dirección logica, donde esta la prox tarea a ejecutar
	result_busqueda* busqueda = buscar_tripulante(tid);
	uint32_t dir_logica_pos_prox_tarea = TAMANIO_PCB + busqueda->index_tripulante * TAMANIO_TCB + TCB_POS_PROX_T;
	uint32_t dir_logica_prox_tarea;
	pag_lectura_de_memoria(busqueda->tabla_pid->pid, dir_logica_pos_prox_tarea, sizeof(uint32_t), &dir_logica_prox_tarea);

	// Obtengo todas las tareas restantes

	uint32_t dir_logica_inicio_tareas = TAMANIO_PCB + busqueda->tabla_pid->c_tripulantes * TAMANIO_TCB; //Direccion logica inicio de tareas
	uint32_t dir_logica_fin_tareas = dir_logica_inicio_tareas + busqueda->tabla_pid->tamanio_tareas; //Direccion logica fin tareas

	uint32_t cant_caracteres_restantes = dir_logica_fin_tareas - dir_logica_prox_tarea;
	char* tareas_restantes = malloc(cant_caracteres_restantes);
	pag_lectura_de_memoria(busqueda->tabla_pid->pid, dir_logica_prox_tarea, cant_caracteres_restantes, tareas_restantes);

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

	// Modifico el valor de prox_tarea (TCB)

	uint32_t nueva_prox_tarea = dir_logica_prox_tarea + avance_posiciones;
	log_info(logger_admin, "[Actualizacion prox inst trip %d] Se avanza: dir_log_ant(%#010X) - dir_log_nueva(%#010X)", tid, dir_logica_prox_tarea, nueva_prox_tarea);
	pag_escritura_a_memoria(busqueda->tabla_pid->pid, dir_logica_pos_prox_tarea, sizeof(uint32_t), &nueva_prox_tarea);
	free(busqueda);
}

void pag_generar_dump_memoria(FILE* archivo) {
	char* linea;
	for(int i = 0; i < cantidad_marcos; i++) {
		if(marcos_array[i].libre)
			linea = string_from_format("Marco: %d\t\tEstado: Libre  \t\tProceso: -\t\tPagina: -\n", i);
		else
			linea = string_from_format("Marco: %d\t\tEstado: Ocupado\t\tProceso: %d\t\tPagina: %d\n", i, marcos_array[i].pid, marcos_array[i].n_pagina);
		txt_write_in_file(archivo, linea);
		free(linea);
	}
}

void pag_liberar_tripulante(uint32_t tid) {
	log_info(logger_admin, "[Liberacion tripulante %d] Se inicia la liberacion", tid);
	result_busqueda* busqueda = buscar_tripulante(tid);
	char* tid_string = string_itoa(tid);

	//LIBERO MEMORIA
	bool es_el_index_tripulante(void* index) {
		return (int) index == busqueda->index_tripulante;
	}
	void analizar_pagina(void* _pagina){
		pagina* pagina_elegida = (pagina*) _pagina;
		if(pagina_elegida->valido && pagina_elegida->usada_solo_trip) { //Si es solo usada por tripulantes, quito el index_tripulante de la lista
			list_remove_by_condition(pagina_elegida->index_trip_ocupantes, es_el_index_tripulante);
			if(list_is_empty(pagina_elegida->index_trip_ocupantes)) {
				liberar_pagina(pagina_elegida, busqueda->tabla_pid);
			}
		}
	}
	list_iterate(busqueda->tabla_pid->paginas, analizar_pagina);
	busqueda->tabla_pid->tripulantes_finalizados ++;
	log_info(logger_admin, "[Liberacion tripulante %d] Se libero el tripulante", tid);

	//Saco key-value del diccionario
	pag_tabla* tabla = (pag_tabla*) dictionary_remove(dic_tid_tabla, tid_string);

	//Compruebo si todos los tripulantes ya fueron liberados
	if(busqueda->tabla_pid->tripulantes_finalizados == busqueda->tabla_pid->c_tripulantes) {
		log_info(logger_admin, "[Liberacion tripulante %d] Es el ultimo tripulante activo, se libera su patota %d", tid, busqueda->tabla_pid->pid);
		_pag_liberar_patota(tabla);
	}
	log_info(logger_admin, "[Liberacion tripulante %d] Se finaliza la liberacion", tid);
	free(busqueda);
	free(tid_string);
}

static void _pag_liberar_patota(pag_tabla* tabla_a_eliminar) {

	//Libero todas las paginas activas

	void liberar_pagina_activa(void* _pagina) {
		pagina* pagina_a_liberar = (pagina*) _pagina;
		if(pagina_a_liberar->valido) {
			liberar_pagina(pagina_a_liberar, tabla_a_eliminar);
		}
	}
	list_iterate(tabla_a_eliminar->paginas, liberar_pagina_activa);

	//Elimino de lista de tablas paginacion
	bool es_la_tabla_del_pid(void* _tabla) {
		return ((pag_tabla*) _tabla)->pid == tabla_a_eliminar->pid;
	}
	list_remove_by_condition(tablas_paginacion, es_la_tabla_del_pid);

	// Realizo todos los free's
	list_destroy(tabla_a_eliminar->tid_tripulantes);
	list_destroy_and_destroy_elements(tabla_a_eliminar->paginas, free);
	free(tabla_a_eliminar);
}

//Caso

static respuesta_solicitud_memoria solicitar_memoria_disponible() {
	respuesta_solicitud_memoria respuesta;

	//Hay marcos disponibles...
	if(cantidad_marcos_libres > 0) {
		for(int i = 0; i < cantidad_marcos; i++) {
			if(marcos_array[i].libre) {
				marcos_array[i].libre = false;
				cantidad_marcos_libres--;
				respuesta.index = i;
				respuesta.tipo = MARCO;
				return respuesta;
			}
		}
	}
	//Hay espacio en swap...
	else if(cantidad_paginas_swap_libres > 0) {
		for(int i = 0; i < cantidad_paginas_swap; i++) {
			if(!paginas_swap[i].utilizado) {
				paginas_swap[i].utilizado = true;
				cantidad_paginas_swap_libres--;
				respuesta.index = i;
				respuesta.tipo = SWAP;
				return respuesta;
			}
		}
	}
	//Error...
	respuesta.index = -1;
	respuesta.tipo = FALLA;
	return respuesta;
}

static uint32_t buscar_index_swap(uint32_t pid, uint32_t n_pagina) {
	int i;
	for(i = 0; i < cantidad_paginas_swap; i++) {
		if(paginas_swap[i].utilizado && paginas_swap[i].n_pagina == n_pagina && paginas_swap[i].pid == pid) {
			return i;
		}
	}
	return UINT32_MAX; // A modo de error
}


static uint32_t eleccion_marco_victima() {

	//Reviso si hay marcos libresss..
	for(int i = 0; i < cantidad_marcos; i++) {
		if(marcos_array[i].libre) {
			return i;
		}
	}

	//LRU (?
	return (uint32_t) list_remove(cola_marcos_usados, 0);
}


static void pag_escritura_a_memoria(uint32_t pid, uint32_t direccion_logica, uint32_t size, void* buffer) {
	uint32_t size_restante = size;
	uint32_t prox_direccion = direccion_logica;
	uint32_t offset_buffer = 0;

	while(size_restante > 0) {
		uint32_t n_pagina = obtener_n_pagina(prox_direccion);
		uint32_t max_direccion = (n_pagina + 1) * TAMANIO_PAGINA;
		uint32_t bytes_a_escribir = min_number(size_restante, max_direccion - prox_direccion);

		uint32_t direccion_fisica = segmentacion_mmu(pid, prox_direccion);
		escritura_a_memoria(direccion_fisica, bytes_a_escribir, buffer + offset_buffer);

		prox_direccion += bytes_a_escribir;
		offset_buffer += bytes_a_escribir;
		size_restante -= bytes_a_escribir;
	}
}

static void pag_lectura_de_memoria(uint32_t pid, uint32_t direccion_logica, uint32_t size, void* buffer) {
	uint32_t size_restante = size;
	uint32_t prox_direccion = direccion_logica;
	uint32_t offset_buffer = 0;

	while(size_restante > 0) {
		uint32_t n_pagina = obtener_n_pagina(prox_direccion);
		uint32_t max_direccion = (n_pagina + 1) * TAMANIO_PAGINA;
		uint32_t bytes_a_leer = min_number(size_restante, max_direccion - prox_direccion);

		uint32_t direccion_fisica = segmentacion_mmu(pid, prox_direccion);
		lectura_de_memoria(buffer + offset_buffer, direccion_fisica, bytes_a_leer);

		prox_direccion += bytes_a_leer;
		offset_buffer += bytes_a_leer;
		size_restante -= bytes_a_leer;
	}
}

// devuelve direccion fisica
static uint32_t segmentacion_mmu(uint32_t pid, uint32_t direccion_logica) {
	pag_tabla* tabla = buscar_tabla(pid);

	uint32_t n_pagina = obtener_n_pagina(direccion_logica);
	uint32_t offset = obtener_offset(direccion_logica);
	pagina* pagina_buscada = list_get(tabla->paginas, n_pagina);
	if(pagina_buscada == NULL){
		log_error(logger_admin, "[OJOOO] No deberia pasar estoooo");
		return UINT32_MAX; // Como error
	}

	//Chequeo si esta presente:

	if(pagina_buscada->presente) {
		uint32_t n_frame = pagina_buscada->n_frame;

		//Agrego al final de la pila
		bool es_el_frame(void* _frame) {
			return ((uint32_t) _frame) == n_frame;
		}
		list_remove_by_condition(cola_marcos_usados, es_el_frame);
		list_add(cola_marcos_usados, (void*) n_frame);
		return n_frame * TAMANIO_PAGINA + offset;
	}
	else {
		log_info(logger_admin, "No esta la pagina en RAM, se va a traer desde la swap");
		//Busco index_swap
		uint32_t index_swap = buscar_index_swap(pid, n_pagina);
		if(index_swap == UINT32_MAX) {
			log_error(logger_admin, "[SWAP] - Se detecto un error al buscar la pagina de swap");
		}

		uint32_t frame_victima = eleccion_marco_victima();
		log_info(logger_admin, "[SWAP] Comienza intercambio entre: index_swap(%d) y frame_victima(%d)", index_swap, frame_victima);
		log_info(logger_admin, "[SWAP] index_swap(%d) es usado por PID(%d)-N_PAG(%d)", index_swap, paginas_swap[index_swap].pid, paginas_swap[index_swap].n_pagina);

		//Paso de Swap -> Temp
		void* buffer_temp = malloc(TAMANIO_PAGINA);
		memcpy(buffer_temp, memoria_swap + (index_swap * TAMANIO_PAGINA), TAMANIO_PAGINA);

		if(!marcos_array[frame_victima].libre) {
			//Si el marco no estaba libre, tengo que pasar Marco -> Swap
			log_info(logger_admin, "[SWAP] frame_victima(%d) es usado por PID(%d)-N_PAG(%d)", index_swap, marcos_array[frame_victima].pid, marcos_array[frame_victima].n_pagina);
			pag_tabla* ant_tabla = buscar_tabla(marcos_array[frame_victima].pid);
			pagina* ant_pagina = (pagina*) list_get(ant_tabla->paginas, marcos_array[frame_victima].n_pagina);

			//Paso lo que estaba antes, de Marco a Swap
			 //memcpy(memoria_swap + (index_swap * TAMANIO_PAGINA), buffer_temp, TAMANIO_PAGINA);
			memcpy(memoria_swap + (index_swap * TAMANIO_PAGINA), mem_principal->bloque + (frame_victima * TAMANIO_PAGINA), TAMANIO_PAGINA);
			paginas_swap[index_swap].pid = ant_tabla->pid;
			paginas_swap[index_swap].n_pagina = ant_pagina->n_pagina;
			ant_pagina->presente = false;
		}
		else {
			//Marco estaba libre...
			marcos_array[frame_victima].libre = false;
			cantidad_marcos_libres--;
		}

		//Paso de Temp a Marco
		escritura_a_memoria(frame_victima * TAMANIO_PAGINA, TAMANIO_PAGINA, buffer_temp);
		marcos_array[frame_victima].n_pagina = n_pagina;
		marcos_array[frame_victima].pid = pid;
		free(buffer_temp);

		//Marco pagina como presente!
		pagina_buscada->presente = true;
		pagina_buscada->n_frame = frame_victima;
		list_add(cola_marcos_usados, (void*) frame_victima);

		log_info(logger_admin, "[SWAP] Finalizo swap entre: index_swap(%d) y frame_victima(%d)", index_swap, frame_victima);
		log_info(logger_admin, "[SWAP] index_swap(%d) es usado por PID(%d)-N_PAG(%d)", index_swap, paginas_swap[index_swap].pid, paginas_swap[index_swap].n_pagina);
		log_info(logger_admin, "[SWAP] frame_victima(%d) es usado por PID(%d)-N_PAG(%d)", index_swap, marcos_array[frame_victima].pid, marcos_array[frame_victima].n_pagina);

		//Retorno la direccion fisica!
		return frame_victima * TAMANIO_PAGINA + offset;
	}
}

static void liberar_pagina(pagina* pag_a_liberar, pag_tabla* tabla) {

	log_info(logger_admin, "[Liberacion pagina] - Se va a liberar pagina: %d - del proceso %d", pag_a_liberar->n_pagina, tabla->pid);
	// Invalido pagina
	pag_a_liberar->valido = false;
	list_destroy(pag_a_liberar->index_trip_ocupantes);

	if(pag_a_liberar->presente) {
		marcos_array[pag_a_liberar->n_frame].libre = true;
		cantidad_marcos_libres++;
		//Quito de la pila
		bool es_el_marco(void* _marco) {
			return ((uint32_t) _marco) == pag_a_liberar->n_frame;
		}
		list_remove_by_condition(cola_marcos_usados, es_el_marco);
		log_info(logger_admin, "[Liberacion pagina] - Se finalizo de liberar la pagina. Se libero el marco %d", pag_a_liberar->n_frame);
	}
	else {
		//Esta en swap ...
		uint32_t index_swap = buscar_index_swap(tabla->pid, pag_a_liberar->n_pagina);
		paginas_swap[index_swap].utilizado = false;
		cantidad_paginas_swap_libres++;
		log_info(logger_admin, "[Liberacion pagina] - Se finalizo de liberar la pagina. Se libero la pagina de swap marco %d", index_swap);
	}
}

static pag_tabla* buscar_tabla(uint32_t pid) {
	bool tabla_es_del_pid(void* tabla) {
		return ((pag_tabla*) tabla)->pid == pid;
	}
	pag_tabla* tabla_encontrada = (pag_tabla*)list_find(tablas_paginacion, tabla_es_del_pid);
	return tabla_encontrada;
}

static uint32_t obtener_n_pagina(uint32_t direccion_logica) {
	return direccion_logica / TAMANIO_PAGINA;
}

static uint32_t obtener_offset(uint32_t direccion_logica) {
	return direccion_logica % TAMANIO_PAGINA;
}

static uint32_t min_number(uint32_t number1, uint32_t number2) {
	return number1 < number2 ? number1 : number2;
}

static result_busqueda* buscar_tripulante(uint32_t tid) {

	int i;
	result_busqueda* resultado = malloc(sizeof(result_busqueda));
	char* tid_string = string_itoa(tid);
	pag_tabla* tabla_pid = (pag_tabla*) dictionary_get(dic_tid_tabla, tid_string);

	for(i = 0; i < tabla_pid->c_tripulantes && ((uint32_t) list_get(tabla_pid->tid_tripulantes, i)) != tid; i++);
	if(i >= tabla_pid->c_tripulantes) {
		log_error(logger_admin, "ERRORRR EN BUSCAR TRIPULANTE");
		return NULL;
	}
	resultado->tabla_pid = tabla_pid;
	resultado->index_tripulante = i;
	free(tid_string);
	return resultado;
}



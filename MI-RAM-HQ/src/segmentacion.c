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
static t_list* tablas_segmentacion; //Aca van las tablas de segmentos
static t_list* areas_libres_ordenadas; //Areas de memoria libres, ordenadas por posicion de inicio
static t_list* segmentos_ordenados; //Segmentos ordenados por posicion de inicio
static t_dictionary* dic_tid_tabla; //Sin este diccionario, para buscar la tabla, habría que buscar en cada tabla y recorrer cada registro...
static uint32_t memoria_disponible; //Indica la cantidad de memoria libre

static void _seg_liberar_patota(seg_tabla* tabla);

static seg_tabla* buscar_tabla(uint32_t);
static result_busqueda* buscar_tripulante(uint32_t tid);
static segmento* seg_tabla_get_segmento(seg_tabla*, int);
static t_list* seg_tabla_get_segmentos_tripulantes(seg_tabla* tabla);
static bool area_esta_antes(void* area1, void* area2);
static bool segmento_esta_antes(void* seg1, void* seg2);
static bool segmento_no_esta_inicializado(void* _seg);

//TRADUCCION DE DIRECCIONES

static uint32_t obtener_direccion_logica (uint32_t, uint32_t);
static uint32_t obtener_n_segmento (uint32_t);
static uint32_t obtener_desplazamiento (uint32_t);
static uint32_t segmentacion_mmu(uint32_t pid, uint32_t direccion_logica, uint32_t size);

//RESERVA / LIBERACION / LECTURA / ESCRITURA / COMPACTACION: MEMORIA

static segmento* solicitud_reserva_memoria(uint32_t tamanio_pedido);
static seg_area_libre* (*critero_busqueda_memoria)(uint32_t tamanio_pedido);
static seg_area_libre* busqueda_first_fit(uint32_t tamanio_pedido);
static seg_area_libre* busqueda_best_fit(uint32_t tamanio_pedido);
static segmento* reservar_memoria(uint32_t tamanio_pedido, seg_area_libre* area_libre);
static void liberar_segmento(segmento* seg_a_liberar, seg_tabla* tabla);
static void seg_lectura_de_memoria(void* buffer, uint32_t pid, uint32_t direccion_logica, uint32_t size);
static void seg_escritura_a_memoria(uint32_t pid, uint32_t direccion_logica, uint32_t size, void* buffer);

static void seg_compactacion();

/* PUBLIC */

void seg_inicializacion() {
	areas_libres_ordenadas = list_create();
	tablas_segmentacion = list_create();
	dic_tid_tabla = dictionary_create();
	segmentos_ordenados = list_create();

	seg_area_libre* area_inicial = malloc(sizeof(seg_area_libre));
	area_inicial->inicio = 0;
	area_inicial->tamanio = mem_principal->tamanio;
	memoria_disponible = mem_principal->tamanio;
	list_add(areas_libres_ordenadas, area_inicial);

	if(strcmp(CRITERIO_SELECCION, "FF") == 0) {
		critero_busqueda_memoria = busqueda_first_fit;
	}
	else if(strcmp(CRITERIO_SELECCION, "BF") == 0) {
		critero_busqueda_memoria = busqueda_best_fit;
	}
	else {
		log_error(logger_admin, "Ha ocurrido un error, no se selecciono un CRITERIO_SELECCION VALIDO");
	}
	log_info(logger_admin, "Se inicia la administracion de memoria: SEGMENTACION");
}

void seg_guardar_nueva_patota(datos_patota* nueva_patota) {

	log_info(logger_admin, "[Almacenamiento patota %d] Se comienza la operacion", nueva_patota->pid);

	uint32_t cantidad_tripulantes = nueva_patota->tripulantes;
	uint32_t tamanio_tareas = strlen(nueva_patota->tareas) + 1;

	//ARMO TABLA DE SEGMENTOS:

	seg_tabla* nueva_tabla = malloc(sizeof(seg_tabla));

	nueva_tabla->segmentos = list_create();
	nueva_tabla->c_tripulantes = nueva_patota->tripulantes;
	nueva_tabla->pid = nueva_patota->pid;

	log_info(logger_admin, "[Almacenamiento patota %d] Se armo la tabla de segmentos", nueva_patota->pid);

	//RESERVO MEMORIA PRIMERO:

	if(memoria_disponible < TAMANIO_PCB + cantidad_tripulantes * TAMANIO_TCB + tamanio_tareas) {
		log_error(logger_admin, "Se encontro una imposibilidad factica para asignar memoria, no hay mas");
		//QUE HACER EN ESTE CASOO!!!
	}

	segmento* segmento_pcb = solicitud_reserva_memoria(TAMANIO_PCB);
	if(segmento_pcb == NULL) {
		log_error(logger_admin, "Se encontro una imposibilidad factica para asignar memoria, revisar");
	}
	segmento_pcb->inicializado = true;
	segmento_pcb->n_segmento = 0;
	list_add(nueva_tabla->segmentos, segmento_pcb);
	for(int i = 1; i <= cantidad_tripulantes; i++) {
		segmento* un_segmento_tcb = solicitud_reserva_memoria(TAMANIO_TCB);
		if(un_segmento_tcb == NULL) {
			log_error(logger_admin, "Se encontro una imposibilidad factica para asignar memoria, revisar");
		}
		un_segmento_tcb->inicializado = false;
		un_segmento_tcb->n_segmento = i;
		list_add(nueva_tabla->segmentos, un_segmento_tcb);
	}
	segmento* segmento_tareas = solicitud_reserva_memoria(tamanio_tareas);
	if(segmento_tareas == NULL) {
		log_error(logger_admin, "Se encontro una imposibilidad factica para asignar memoria, revisar");
	}
	segmento_tareas->inicializado = true;
	segmento_tareas->n_segmento = cantidad_tripulantes + 1;
	list_add(nueva_tabla->segmentos, segmento_tareas);
	log_info(logger_admin, "[Almacenamiento patota %d] Se reservo la memoria de todos los segmentos", nueva_patota->pid);


	//SUMO LA TABLA A LAS TABLAS DE SEGMENTACION:

	list_add(tablas_segmentacion, nueva_tabla);

	//CARGO PRIMER REGISTRO DE TABLA = PCB, Y EL ULTIMO: TAREAS
	void* buffer = malloc(TAMANIO_PCB);
	uint32_t desp_buffer = 0;
	uint32_t dir_logica_pcb = obtener_direccion_logica(0, 0); //Segmento 0, al inicio
	uint32_t dir_logica_tareas = obtener_direccion_logica(cantidad_tripulantes + 1, 0);

	memcpy(buffer + desp_buffer, &(nueva_patota->pid), sizeof(uint32_t));
	desp_buffer += sizeof(uint32_t);
	memcpy(buffer + desp_buffer, &dir_logica_tareas ,sizeof(uint32_t));
	seg_escritura_a_memoria(nueva_patota->pid, dir_logica_pcb, TAMANIO_PCB, buffer);

	buffer = realloc(buffer, tamanio_tareas);
	memcpy(buffer, nueva_patota->tareas, tamanio_tareas);
	seg_escritura_a_memoria(nueva_patota->pid, dir_logica_tareas, tamanio_tareas, buffer);

	log_info(logger_admin, "[Almacenamiento patota %d] Se cargo los segmentos PCB y Tareas", nueva_patota->pid);

	//LIBERO
	free(buffer);

	log_info(logger_admin, "[Almacenamiento patota %d] Se finalizo la operacion", nueva_patota->pid);
}

void seg_guardar_nuevo_tripulante(datos_tripulante* nuevo_tripulante) {

	log_info(logger_admin, "[Almacenamiento tripulante %d] Se comienza la operacion", nuevo_tripulante->tid);

	//BUSCO UN SEGMENTO TRIP NO INICIALIZADO
	seg_tabla* tabla_pid = buscar_tabla(nuevo_tripulante->pid);
	t_list* seg_tripulantes = seg_tabla_get_segmentos_tripulantes(tabla_pid);
	segmento* trip_no_inicializado = list_find(seg_tripulantes, segmento_no_esta_inicializado);
	if(trip_no_inicializado == NULL) {
		log_error(logger_admin, "[Almacenamiento tripulante %d] NO SE ENCONTRO UN SEGMENTO DISPONIBLE PARA LA PATOTA %d", nuevo_tripulante->tid, nuevo_tripulante->pid);
		return;
	}
	int n_registro = trip_no_inicializado->n_segmento;

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

	//ESCRIBO EN MEMORIA
	seg_escritura_a_memoria(nuevo_tripulante->pid, dir_logica_tcb, TAMANIO_TCB, buffer);

	log_info(logger_admin, "[Almacenamiento tripulante %d] Se guardo en memoria el TCB", nuevo_tripulante->tid);

	//MARCO SEGMENTO COMO INICIALIZADO
	seg_tabla_get_segmento(tabla_pid, n_registro)->inicializado = true;

	char* tid_string = string_itoa(nuevo_tripulante->tid);

	dictionary_put(dic_tid_tabla, tid_string, (void*)tabla_pid);
	free(seg_tripulantes);
	free(tid_string);
	free(buffer);

	log_info(logger_admin, "[Almacenamiento tripulante %d] Se finalizo la operacion", nuevo_tripulante->tid);
}

char seg_obtener_estado_tripulante(uint32_t tid) {
	log_info(logger_admin, "[Obtención estado trip %d] Se inicia la operacion", tid);
	result_busqueda* busqueda = buscar_tripulante(tid);
	uint32_t dir_logica = obtener_direccion_logica(busqueda->n_segmento, TCB_POS_ESTADO);
	char estado;
	seg_lectura_de_memoria(&estado, busqueda->tabla_pid->pid, dir_logica, sizeof(char));
	free(busqueda);
	log_info(logger_admin, "[Obtención estado trip %d] Se finaliza la operacion - estado actual: %c", tid, estado);
	return estado;
}

char* seg_obtener_prox_instruccion_tripulante(uint32_t tid) {

	log_info(logger_admin, "[Obtención prox inst trip %d] Se inicia la operacion", tid);
	// Obtengo la dirección logica, donde esta la prox tarea a ejecutar
	result_busqueda* busqueda = buscar_tripulante(tid);
	uint32_t dir_logica_pos_prox_tarea = obtener_direccion_logica(busqueda->n_segmento, TCB_POS_PROX_T);
	uint32_t dir_logica_prox_tarea;

	seg_lectura_de_memoria(&dir_logica_prox_tarea, busqueda->tabla_pid->pid, dir_logica_pos_prox_tarea, sizeof(uint32_t));

	// Obtengo todas las tareas restantes

	int n_segmento_tareas = obtener_n_segmento(dir_logica_prox_tarea);
	uint32_t desplazamiento_tareas = obtener_desplazamiento(dir_logica_prox_tarea);
	uint32_t tam_seg_tareas = seg_tabla_get_segmento(busqueda->tabla_pid, n_segmento_tareas)->tamanio;
	uint32_t cant_caracteres_restantes = tam_seg_tareas - desplazamiento_tareas;
	char* tareas_restantes = malloc(cant_caracteres_restantes);
	seg_lectura_de_memoria(tareas_restantes, busqueda->tabla_pid->pid, dir_logica_prox_tarea, cant_caracteres_restantes);

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
	seg_escritura_a_memoria(busqueda->tabla_pid->pid, dir_logica, TCB_POS_ESTADO, &nuevo_estado);
	free(busqueda);
}

void seg_actualizar_posicion_tripulante(uint32_t tid, uint32_t posX, uint32_t posY) {
	result_busqueda* busqueda = buscar_tripulante(tid);
	uint32_t pid = busqueda->tabla_pid->pid;
	uint32_t dir_logica = obtener_direccion_logica(busqueda->n_segmento, TCB_POS_POSX);
	seg_escritura_a_memoria(pid, dir_logica, sizeof(uint32_t), &posX);
	dir_logica = obtener_direccion_logica(busqueda->n_segmento, TCB_POS_POSY);
	seg_escritura_a_memoria(pid, dir_logica, sizeof(uint32_t), &posY);
	free(busqueda);
}

void seg_actualizar_instruccion_tripulante(uint32_t tid) {

	log_info(logger_admin, "[Actualizacion prox inst trip %d] Se inicia la operacion", tid);

	// Obtengo la dirección logica, donde esta la prox tarea a ejecutar
	result_busqueda* busqueda = buscar_tripulante(tid);
	uint32_t dir_logica_pos_prox_tarea = obtener_direccion_logica(busqueda->n_segmento, TCB_POS_PROX_T);
	uint32_t dir_logica_prox_tarea;
	seg_lectura_de_memoria(&dir_logica_prox_tarea, busqueda->tabla_pid->pid, dir_logica_pos_prox_tarea, sizeof(uint32_t));

	// Obtengo todas las tareas restantes

	int n_segmento_tareas = obtener_n_segmento(dir_logica_prox_tarea);
	uint32_t desplazamiento_tareas = obtener_desplazamiento(dir_logica_prox_tarea);
	uint32_t tam_seg_tareas = seg_tabla_get_segmento(busqueda->tabla_pid, n_segmento_tareas)->tamanio;
	uint32_t cant_caracteres_restantes = tam_seg_tareas - desplazamiento_tareas;
	char* tareas_restantes = malloc(cant_caracteres_restantes);
	seg_lectura_de_memoria(tareas_restantes, busqueda->tabla_pid->pid, dir_logica_prox_tarea, cant_caracteres_restantes);
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
	seg_escritura_a_memoria(busqueda->tabla_pid->pid, dir_logica_pos_prox_tarea, sizeof(uint32_t), &nueva_prox_tarea);
	free(busqueda);
}

void seg_generar_dump_memoria(FILE* archivo) {

	void escritura_por_patota(void* _patota) {
		seg_tabla* tabla_patota = (seg_tabla*) _patota;
		void escritura_por_segmento(void* _seg) {
			segmento* seg = (segmento*) _seg;
			char* linea = string_from_format("Proceso: %d\tSegmento: %d\tInicio: %#010X\tTam: %dB\n", tabla_patota->pid, seg->n_segmento, seg->inicio, seg->tamanio);
			txt_write_in_file(archivo, linea);
			free(linea);
		}
		list_iterate(tabla_patota->segmentos, escritura_por_segmento);
	}
	list_iterate(tablas_segmentacion, escritura_por_patota);

	void escritura_por_libre(void* _libre) {
		seg_area_libre* area = (seg_area_libre*) _libre;
		char* linea = string_from_format("Area libre:\tInicio: %d\tTam: %dB\n", area->inicio, area->tamanio);
		txt_write_in_file(archivo, linea);
	}
	list_iterate(areas_libres_ordenadas, escritura_por_libre);
	char* linea = string_from_format("Area libre total: %d\n", memoria_disponible);
	txt_write_in_file(archivo, linea);
	free(linea);
}

void seg_receptor_sigusr2() {
	log_info(logger_admin, "Se inicia compactacion por SIGNAL");
	seg_compactacion();
}

void seg_liberar_tripulante(uint32_t tid) {
	log_info(logger_admin, "[Liberacion tripulante %d] Se inicia la liberacion", tid);
	result_busqueda* busqueda = buscar_tripulante(tid);
	char* tid_string = string_itoa(tid);

	//LIBERO MEMORIA
	segmento* seg_tripulante = seg_tabla_get_segmento(busqueda->tabla_pid, busqueda->n_segmento);
	liberar_segmento(seg_tripulante, busqueda->tabla_pid);
	log_info(logger_admin, "[Liberacion tripulante %d] Se libero el segmento tripulante", tid);


	//Saco key-value del diccionario
	seg_tabla* tabla = (seg_tabla*) dictionary_remove(dic_tid_tabla, tid_string);

	//Compruebo si todos los tripulantes ya fueron liberados
	if (list_size(busqueda->tabla_pid->segmentos) == 2) {
		log_info(logger_admin, "[Liberacion tripulante %d] Es el ultimo tripulante activo, se libera su patota %d", tid, busqueda->tabla_pid->pid);
		_seg_liberar_patota(tabla);
	}
	log_info(logger_admin, "[Liberacion tripulante %d] Se finaliza la liberacion", tid);
	free(busqueda);
	free(tid_string);
}

static void _seg_liberar_patota(seg_tabla* tabla_a_eliminar) {
	segmento* segmento_pcb = seg_tabla_get_segmento(tabla_a_eliminar, 0);
	segmento* segmento_tareas = seg_tabla_get_segmento(tabla_a_eliminar, tabla_a_eliminar->c_tripulantes + 1);
	liberar_segmento(segmento_pcb, tabla_a_eliminar);
	liberar_segmento(segmento_tareas, tabla_a_eliminar);

	bool es_la_tabla_del_pid(void* _tabla) {
		return ((seg_tabla*) _tabla)->pid == tabla_a_eliminar->pid;
	}
	list_remove_by_condition(tablas_segmentacion, es_la_tabla_del_pid);
	free(tabla_a_eliminar);
}

static void seg_compactacion() {
	uint32_t posicion = 0;
	log_info(logger_admin, "[COMPACTACION] - Se inicia operacion");
	void reasignacion_inicio(void* _seg) {
		segmento* seg = (segmento*) _seg;
		void* buffer = malloc(seg->tamanio);
		lectura_de_memoria(buffer, seg->inicio, seg->tamanio);
		escritura_a_memoria(posicion, seg->tamanio, buffer);
		seg->inicio = posicion;
		posicion += seg->tamanio;
		free(buffer);
	}
	list_iterate(segmentos_ordenados, reasignacion_inicio);
	list_clean_and_destroy_elements(areas_libres_ordenadas, free);
	seg_area_libre* unica_area_libre = malloc(sizeof(seg_area_libre));
	unica_area_libre->inicio = posicion;
	unica_area_libre->tamanio = memoria_disponible;
	list_add(areas_libres_ordenadas, unica_area_libre);
	log_info(logger_admin, "[COMPACTACION] - Se finaliza operacion");
}


//Devuelve null si no se encontro un lugar disponible
//Se encarga de buscar un segmento libree
//Devuelve el segmento donde se reservo la informacion

static segmento* solicitud_reserva_memoria(uint32_t tamanio_pedido) {
	log_info(logger_admin, "[Reserva memoria] %d bytes - Se comienza operacion", tamanio_pedido);
	//Busco un area disponible
	seg_area_libre* area_libre = critero_busqueda_memoria(tamanio_pedido);
	if(area_libre == NULL) {
		log_info(logger_admin, "[Reserva memoria] %d bytes - No se encontro memoria disponible, se procede a realizar compactacion y volver a intentar");
		seg_compactacion();
		area_libre = critero_busqueda_memoria(tamanio_pedido);
		if(area_libre == NULL) {
			log_error(logger_admin, "[Reserva memoria] %d bytes - No encontro memoria luego de compactar, NO DEBERIA PASAR ESTOOOO");
			return NULL;
		}
	}
	log_info(logger_admin, "[Reserva memoria] Pedido %d bytes - "
			"Se encontro un lugar disponible de: "
			"inicio(%d) / tamanio (%d)", tamanio_pedido, area_libre->inicio, area_libre->tamanio);
	return reservar_memoria(tamanio_pedido, area_libre);
}

static seg_area_libre* busqueda_first_fit(uint32_t tamanio_pedido) {
	log_info(logger_admin, "[Busqueda FIRST FIT] Pedido %d bytes - Se comienza operacion", tamanio_pedido);
	bool area_con_tamanio_suf(void* _area) {
		return ((seg_area_libre*) _area)->tamanio >= tamanio_pedido;
	}
	seg_area_libre* area_a_utilizar = list_find(areas_libres_ordenadas, area_con_tamanio_suf);
	if(area_a_utilizar == NULL) {
		log_error(logger_admin, "[Busqueda FIRST FIT] Pedido %d bytes - Ocurrio un error, no hay memoria suficiente", tamanio_pedido);
		return NULL;
	}
	return area_a_utilizar;
}

static seg_area_libre* busqueda_best_fit(uint32_t tamanio_pedido) {
	log_info(logger_admin, "[Busqueda BEST FIT] Pedido %d bytes - Se comienza operacion", tamanio_pedido);
	seg_area_libre* mejor_area = NULL;
	void buscador_del_mejor(void* _area) {
		seg_area_libre* area = (seg_area_libre*) _area;
		if(area->tamanio < tamanio_pedido) return;
		if(mejor_area == NULL || area->tamanio < mejor_area->tamanio)
			mejor_area = area;
	}
	list_iterate(areas_libres_ordenadas, buscador_del_mejor);
	if(mejor_area == NULL) {
		log_error(logger_admin, "[Busqueda BEST FIT] Pedido %d bytes - Ocurrio un error, no hay memoria suficiente", tamanio_pedido);
		return NULL;
	}
	return mejor_area;
}

static segmento* reservar_memoria(uint32_t tamanio_pedido, seg_area_libre* area_libre) {

	uint32_t inicio_anterior = area_libre->inicio;
	(area_libre->inicio) += tamanio_pedido;
	(area_libre->tamanio) -= tamanio_pedido;
	segmento* nuevo_segmento = malloc(sizeof(segmento));
	nuevo_segmento->inicio = inicio_anterior;
	nuevo_segmento->tamanio = tamanio_pedido;
	list_add_sorted(segmentos_ordenados, nuevo_segmento, segmento_esta_antes);
	memoria_disponible -= tamanio_pedido;
	log_info(logger_admin, "[Reserva mem] - Se reserva memoria para nuevo segmento: [%d-%d] ", inicio_anterior, inicio_anterior + tamanio_pedido - 1);
	return nuevo_segmento;
}

static void liberar_segmento(segmento* seg_a_liberar, seg_tabla* tabla) {

	log_info(logger_admin, "[Liberacion segmento] - Se va a liberar segmento: [%d-%d]", seg_a_liberar->inicio, seg_a_liberar->inicio + seg_a_liberar->tamanio - 1);
	//Se quita de las listas y se hace el free
	bool es_el_segmento(void* _seg){
		segmento* seg = (segmento*) _seg;
		return seg->inicio == seg_a_liberar->inicio;
	}
	list_remove_by_condition(tabla->segmentos, es_el_segmento);
	list_remove_by_condition(segmentos_ordenados, es_el_segmento);
	log_info(logger_admin, "[Liberacion segmento] - Se borro segmento, se va a armar el area libre");

	//Genero area libre nueva, y lo sumo a la lista de areas libres
	seg_area_libre* area_nueva = malloc(sizeof(seg_area_libre));
	area_nueva->inicio = seg_a_liberar->inicio;
	area_nueva->tamanio = seg_a_liberar->tamanio;
	memoria_disponible += seg_a_liberar->tamanio;
	free(seg_a_liberar); //Ahora si hago el free
	int index = list_add_sorted(areas_libres_ordenadas, area_nueva, area_esta_antes);
	log_info(logger_admin, "[Liberacion segmento] - Armo area libre [%d-%d], index: %d", area_nueva->inicio, area_nueva->inicio + area_nueva->tamanio - 1, index);

	//Consolido areas libres
	/* -- SE COMENTA PARA PROBAR COMPACTACION
	seg_area_libre* ant = index > 0 ? list_get(areas_libres_ordenadas, index - 1) : NULL;
	seg_area_libre* post = index < list_size(areas_libres_ordenadas) - 1? list_get(areas_libres_ordenadas, index + 1) : NULL;
	if(post != NULL && post->inicio == area_nueva->inicio + area_nueva->tamanio) {
		log_info(logger_admin, "[Liberacion segmento] - Se va a unir el area libre con la posterior");
		(area_nueva->tamanio) += post->tamanio;
		list_remove_and_destroy_element(areas_libres_ordenadas, index + 1, free);
		log_info(logger_admin, "[Liberacion segmento] - Ahora quedo así [%d-%d], index: %d", area_nueva->inicio, area_nueva->inicio + area_nueva->tamanio - 1, index);
	}
	if(ant != NULL && ant->inicio + ant->tamanio == area_nueva->inicio) {
		log_info(logger_admin, "[Liberacion segmento] - Se va a unir el area libre con la anterior");
		area_nueva->inicio = ant->inicio;
		(area_nueva->tamanio) += ant->tamanio;
		list_remove_and_destroy_element(areas_libres_ordenadas, index - 1, free);
		log_info(logger_admin, "[Liberacion segmento] - Ahora quedo así [%d-%d], index: %d", area_nueva->inicio, area_nueva->inicio + area_nueva->tamanio - 1, index - 1);
	}
	*/
	log_info(logger_admin, "[Liberacion segmento] - Se libero un segmento y se consolido un espacio libre");
}

//Devuelve la direccion fisica
//Si falla, devuelve UINT32_MAX
static uint32_t segmentacion_mmu(uint32_t pid, uint32_t direccion_logica, uint32_t size) {
	seg_tabla* tabla = buscar_tabla(pid);
	uint32_t n_segmento = obtener_n_segmento(direccion_logica);
	uint32_t desplazamiento = obtener_desplazamiento(direccion_logica);
	segmento* segmento = seg_tabla_get_segmento(tabla, n_segmento);
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

static void seg_lectura_de_memoria(void* buffer, uint32_t pid, uint32_t direccion_logica, uint32_t size) {
	uint32_t direccion_fisica = segmentacion_mmu(pid, direccion_logica, size);
	lectura_de_memoria(buffer, direccion_fisica, size);
}

static void seg_escritura_a_memoria(uint32_t pid, uint32_t direccion_logica, uint32_t size, void* buffer) {
	uint32_t direccion_fisica = segmentacion_mmu(pid, direccion_logica, size);
	escritura_a_memoria(direccion_fisica, size, buffer);
}

static bool area_esta_antes(void* area1, void* area2) {
	return ((seg_area_libre*) area1)->inicio < ((seg_area_libre*) area2)->inicio;
}

static bool segmento_esta_antes(void* seg1, void* seg2) {
	return ((segmento*) seg1)->inicio < ((segmento*) seg2)->inicio;
}

static seg_tabla* buscar_tabla(uint32_t pid) {
	bool tabla_es_del_pid(void* tabla) {
		return ((seg_tabla*) tabla)->pid == pid;
	}
	seg_tabla* tabla_encontrada = (seg_tabla*)list_find(tablas_segmentacion, tabla_es_del_pid);
	return tabla_encontrada;
}

static segmento* seg_tabla_get_segmento(seg_tabla* tabla, int n_segmento) {
	bool es_el_segmento(void* _seg) {
		return ((segmento*) _seg)->n_segmento == n_segmento;
	}
	return list_find(tabla->segmentos, es_el_segmento);
}

static t_list* seg_tabla_get_segmentos_tripulantes(seg_tabla* tabla) {
	int index_tareas = tabla->c_tripulantes + 1;
	bool es_tripululante(void* _seg) {
		segmento* seg = (segmento*) _seg;
		return seg->n_segmento != 0 && seg->n_segmento != index_tareas; //No me traigo el segmento PCB ni el de las tareas
	}
	return list_filter(tabla->segmentos, es_tripululante);
}

//Devuelve el numero de segmento donde esta el tripulante
static result_busqueda* buscar_tripulante(uint32_t tid) {

	result_busqueda* resultado = malloc(sizeof(result_busqueda));
	char* tid_string = string_itoa(tid);
	seg_tabla* tabla_pid = (seg_tabla*) dictionary_get(dic_tid_tabla, tid_string);

	uint32_t* buffer_tid = malloc(sizeof(uint32_t));

	bool tiene_el_tid_buscado(void* _seg) {
		segmento* seg = (segmento*) _seg;
		if(!seg->inicializado) //Si no esta inicializado, no voy a memoria
			return false;
		uint32_t dir_logica = obtener_direccion_logica(seg->n_segmento, 0); //El TID esta en el inicio...
		uint32_t dir_fisica = segmentacion_mmu(tabla_pid->pid, dir_logica, sizeof(uint32_t));
		lectura_de_memoria(buffer_tid, dir_fisica, sizeof(uint32_t));
		return tid == *buffer_tid;
	}
	t_list* seg_tripulantes = seg_tabla_get_segmentos_tripulantes(tabla_pid);
	segmento* trip_encontrado = list_find(seg_tripulantes, tiene_el_tid_buscado);
	resultado->tabla_pid = tabla_pid;
	resultado->n_segmento = trip_encontrado != NULL ? trip_encontrado->n_segmento : -1;

	free(seg_tripulantes);
	free(tid_string);
	free(buffer_tid);
	return resultado;
}

static bool segmento_no_esta_inicializado(void* _seg) {
	return !((segmento*) _seg)->inicializado;
}

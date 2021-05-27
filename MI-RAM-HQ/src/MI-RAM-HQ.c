/*
 ============================================================================
 Name        : MI-RAM-HQ.c
 Author      : 
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include "MI-RAM-HQ.h"

int main(void) {
	inicializarVariables();

	int socket_escucha = iniciarServidor(IP_MI_RAM,PUERTO_MI_RAM);
	log_info(loggerMiRam, "MI-RAM-HQ Listo para recibir a los Tripulantes!");

	while(1) {
		int socket_cliente = esperar_cliente(socket_escucha);

		int tipo_msg = recibir_operacion(socket_cliente);

		pthread_t hilo_receptor;

		switch(tipo_msg) {
		case INICIAR_PATOTA:
			pthread_create(&hilo_receptor, NULL, (void*) recibir_datos_patota,(void*) socket_cliente);
			pthread_detach(hilo_receptor);
			break;
		case INICIAR_TRIPULANTE:
			pthread_create(&hilo_receptor, NULL,(void*) atenderTripulante, (void*) socket_cliente);
			pthread_detach(hilo_receptor);
			break;
		default:
			log_warning(loggerMiRam, "Tipo de mensaje desconocido!!!. Cierro conexion con dicho cliente");
			close(socket_escucha);
			return -1;
			break;
		}
	}

	close(socket_escucha);
	terminar_programa();
	return EXIT_SUCCESS;
}

void inicializarVariables(){

	configuracionMiRam = config_create("/home/utnso/workspace/tp-2021-1c-No-C-Aprueba-/MI-RAM-HQ/miram.config");
	loggerMiRam = log_create("/home/utnso/workspace/tp-2021-1c-No-C-Aprueba-/MI-RAM-HQ/miram.log", "MIRA-RAM-HQ", 1, LOG_LEVEL_INFO);
	TAMANIO_MEMORIA = config_get_int_value(configuracionMiRam,"TAMANIO_MEMORIA");
	TAMANIO_PAGINA = config_get_int_value(configuracionMiRam,"TAMANIO_PAGINA");
	TAMANIO_SWAP = config_get_int_value(configuracionMiRam,"TAMANIO_SWAP");
	IP_MI_RAM = config_get_string_value(configuracionMiRam,"IP_MI_RAM");
	PUERTO_MI_RAM = config_get_string_value(configuracionMiRam,"PUERTO");
	ESQUEMA_MEMORIA = config_get_string_value(configuracionMiRam,"ESQUEMA_MEMORIA");
	PATH_SWAP = config_get_string_value(configuracionMiRam,"PATH_SWAP");
	ALGORITMO_REEMPLAZO = config_get_string_value(configuracionMiRam,"ALGORITMO_REEMPLAZO");
	tripulantes = list_create();
	patotas = list_create();
	pthread_mutex_init(&mutexTripulantes,NULL);
	pthread_mutex_init(&mutexPatotas,NULL);
}

void atenderTripulante(void* _cliente) {
	log_info(loggerMiRam, "Se conectó un Tripulante!");
	log_info(loggerMiRam, "Primero recibo sus datos y armo TCB");

	int socket_tripulante = (int) _cliente;

	TCB* tripulante = recibir_datos_tripulante(socket_tripulante);

	log_info(loggerMiRam, "[TRIPULANTE %d] Se ha creado TCB . Pertenece a la patota %d. Posicion: %d|%d", tripulante->tid, tripulante->direccionPCB->pid, tripulante->posX, tripulante->posY);

	while(1){
		int tipo_msg = recibir_operacion(socket_tripulante);

		switch(tipo_msg){
		case PROXIMA_TAREA:
			enviar_proxima_tarea(tripulante, socket_tripulante);
			avanzar_proxima_instruccion(tripulante);
			log_info(loggerMiRam, "[TRIPULANTE %d] Se envio proxima tarea para el tripulante", tripulante->tid);
			break;
		case INFORMAR_MOVIMIENTO:
			recibir_movimiento_tripulante(tripulante, socket_tripulante);
			break;
		case CAMBIO_ESTADO:{
			char nuevoEstado;
			recv(socket_tripulante,&nuevoEstado,sizeof(char),0),
			log_info(loggerMiRam, "[TRIPULANTE %d] Pasa de estado %c a %c", tripulante->tid, tripulante->estado, nuevoEstado);

			pthread_mutex_lock(&mutexTripulantes);
			tripulante->estado = nuevoEstado;
			pthread_mutex_unlock(&mutexTripulantes);
			break;
		}
		case EXPULSAR_TRIPULANTE:
			log_info(loggerMiRam, "[TRIPULANTE %d] EXPULSADO.",tripulante->tid);
			close(socket_tripulante);
			return;
			break;
		default:
			//ACÁ HABRÍA QUE BORRAR AL TRIPULANTE DE LA MEMORIA
			log_info(loggerMiRam, "[TRIPULANTE %d] Tipo de mensaje desconocido!!!",tripulante->tid);
			close(socket_tripulante);
			return;
			break;
		}
	}

	close(socket_tripulante);
}

void recibir_datos_patota(void* _cliente) {
	int socket_cliente = (int) _cliente;
	void* buffer;
	uint32_t buffer_size;
	uint32_t desplazamiento = 0;
	uint32_t inst_len;
	uint32_t cantidadTripulantes;

	PCB* nuevo_pcb = malloc(sizeof(PCB));

	log_info(loggerMiRam,"Me llegan los datos de una patota");

	buffer = recibir_buffer(&buffer_size, socket_cliente);

	//leo PID
	memcpy(&(nuevo_pcb->pid), buffer + desplazamiento, sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);

	//leo cant_tripulantes
	memcpy(&cantidadTripulantes, buffer + desplazamiento, sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);

	//leo longitud de instrucciones
	memcpy(&inst_len, buffer + desplazamiento, sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);

	//leo instrucciones, va sin &, porque tareas es un puntero
	nuevo_pcb->tareas = malloc(inst_len);
	memcpy(nuevo_pcb->tareas, buffer + desplazamiento, inst_len);

	pthread_mutex_lock(&mutexPatotas);
	list_add(patotas, nuevo_pcb);
	pthread_mutex_unlock(&mutexPatotas);

	//Envio el OK
	enviar_respuesta(OK, socket_cliente);

	log_info(loggerMiRam, "Se ha creado el PCB: %d\nSus instrucciones son: %s", nuevo_pcb->pid, nuevo_pcb->tareas);

	close(socket_cliente);
	free(buffer);
}

TCB* recibir_datos_tripulante(int socket_tripulante) {
	void* buffer;
	uint32_t buffer_size;
	uint32_t desplazamiento = 0;
	uint32_t posible_pid;
	PCB* patota;

	TCB* nuevo_tcb = malloc(sizeof(TCB));

	buffer = recibir_buffer(&buffer_size, socket_tripulante);

	memcpy(&posible_pid, buffer + desplazamiento, sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);

	patota = buscar_patota(posible_pid);
	nuevo_tcb->direccionPCB = patota;
	nuevo_tcb->proxInstruccion = patota->tareas;

	memcpy(&(nuevo_tcb->tid), buffer + desplazamiento, sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);

	memcpy(&(nuevo_tcb->estado), buffer + desplazamiento, sizeof(char));
	desplazamiento += sizeof(char);

	memcpy(&(nuevo_tcb->posX), buffer + desplazamiento, sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);

	memcpy(&(nuevo_tcb->posY), buffer + desplazamiento, sizeof(uint32_t));

	pthread_mutex_lock(&mutexTripulantes);
	list_add(tripulantes,nuevo_tcb);
	pthread_mutex_unlock(&mutexTripulantes);

	free(buffer);

	return nuevo_tcb;
}

void enviar_proxima_tarea(TCB* tripulante, int socket_tripulante) {

	char* instrucciones_proximas;
	char* pos_delimitador;
	char* instruccion_enviar;

	instrucciones_proximas = tripulante->proxInstruccion;

	//No puedo en enviar toda la cadena, sino hasta encontrar un delimitador
	pos_delimitador = strchr(instrucciones_proximas, '|');

	if(pos_delimitador != NULL) {
		int size = pos_delimitador - instrucciones_proximas;
		instruccion_enviar = string_substring(instrucciones_proximas, 0, size);
	}
	else {
		instruccion_enviar = string_duplicate(instrucciones_proximas);
	}

	int size_instruccion_enviar = strlen(instruccion_enviar) + 1;

	tipo_tarea cod_tarea;
	if(string_contains(instruccion_enviar," "))
		cod_tarea = ENTRADA_SALIDA;
	else
		cod_tarea = COMUN;

	//Preparo paquete para enviar
	t_buffer* buffer = malloc(sizeof(t_buffer));
	buffer->size = sizeof(uint32_t) + size_instruccion_enviar;
	buffer->stream = malloc(buffer->size);

	int desplazamiento = 0;

	memcpy(buffer->stream + desplazamiento, &size_instruccion_enviar, sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);
	memcpy(buffer->stream + desplazamiento, instruccion_enviar, size_instruccion_enviar);

	send(socket_tripulante,&cod_tarea,sizeof(int),0);
	enviar_buffer(buffer, socket_tripulante);

	free(buffer->stream);
	free(buffer);
	free(instruccion_enviar);
}

void recibir_movimiento_tripulante(TCB* tripulante, int socket_tripulante) {

	void* buffer;
	uint32_t buffer_size;
	uint32_t desplazamiento = 0;

	buffer = recibir_buffer(&buffer_size, socket_tripulante);

	memcpy(&(tripulante->posX), buffer + desplazamiento, sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);

	memcpy(&(tripulante->posY), buffer + desplazamiento, sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);

	log_info(loggerMiRam, "[TRIPULANTE %d] Se movió a %d|%d", tripulante->tid, tripulante->posX, tripulante->posY);

	free(buffer);
}

void avanzar_proxima_instruccion(TCB* tripulante) {

	char* instruccion_actual = tripulante->proxInstruccion;
	char* pos_delimitador = strchr(instruccion_actual, '|');

	if(&(tripulante->proxInstruccion) == 0) {
		printf("OJOOOO\n");
		return;
	}

	if(pos_delimitador != NULL) {
		tripulante->proxInstruccion = pos_delimitador + 1;
	}
	else {
		// Voy al final de la cadena
		tripulante->proxInstruccion = instruccion_actual + strlen(instruccion_actual);
	}
}

PCB* buscar_patota(uint32_t pid) {
	bool patota_tiene_el_pid(void* pcb) {
		return ((PCB*) pcb)->pid == pid;
	}
	pthread_mutex_lock(&mutexPatotas);
	PCB* patota = list_find(patotas, patota_tiene_el_pid);
	pthread_mutex_unlock(&mutexPatotas);

	return patota;
}

void terminar_programa() {

	config_destroy(configuracionMiRam);
	list_destroy_and_destroy_elements(tripulantes,free);
	list_destroy_and_destroy_elements(patotas,free);
	log_info(loggerMiRam, "Se termina el programa...");
	log_destroy(loggerMiRam);
}

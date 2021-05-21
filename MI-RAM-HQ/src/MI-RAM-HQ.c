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

	int socket_escucha, socket_cliente;
	pthread_t hilo_receptor;

	inicializarVariables();

	socket_escucha = iniciarServidor(IP_MI_RAM,PUERTO_MI_RAM);
	log_info(loggerMiRam, "MI-RAM-HQ listo para recibir conexiones");

	while(1) {
		socket_cliente = esperar_cliente(socket_escucha);
		log_info(loggerMiRam, "Se recibio una conexion del cliente %d", socket_cliente);

		tipo_mensaje tipo_msg = recibir_operacion(socket_cliente);

		switch(tipo_msg) {

		case INICIAR_PATOTA:
			pthread_create(&hilo_receptor, NULL, recibir_datos_patota, (void*) socket_cliente);
			pthread_detach(hilo_receptor);
			break;
		case INICIAR_TRIPULANTE:
			pthread_create(&hilo_receptor, NULL, receptor_conexion_tripulante, (void*) socket_cliente);
			pthread_detach(hilo_receptor);
			break;
		default:
			log_warning(loggerMiRam, "Tipo de mensaje desconocido!!!. Cierro conexion con dicho cliente");
			close(socket_escucha);
			break;
		}
	}

	close(socket_escucha);
	//terminar_programa();
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
	log_info(loggerMiRam, "Se lee config e inician variables iniciales");
}

void* receptor_conexion_tripulante(void* _cliente) {

	TCB* tripulante;
	int socket_tripulante = (int) _cliente;

	log_info(loggerMiRam, "Se conectó un Tripulante!");
	log_info(loggerMiRam, "Primero recibo sus datos y armo TCB");

	tripulante = recibir_datos_tripulante(socket_tripulante);

	log_info(loggerMiRam, "[TRIPULANTE %d] Se ha creado TCB . Pertenece a la patota %d. Posiciones: %d|%d", tripulante->tid, tripulante->direccionPCB->pid, tripulante->posX, tripulante->posY);

	while(1) {

		log_info(loggerMiRam, "[TRIPULANTE %d] Espero proxima operacion del tripulante", tripulante->tid);
		tipo_mensaje tipo_msg = recibir_operacion(socket_tripulante);

		switch(tipo_msg) {

		case PROXIMA_TAREA:
			log_info(loggerMiRam, "[TRIPULANTE %d] Se ha recibido solicitud de proxima tarea", tripulante->tid);
			enviar_proxima_tarea(tripulante, socket_tripulante);
			log_info(loggerMiRam, "[TRIPULANTE %d] Se envio proxima tarea para el tripulante", tripulante->tid);
			break;
		case INFORMAR_MOVIMIENTO:
			log_info(loggerMiRam, "[TRIPULANTE %d] Se ha recibido un movimiento de este tripulante. Se procede a actualizar..", tripulante->tid);
			recibir_movimiento_tripulante(tripulante, socket_tripulante);
			avanzar_proxima_instruccion(tripulante);
			break;
		case -1:
			log_info(loggerMiRam, "[TRIPULANTE %d] Se ha cortado la conexionn", tripulante->tid);
			close(socket_tripulante);
			return NULL;
			break;
		default:
			log_warning(loggerMiRam, "Tipo de mensaje desconocido!!!");
			break;
		}
	}

	close(socket_tripulante);
	return NULL;
}

void* recibir_datos_patota(void* _cliente) {

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

	//leo instrucciones, va sin &, porque tareas es un punterooo
	nuevo_pcb->tareas = malloc(inst_len);
	memcpy(nuevo_pcb->tareas, buffer + desplazamiento, inst_len);

	list_add(patotas, nuevo_pcb);

	//Envio el OK
	enviar_respuesta(OK, socket_cliente);

	log_info(loggerMiRam, "Se ha creado el PCB: %d\nSus instrucciones son: %s", nuevo_pcb->pid, nuevo_pcb->tareas);
	log_info(loggerMiRam, "Se finaliza la operacion [INICIAR_PATOTA] con el cliente %d", socket_cliente);

	close(socket_cliente);
	free(buffer);
	return NULL;
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

	list_add(tripulantes,nuevo_tcb);

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

	//Preparo paquete para enviar
	t_paquete* paquete = malloc(sizeof(t_paquete));
	paquete->buffer = malloc(sizeof(t_buffer));
	int size_instruccion_enviar = strlen(instruccion_enviar) + 1;
	int desplazamiento = 0;

	paquete->codigo_operacion = OK;
	paquete->buffer->size = 2 * sizeof(uint32_t) + size_instruccion_enviar;
	paquete->buffer->stream = malloc(paquete->buffer->size);

	memcpy(paquete->buffer->stream + desplazamiento, &(tripulante->tid), sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);
	memcpy(paquete->buffer->stream + desplazamiento, &size_instruccion_enviar, sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);
	memcpy(paquete->buffer->stream + desplazamiento, instruccion_enviar, size_instruccion_enviar);

	enviar_paquete(paquete, socket_tripulante);
	eliminar_paquete(paquete);

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

	log_info(loggerMiRam, "[TRIPULANTE %d] Se ha movido a %d|%d", tripulante->tid, tripulante->posX, tripulante->posY);

	free(buffer);
}


void avanzar_proxima_instruccion(TCB* tripulante) {

	char* instruccion_actual = tripulante->proxInstruccion;
	char* pos_delimitador = strchr(instruccion_actual, '|');

	if(&(tripulante->proxInstruccion) == 0) {
		printf("OJOOOO");
		return;
	}

	if(pos_delimitador != NULL) {
		tripulante->proxInstruccion = pos_delimitador + 1;
	}
	else {
		// Voy al final de la cadenaaa
		tripulante->proxInstruccion = instruccion_actual + strlen(instruccion_actual);
	}
}

PCB* buscar_patota(uint32_t pid) {
	bool patota_tiene_el_pid(void* pcb) {
		return ((PCB*) pcb)->pid == pid;
	}
	return (PCB*) list_find(patotas, patota_tiene_el_pid);
}

void terminar_programa() {

	config_destroy(configuracionMiRam);
	list_destroy_and_destroy_elements(tripulantes,free);
	list_destroy_and_destroy_elements(patotas,free);
	log_info(loggerMiRam, "Se termina el programa...");
	log_destroy(loggerMiRam);
}

/* Temporal por el momento
void asignarTareasAPatota(PCB* patota, char* tareasEncadenadas){
	char** tareas = string_split(tareasEncadenadas,"|");
	for(int i=0; tareas[i] != NULL ;i++){
		char** tareaSpliteada = string_split(tareas[i]," ");
		char* parametros = tareaSpliteada[1];
		char** parametrosSpliteados = string_split(parametros,";");

		int tiempo = atoi(parametrosSpliteados[3]); //SI NO LO HAGO ASI, tarea->tiempo QUEDA IGUAL A 0 (NO SE POR QUÉ)
		tarea* tarea = malloc(sizeof(tarea));
		tarea->nombre = tareaSpliteada[0];
		tarea->longNombre = strlen(tarea->nombre) + 1;
		tarea->parametro = atoi(parametrosSpliteados[0]);
		tarea->posicion.posX = atoi(parametrosSpliteados[1]);
		tarea->posicion.posY = atoi(parametrosSpliteados[2]);
		tarea->tiempo = tiempo;

		list_add(patota->tareas,tarea);
	}
}*/

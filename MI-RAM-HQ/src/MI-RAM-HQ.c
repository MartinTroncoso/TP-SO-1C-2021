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

		pthread_create(&hilo_receptor, NULL, receptor_conexion, (void*)socket_cliente);
		pthread_detach(hilo_receptor);
	}

	close(socket_escucha);
	//terminar_programa();
	return EXIT_SUCCESS;
}



void* receptor_conexion(void* socket_cliente)
{
	int cliente = (int) socket_cliente;
	tipo_mensaje tipo_msg = recibir_operacion(cliente);

	switch(tipo_msg)
	{
	case INICIAR_PATOTA:
		recibir_datos_patota(cliente);
		break;
	case INICIAR_TRIPULANTE:
		recibir_datos_tripulante(cliente);
		break;
	case PROXIMA_TAREA:
		printf("Todavia no hago nothing\n");
		break;
	case INFORMAR_MOVIMIENTO:
		printf("Todavia no hago nothing\n");
		break;
	default:
		log_warning(loggerMiRam, "Tipo de mensaje desconocido!!!");
		break;
	}

	log_info(loggerMiRam, "Se cerra la conexión con el cliente %d", cliente);
	close(cliente);
	return NULL;
}

void recibir_datos_patota(int socket_cliente) {

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

	log_info(loggerMiRam, "Se ha creado el PCB: %d\nSus instrucciones son: %s", nuevo_pcb->pid, nuevo_pcb->tareas);

	free(buffer);
}

void recibir_datos_tripulante(int socket_tripulante) {

	log_info(loggerMiRam, "Se conectó un Tripulante!");

	void* buffer;
	uint32_t buffer_size;
	uint32_t desplazamiento = 0;

	TCB* nuevo_tcb = malloc(sizeof(TCB));
	nuevo_tcb->direccionPCB = 0; //por ahora
	nuevo_tcb->proxInstruccion = 0; //por ahora
	nuevo_tcb->posicion = malloc(sizeof(posicion));

	buffer = recibir_buffer(&buffer_size, socket_tripulante);

	memcpy(&(nuevo_tcb->pid), buffer + desplazamiento, sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);

	memcpy(&(nuevo_tcb->tid), buffer + desplazamiento, sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);

	memcpy(&(nuevo_tcb->estado), buffer + desplazamiento, sizeof(char));
	desplazamiento += sizeof(char);

	memcpy(&(nuevo_tcb->posicion->posX), buffer + desplazamiento, sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);

	memcpy(&(nuevo_tcb->posicion->posY), buffer + desplazamiento, sizeof(uint32_t));

	list_add(tripulantes,nuevo_tcb);

	log_info(loggerMiRam,"El Tripulante %d pertenece a la patota %d. Posiciones: %d|%d", nuevo_tcb->tid, nuevo_tcb->pid, nuevo_tcb->posicion->posX, nuevo_tcb->posicion->posY);

	free(buffer);
}

void inicializarVariables() {

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

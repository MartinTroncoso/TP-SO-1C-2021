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
}

void asignarTareasAPatota(PCB* patota, char* tareasEncadenadas){
	char** tareas = string_split(tareasEncadenadas,"|");
	for(int i=0; tareas[i] != NULL ;i++){
		char** tareaSpliteada = string_split(tareas[i]," ");
		char* parametros = tareaSpliteada[1];
		char** parametrosSpliteados = string_split(parametros,";");

		tarea* tarea = malloc(sizeof(tarea));
		tarea->nombre = tareaSpliteada[0];
		tarea->longNombre = strlen(tarea->nombre) + 1;
		tarea->parametro = atoi(parametrosSpliteados[0]);
		tarea->posicion.posX = atoi(parametrosSpliteados[1]);
		tarea->posicion.posY = atoi(parametrosSpliteados[2]);
		tarea->tiempo = atoi(parametrosSpliteados[3]); //ESTA AGARRANDO UN 0 Y NO SE POR QUÉ

		list_add(patota->tareas,tarea);
	}
}

void atenderConexionTripulantes(int socket_escucha){
	pthread_t hiloAtenderTripulantes;
	pthread_create(&hiloAtenderTripulantes,NULL,(void*) atenderConexiones,&socket_escucha);
	pthread_detach(hiloAtenderTripulantes);
}

void atenderConexiones(int socket_escucha){
	while(1){
		int socket_tripulante = esperar_cliente(socket_escucha);
		atenderComandosDiscordiador(socket_tripulante);
	}
}

void atenderTripulante(TCB* tripulante){
	printf("IdTripulante: %d		Estado: %c	Posicion: %d|%d\n",tripulante->tid,tripulante->estado,tripulante->posicion->posX,tripulante->posicion->posY);
	while(1){

	}
}

void atenderComandosDiscordiador(int socket_tripulante){
	tipo_mensaje op_code = recibir_operacion(socket_tripulante);
	switch(op_code){
	case INICIAR_PATOTA:{
		log_info(loggerMiRam,"Me llegan los datos de una patota");
		PCB* patota = malloc(sizeof(PCB));
		patota->tareas = list_create();
		patota->direccionTareas = 0; //por ahora
		uint32_t cantidadTripulantes;
		uint32_t size;
		int sizeCadenaTareas;
		void* buffer = recibir_buffer(&size,socket_tripulante);

		int desplazamiento = 0;

		memcpy(&(patota->pid), buffer + desplazamiento, sizeof(uint32_t));
		desplazamiento += sizeof(uint32_t);

		memcpy(&cantidadTripulantes, buffer + desplazamiento, sizeof(uint32_t));
		desplazamiento += sizeof(uint32_t);

		memcpy(&sizeCadenaTareas, buffer + desplazamiento, sizeof(uint32_t));
		desplazamiento += sizeof(uint32_t);

		char* tareasEncadenadas = malloc(sizeCadenaTareas);
		memcpy(tareasEncadenadas, buffer + desplazamiento, sizeCadenaTareas);

		list_add(patotas,patota);

		asignarTareasAPatota(patota,tareasEncadenadas);

		free(buffer);
		free(tareasEncadenadas);
		close(socket_tripulante);
		break;
	}
	case INICIAR_TRIPULANTE:{
		log_info(loggerMiRam,"Se conectó un Tripulante!");
		TCB* tripulante = malloc(sizeof(TCB));
		tripulante->direccionPCB = 0; //por ahora
		tripulante->proxInstruccion = 0; //por ahora
		tripulante->posicion = malloc(sizeof(posicion));
		uint32_t sizeTripulante;
		uint32_t idPatota;
		void* tripulanteSerializado = recibir_buffer(&sizeTripulante,socket_tripulante);

		uint32_t desplazamiento = 0;

		memcpy(&idPatota, tripulanteSerializado + desplazamiento, sizeof(uint32_t));
		desplazamiento += sizeof(uint32_t);

		memcpy(&(tripulante->tid), tripulanteSerializado + desplazamiento, sizeof(uint32_t));
		desplazamiento += sizeof(uint32_t);

		memcpy(&(tripulante->estado), tripulanteSerializado + desplazamiento, sizeof(char));
		desplazamiento += sizeof(char);

		memcpy(&(tripulante->posicion->posX), tripulanteSerializado + desplazamiento, sizeof(uint32_t));
		desplazamiento += sizeof(uint32_t);

		memcpy(&(tripulante->posicion->posY), tripulanteSerializado + desplazamiento, sizeof(uint32_t));

		list_add(tripulantes,tripulante);

		log_info(loggerMiRam,"El Tripulante %d pertenece a la patota %d",tripulante->tid,idPatota);

		pthread_t hiloTripulante;
		pthread_create(&hiloTripulante,NULL,(void*) atenderTripulante, tripulante);
		pthread_detach(hiloTripulante);

		free(tripulanteSerializado);

		close(socket_tripulante);
		break;
	}
	case EXPULSAR_TRIPULANTE:{
		printf("Todavia no hago nothing\n");
		close(socket_tripulante);
		break;
	}
	case PROXIMA_TAREA:{
		printf("Todavia no hago nothing\n");
		close(socket_tripulante);
		break;
	}
	case INFORMAR_MOVIMIENTO:{
		printf("Todavia no hago nothing\n");
		close(socket_tripulante);
		break;
	}
	default:{
		printf("No reconozco ese comando\n");
		close(socket_tripulante);
		break;
	}
	}
}

void terminar_programa(int socket_escucha){
	close(socket_escucha);
	log_destroy(loggerMiRam);
	config_destroy(configuracionMiRam);
	list_destroy_and_destroy_elements(tripulantes,free);
	list_destroy_and_destroy_elements(patotas,free);
}

int main(void) {

	inicializarVariables();

	int socket_escucha = iniciarServidor(IP_MI_RAM,PUERTO_MI_RAM);

	atenderConexiones(socket_escucha);

	terminar_programa(socket_escucha);

	return EXIT_SUCCESS;
}

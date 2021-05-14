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

void atenderConexionTripulantes(int socket_escucha){
	pthread_t hiloAtenderTripulantes;
	pthread_create(&hiloAtenderTripulantes,NULL,(void*) atenderConexiones,&socket_escucha);
	pthread_join(hiloAtenderTripulantes,NULL);
}

void atenderConexiones(int socket_escucha){
	while(1){
		int socket_tripulante = esperar_cliente(socket_escucha);
		atenderComandosDiscordiador(socket_tripulante);
	}
}

void atenderTripulante(TCB* tripulante){
	printf("IdTripulante: %d		Estado: %c	Posicion: %d|%d\n",
			tripulante->tid,
			tripulante->estado,
			tripulante->posicion->posX,
			tripulante->posicion->posY
	);
}

void atenderComandosDiscordiador(int socket_tripulante){
	tipo_mensaje op_code = recibir_operacion(socket_tripulante);
	switch(op_code){
	case INICIAR_PATOTA:{
		printf("Me llegan los datos de una patota.\n");
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

		printf("%s\n",tareasEncadenadas);
		free(buffer);
		free(tareasEncadenadas);
		close(socket_tripulante);
		break;
	}
	case INICIAR_TRIPULANTE:{
		printf("Me llegan los datos de un tripulante.\n");
		TCB* tripulante = malloc(sizeof(TCB));
		tripulante->direccionPCB = 0; //por ahora
		tripulante->proxInstruccion = 0; //por ahora
		tripulante->posicion = malloc(sizeof(posicion));
		uint32_t sizeTripulante;
		void* tripulanteSerializado = recibir_buffer(&sizeTripulante,socket_tripulante);

		uint32_t desplazamiento = 0;

		memcpy(&(tripulante->tid),tripulanteSerializado + desplazamiento,sizeof(uint32_t));
		desplazamiento += sizeof(uint32_t);

		memcpy(&(tripulante->estado),tripulanteSerializado + desplazamiento,sizeof(char));
		desplazamiento += sizeof(char);

		memcpy(&(tripulante->posicion->posX),tripulanteSerializado + desplazamiento,sizeof(uint32_t));
		desplazamiento += sizeof(uint32_t);

		memcpy(&(tripulante->posicion->posY),tripulanteSerializado + desplazamiento,sizeof(uint32_t));

		pthread_t hiloTripulante;
		pthread_create(&hiloTripulante,NULL,(void*) atenderTripulante, tripulante);
		pthread_join(hiloTripulante,NULL);

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

int main(void) {

	inicializarVariables();

	int socket_escucha = iniciarServidor(IP_MI_RAM,PUERTO_MI_RAM);

	atenderConexiones(socket_escucha);

	close(socket_escucha);
	log_destroy(loggerMiRam);
	config_destroy(configuracionMiRam);

	return EXIT_SUCCESS;
}

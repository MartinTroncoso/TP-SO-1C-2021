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
	PUERTO = config_get_string_value(configuracionMiRam,"PUERTO");
	ESQUEMA_MEMORIA = config_get_string_value(configuracionMiRam,"ESQUEMA_MEMORIA");
	PATH_SWAP = config_get_string_value(configuracionMiRam,"PATH_SWAP");
	ALGORITMO_REEMPLAZO = config_get_string_value(configuracionMiRam,"ALGORITMO_REEMPLAZO");
	tripulantes = list_create();
	patotas = list_create();
	socket_servidor = iniciarServidor("127.0.0.1",PUERTO);
	log_info(loggerMiRam, "MI-RAM-HQ listo para recibir al Discordiador");
	socket_discordiador = esperar_cliente(socket_servidor);
	printf("SE CONECTÓ EL DISCORDIADOR!\n");
}

void atenderTripulante(TCB* tripulante){
	printf("IdTripulante: %d\n",tripulante->tid);
	printf("Estado: %c\n",tripulante->estado);
	printf("Posicion: %d|%d\n",tripulante->posicion->coordenadaX,tripulante->posicion->coordenadaY);
}

void atenderComandosDiscordiador(){
	while(1){
		int op_code = recibir_operacion(socket_discordiador);
		switch(op_code){
		case INICIAR_PATOTA:{
			PCB* patota = malloc(sizeof(PCB));
			uint32_t cantidadTripulantes;
			uint32_t size;
			void* buffer = recibir_buffer(&size,socket_discordiador);
			memcpy(&(patota->pid), buffer, sizeof(uint32_t));
			memcpy(&cantidadTripulantes, buffer + sizeof(uint32_t), sizeof(uint32_t));
			patota->direccionTareas = 0; //por ahora
			list_add(patotas,patota);

			for(uint32_t i=0; i<cantidadTripulantes; i++){
				TCB* tripulante = malloc(sizeof(TCB));
				tripulante->posicion = malloc(sizeof(coordenadasTripulante));
				uint32_t sizeTripulante;
				void* tripulanteSerializado = recibir_buffer(&sizeTripulante,socket_discordiador);

				uint32_t desplazamiento = 0;
				memcpy(&(tripulante->tid),tripulanteSerializado + desplazamiento,sizeof(uint32_t));
				desplazamiento+=sizeof(uint32_t);
				memcpy(&(tripulante->estado),tripulanteSerializado + desplazamiento,sizeof(char));
				desplazamiento+=sizeof(char);
				memcpy(&(tripulante->posicion->coordenadaX),tripulanteSerializado + desplazamiento,sizeof(uint32_t));
				desplazamiento+=sizeof(uint32_t);
				memcpy(&(tripulante->posicion->coordenadaY),tripulanteSerializado + desplazamiento,sizeof(uint32_t));
				tripulante->direccionPCB = 0; //por ahora
				tripulante->proxInstruccion = 0; //por ahora
				list_add(tripulantes,tripulante);

				pthread_t hiloTripulante;
				pthread_create(&hiloTripulante,NULL,(void*) atenderTripulante, tripulante);
				pthread_join(hiloTripulante,NULL);

				free(tripulanteSerializado);
			}
			break;
		}
		case EXPULSAR_TRIPULANTE:{
			printf("Todavia no hago nothing\n");
			break;
		}
		default:{
			printf("No reconozco ese comando\n");
			break;
		}
		}
	}
}

int main(void) {

	inicializarVariables();
//	void* memoria = malloc(TAMANIO_MEMORIA);

//	void iterator(char* value)
//	{
//		printf("%s\n", value);
//	}
//
//	t_list* lista;
//	while(1)
//	{
//		int cod_op = recibir_operacion(socket_discordiador);
//		switch(cod_op)
//		{
//		case MENSAJE:
//			recibir_mensaje(socket_discordiador);
//			break;
//		case PAQUETE:
//			lista = recibir_paquete(socket_discordiador);
//			printf("ME LLEGARON LOS SIGUIENTES VALORES:\n");
//			list_iterate(lista, (void*) iterator);
//			break;
//		case -1:
//			log_error(loggerMiRam, "SE DESCONECTÓ EL DISCORDIADOR. FINALIZO");
//			return EXIT_FAILURE;
//		default:
//			log_warning(loggerMiRam, "Operacion desconocida. No quieras meter la pata");
//			break;
//		}
//	}
//

	atenderComandosDiscordiador();

	close(socket_servidor);
	close(socket_discordiador);
	log_destroy(loggerMiRam);
	config_destroy(configuracionMiRam);

	return EXIT_SUCCESS;
}

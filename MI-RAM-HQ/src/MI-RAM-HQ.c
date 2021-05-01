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

	socket_servidor = iniciarServidor("127.0.0.1",PUERTO);
	log_info(loggerMiRam, "MI-RAM-HQ listo para recibir al Discordiador");
	socket_discordiador = esperar_cliente(socket_servidor);
	printf("SE CONECTÓ EL DISCORDIADOR!\n");
}

//void iniciar_tripulante(t_tripulante tripulante){
//	///
//}

int main(void) {

	inicializarVariables();
//	void* memoria = malloc(TAMANIO_MEMORIA);

	void iterator(char* value)
	{
		printf("%s\n", value);
	}

	t_list* lista;
	while(1)
	{
		int cod_op = recibir_operacion(socket_discordiador);
		switch(cod_op)
		{
		case MENSAJE:
			recibir_mensaje(socket_discordiador);
			break;
		case PAQUETE:
			lista = recibir_paquete(socket_discordiador);
			printf("ME LLEGARON LOS SIGUIENTES VALORES:\n");
			list_iterate(lista, (void*) iterator);
			break;
		case -1:
			log_error(loggerMiRam, "SE DESCONECTÓ EL DISCORDIADOR. FINALIZO");
			return EXIT_FAILURE;
		default:
			log_warning(loggerMiRam, "Operacion desconocida. No quieras meter la pata");
			break;
		}
	}

	log_destroy(loggerMiRam);
	config_destroy(configuracionMiRam);

	return EXIT_SUCCESS;
}

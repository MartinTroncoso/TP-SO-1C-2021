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

void leerConfiguracion(t_config* config)
{
	//NUMERICOS
	TAMANIO_MEMORIA = config_get_int_value(config,"TAMANIO_MEMORIA");
	TAMANIO_PAGINA = config_get_int_value(config,"TAMANIO_PAGINA");
	TAMANIO_SWAP = config_get_int_value(config,"TAMANIO_SWAP");
	//STRINGS
	PUERTO = config_get_string_value(config,"PUERTO");
	ESQUEMA_MEMORIA = config_get_string_value(config,"ESQUEMA_MEMORIA");
	PATH_SWAP = config_get_string_value(config,"PATH_SWAP");
	ALGORITMO_REEMPLAZO = config_get_string_value(config,"ALGORITMO_REEMPLAZO");
}

void logearConfiguracion(t_log* logger)
{
	log_info(logger, "%d",TAMANIO_MEMORIA);
	log_info(logger,ESQUEMA_MEMORIA);
	log_info(logger, "%d",TAMANIO_PAGINA);
	log_info(logger,"%d",TAMANIO_SWAP);
	log_info(logger,PATH_SWAP);
	log_info(logger,ALGORITMO_REEMPLAZO);
	log_info(logger,"%d",PUERTO);
}

int main(void) {
	t_config* configuracionMiRam = config_create("/home/utnso/workspace/tp-2021-1c-No-C-Aprueba-/MI-RAM-HQ/miram.config");
	t_log* loggerMiRam = log_create("/home/utnso/workspace/tp-2021-1c-No-C-Aprueba-/MI-RAM-HQ/miram.log", "MIRA-RAM-HQ", 1, LOG_LEVEL_INFO);
	leerConfiguracion(configuracionMiRam);
	logearConfiguracion(loggerMiRam);

	void iterator(char* value)
	{
		printf("%s\n", value);
	}

	int server_fd = iniciarServidor("127.0.0.1",PUERTO);
	log_info(loggerMiRam, "MI-RAM-HQ listo para recibir al Discordiador");
	int cliente_fd = esperar_cliente(server_fd);
	printf("SE CONECTÓ EL DISCORDIADOR!\n");

	t_list* lista;
	while(1)
	{
		int cod_op = recibir_operacion(cliente_fd);
		switch(cod_op)
		{
		case MENSAJE:
			recibir_mensaje(cliente_fd);
			break;
		case PAQUETE:
			lista = recibir_paquete(cliente_fd);
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

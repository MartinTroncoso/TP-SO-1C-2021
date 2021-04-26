/*
 ============================================================================
 Name        : I-MONGO-STORE.c
 Author      : 
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include "I-MONGO-STORE.h"

void leerConfiguracion(t_config* config)
{
	//NUMERICOS
	TIEMPO_SINCRONIZACION = config_get_int_value(config,"TIEMPO_SINCRONIZACION");
	//STRINGS
	PUNTO_MONTAJE = config_get_string_value(config,"PUNTO_MONTAJE");
	PUERTO = config_get_string_value(config,"PUERTO");
}

void logearConfiguracion(t_log* logger)
{
	log_info(logger, PUNTO_MONTAJE);
	log_info(logger, PUERTO);
	log_info(logger,"%d",TIEMPO_SINCRONIZACION);
}

int main(void) {
	t_config* configuracionMongo = config_create("/home/utnso/workspace/tp-2021-1c-No-C-Aprueba-/I-MONGO-STORE/mongo.config");
	t_log* loggerMongo = log_create("/home/utnso/workspace/tp-2021-1c-No-C-Aprueba-/I-MONGO-STORE/mongo.log", "I-MONGO-STORE", 1, LOG_LEVEL_INFO);
	leerConfiguracion(configuracionMongo);
	logearConfiguracion(loggerMongo);

	void iterator(char* value)
	{
		printf("%s\n", value);
	}

	int server_fd = iniciarServidor("127.0.0.1",PUERTO);
	log_info(loggerMongo, "I-MONGO-STORE listo para recibir al Discordiador");
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
			log_error(loggerMongo, "SE DESCONECTÓ EL DISCORDIADOR. FINALIZO");
			return EXIT_FAILURE;
		default:
			log_warning(loggerMongo, "Operacion desconocida. No quieras meter la pata");
			break;
		}
	}

	log_destroy(loggerMongo);
	config_destroy(configuracionMongo);

	return EXIT_SUCCESS;
}



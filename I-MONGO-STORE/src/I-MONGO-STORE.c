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

void inicializarVariables(){
	configuracionMongo = config_create("/home/utnso/workspace/tp-2021-1c-No-C-Aprueba-/I-MONGO-STORE/mongo.config");
	loggerMongo = log_create("/home/utnso/workspace/tp-2021-1c-No-C-Aprueba-/I-MONGO-STORE/mongo.log", "I-MONGO-STORE", 1, LOG_LEVEL_INFO);
	TIEMPO_SINCRONIZACION = config_get_int_value(configuracionMongo,"TIEMPO_SINCRONIZACION");
	PUNTO_MONTAJE = config_get_string_value(configuracionMongo,"PUNTO_MONTAJE");
	PUERTO = config_get_string_value(configuracionMongo,"PUERTO");

	socket_servidor = iniciarServidor("127.0.0.1",PUERTO);
	log_info(loggerMongo, "I-MONGO-STORE listo para recibir al Discordiador");
	socket_discordiador = esperar_cliente(socket_servidor);
	printf("SE CONECTÓ EL DISCORDIADOR!\n");
}

int main(void) {
	inicializarVariables();

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



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

int main(void) {
	t_config* configuracionMongo = config_create("/home/utnso/workspace/tp-2021-1c-No-C-Aprueba-/I-MONGO-STORE/mongo.config");
	t_log* loggerMongo = log_create("/home/utnso/workspace/tp-2021-1c-No-C-Aprueba-/I-MONGO-STORE/mongo.log", "I-MONGO-STORE", 1, LOG_LEVEL_INFO);
	leerConfiguracion(configuracionMongo);
	logearConfiguracion(loggerMongo);
	holiwis();
	comoAndas();
	bien();
	comoAndas();
	return EXIT_SUCCESS;
}

void leerConfiguracion(t_config* config)
{
	//NUMERICOS
	TAMANIO_MEMORIA = config_get_int_value(config,"TAMANIO_MEMORIA");
	TAMANIO_PAGINA = config_get_int_value(config,"TAMANIO_PAGINA");
	TAMANIO_SWAP = config_get_int_value(config,"TAMANIO_SWAP");
	PUERTO = config_get_int_value(config,"PUERTO");
	//STRINGS
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

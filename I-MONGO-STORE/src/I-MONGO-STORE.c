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
	PUERTO = config_get_int_value(config,"PUERTO");
	TIEMPO_SINCRONIZACION = config_get_int_value(config,"TIEMPO_SINCRONIZACION");
	//STRINGS
	PUNTO_MONTAJE = config_get_string_value(config,"PUNTO_MONTAJE");
}

void logearConfiguracion(t_log* logger)
{
	log_info(logger,PUNTO_MONTAJE);
	log_info(logger, "%d",PUERTO);
	log_info(logger,"%d",TIEMPO_SINCRONIZACION);
}

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
	t_config* configuracionMiRam = config_create("/home/utnso/workspace/tp-2021-1c-No-C-Aprueba-/MI-RAM-HQ/miram.config");
	t_log* loggerMiRam = log_create("/home/utnso/workspace/tp-2021-1c-No-C-Aprueba-/MI-RAM-HQ/miram.log", "MIRA-RAM-HQ", 1, LOG_LEVEL_INFO);
	leerConfiguracion(configuracionMiRam);
	logearConfiguracion(loggerMiRam);
	holiwis();
	comoAndas();
	bien();
	holiwis();
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

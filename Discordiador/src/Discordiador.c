/*
 ============================================================================
 Name        : Discordiador.c
 Author      : 
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include "Discordiador.h"


int main(void)
{

	t_config* configuracionDiscordiador = config_create("/home/utnso/workspace/tp-2021-1c-No-C-Aprueba-/Discordiador/discordiador.config");
	t_log* loggerDiscordiador = log_create("/home/utnso/workspace/tp-2021-1c-No-C-Aprueba-/Discordiador/discordiador.log", "Discordiador", 1, LOG_LEVEL_INFO);
	leerConfiguracion(configuracionDiscordiador);
	logearConfiguracion(loggerDiscordiador);
	holiwis();
	comoAndas();
	bien();
	return EXIT_SUCCESS;
}

void leerConfiguracion(t_config* configuracion)
{
	//STRINGS
	IP_MI_RAM = config_get_string_value(configuracion,"IP_MI_RAM_HQ");
	IP_I_MONGO_STORE = config_get_string_value(configuracion,"IP_I_MONGO_STORE");
	ALGORITMO = config_get_string_value(configuracion,"ALGORITMO");
	//NUMERICOS
	PUERTO_MI_RAM = config_get_int_value(configuracion,"PUERTO_MI_RAM_HQ");
	PUERTO_I_MONGO_STORE = config_get_int_value(configuracion,"PUERTO_I_MONGO_STORE");
	GRADO_MULTITAREA = config_get_int_value(configuracion,"GRADO_MULTITAREA");
	QUANTUM = config_get_int_value(configuracion,"QUANTUM");
	DURACION_SABOTAJE = config_get_int_value(configuracion,"DURACION_SABOTAJE");
	RETARDO_CICLO_CPU = config_get_int_value(configuracion,"RETARDO_CICLO_CPU");
}

void logearConfiguracion(t_log* log)
{
	log_info(log, IP_MI_RAM);
	log_info(log, IP_I_MONGO_STORE);
	log_info(log, ALGORITMO);
	log_info(log, "%d",PUERTO_MI_RAM);
	log_info(log, "%d",PUERTO_I_MONGO_STORE);
	log_info(log, "%d",GRADO_MULTITAREA);
	log_info(log, "%d",QUANTUM);
	log_info(log, "%d",DURACION_SABOTAJE);
	log_info(log, "%d",RETARDO_CICLO_CPU);
}

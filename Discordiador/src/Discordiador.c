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

void leer_consola(t_log* logger)
{
	char* leido;
	leido = readline(">");

	while(strcmp(leido,"")!=0){
		log_info(logger,leido);
		free(leido);
		leido = readline(">");
	}

	free(leido);
}

void paquete(int conexion)
{
	char* leido;
	t_paquete* paquete = crear_paquete();
	leido = readline(">");
	while(strcmp(leido,"")!=0){
		agregar_a_paquete(paquete,leido,strlen(leido) + 1);
		free(leido);
		leido = readline(">");
	}

	free(leido);
	enviar_paquete(paquete,conexion);

	eliminar_paquete(paquete);
}

void terminar_programa(int conexion, t_log* logger, t_config* config)
{
	close(conexion);
	log_destroy(logger);
	config_destroy(config);
}

void leerConfiguracion(t_config* configuracion)
{
	//STRINGS
	IP_MI_RAM = config_get_string_value(configuracion,"IP_MI_RAM_HQ");
	IP_I_MONGO_STORE = config_get_string_value(configuracion,"IP_I_MONGO_STORE");
	ALGORITMO = config_get_string_value(configuracion,"ALGORITMO");
	PUERTO_MI_RAM = config_get_string_value(configuracion,"PUERTO_MI_RAM_HQ");
	PUERTO_I_MONGO_STORE = config_get_string_value(configuracion,"PUERTO_I_MONGO_STORE");
	//NUMERICOS
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
	log_info(log, PUERTO_MI_RAM);
	log_info(log, PUERTO_I_MONGO_STORE);
	log_info(log, "%d",GRADO_MULTITAREA);
	log_info(log, "%d",QUANTUM);
	log_info(log, "%d",DURACION_SABOTAJE);
	log_info(log, "%d",RETARDO_CICLO_CPU);
}

int main(void)
{

	t_config* configuracionDiscordiador = config_create("/home/utnso/workspace/tp-2021-1c-No-C-Aprueba-/Discordiador/discordiador.config");
	t_log* loggerDiscordiador = log_create("/home/utnso/workspace/tp-2021-1c-No-C-Aprueba-/Discordiador/discordiador.log", "Discordiador", 1, LOG_LEVEL_INFO);
	leerConfiguracion(configuracionDiscordiador);
	logearConfiguracion(loggerDiscordiador);

	//EL DISCORDIADOR SE CONECTA A MI-RAM (HAY QUE CORRERLO A ESTE ANTES)
	int conexion = crearConexionCliente(IP_I_MONGO_STORE, PUERTO_I_MONGO_STORE);

	//SE ENVIA A MI-RAM EL VALOR
	enviar_mensaje(ALGORITMO,conexion);

	//SE ARMA UN PAQUETE CON LOS MENSAJES QUE SE ESCRIBAN EN CONSOLA Y CUANDO SE APRIETA 'ENTER' SE MANDA TODO JUNTO
	paquete(conexion);

	terminar_programa(conexion, loggerDiscordiador, configuracionDiscordiador);

	return EXIT_SUCCESS;
}



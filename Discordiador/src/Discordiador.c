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

void leer_consola(t_dictionary* diccionario,t_log* logger)
{
	char* leido;
	leido = readline(">");

	while(strcmp(leido,"")!=0){
		log_info(logger,leido);
		switch((int) dictionary_get(diccionario,strtok(leido," ")))
		{
		case 1:
			printf("Comando leido: 	INICIAR_PATOTA\n");
			break;
		case 2:
			printf("Comando leido: 	LISTAR_TRIPULANTES\n");
			break;
		case 3:
			printf("Comando leido: 	EXPULSAR_TRIPULANTE\n");
			break;
		case 4:
			printf("Comando leido: 	INICIAR_PLANIFICACION\n");
			break;
		case 5:
			printf("Comando leido: 	PAUSAR_PLANIFICACION\n");
			break;
		case 6:
			printf("Comando leido: 	OBTENER_BITACORA\n");
			break;
		default:
			printf("Comando no reconocido\n");
			break;
		}
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

//DICCIONARIO PARA LEER LOS COMANDOS ESCRITOS POR CONSOLA
void crearDiccionarioComandos(t_dictionary* diccionario)
{
	dictionary_put(diccionario,"INICIAR_PATOTA",(int*) 1);
	dictionary_put(diccionario,"LISTAR_TRIPULANTES",(int*) 2);
	dictionary_put(diccionario,"EXPULSAR_TRIPULANTE",(int*) 3);
	dictionary_put(diccionario,"INICIAR_PLANIFICACION",(int*) 4);
	dictionary_put(diccionario,"PAUSAR_PLANIFICACION",(int*) 5);
	dictionary_put(diccionario,"OBTENER_BITACORA",(int*) 6);

	//printf("diccionario %d\n", (int) dictionary_get(diccionario,"OBTENER_BITACORA"));
}

int main(void)
{

	t_config* configuracionDiscordiador = config_create("/home/utnso/workspace/tp-2021-1c-No-C-Aprueba-/Discordiador/discordiador.config");
	t_log* loggerDiscordiador = log_create("/home/utnso/workspace/tp-2021-1c-No-C-Aprueba-/Discordiador/discordiador.log", "Discordiador", 1, LOG_LEVEL_INFO);
	t_dictionary* diccionarioDiscordiador = dictionary_create();
	crearDiccionarioComandos(diccionarioDiscordiador);
	leerConfiguracion(configuracionDiscordiador);
	logearConfiguracion(loggerDiscordiador);

	leer_consola(diccionarioDiscordiador,loggerDiscordiador);

	//EL DISCORDIADOR SE CONECTA A MI-RAM (HAY QUE CORRERLO A ESTE ANTES)
	int conexion = crearConexionCliente(IP_I_MONGO_STORE, PUERTO_I_MONGO_STORE);

	//SE ENVIA A MI-RAM EL VALOR
	enviar_mensaje(ALGORITMO,conexion);

	//SE ARMA UN PAQUETE CON LOS MENSAJES QUE SE ESCRIBAN EN CONSOLA Y CUANDO SE APRIETA 'ENTER' SE MANDA TODO JUNTO
	paquete(conexion);

	terminar_programa(conexion, loggerDiscordiador, configuracionDiscordiador);

	return EXIT_SUCCESS;
}



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

void inicializarVariables(){
	configuracionDiscordiador = config_create("/home/utnso/workspace/tp-2021-1c-No-C-Aprueba-/Discordiador/discordiador.config");
	loggerDiscordiador = log_create("/home/utnso/workspace/tp-2021-1c-No-C-Aprueba-/Discordiador/discordiador.log","Discordiador",1,LOG_LEVEL_INFO);
	diccionarioDiscordiador = dictionary_create();
	IP_MI_RAM = config_get_string_value(configuracionDiscordiador,"IP_MI_RAM_HQ");
	IP_I_MONGO_STORE = config_get_string_value(configuracionDiscordiador,"IP_I_MONGO_STORE");
	ALGORITMO = config_get_string_value(configuracionDiscordiador,"ALGORITMO");
	PUERTO_MI_RAM = config_get_string_value(configuracionDiscordiador,"PUERTO_MI_RAM_HQ");
	PUERTO_I_MONGO_STORE = config_get_string_value(configuracionDiscordiador,"PUERTO_I_MONGO_STORE");
	GRADO_MULTITAREA = config_get_int_value(configuracionDiscordiador,"GRADO_MULTITAREA");
	QUANTUM = config_get_int_value(configuracionDiscordiador,"QUANTUM");
	DURACION_SABOTAJE = config_get_int_value(configuracionDiscordiador,"DURACION_SABOTAJE");
	RETARDO_CICLO_CPU = config_get_int_value(configuracionDiscordiador,"RETARDO_CICLO_CPU");
	crearDiccionarioComandos(diccionarioDiscordiador);

	socket_cliente = crearConexionCliente(IP_I_MONGO_STORE, PUERTO_I_MONGO_STORE);
}

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

void leer_consola(t_dictionary* diccionario,t_log* logger)
{
	char* palabras;
	char* leido;
	leido = readline(">");
	while(strcmp(leido,"")!=0){
		log_info(logger,leido);
		switch((int) dictionary_get(diccionario,strtok(leido," ")))
		{
		case 1:
			printf("Comando leido: 	INICIAR_PATOTA\n");
			palabras = leido;
			partirCadena(palabras);
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

void partirCadena(char* cadena)
{
	comandoIniciarPatota* parametrosPatota = malloc(sizeof(comandoIniciarPatota));
	t_list* listaCoordenadas = list_create();
	char* token = strtok(NULL," ");
	if(atoi(token)!=0)
	{
		parametrosPatota->cantidadTripulantes = atoi(token);
	}else
	{
		printf("Error leyendo cantidadTripulantes\n");
		token = NULL;
	}
	//parametrosPatota->cantidadTripulantes = atoi(token);
	token = strtok(NULL," ");
	if(token!=NULL)
	{

		parametrosPatota->rutaDeTareas = token;
		token = strtok(NULL," |");
		printf("%d\n",parametrosPatota->cantidadTripulantes);
		printf("%s\n",parametrosPatota->rutaDeTareas);
		for(int i = 0;i<parametrosPatota->cantidadTripulantes;i++)
		{
			if(token!=NULL)
			{
				coordenadasTripulante* posicionTripulante=malloc(sizeof(coordenadasTripulante));
				if(strcmp(token,"0")==0)
				{
					posicionTripulante->coordenadaX = 0;
					token = strtok(NULL," |");
				}else if(atoi(token)!=0)
				{
					posicionTripulante->coordenadaX = atoi(token);
					token = strtok(NULL," |");
				}else
				{
					printf("Coordenadas mal ingresadas X\n");
				}
				/*posicionTripulante->coordenadaX = atoi(token);
				token = strtok(NULL," |");*/
				if(strcmp(token,"0")==0)
				{
					posicionTripulante->coordenadaY = 0;
					token = strtok(NULL," |");
				}else if(atoi(token)!=0)
				{
					posicionTripulante->coordenadaY = atoi(token);
					token = strtok(NULL," |");
				}else
				{
					printf("Coordenadas mal ingresadas Y\n");
				}
				printf("Posicion x e y %d %d\n",posicionTripulante->coordenadaX,posicionTripulante->coordenadaY);
				list_add(listaCoordenadas,posicionTripulante);
			}else
			{
				coordenadasTripulante* posicionTripulante = malloc(sizeof(coordenadasTripulante));
				posicionTripulante->coordenadaX=0;
				posicionTripulante->coordenadaY=0;
				printf("Posicion 0|0 es %d %d\n",posicionTripulante->coordenadaX,posicionTripulante->coordenadaY);
				list_add(listaCoordenadas,posicionTripulante);
			}
		}
}

	/*while(token!=NULL)
	{
		printf("%s\n",token);
		token = strtok(NULL," ");
	}*/
}

int main(void)
{
	inicializarVariables();

	char palabras[] = "COMANDO 5 1|4 1|7";
	char* token = strtok(palabras," |");
	while(token!=NULL)
	{
		printf("%s\n",token);
		token = strtok(NULL," |");
	}

	leer_consola(diccionarioDiscordiador,loggerDiscordiador);

	//SE ENVIA A MI-RAM EL VALOR
	enviar_mensaje(ALGORITMO,socket_cliente);

	//SE ARMA UN PAQUETE CON LOS MENSAJES QUE SE ESCRIBAN EN CONSOLA Y CUANDO SE APRIETA 'ENTER' SE MANDA TODO JUNTO
	paquete(socket_cliente);

	terminar_programa(socket_cliente, loggerDiscordiador, configuracionDiscordiador);

	return EXIT_SUCCESS;
}



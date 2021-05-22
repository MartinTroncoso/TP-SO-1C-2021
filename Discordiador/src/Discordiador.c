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
	diccionarioComandos= dictionary_create();
	diccionarioTareas = dictionary_create();

	IP_MI_RAM = config_get_string_value(configuracionDiscordiador,"IP_MI_RAM_HQ");
	IP_I_MONGO_STORE = config_get_string_value(configuracionDiscordiador,"IP_I_MONGO_STORE");
	ALGORITMO = config_get_string_value(configuracionDiscordiador,"ALGORITMO");
	PUERTO_MI_RAM = config_get_string_value(configuracionDiscordiador,"PUERTO_MI_RAM_HQ");
	PUERTO_I_MONGO_STORE = config_get_string_value(configuracionDiscordiador,"PUERTO_I_MONGO_STORE");
	GRADO_MULTITAREA = config_get_int_value(configuracionDiscordiador,"GRADO_MULTITAREA");
	QUANTUM = config_get_int_value(configuracionDiscordiador,"QUANTUM");
	DURACION_SABOTAJE = config_get_int_value(configuracionDiscordiador,"DURACION_SABOTAJE");
	RETARDO_CICLO_CPU = config_get_int_value(configuracionDiscordiador,"RETARDO_CICLO_CPU");
	crearDiccionarioComandos(diccionarioComandos);
	crearDiccionarioTareasEntradaSalida(diccionarioTareas);
	tripulantes = list_create();
	patotas = list_create();
	colaReady = list_create();

	idTripulante = 1;
	idPatota = 1;

	sem_init(&mutexTripulantes,0,1);
	sem_init(&multiprocesamiento,0,GRADO_MULTITAREA);
}

void crearDiccionarioComandos(t_dictionary* diccionario)
{
	dictionary_put(diccionario,"INICIAR_PATOTA",(int*) 1);
	dictionary_put(diccionario,"LISTAR_TRIPULANTES",(int*) 2);
	dictionary_put(diccionario,"EXPULSAR_TRIPULANTE",(int*) 3);
	dictionary_put(diccionario,"INICIAR_PLANIFICACION",(int*) 4);
	dictionary_put(diccionario,"PAUSAR_PLANIFICACION",(int*) 5);
	dictionary_put(diccionario,"OBTENER_BITACORA",(int*) 6);
}

void crearDiccionarioTareasEntradaSalida(t_dictionary* diccionario)
{
	dictionary_put(diccionario,"GENERAR_OXIGENO",(int*) 1);
	dictionary_put(diccionario,"CONSUMIR_OXIGENO",(int*) 2);
	dictionary_put(diccionario,"GENERAR_COMIDA",(int*) 3);
	dictionary_put(diccionario,"CONSUMIR_COMIDA",(int*) 4);
	dictionary_put(diccionario,"GENERAR_BASURA",(int*) 5);
	dictionary_put(diccionario,"DESCARTAR_BASURA",(int*) 6);
}

t_algoritmo getAlgoritmoPlanificacion(){
	if(strcmp(ALGORITMO,"FIFO") == 0){
		return FIFO;
	}
	else
	{
		if(strcmp(ALGORITMO,"RR") == 0){
			return RR;
		}
	}
	log_info(loggerDiscordiador,"No se ingresó un algoritmo válido, se elige FIFO");
	return FIFO;
}

void ingresar_comandos()
{
	char** palabras;
	char* comando = readline(">");

	while(1){
		palabras = string_split(comando, " ");
		switch((int) dictionary_get(diccionarioComandos,palabras[0]))
		{
		case 1:{
			//INICIAR_PATOTA [cant_tripulantes] [path] [pos1] [pos2] ...
			//PRIMER COMANDO A MANDAR

			if(palabras[1]!=NULL && palabras[2]!=NULL){
				t_iniciar_patota* datosPatota = obtenerDatosPatota(palabras);
				iniciarPatota(datosPatota);

				list_destroy(datosPatota->coordenadasTripulantes);
				free(datosPatota);
			}
			else
			{
				log_info(loggerDiscordiador,"Metiste mal el comando");
			}
			break;
		}
		case 2:{
			//LISTAR_TRIPULANTES
			listarTripulantes();
			break;
		}
		case 3:{
			//EXPULSAR_TRIPULANTE [idTripulante]
			expulsarTripulante(idTripulante);
			break;
		}
		case 4:{
			//INICIAR_PLANIFICACION
			iniciarPlanificacion();
			break;
		}
		case 5:{
			//PAUSAR_PLANIFICACION
			pausarPlanificacion();
			break;
		}
		case 6:{
			//OBTENER_BITACORA [idTripulante]
			obtenerBitacora((uint32_t) atoi(palabras[1]));
			break;
		}
		default:
			printf("Comando no reconocido\n");
			break;
		}
		free(comando);
		comando = readline(">");
	}

	free(comando);
}

void terminar_programa()
{
	log_destroy(loggerDiscordiador);
	config_destroy(configuracionDiscordiador);
	list_destroy_and_destroy_elements(tripulantes,free);
	list_destroy_and_destroy_elements(patotas,free);
	dictionary_destroy(diccionarioComandos);
	dictionary_destroy(diccionarioTareas);
}

int main(void)
{
	inicializarVariables();

	ingresar_comandos();

	terminar_programa();

	return EXIT_SUCCESS;
}

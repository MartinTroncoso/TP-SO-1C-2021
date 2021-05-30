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
	IP_DISCORDIADOR = config_get_string_value(configuracionDiscordiador,"IP_DISCORDIADOR");
	ALGORITMO = config_get_string_value(configuracionDiscordiador,"ALGORITMO");
	PUERTO_MI_RAM = config_get_string_value(configuracionDiscordiador,"PUERTO_MI_RAM_HQ");
	PUERTO_I_MONGO_STORE = config_get_string_value(configuracionDiscordiador,"PUERTO_I_MONGO_STORE");
	PUERTO_DISCORDIADOR = config_get_string_value(configuracionDiscordiador,"PUERTO_DISCORDIADOR");
	GRADO_MULTITAREA = config_get_int_value(configuracionDiscordiador,"GRADO_MULTITAREA");
	QUANTUM = config_get_int_value(configuracionDiscordiador,"QUANTUM");
	DURACION_SABOTAJE = config_get_int_value(configuracionDiscordiador,"DURACION_SABOTAJE");
	RETARDO_CICLO_CPU = config_get_int_value(configuracionDiscordiador,"RETARDO_CICLO_CPU");
	crearDiccionarioComandos(diccionarioComandos);
	crearDiccionarioTareasEntradaSalida(diccionarioTareas);
	tripulantes = list_create();
	patotas = list_create();
	colaReady = list_create();
	colaExec = list_create();
	colaBlockIO = list_create();
	colaExit = list_create();

	idTripulante = 1;
	idPatota = 1;
	planificacionActivada = false;
	planificacionFueActivadaAlgunaVez = false; //PARA QUE SE SI SE PAUSA Y SE CREA UNA PATOTA, LOS TRIPULANTES QUEDEN EN NEW Y NO EN READY

	pthread_mutex_init(&mutexTripulantes,NULL);
	pthread_mutex_init(&mutexColaReady,NULL);
	pthread_mutex_init(&mutexColaExec,NULL);
	pthread_mutex_init(&mutexColaExit,NULL);
	pthread_mutex_init(&mutexColaBlockIO,NULL);
	pthread_mutex_init(&mutexActivarPlanificacion,NULL);
	pthread_mutex_init(&mutexEjecutarIO,NULL);
}

void crearDiccionarioComandos(t_dictionary* diccionario){
	dictionary_put(diccionario,"INICIAR_PATOTA",(int*) 1);
	dictionary_put(diccionario,"LISTAR_TRIPULANTES",(int*) 2);
	dictionary_put(diccionario,"EXPULSAR_TRIPULANTE",(int*) 3);
	dictionary_put(diccionario,"INICIAR_PLANIFICACION",(int*) 4);
	dictionary_put(diccionario,"PAUSAR_PLANIFICACION",(int*) 5);
	dictionary_put(diccionario,"OBTENER_BITACORA",(int*) 6);
}

void crearDiccionarioTareasEntradaSalida(t_dictionary* diccionario){
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
				free(datosPatota->rutaDeTareas);
				free(datosPatota);
			}
			else
				log_info(loggerDiscordiador,"Metiste mal el comando");
			break;
		}
		case 2:{
			//LISTAR_TRIPULANTES
			listarTripulantes();
			break;
		}
		case 3:{
			//EXPULSAR_TRIPULANTE [idTripulante]
			uint32_t idTripulanteAExpulsar = atoi(palabras[1]);
			if(existeElTripulante(idTripulanteAExpulsar)){
				expulsarTripulante(idTripulanteAExpulsar);
				habilitarProximoAEjecutar();
			}
			else
				log_info(loggerDiscordiador,"EL TRIPULANTE INDICADO NO EXISTE");
			break;
		}
		case 4:{
			//INICIAR_PLANIFICACION
			if(!planificacionActivada){
				if(list_size(tripulantes)!=list_size(colaExit))
					iniciarPlanificacion();
				else
					log_info(loggerDiscordiador,"NO HAY TRIPULANTES PARA PLANIFICAR!");
			}
			else
				log_info(loggerDiscordiador,"LA PLANIFICACIÓN YA ESTÁ ACTIVADA");
			break;
		}
		case 5:{
			//PAUSAR_PLANIFICACION
			if(planificacionActivada)
				pausarPlanificacion();
			else
				log_info(loggerDiscordiador,"LA PLANIFICACIÓN NO EMPEZÓ O YA ESTÁ PAUSADA");
			break;
		}
		case 6:{
			//OBTENER_BITACORA [idTripulante]
			obtenerBitacora((uint32_t) atoi(palabras[1]));
			break;
		}
		default:
			log_info(loggerDiscordiador,"Comando no reconocido");
			break;
		}
		free(comando);
		comando = readline(">");
	}
	free(comando);
}

void destruirSemaforos(){
	pthread_mutex_destroy(&mutexTripulantes);
	pthread_mutex_destroy(&mutexColaReady);
	pthread_mutex_destroy(&mutexColaExec);
	pthread_mutex_destroy(&mutexColaExit);
	pthread_mutex_destroy(&mutexColaBlockIO);
	pthread_mutex_destroy(&mutexActivarPlanificacion);
	pthread_mutex_destroy(&mutexEjecutarIO);
}

void destruirListasYDiccionarios(){
	list_destroy_and_destroy_elements(colaReady,free);
	list_destroy_and_destroy_elements(colaExec,free);
	list_destroy_and_destroy_elements(colaBlockIO,free);
	list_destroy_and_destroy_elements(colaExit,free);
	list_destroy_and_destroy_elements(tripulantes,free);
	list_destroy_and_destroy_elements(patotas,free);
	dictionary_destroy(diccionarioComandos);
	dictionary_destroy(diccionarioTareas);
}

void terminar_programa(){
	log_destroy(loggerDiscordiador);
	config_destroy(configuracionDiscordiador);
	destruirListasYDiccionarios();
	destruirSemaforos();
}


int main(void){
	inicializarVariables();

//	int socket_escucha_MONGO = iniciarServidor(IP_DISCORDIADOR,PUERTO_DISCORDIADOR);

	ingresar_comandos();

	terminar_programa();

	return EXIT_SUCCESS;
}

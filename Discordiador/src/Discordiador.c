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
	colaBlockEmergencia = list_create();
	colaExit = list_create();
	posicionSabotajeActual = malloc(sizeof(posicion));

	socket_escucha_iMongo = iniciarServidor(IP_DISCORDIADOR,PUERTO_DISCORDIADOR);

	idTripulante = 1;
	idPatota = 1;
	idTripulanteResolviendoSabotaje = 0; //NINGUN TRIPULANTE TIENE ESE ID
	planificacionActivada = false;
	planificacionFueActivadaAlgunaVez = false; //PARA QUE SE SI SE PAUSA Y SE CREA UNA PATOTA, LOS TRIPULANTES QUEDEN EN NEW Y NO EN READY
	haySituacionDeEmergencia = false;

	pthread_mutex_init(&mutexTripulantes,NULL);
	pthread_mutex_init(&mutexColaReady,NULL);
	pthread_mutex_init(&mutexColaExec,NULL);
	pthread_mutex_init(&mutexColaExit,NULL);
	pthread_mutex_init(&mutexColaBlockIO,NULL);
	pthread_mutex_init(&mutexColaBlockSabotaje,NULL);
	pthread_mutex_init(&mutexActivarPlanificacion,NULL);
	pthread_mutex_init(&mutexEjecutarIO,NULL);
	pthread_mutex_init(&mutexIdTripulanteSabotaje,NULL);
	pthread_mutex_init(&mutexSituacionEmergencia,NULL);
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

void ingresarComandos()
{
	char* comando = readline(">");
	char** palabras = string_split(comando, " ");

	while(1){
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
					if(!haySituacionDeEmergencia)
						iniciarPlanificacion();
					else
						log_info(loggerDiscordiador,"SE ESTÁ RESOLVIENDO UN SABOTAJE!");

				else
					log_info(loggerDiscordiador,"NO HAY TRIPULANTES PARA PLANIFICAR!");
			}
			else
				log_info(loggerDiscordiador,"LA PLANIFICACIÓN YA ESTÁ ACTIVADA");
			break;
		}
		case 5:{
			//PAUSAR_PLANIFICACION
			pthread_mutex_lock(&mutexActivarPlanificacion);
			if(planificacionActivada){
				pthread_mutex_unlock(&mutexActivarPlanificacion);
				pthread_mutex_lock(&mutexSituacionEmergencia);
				if(!haySituacionDeEmergencia){
					pthread_mutex_unlock(&mutexSituacionEmergencia);
					pausarPlanificacion();
				}
				else
				{
					pthread_mutex_unlock(&mutexSituacionEmergencia);
					log_info(loggerDiscordiador,"SE ESTÁ RESOLVIENDO UN SABOTAJE!");
				}
			}
			else{
				pthread_mutex_unlock(&mutexActivarPlanificacion);
				log_info(loggerDiscordiador,"LA PLANIFICACIÓN NO EMPEZÓ O YA ESTÁ PAUSADA");
			}

			break;
		}
		case 6:{
			//OBTENER_BITACORA [idTripulante]
			obtenerBitacora(atoi(palabras[1]));
			break;
		}
		default:
			log_info(loggerDiscordiador,"Comando no reconocido");
			break;
		}
		liberarArray(palabras);
		free(comando);
		comando = readline(">");
		palabras = string_split(comando, " ");
	}
	free(comando);
}

void atenderSabotajes(){
	while(1){
		int socket_cliente_mongo = esperar_cliente(socket_escucha_iMongo);

		uint32_t posSabotajeX, posSabotajeY;
		recv(socket_cliente_mongo,&posSabotajeX,sizeof(uint32_t),0);
		recv(socket_cliente_mongo,&posSabotajeY,sizeof(uint32_t),0);

		pthread_mutex_lock(&mutexColaReady);
		pthread_mutex_lock(&mutexColaExec);
		if(!list_is_empty(colaExec) || !list_is_empty(colaReady)){
			pthread_mutex_unlock(&mutexColaExec);
			pthread_mutex_unlock(&mutexColaReady);
			posicionSabotajeActual->posX = posSabotajeX;
			posicionSabotajeActual->posY = posSabotajeY;

			log_info(loggerDiscordiador,"¡¡¡SE PRODUJO UN SABOTAJE EN %d|%d!!!",posicionSabotajeActual->posX,posicionSabotajeActual->posY);

			gestionarSabotaje();
		}
		else
		{
			pthread_mutex_unlock(&mutexColaExec);
			pthread_mutex_unlock(&mutexColaReady);
			log_info(loggerDiscordiador,"NO HAY TRIPULANTES PARA RESOLVER EL SABOTAJE");
		}

		close(socket_cliente_mongo);
	}
}

void esperarSabotajes(){
	pthread_t thread_sabotajes;
	pthread_create(&thread_sabotajes,NULL,(void*) atenderSabotajes,NULL);
	pthread_detach(thread_sabotajes);
}

void destruirTripulantes(){
	for(int i=0; i<list_size(tripulantes) ;i++){
		pthread_mutex_lock(&mutexTripulantes);
		t_tripulante* tripulante = (t_tripulante*) list_get(tripulantes,i);
		free(tripulante->posicion);
		free(tripulante->proxTarea->nombre);
		free(tripulante->proxTarea);
		sem_destroy(&(tripulante->semaforoPlanificacion));
		sem_destroy(&(tripulante->puedeEjecutar));
		close(tripulante->socket_MIRAM);
		close(tripulante->socket_MONGO);
		pthread_mutex_unlock(&mutexTripulantes);
	}

	list_destroy(tripulantes);
}

void destruirPatotas(){
	for(int i=0; i<list_size(patotas) ;i++){
		t_patota* patota = (t_patota*) list_get(patotas,i);
		list_destroy(patota->tripulantes);
		free(patota);
	}

	list_destroy(patotas);
}

void destruirSemaforos(){
	pthread_mutex_destroy(&mutexTripulantes);
	pthread_mutex_destroy(&mutexColaReady);
	pthread_mutex_destroy(&mutexColaExec);
	pthread_mutex_destroy(&mutexColaExit);
	pthread_mutex_destroy(&mutexColaBlockIO);
	pthread_mutex_destroy(&mutexColaBlockSabotaje);
	pthread_mutex_destroy(&mutexActivarPlanificacion);
	pthread_mutex_destroy(&mutexEjecutarIO);
	pthread_mutex_destroy(&mutexIdTripulanteSabotaje);
	pthread_mutex_destroy(&mutexSituacionEmergencia);
}

void destruirListasYDiccionarios(){
	list_destroy(colaReady);
	list_destroy(colaExec);
	list_destroy(colaBlockIO);
	list_destroy(colaBlockEmergencia);
	list_destroy(colaExit);
	dictionary_destroy(diccionarioComandos);
	dictionary_destroy(diccionarioTareas);
}

void destruirConfig(){
	free(IP_MI_RAM);
	free(IP_I_MONGO_STORE);
	free(IP_DISCORDIADOR);
	free(ALGORITMO);
	free(PUERTO_MI_RAM);
	free(PUERTO_I_MONGO_STORE);
	free(PUERTO_DISCORDIADOR);
//	config_destroy(configuracionDiscordiador);
}

void terminarPrograma(){
	log_info(loggerDiscordiador,"Finaliza el Discordiador...");
	destruirTripulantes();
	destruirPatotas();
	destruirListasYDiccionarios();
	destruirSemaforos();
	destruirConfig();
	log_destroy(loggerDiscordiador);
	free(posicionSabotajeActual);
	close(socket_escucha_iMongo);
	exit(0);
}

int main(void){
	signal(SIGINT,terminarPrograma); //ctrl + C

	inicializarVariables();
	log_info(loggerDiscordiador,"Inicia el Discordiador...");

	esperarSabotajes();

	ingresarComandos();

	return EXIT_SUCCESS;
}

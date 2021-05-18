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
	tripulantes = list_create();
	patotas = list_create();

	idTripulante = 1;
	idPatota = 1;
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

char* obtenerTareasComoCadena(char* path){
	FILE* tareas = fopen(path,"r");
	if(tareas == NULL){
		return "NO SE PUDO ABRIR EL ARCHIVO DE TAREAS";
	}

	int MAX_PER_LINE = 50;
	int MAX_TOTAL = 400;
	char* buffer = malloc(sizeof(char) * MAX_PER_LINE);
	char* buffer_total = malloc(sizeof(char) * MAX_TOTAL);
	char* result_string;
	int linesize;

	while(fgets(buffer, MAX_PER_LINE, tareas) != NULL){
		linesize = strlen(buffer);
		if(linesize>0 && buffer[linesize - 1] == 10)
			buffer[linesize - 1] = 0;

		strcat(buffer_total, buffer);
		strcat(buffer_total, "|");
	}

	linesize = strlen(buffer_total);
	if(linesize > 0 && buffer_total[linesize - 1] == '|')
		buffer_total[linesize - 1] = 0;

	result_string = malloc(sizeof(char)*(strlen(buffer_total) + 1));
	strcpy(result_string,buffer_total);

	fclose(tareas);
	free(buffer);
	free(buffer_total);

	return result_string;
}

void sumarIdTripulante(){
	idTripulante++;
}

void sumarIdPatota(){
	idPatota++;
}

//SOLO METO EN EL BUFFER EL ID, EL ESTADO Y LA POSICION, QUE ES LO QUE NECESITA MI-RAM
void* serializar_tripulante(t_tripulante* tripulante){
	int bytes = 2*sizeof(uint32_t) + sizeof(char) + sizeof(posicion);
	void* magic = malloc(bytes);
	int desplazamiento = 0;

	memcpy(magic + desplazamiento, &(tripulante->idPatota), sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);
	memcpy(magic + desplazamiento, &(tripulante->tid), sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);
	memcpy(magic + desplazamiento, &(tripulante->estado), sizeof(char));
	desplazamiento += sizeof(char);
	memcpy(magic + desplazamiento, &(tripulante->posicion->posX), sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);
	memcpy(magic + desplazamiento, &(tripulante->posicion->posY), sizeof(uint32_t));

	return magic;
}

t_iniciar_patota* obtenerDatosPatota(char** array){
	t_iniciar_patota* parametrosPatota = malloc(sizeof(t_iniciar_patota));
	parametrosPatota->coordenadasTripulantes = list_create();

	int flag = 3;
	if(atoi(array[1])!=0 && array[1]!=NULL){
		parametrosPatota->cantidadTripulantes = atoi(array[1]);
		if(array[2]!=NULL)
		{
			parametrosPatota->rutaDeTareas = array[2];
			while(array[flag]!=NULL)
			{
				posicion* posicionTripulante = malloc(sizeof(posicion));
				char** coordenadas = string_split(array[flag], "|");
				posicionTripulante->posX = atoi(coordenadas[0]);
				posicionTripulante->posY = atoi(coordenadas[1]);
				list_add(parametrosPatota->coordenadasTripulantes,posicionTripulante);
				flag++;
			}

			if(parametrosPatota->cantidadTripulantes - list_size(parametrosPatota->coordenadasTripulantes)!=0)
			{
				int tripulantesFaltantes = parametrosPatota->cantidadTripulantes - list_size(parametrosPatota->coordenadasTripulantes);
				for(int i=0; i<tripulantesFaltantes ;i++)
				{
					posicion* posicionTripulante = malloc(sizeof(posicion));
					posicionTripulante->posX = 0;
					posicionTripulante->posY = 0;
					list_add(parametrosPatota->coordenadasTripulantes, posicionTripulante);
				}
			}
		}
		else
		{
			parametrosPatota->rutaDeTareas = "";
			for(int i = 0; i<parametrosPatota->cantidadTripulantes; i++)
			{
				posicion* posicionTripulante = malloc(sizeof(posicion));
				posicionTripulante->posX= 0;
				posicionTripulante->posY= 0;
				list_add(parametrosPatota->coordenadasTripulantes,posicionTripulante);
			}
		}
	}

	return parametrosPatota;
}

void iniciarPatota(t_iniciar_patota* estructura){
	int socket_cliente_MIRAM = crearConexionCliente(IP_MI_RAM,PUERTO_MI_RAM);

	t_patota* patota = malloc(sizeof(t_patota));
	patota->tripulantes = list_create();
	patota->pid = idPatota;
	patota->archivoTareas = estructura->rutaDeTareas;
	char* tareas = obtenerTareasComoCadena(patota->archivoTareas);
	int sizeTareas = strlen(tareas) + 1;
	sumarIdPatota();
	list_add(patotas,patota);

	t_paquete* paquete = malloc(sizeof(t_paquete));
	paquete->codigo_operacion = INICIAR_PATOTA;
	paquete->buffer = malloc(sizeof(t_buffer));
	paquete->buffer->size = 3*sizeof(uint32_t) + strlen(tareas) + 1;
	paquete->buffer->stream = malloc(paquete->buffer->size);

	int desplazamiento = 0;
	memcpy(paquete->buffer->stream + desplazamiento, &(patota->pid), sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);
	memcpy(paquete->buffer->stream + desplazamiento, &(estructura->cantidadTripulantes), sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);
	memcpy(paquete->buffer->stream + desplazamiento, &sizeTareas, sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);
	memcpy(paquete->buffer->stream + desplazamiento, tareas, sizeTareas);

	enviar_paquete(paquete,socket_cliente_MIRAM);
	eliminar_paquete(paquete);
	free(tareas);

	//Espero el OK
	tipo_respuesta cod_respuesta = recibir_respuesta(socket_cliente_MIRAM);
	switch(cod_respuesta) {
	case OK:
		log_info(loggerDiscordiador, "MIRAM envio el OK");
		break;
	default:
		log_error(loggerDiscordiador, "MIRAM fallo, me salgo");
		return;
	}

	close(socket_cliente_MIRAM);

	for(int i=0; i<estructura->cantidadTripulantes; i++){
		t_tripulante* tripulante = malloc(sizeof(t_tripulante));
		tripulante->idPatota = patota->pid;
		tripulante->tid = idTripulante;
		tripulante->estado = 'N';
		tripulante->posicion = list_get(estructura->coordenadasTripulantes,i);
		list_add(patota->tripulantes,tripulante);
		list_add(tripulantes,tripulante);

		sumarIdTripulante();

		pthread_t hiloTripulante;
		pthread_create(&hiloTripulante,NULL, (void*) gestionarTripulante, tripulante);
		pthread_detach(hiloTripulante);
	}
}

void gestionarTripulante(t_tripulante* tripulante){
	int socket_cliente_MIRAM = crearConexionCliente(IP_MI_RAM,PUERTO_MI_RAM);

	t_paquete* paquete = malloc(sizeof(t_paquete));
	paquete->codigo_operacion = INICIAR_TRIPULANTE;
	paquete->buffer = malloc(sizeof(t_buffer));
	paquete->buffer->size = 2*sizeof(uint32_t) + sizeof(char) + sizeof(posicion);
	paquete->buffer->stream = serializar_tripulante(tripulante);
	enviar_paquete(paquete,socket_cliente_MIRAM);
	eliminar_paquete(paquete);

	close(socket_cliente_MIRAM);

	while(1){
		//solicita una tarea a MI-RAM
		//MI-RAM le manda la primer tarea del archivo
		//
	}
}

void listarTripulantes(){
	if(list_is_empty(tripulantes)){
		log_info(loggerDiscordiador,"NO HAY TRIPULANTES!\n");
	}
	else
	{
		printf("--------------------------------------------\n");
		printf("Estado de la Nave: %s\n",temporal_get_string_time("%d/%m/%y %H:%M:%S"));
		for(int i=0; i<list_size(tripulantes); i++){
			t_tripulante* tripulante = list_get(tripulantes,i);
			printf("Tripulante: %d	Patota: %d	Status: %c\n",tripulante->tid, tripulante->idPatota, tripulante->estado);
		}
		printf("--------------------------------------------\n");
	}
}

void expulsarTripulante(int idTripulante){
	//se finaliza un tripulante
	//avisarle a MI-RAM que lo borre del mapa
	//en caso de ser necesario tambien elimina su segmento de tareas

	//int op_code = 2;
	//send(socket_cliente_miRam,&op_code,sizeof(int),0);
	//send(socket_cliente_miRam,&idTripulante,sizeof(int),0);
}

void iniciarPlanificacion(){
	//se inicia la planificacion
	//hasta este punto se supone que no hubo movimientos entre colas de planificacion ni de los tripulantes
}

void pausarPlanificacion(){
	//se pausa la planificacion
}

void obtenerBitacora(uint32_t idTripulante){ //debe devolver un stream o string de la bitacora
	//se manda un mensaje a I-MONGO solicitando la bitacora de un tripulante
	int socketClienteIMONGO = crearConexionCliente(IP_I_MONGO_STORE,PUERTO_I_MONGO_STORE);
	t_paquete* paquete = malloc(sizeof(t_paquete));
	paquete->codigo_operacion = OBTENER_BITACORA;
	paquete->buffer = malloc(sizeof(t_buffer));
	paquete->buffer->size = sizeof(uint32_t);
	paquete->buffer->stream = malloc(paquete->buffer->size);

	memcpy(paquete->buffer->stream,&idTripulante,sizeof(uint32_t));

	enviar_paquete(paquete,socketClienteIMONGO);

//	tipo_respuesta respuesta = recibir_respuesta(socketClienteIMONGO);
//	switch(respuesta)
//	{
//	case OK:
//		printf("TODO OK\n");
//		break;
//	default:
//		break;
//	}

	close(socketClienteIMONGO);
	//levanto un socket server? para recibir la respuesta? mando el socket int
	//return "Funciona";
}

void ingresar_comandos()
{
	char** palabras;
	char* comando = readline(">");

	while(1){
		palabras = string_split(comando, " ");
		switch((int) dictionary_get(diccionarioDiscordiador,palabras[0]))
		{
		case 1:{
			//INICIAR_PATOTA [cant_tripulantes] [path] [pos1] [pos2] ...
			//PRIMER COMANDO A MANDAR

			if(palabras[1]!=NULL){
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
	dictionary_destroy(diccionarioDiscordiador);
}

int main(void)
{
	inicializarVariables();

	ingresar_comandos();

	terminar_programa();

	return EXIT_SUCCESS;
}

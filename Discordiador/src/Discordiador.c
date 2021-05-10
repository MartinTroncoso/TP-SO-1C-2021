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

	socket_cliente_miRam = crearConexionCliente(IP_MI_RAM, PUERTO_MI_RAM);
	socket_cliente_iMongo = crearConexionCliente(IP_I_MONGO_STORE, PUERTO_I_MONGO_STORE);
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

void sumarIdTripulante(){
	idTripulante++;
}

void sumarIdPatota(){
	idPatota++;
}

//SOLO METO EN EL BUFFER EL ID, EL ESTADO Y LA POSICION, QUE ES LO QUE NECESITA MI-RAM
void* serializar_tripulante(t_tripulante* tripulante){
	int bytes = sizeof(uint32_t) + sizeof(coordenadasTripulante) + sizeof(char);
	void* magic = malloc(bytes);
	int desplazamiento = 0;

	memcpy(magic + desplazamiento,&(tripulante->tid),sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);
	memcpy(magic + desplazamiento,&(tripulante->estado),sizeof(char));
	desplazamiento += sizeof(char);
	memcpy(magic + desplazamiento,&(tripulante->posicion->coordenadaX),sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);
	memcpy(magic + desplazamiento,&(tripulante->posicion->coordenadaY),sizeof(uint32_t));

	return magic;
}

void iniciarPatota(t_iniciar_patota* estructura){
	t_patota* patota = malloc(sizeof(t_patota));
	patota->tripulantes = list_create();
	patota->pid = idPatota;
	patota->archivoTareas = estructura->rutaDeTareas;
	sumarIdPatota();
	list_add(patotas,patota);

	t_paquete* paquete = malloc(sizeof(t_paquete));
	paquete->codigo_operacion = INICIAR_PATOTA;
	paquete->buffer = malloc(sizeof(t_buffer));
	paquete->buffer->size = 2*sizeof(uint32_t);
	paquete->buffer->stream = malloc(paquete->buffer->size);
	memcpy(paquete->buffer->stream, &(patota->pid), sizeof(uint32_t));
	memcpy(paquete->buffer->stream + sizeof(uint32_t), &(estructura->cantidadTripulantes), sizeof(uint32_t));
	enviar_paquete(paquete,socket_cliente_miRam);
	eliminar_paquete(paquete);

	for(int i=0; i<estructura->cantidadTripulantes; i++){
		t_tripulante* tripulante = malloc(sizeof(t_tripulante));
		tripulante->estado = 'N';
		tripulante->tid = idTripulante;
		tripulante->idPatota = patota->pid;
		tripulante->direccionPCB = 0; //capaz esto no vaya en esta estructura, solo iria en el TCB de MI-RAM
		tripulante->proxInstruccion = 0; //idem
		tripulante->posicion = (void*) list_get(estructura->coordenadasTripulantes,i);
		list_add(patota->tripulantes,tripulante);
		list_add(tripulantes,tripulante);

		sumarIdTripulante();

		pthread_t hiloTripulante;
		pthread_create(&hiloTripulante,NULL, (void*) gestionarTripulante, tripulante);
		pthread_join(hiloTripulante,NULL);
	}
}

void gestionarTripulante(t_tripulante* tripulante){
	t_buffer* buffer = malloc(sizeof(t_buffer));
	buffer->size = sizeof(uint32_t) + sizeof(coordenadasTripulante) + sizeof(char);
	buffer->stream = serializar_tripulante(tripulante);
	enviar_buffer(buffer,socket_cliente_miRam);
	free(buffer->stream);
	free(buffer);
//	while(1){
//		//solicita una tarea a MI-RAM
//		//MI-RAM le manda la primer tarea del archivo
//		//
//	}
}

void listarTripulantes(){
	if(list_is_empty(tripulantes)){
		printf("NO HAY TRIPULANTES!\n");
	}
	else
	{
		printf("--------------------------------------------\n");
		printf("Estado de la Nave: %s\n",temporal_get_string_time());
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

void obtenerBitacora(int idTripulante){
	//se manda un mensaje a I-MONGO solicitando la bitacora de un tripulante
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
			obtenerBitacora(idTripulante);
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

void paquete(int socket_cliente, int otro_socket_cliente)
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
	enviar_paquete(paquete,socket_cliente);
	enviar_paquete(paquete,otro_socket_cliente);

	eliminar_paquete(paquete);
}

void terminar_programa()
{
	close(socket_cliente_iMongo);
	close(socket_cliente_miRam);
	log_destroy(loggerDiscordiador);
	config_destroy(configuracionDiscordiador);
	list_destroy_and_destroy_elements(tripulantes,free);
	list_destroy_and_destroy_elements(patotas,free);
	dictionary_destroy(diccionarioDiscordiador);
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
				coordenadasTripulante* posicionTripulante = malloc(sizeof(coordenadasTripulante));
				if(array[flag]!=NULL)
				{
					char** coordenadas = string_split(array[flag], "|");
					posicionTripulante->coordenadaX = atoi(coordenadas[0]);
					posicionTripulante->coordenadaY = atoi(coordenadas[1]);
					list_add(parametrosPatota->coordenadasTripulantes,posicionTripulante);
					flag++;
				}
			}

			if(parametrosPatota->cantidadTripulantes-list_size(parametrosPatota->coordenadasTripulantes)!=0)
			{
				int tripulantesFaltantes = parametrosPatota->cantidadTripulantes - list_size(parametrosPatota->coordenadasTripulantes);
				for(int i = 0; i<tripulantesFaltantes;i++)
				{
					coordenadasTripulante* posicionTripulante = malloc(sizeof(coordenadasTripulante));
					posicionTripulante->coordenadaX= 0;
					posicionTripulante->coordenadaY= 0;
					list_add(parametrosPatota->coordenadasTripulantes,posicionTripulante);
				}
			}
		}
		else
		{
			parametrosPatota->rutaDeTareas = "";
			for(int i = 0; i<parametrosPatota->cantidadTripulantes; i++)
			{
				coordenadasTripulante* posicionTripulante = malloc(sizeof(coordenadasTripulante));
				posicionTripulante->coordenadaX= 0;
				posicionTripulante->coordenadaY= 0;
				list_add(parametrosPatota->coordenadasTripulantes,posicionTripulante);
			}
		}
	}

	return parametrosPatota;
}

int main(void)
{
	inicializarVariables();

	ingresar_comandos();

	terminar_programa();

	return EXIT_SUCCESS;
}

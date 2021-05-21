/*
 * Tripulantes.c
 *
 *  Created on: 18 may. 2021
 *      Author: utnso
 */

#include "Discordiador.h"

void sumarIdTripulante(){
	idTripulante++;
}

void sumarIdPatota(){
	idPatota++;
}

void moverXDelTripulante(t_tripulante* tripulante){
	if(tripulante->posicion->posX > tripulante->proxTarea->posicion.posX){
		tripulante->posicion->posX--;
	}
	else
	{
		if(tripulante->posicion->posX < tripulante->proxTarea->posicion.posX){
			tripulante->posicion->posX++;
		}
		else
		{
			log_info(loggerDiscordiador,"El tripulante %d ya llegó a la posición X de la tarea!",tripulante->tid);
		}

	}
}

void moverYDelTripulante(t_tripulante* tripulante){
	if(tripulante->posicion->posY > tripulante->proxTarea->posicion.posY){
		tripulante->posicion->posY--;
	}
	else
	{
		if(tripulante->posicion->posY < tripulante->proxTarea->posicion.posY){
			tripulante->posicion->posY++;
		}
		else
		{
			log_info(loggerDiscordiador,"El tripulante %d ya llegó a la posición Y de la tarea!",tripulante->tid);
		}

	}
}

void planificarTripulanteFIFO(t_tripulante* tripulante){
	while(tripulante->posicion->posX != tripulante->proxTarea->posicion.posX){
		moverXDelTripulante(tripulante);
		sleep(RETARDO_CICLO_CPU);
	}

	while(tripulante->posicion->posY != tripulante->proxTarea->posicion.posY){
		moverYDelTripulante(tripulante);
		sleep(RETARDO_CICLO_CPU);
	}
}

void planificarTripulante(t_tripulante* tripulante){
	t_algoritmo algoritmo = getAlgoritmoPlanificacion();
	switch(algoritmo){
	case FIFO:{
		planificarTripulanteFIFO(tripulante);
		break;
	}
	case RR:{
//		planificarTripulanteRR(tripulante);
		break;
	}
	default:{
		log_error(loggerDiscordiador,"Hubo un error con el algoritmo de planificación.");
	}
	}
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

int getCantidadTareasPatota(char* cadena){
	int cantidad = 0;
	for(int i=0; cadena[i]!='\0' ;i++){
		if(cadena[i]=='|')
			cantidad++;
	}

	return cantidad+1;
}

t_patota* buscarPatotaPorId(uint32_t idPatota){
	bool encontrarPatota(void* elemento){
		t_patota* patota = elemento;
		return patota->pid == idPatota;
	}

	return list_find(patotas,encontrarPatota);
}

bool tieneTareasPendientes(t_tripulante* tripulante){
	return tripulante->tareasPendientes > 0;
}

void informarMovimiento(int socket_cliente, posicion* origen, posicion* destino){

	//Preparo paquete para enviar
	t_paquete* paquete = malloc(sizeof(t_paquete));
	paquete->buffer = malloc(sizeof(t_buffer));
	int desplazamiento = 0;

	paquete->codigo_operacion = INFORMAR_MOVIMIENTO;
	paquete->buffer->size = 2 * sizeof(uint32_t);
	paquete->buffer->stream = malloc(paquete->buffer->size);

	memcpy(paquete->buffer->stream + desplazamiento, &(destino->posX), sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);
	memcpy(paquete->buffer->stream + desplazamiento, &(destino->posY), sizeof(uint32_t));

	fflush(stdout);
	printf("Envioo");
	enviar_paquete(paquete, socket_cliente);
	eliminar_paquete(paquete);
}

tarea* solitarProximaTarea(int idTripulante){
//	tarea* proximaTarea = malloc(sizeof(tarea));
	int socket_cliente_MIRAM = crearConexionCliente(IP_MI_RAM,PUERTO_MI_RAM);

	t_paquete* paquete = malloc(sizeof(t_paquete));
	paquete->buffer = malloc(sizeof(t_buffer));
	paquete->codigo_operacion = PROXIMA_TAREA;
	paquete->buffer->size = sizeof(int);
	paquete->buffer->stream = malloc(sizeof(paquete->buffer->size));
	memcpy(paquete->buffer->stream, &idTripulante, sizeof(int));
	enviar_paquete(paquete,socket_cliente_MIRAM);
	eliminar_paquete(paquete);

//	uint32_t size;
//	void* buffer = recibir_buffer(&size,socket_cliente_MIRAM);

	return NULL;
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

void gestionarTripulante(t_tripulante* tripulante){
	int socket_cliente_MIRAM = crearConexionCliente(IP_MI_RAM,PUERTO_MI_RAM);

	t_paquete* paquete = malloc(sizeof(t_paquete));
	paquete->codigo_operacion = INICIAR_TRIPULANTE;
	paquete->buffer = malloc(sizeof(t_buffer));
	paquete->buffer->size = 2*sizeof(uint32_t) + sizeof(char) + sizeof(posicion);
	paquete->buffer->stream = serializar_tripulante(tripulante);
	enviar_paquete(paquete,socket_cliente_MIRAM);
	eliminar_paquete(paquete);

	//close(socket_cliente_MIRAM);

	sleep(5);
	posicion* posicion_ej = malloc(sizeof(posicion));
	posicion_ej->posX = 10;
	posicion_ej->posY = 50;
	informarMovimiento(socket_cliente_MIRAM, posicion_ej, posicion_ej);

	sleep(5);
	posicion_ej->posY = 2;
	informarMovimiento(socket_cliente_MIRAM, posicion_ej, posicion_ej);

	free(posicion_ej);


	while(1){
		if(tieneTareasPendientes(tripulante)){
//			tarea* proximaTarea = solitarProximaTarea(tripulante->tid);
//			tripulante->proxTarea = proximaTarea;
//			planificarTripulante(tripulante);
//			switch((int) dictionary_get(diccionarioTareas,proximaTarea->nombre)){
//			case 1:{
//				//GENERAR_OXIGENO
//				break;
//			}
//			case 2:{
//				//CONSUMIR_OXIGENO
//				break;
//			}
//			case 3:{
//				//GENERAR_COMIDA
//				break;
//			}
//			case 4:{
//				//CONSUMIR_COMIDA
//				break;
//			}
//			case 5:{
//				//GENERAR_BASURA
//				break;
//			}
//			case 6:{
//				//DESCARTAR_BASURA
//				break;
//			}
//			default:{
//				break;
//			}
//			}
		}
	}
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
	patota->cantidadTareas = getCantidadTareasPatota(tareas);
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
		tripulante->tareasPendientes = patota->cantidadTareas;
		list_add(patota->tripulantes,tripulante);
		list_add(tripulantes,tripulante);

		sumarIdTripulante();

		pthread_t hiloTripulante;
		pthread_create(&hiloTripulante,NULL, (void*) gestionarTripulante, tripulante);
		pthread_detach(hiloTripulante);
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

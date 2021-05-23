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

bool esDeEntradaSalida(Tarea* tarea){
	return dictionary_get(diccionarioTareas,tarea->nombre) != NULL;
}

bool puedePasarAExec(t_tripulante* tripulante){
	bool buscarTripulante(void* elemento){
		return ((t_tripulante*) elemento)->tid == tripulante->tid;
	}

	return list_any_satisfy(list_take(colaReady,GRADO_MULTITAREA),buscarTripulante);
}

void agregarTripulanteAReady(t_tripulante* tripulante){
	sem_wait(&mutexColaReady);
	list_add(colaReady,tripulante);
	sem_post(&mutexColaReady);

	tripulante->estado = 'R';
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

void informarMovimiento(int socket_cliente, t_tripulante* tripulante){

	//Preparo paquete para enviar
	t_paquete* paquete = malloc(sizeof(t_paquete));
	paquete->buffer = malloc(sizeof(t_buffer));
	int desplazamiento = 0;

	paquete->codigo_operacion = INFORMAR_MOVIMIENTO;
	paquete->buffer->size = 2 * sizeof(uint32_t);
	paquete->buffer->stream = malloc(paquete->buffer->size);

	memcpy(paquete->buffer->stream + desplazamiento, &(tripulante->posicion->posX), sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);
	memcpy(paquete->buffer->stream + desplazamiento, &(tripulante->posicion->posY), sizeof(uint32_t));

	fflush(stdout);

	enviar_paquete(paquete, socket_cliente);
	eliminar_paquete(paquete);
}

Tarea* solitarProximaTarea(int idTripulante, int socket_cliente_MIRAM){
	Tarea* proximaTarea = malloc(sizeof(Tarea));

	tipo_mensaje mensaje = PROXIMA_TAREA;
	send(socket_cliente_MIRAM,&mensaje,sizeof(tipo_mensaje),0);

	uint32_t sizeBuffer;
	uint32_t sizeTarea;
	int desplazamiento = 0;
	void* buffer = recibir_buffer(&sizeBuffer, socket_cliente_MIRAM);

	memcpy(&sizeTarea,buffer + desplazamiento, sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);

	char* stringCadena = malloc(sizeTarea);
	memcpy(stringCadena,buffer + desplazamiento,sizeTarea);

	char** tareaSpliteada = string_split(stringCadena," ");
	char* parametros = tareaSpliteada[1];
	char** parametrosSpliteados = string_split(parametros,";");

	int tiempo = atoi(parametrosSpliteados[3]); //SI NO LO HAGO ASI, proximaTarea->tiempo QUEDA IGUAL A 0 (NO SE POR QUÉ)
	proximaTarea->nombre = tareaSpliteada[0];
	proximaTarea->longNombre = strlen(proximaTarea->nombre) + 1;
	proximaTarea->parametro = atoi(parametrosSpliteados[0]);
	proximaTarea->posicion.posX = atoi(parametrosSpliteados[1]);
	proximaTarea->posicion.posY = atoi(parametrosSpliteados[2]);
	proximaTarea->tiempo = tiempo;

	free(buffer);

	return proximaTarea;
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

void habilitarProximoAEjecutar(){
	bool primerTripulanteDeshabilitado(void* elemento){
		return ((t_tripulante*) elemento)->habilitado == false;
	}

	//AGARRO AL PRIMER TRIPULANTE DE LA COLA DE READY QUE ESTÉ DESHABILITADO (QUE DEBERÍA SER SIEMPRE EL PRIMERO)
	//PERO PUEDE HABER PROBLEMAS DE CONCURRENCIA, VER MEJOR ESTO
	if(!list_is_empty(colaReady)){
		sem_wait(&mutexColaReady);
		sem_post(&((t_tripulante*) list_find(colaReady,primerTripulanteDeshabilitado))->puedeEjecutar);
		((t_tripulante*) list_find(colaReady,primerTripulanteDeshabilitado))->habilitado = true;
		sem_post(&mutexColaReady);
	}
}

void habilitarSiCorresponde(t_tripulante* tripulante){
	bool buscarTripulante(void* elemento){
		return ((t_tripulante*) elemento)->tid == tripulante->tid;
	}

	if(list_any_satisfy(list_take(colaReady,GRADO_MULTITAREA),buscarTripulante) && list_size(colaExec)<GRADO_MULTITAREA && !tripulante->habilitado){
		sem_post(&(tripulante->puedeEjecutar));
		tripulante->habilitado = true;
	}
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

void planificarTripulanteFIFO(t_tripulante* tripulante, int socket_cliente_MIRAM){
	bool buscarTripulante(void* elemento){
		return ((t_tripulante*) elemento)->tid == tripulante->tid;
	}

	log_info(loggerDiscordiador,"[TRIPULANTE %d] TENGO QUE LLEGAR A %d|%d",tripulante->tid,tripulante->proxTarea->posicion.posX,tripulante->proxTarea->posicion.posY);

	sem_wait(&mutexColaReady);
	list_remove_by_condition(colaReady,buscarTripulante);
	sem_post(&mutexColaReady);

	sem_wait(&mutexColaExec);
	list_add(colaExec,tripulante);
	sem_post(&mutexColaExec);

	tripulante->estado = 'E';
	log_info(loggerDiscordiador,"[TRIPULANTE %d] COMIENZO A EJECUTAR",tripulante->tid);

	while(tripulante->posicion->posX != tripulante->proxTarea->posicion.posX){
		sem_wait(&(tripulante->semaforoPlanificacion));
		moverXDelTripulante(tripulante);
		sem_post(&(tripulante->semaforoPlanificacion));

		informarMovimiento(socket_cliente_MIRAM,tripulante);
		sleep(RETARDO_CICLO_CPU);
	}

	while(tripulante->posicion->posY != tripulante->proxTarea->posicion.posY){
		sem_wait(&(tripulante->semaforoPlanificacion));
		moverYDelTripulante(tripulante);
		sem_post(&(tripulante->semaforoPlanificacion));

		informarMovimiento(socket_cliente_MIRAM,tripulante);
		sleep(RETARDO_CICLO_CPU);
	}

	sem_wait(&mutexColaExec);
	list_remove_by_condition(colaExec,buscarTripulante);
	sem_post(&mutexColaExec);

	tripulante->estado = 'R';
}

void planificarTripulante(t_tripulante* tripulante, int socket_cliente_MIRAM){
	t_algoritmo algoritmo = getAlgoritmoPlanificacion();
	switch(algoritmo){
	case FIFO:{
		planificarTripulanteFIFO(tripulante, socket_cliente_MIRAM);
		break;
	}
	case RR:{
//		planificarTripulanteRR(tripulante, socket_cliente_MIRAM);
		break;
	}
	default:{
		log_error(loggerDiscordiador,"Hubo un error con el algoritmo de planificación.");
	}
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

	if(planificacionActivada)
		sem_post(&(tripulante->semaforoPlanificacion));

	while(1){
		if(tieneTareasPendientes(tripulante)){
			sem_wait(&(tripulante->semaforoPlanificacion));
			Tarea* proximaTarea = solitarProximaTarea(tripulante->tid, socket_cliente_MIRAM);
			tripulante->proxTarea = proximaTarea;

			agregarTripulanteAReady(tripulante);
			sem_post(&(tripulante->semaforoPlanificacion));

			//HABILITA AL TRIPULANTE A EJECUTAR SOLO SI HAY COMO MUCHO 'GRADO_TAREA'-1 TRIPULANTES EN LA COLA DE EXEC ESTO VA A PASAR SOLO CUANDO SE CREE
			//LA PRIMERA PATOTA O CUANDO TERMINEN DE EJECUTAR TODOS LOS TRIPULANTES Y SE VUELVA A CREAR UNA NUEVA PATOTA (MEDIO FIERO, PERO PARECE QUE ANDA xD)
			habilitarSiCorresponde(tripulante);

			sem_wait(&(tripulante->puedeEjecutar));
			planificarTripulante(tripulante, socket_cliente_MIRAM);

			if(esDeEntradaSalida(proximaTarea)){
				switch((int) dictionary_get(diccionarioTareas,proximaTarea->nombre)){
				case 1:{
					//GENERAR_OXIGENO
					break;
				}
				case 2:{
					//CONSUMIR_OXIGENO
					break;
				}
				case 3:{
					//GENERAR_COMIDA
					break;
				}
				case 4:{
					//CONSUMIR_COMIDA
					break;
				}
				case 5:{
					//GENERAR_BASURA
					break;
				}
				case 6:{
					//DESCARTAR_BASURA
					break;
				}
				default:{
					break;
				}
				}
			}

			//A VECES EN EL NOMBRE IMPRIME COSAS QUE NADA QUE VER
			log_info(loggerDiscordiador,"[TRIPULANTE %d] LLEGUÉ A LA TAREA %s",tripulante->tid,proximaTarea->nombre);
			tripulante->tareasPendientes--;
			tripulante->habilitado = false;
			habilitarProximoAEjecutar();
		}
		else
		{
			log_info(loggerDiscordiador,"[TRIPULANTE %d] TERMINÉ",tripulante->tid);
			tripulante->estado = 'F';

			sem_wait(&mutexColaExit);
			list_add(colaExit,tripulante);
			sem_post(&mutexColaExit);

			if(list_size(colaExit)==list_size(tripulantes)){
				log_info(loggerDiscordiador,"NO HAY MÁS TRIPULANTES PARA PLANIFICAR. SE CIERRA LA PLANIFICACIÓN");
				planificacionActivada = false;
			}

			tipo_mensaje finalizar = FINALIZA_TRIPULANTE;
			send(socket_cliente_MIRAM,&finalizar,sizeof(tipo_mensaje),0);
			close(socket_cliente_MIRAM);
			break;
		}
	}
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
	paquete->buffer->size = 3*sizeof(uint32_t) + sizeTareas;
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
		log_info(loggerDiscordiador,"Se creó la patota %d",patota->pid);
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
		tripulante->habilitado = false;
		sem_init(&(tripulante->semaforoPlanificacion),0,0);
		sem_init(&(tripulante->puedeEjecutar),0,0);

		//USO SEMÁFORO PORQUE SON LISTAS GLOBALES (REGIÓN CRÍTICA)
		sem_wait(&mutexTripulantes);
		list_add(patota->tripulantes,tripulante);
		list_add(tripulantes,tripulante);
		sumarIdTripulante();
		sem_post(&mutexTripulantes);

		pthread_t hiloTripulante;
		pthread_create(&hiloTripulante,NULL, (void*) gestionarTripulante, tripulante);
		pthread_detach(hiloTripulante);
	}

	if(patota->pid == 1)
		log_info(loggerDiscordiador,"Discordiador LISTO PARA PLANIFICAR");
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
	void habilitarSemaforo(void* elemento){
		t_tripulante* tripulante = (t_tripulante*) elemento;
		sem_post(&tripulante->semaforoPlanificacion);
	}

	list_map(tripulantes,(void*) habilitarSemaforo);

	planificacionActivada = true;

	log_info(loggerDiscordiador,"SE INICIA LA PLANIFICACIÓN");
}

void pausarPlanificacion(){
	void deshabilitarSemaforo(void* elemento){
		t_tripulante* tripulante = (t_tripulante*) elemento;
		sem_wait(&tripulante->semaforoPlanificacion);
	}

	list_map(tripulantes,(void*) deshabilitarSemaforo);

	planificacionActivada = false;

	log_info(loggerDiscordiador,"SE PAUSA LA PLANIFICACIÓN");
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

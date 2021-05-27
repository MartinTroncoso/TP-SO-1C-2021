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

char getEstadoComoCaracter(estado estado){
	return estado==NEW?'N':estado==READY?'R':estado==EXEC?'E':estado==EXIT?'F':'B';
}

char* getEstadoComoCadena(estado estado){
	return estado==NEW?"NEW":estado==READY?"READY":estado==EXEC?"EXEC":estado==EXIT?"EXIT":estado==BLOCK_IO?"BLOCK I/O":"BLOCK SABOTAJE";
}

void notificarCambioDeEstado(t_tripulante* tripulante){

}

void agregarAReady(t_tripulante* tripulante){
	pthread_mutex_lock(&mutexColaReady);
	list_add(colaReady,tripulante);
	pthread_mutex_unlock(&mutexColaReady);

	pthread_mutex_lock(&mutexTripulantes);
	tripulante->estado = READY;
	pthread_mutex_unlock(&mutexTripulantes);
}

void agregarAExec(t_tripulante* tripulante){
	pthread_mutex_lock(&mutexColaExec);
	list_add(colaExec,tripulante);
	pthread_mutex_unlock(&mutexColaExec);

	pthread_mutex_lock(&mutexTripulantes);
	tripulante->estado = EXEC;
	pthread_mutex_unlock(&mutexTripulantes);
}

void agregarAExit(t_tripulante* tripulante){
	pthread_mutex_lock(&mutexColaExit);
	list_add(colaExit,tripulante);
	pthread_mutex_unlock(&mutexColaExit);

	pthread_mutex_lock(&mutexTripulantes);
	tripulante->estado = EXIT;
	pthread_mutex_unlock(&mutexTripulantes);
}

void agregarABlockIO(t_tripulante* tripulante){
	pthread_mutex_lock(&mutexColaBlockIO);
	list_add(colaBlockIO,tripulante);
	pthread_mutex_unlock(&mutexColaBlockIO);

	pthread_mutex_lock(&mutexTripulantes);
	tripulante->estado = BLOCK_IO;
	pthread_mutex_unlock(&mutexTripulantes);
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

Tarea* solitarProximaTarea(t_tripulante* tripulante){
	Tarea* proximaTarea = malloc(sizeof(Tarea));

	tipo_mensaje mensaje = PROXIMA_TAREA;
	send(tripulante->socket_MIRAM,&mensaje,sizeof(tipo_mensaje),0);

	uint32_t sizeTarea;
	uint32_t sizeBuffer;
	int op_code = recibir_operacion(tripulante->socket_MIRAM);
	void* buffer = recibir_buffer(&sizeBuffer, tripulante->socket_MIRAM);

	int desplazamiento = 0;

	memcpy(&sizeTarea, buffer + desplazamiento, sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);

	char* stringCadena = malloc(sizeTarea);
	memcpy(stringCadena, buffer + desplazamiento,sizeTarea);

	char** tareaSpliteada;
	int tiempo;
	switch(op_code){
	case ENTRADA_SALIDA:
		//["GENERAR_OXIGENO","12;3;5;2"]
		//["DESCARTAR_BASURA","3;1;7"] ---> sin parámetro
		tareaSpliteada = string_split(stringCadena," ");
		char* parametros = tareaSpliteada[1];
		char** parametrosSpliteados = string_split(parametros,";");

		if(parametrosSpliteados[3]==NULL){
			tiempo = atoi(parametrosSpliteados[2]); //SI NO LO HAGO ASI, proximaTarea->tiempo QUEDA IGUAL A 0 (NO SE POR QUÉ)
			proximaTarea->parametro = -1;
			proximaTarea->posicion.posX = atoi(parametrosSpliteados[0]);
			proximaTarea->posicion.posY = atoi(parametrosSpliteados[1]);
		}
		else
		{
			tiempo = atoi(parametrosSpliteados[3]); //SI NO LO HAGO ASI, proximaTarea->tiempo QUEDA IGUAL A 0 (NO SE POR QUÉ)
			proximaTarea->parametro = atoi(parametrosSpliteados[0]);
			proximaTarea->posicion.posX = atoi(parametrosSpliteados[1]);
			proximaTarea->posicion.posY = atoi(parametrosSpliteados[2]);
		}

		proximaTarea->nombre = tareaSpliteada[0];
		proximaTarea->longNombre = strlen(proximaTarea->nombre) + 1;
		proximaTarea->tiempo = tiempo;
		proximaTarea->esDeEntradaSalida = true;
		break;
	case COMUN:
		//["ABURRIRSE","1","3","4"]
		tareaSpliteada = string_split(stringCadena,";");

		tiempo = atoi(tareaSpliteada[3]); //SI NO LO HAGO ASI, proximaTarea->tiempo QUEDA IGUAL A 0 (NO SE POR QUÉ)
		proximaTarea->nombre = tareaSpliteada[0];
		proximaTarea->longNombre = strlen(proximaTarea->nombre) + 1;
		proximaTarea->parametro = -1;
		proximaTarea->posicion.posX = atoi(tareaSpliteada[1]);
		proximaTarea->posicion.posY = atoi(tareaSpliteada[2]);
		proximaTarea->tiempo = tiempo;
		proximaTarea->esDeEntradaSalida = false;
		break;
	default:
		log_error(loggerDiscordiador,"HUBO UN ERROR AL RECIBIR LA TAREA");
		break;
	}

	free(buffer);

	return proximaTarea;
}

//SOLO METO EN EL BUFFER EL ID, EL ESTADO Y LA POSICION, QUE ES LO QUE NECESITA MI-RAM
void* serializar_tripulante(t_tripulante* tripulante){
	int bytes = 2*sizeof(uint32_t) + sizeof(char) + sizeof(posicion);
	void* magic = malloc(bytes);
	char estado = getEstadoComoCaracter(tripulante->estado);

	int desplazamiento = 0;

	memcpy(magic + desplazamiento, &(tripulante->idPatota), sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);
	memcpy(magic + desplazamiento, &(tripulante->tid), sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);
	memcpy(magic + desplazamiento, &(estado), sizeof(char));
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
	pthread_mutex_lock(&mutexColaReady);
	pthread_mutex_lock(&mutexColaExec);
	if(!list_is_empty(colaReady) && list_size(colaExec)<GRADO_MULTITAREA){
		t_tripulante* tripulante = list_find(colaReady,primerTripulanteDeshabilitado);
		pthread_mutex_lock(&mutexTripulantes);
		tripulante->habilitado = true;
		pthread_mutex_unlock(&mutexTripulantes);

		sem_post(&(tripulante->puedeEjecutar));
	}
	pthread_mutex_unlock(&mutexColaExec);
	pthread_mutex_unlock(&mutexColaReady);
}

void habilitarSiCorresponde(t_tripulante* tripulante){
	bool buscarTripulante(void* elemento){
		return ((t_tripulante*) elemento)->tid == tripulante->tid;
	}

	pthread_mutex_lock(&mutexColaReady);
	pthread_mutex_lock(&mutexColaExec);
	if(list_any_satisfy(list_take(colaReady,GRADO_MULTITAREA),buscarTripulante) && list_size(colaExec)<GRADO_MULTITAREA && !tripulante->habilitado){
		pthread_mutex_lock(&mutexTripulantes);
		tripulante->habilitado = true;
		pthread_mutex_unlock(&mutexTripulantes);

		sem_post(&(tripulante->puedeEjecutar));
	}
	pthread_mutex_unlock(&mutexColaExec);
	pthread_mutex_unlock(&mutexColaReady);
}

bool existeElTripulante(uint32_t idTripulante){
	bool encontrarTripulante(void* elemento){
		return ((t_tripulante*) elemento)->tid == idTripulante;
	}

	return list_find(tripulantes,encontrarTripulante)!=NULL;
}

bool tieneTareasPendientes(t_tripulante* tripulante){
	return tripulante->tareasPendientes > 0;
}

void ejecutarTarea(t_tripulante* tripulante){
	bool buscarTripulante(void* elemento){
		return ((t_tripulante*) elemento)->tid == tripulante->tid;
	}

	if(tripulante->proxTarea->esDeEntradaSalida){

		//realizarPeticionIO(tripulante->socket_MONGO) -> LO HACE EN EXEC, DESPUÉS PASA A BLOCK

		switch((int) dictionary_get(diccionarioTareas,tripulante->proxTarea->nombre)){
		case 1:{
			//GENERAR_OXIGENO
			printf("[TRIPULANTE %d] ESCRIBIR %d O EN EL ARCHIVO Oxigeno.ims\n",tripulante->tid, tripulante->proxTarea->parametro);
			break;
		}
		case 2:{
			//CONSUMIR_OXIGENO
			printf("[TRIPULANTE %d] SACAR %d O DEL ARCHIVO Oxigeno.ims\n",tripulante->tid, tripulante->proxTarea->parametro);
			break;
		}
		case 3:{
			//GENERAR_COMIDA
			printf("[TRIPULANTE %d] ESCRIBIR %d C EN EL ARCHIVO Comida.ims\n",tripulante->tid, tripulante->proxTarea->parametro);
			break;
		}
		case 4:{
			//CONSUMIR_COMIDA
			printf("[TRIPULANTE %d] SACAR %d C DEL ARCHIVO Comida.ims\n",tripulante->tid, tripulante->proxTarea->parametro);
			break;
		}
		case 5:{
			//GENERAR_BASURA
			printf("[TRIPULANTE %d] ESCRIBIR %d B EN EL ARCHIVO Basura.ims\n",tripulante->tid, tripulante->proxTarea->parametro);
			break;
		}
		case 6:{
			//DESCARTAR_BASURA
			printf("[TRIPULANTE %d] ELIMINAR EL ARCHIVO Basura.ims\n",tripulante->tid);
			break;
		}
		default:{
			log_warning(loggerDiscordiador,"SE LEYÓ MAL EL NOMBRE DE LA TAREA");
			break;
		}
		}

		//SI JUSTO EL TRIPULANTE ES EXPULSADO DURANTE LA RÁFAGA EN LA QUE HACE LA PETICIÓN DE I/O, NO ENTRA ACÁ
		if(!tripulante->expulsado){
			pthread_mutex_lock(&mutexColaExec);
			list_remove_by_condition(colaExec,buscarTripulante);
			pthread_mutex_unlock(&mutexColaExec);

			agregarABlockIO(tripulante);

			pthread_mutex_lock(&mutexTripulantes);
			tripulante->habilitado = false;
			pthread_mutex_unlock(&mutexTripulantes);

			habilitarProximoAEjecutar();

			log_info(loggerDiscordiador,"[TRIPULANTE %d] ME BLOQUEO POR I/O",tripulante->tid);
		}

		pthread_mutex_lock(&mutexEjecutarIO);
		for(int i=0; i<tripulante->proxTarea->tiempo && !tripulante->expulsado ;i++){
			sem_wait(&(tripulante->semaforoPlanificacion));
			sleep(RETARDO_CICLO_CPU);
			sem_post(&(tripulante->semaforoPlanificacion));
		}
		pthread_mutex_unlock(&mutexEjecutarIO);

		//SI ES EXPULSADO MIENTRAS EJECUTA LA ENTRADA/SALIDA, NO ENTRA ACÁ
		if(!tripulante->expulsado){
			pthread_mutex_lock(&mutexColaBlockIO);
			list_remove_by_condition(colaBlockIO,buscarTripulante);
			pthread_mutex_unlock(&mutexColaBlockIO);

			pthread_mutex_lock(&mutexTripulantes);
			tripulante->tareasPendientes--;
			tripulante->habilitado = false;
			pthread_mutex_unlock(&mutexTripulantes);

			if(tripulante->tareasPendientes > 0)
				agregarAReady(tripulante);

			log_info(loggerDiscordiador,"[TRIPULANTE %d] TERMINÉ EL BLOQUEO POR I/O",tripulante->tid);
		}
	}
	else
	{
		log_info(loggerDiscordiador,"[TRIPULANTE %d] COMIENZO A EJECUTAR %s",tripulante->tid,tripulante->proxTarea->nombre);

		for(int i=0; i<tripulante->proxTarea->tiempo && !tripulante->expulsado ;i++){
			sem_wait(&(tripulante->semaforoPlanificacion));
			sleep(RETARDO_CICLO_CPU);
			sem_post(&(tripulante->semaforoPlanificacion));
		}

		if(!tripulante->expulsado){
			log_info(loggerDiscordiador,"[TRIPULANTE %d] TERMINÉ DE EJECUTAR %s",tripulante->tid,tripulante->proxTarea->nombre);

			pthread_mutex_lock(&mutexTripulantes);
			tripulante->tareasPendientes--;
			tripulante->habilitado = false;
			pthread_mutex_unlock(&mutexTripulantes);

			if(tripulante->tareasPendientes > 0)
				sem_post(&(tripulante->puedeEjecutar));
			else
			{
				pthread_mutex_lock(&mutexColaExec);
				list_remove_by_condition(colaExec,buscarTripulante);
				pthread_mutex_unlock(&mutexColaExec);

				habilitarProximoAEjecutar();
			}

		}
	}
}

void planificarTripulanteFIFO(t_tripulante* tripulante){
	bool buscarTripulante(void* elemento){
		return ((t_tripulante*) elemento)->tid == tripulante->tid;
	}

	pthread_mutex_lock(&mutexTripulantes);
	Tarea* proximaTarea = solitarProximaTarea(tripulante);
	tripulante->proxTarea = proximaTarea;
	pthread_mutex_unlock(&mutexTripulantes);

	agregarAReady(tripulante);

	sem_wait(&(tripulante->semaforoPlanificacion));
	habilitarSiCorresponde(tripulante);
	sem_post(&(tripulante->semaforoPlanificacion));

	while(tieneTareasPendientes(tripulante) && !tripulante->expulsado){
		sem_wait(&(tripulante->puedeEjecutar));

		//ACÁ ENTRA SI ES EXPULSADO MIENTRAS ESTÁ EN READY
		if(tripulante->expulsado)
			break;

		log_info(loggerDiscordiador,"[TRIPULANTE %d] TENGO QUE LLEGAR A %d|%d",tripulante->tid,tripulante->proxTarea->posicion.posX,tripulante->proxTarea->posicion.posY);

		if(tripulante->estado == READY){
			pthread_mutex_lock(&mutexColaReady);
			list_remove_by_condition(colaReady,buscarTripulante);
			pthread_mutex_unlock(&mutexColaReady);

			agregarAExec(tripulante);
		}

		while(tripulante->posicion->posX != tripulante->proxTarea->posicion.posX && !tripulante->expulsado){
			sem_wait(&(tripulante->semaforoPlanificacion));
			moverXDelTripulante(tripulante);
			sem_post(&(tripulante->semaforoPlanificacion));

			informarMovimiento(tripulante->socket_MIRAM,tripulante);
			sleep(RETARDO_CICLO_CPU);
		}

		while(tripulante->posicion->posY != tripulante->proxTarea->posicion.posY && !tripulante->expulsado){
			sem_wait(&(tripulante->semaforoPlanificacion));
			moverYDelTripulante(tripulante);
			sem_post(&(tripulante->semaforoPlanificacion));

			informarMovimiento(tripulante->socket_MIRAM,tripulante);
			sleep(RETARDO_CICLO_CPU);
		}

		pthread_mutex_lock(&mutexTripulantes);
		if(!tripulante->expulsado)
			log_info(loggerDiscordiador,"[TRIPULANTE %d] LLEGUÉ A LA TAREA %s",tripulante->tid, tripulante->proxTarea->nombre);
		pthread_mutex_unlock(&mutexTripulantes);

		//ACÁ ENTRA SI ES EXPULSADO MIENTRAS SE MOVÍA HACIA SU SIGUIENTE TAREA
		if(tripulante->expulsado)
			break;

		ejecutarTarea(tripulante);

		if(!tripulante->expulsado && tripulante->tareasPendientes > 0){
			pthread_mutex_lock(&mutexTripulantes);
			proximaTarea = solitarProximaTarea(tripulante);
			tripulante->proxTarea = proximaTarea;
			pthread_mutex_unlock(&mutexTripulantes);

			habilitarSiCorresponde(tripulante);
		}
	}

	//SI EL TRIPULANTE ES EXPULSADO MIENTRAS EJECUTA, NO ENTRA ACÁ
	if(!tripulante->expulsado){
		log_info(loggerDiscordiador,"[TRIPULANTE %d] TERMINÉ",tripulante->tid);
		agregarAExit(tripulante);

		pthread_mutex_lock(&mutexColaExit);
		if(list_size(colaExit)==list_size(tripulantes)){
			log_info(loggerDiscordiador,"NO HAY MÁS TRIPULANTES PARA PLANIFICAR. SE CIERRA LA PLANIFICACIÓN");
			pthread_mutex_lock(&mutexActivarPlanificacion);
			planificacionActivada = false;
			pthread_mutex_unlock(&mutexActivarPlanificacion);
		}
		pthread_mutex_unlock(&mutexColaExit);
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
		break;
	}
	}
}

void gestionarTripulante(t_tripulante* tripulante){
	tripulante->socket_MIRAM = crearConexionCliente(IP_MI_RAM,PUERTO_MI_RAM);

	t_paquete* paquete = malloc(sizeof(t_paquete));
	paquete->codigo_operacion = INICIAR_TRIPULANTE;
	paquete->buffer = malloc(sizeof(t_buffer));
	paquete->buffer->size = 2*sizeof(uint32_t) + sizeof(char) + sizeof(posicion);
	paquete->buffer->stream = serializar_tripulante(tripulante);
	enviar_paquete(paquete,tripulante->socket_MIRAM);
	eliminar_paquete(paquete);

	pthread_mutex_lock(&mutexActivarPlanificacion);
	if(planificacionFueActivadaAlgunaVez && !planificacionActivada){
		//SI SE CREA UNA PATOTA DESPUÉS DE HABER PAUSADO LA PLANIFICACIÓN, LOS TRIPULANTES QUEDAN EN NEW ESPERANDO A QUE SE REANUDE
		sem_wait(&(tripulante->semaforoPlanificacion));
		sem_post(&(tripulante->semaforoPlanificacion));
	}
	pthread_mutex_unlock(&mutexActivarPlanificacion);

	//SI ES EXPULSADO MIENTRAS ESTÁ EN NEW CON LA PLANIFICACIÓN PAUSADA, NO PLANIFICA, PASA DIRECTO A EXIT
	if(!tripulante->expulsado)
		planificarTripulante(tripulante);
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
		tripulante->estado = NEW;
		tripulante->posicion = list_get(estructura->coordenadasTripulantes,i);
		tripulante->tareasPendientes = patota->cantidadTareas;
		tripulante->habilitado = false;
		tripulante->expulsado = false;
		sem_init(&(tripulante->semaforoPlanificacion),0,0);
		sem_init(&(tripulante->puedeEjecutar),0,0);

		pthread_mutex_lock(&mutexTripulantes);
		list_add(patota->tripulantes,tripulante);
		list_add(tripulantes,tripulante);
		sumarIdTripulante();
		pthread_mutex_unlock(&mutexTripulantes);

		pthread_t hiloTripulante;
		pthread_create(&hiloTripulante,NULL, (void*) gestionarTripulante, tripulante);
		pthread_detach(hiloTripulante);
	}

	if(patota->pid == 1)
		log_info(loggerDiscordiador,"Discordiador LISTO PARA PLANIFICAR");
}

void listarTripulantes(){
	if(list_is_empty(tripulantes)){
		log_info(loggerDiscordiador,"NO HAY TRIPULANTES!");
	}
	else
	{
		printf("--------------------------------------------\n");
		printf("Estado de la Nave: %s\n",temporal_get_string_time("%d/%m/%y %H:%M:%S"));
		for(int i=0; i<list_size(tripulantes); i++){
			t_tripulante* tripulante = list_get(tripulantes,i);
			pthread_mutex_lock(&mutexTripulantes);
			printf("Tripulante: %d	Patota: %d	Status: %s\n",tripulante->tid, tripulante->idPatota, getEstadoComoCadena(tripulante->estado));
			pthread_mutex_unlock(&mutexTripulantes);
		}
		printf("--------------------------------------------\n");
	}
}

void expulsarTripulante(int id_tripulante){
	bool buscarTripulante(void* elemento){
		return ((t_tripulante*) elemento)->tid == id_tripulante;
	}

	pthread_mutex_lock(&mutexTripulantes);
	t_tripulante* tripulante = list_find(tripulantes,buscarTripulante);
	pthread_mutex_unlock(&mutexTripulantes);

	switch(tripulante->estado){
	case EXIT:
		log_info(loggerDiscordiador,"EL TRIPULANTE %d YA TERMINÓ O YA FUE EXPULSADO",tripulante->tid);
		return;
		break;
	case NEW:
		agregarAExit(tripulante);
		sem_post(&(tripulante->semaforoPlanificacion));
		break;
	case READY:
		pthread_mutex_lock(&mutexColaReady);
		list_remove_by_condition(colaReady,buscarTripulante);
		pthread_mutex_unlock(&mutexColaReady);

		agregarAExit(tripulante);
		break;
	case EXEC:
		pthread_mutex_lock(&mutexColaExec);
		if(list_size(colaExec)==1){
			pthread_mutex_lock(&mutexActivarPlanificacion);
			planificacionActivada = false;
			pthread_mutex_unlock(&mutexActivarPlanificacion);
		}

		list_remove_by_condition(colaExec,buscarTripulante);
		pthread_mutex_unlock(&mutexColaExec);

		agregarAExit(tripulante);
		break;
	case BLOCK_IO:
		pthread_mutex_lock(&mutexColaBlockIO);
		list_remove_by_condition(colaBlockIO,buscarTripulante);
		pthread_mutex_unlock(&mutexColaBlockIO);

		agregarAExit(tripulante);
		break;
	case BLOCK_SABOTAJE:
		//se lo saca de la cola de bloqueados por sabotaje
		agregarAExit(tripulante);
		break;
	default:
		log_warning(loggerDiscordiador,"Hubo un error con el estado del Tripulante");
		break;
	}

	tipo_mensaje finalizar = EXPULSAR_TRIPULANTE;
	send(tripulante->socket_MIRAM,&finalizar,sizeof(tipo_mensaje),0);

	close(tripulante->socket_MIRAM);

	pthread_mutex_lock(&mutexTripulantes);
	tripulante->expulsado = true;
	pthread_mutex_unlock(&mutexTripulantes);
	sem_post(&(tripulante->puedeEjecutar)); //PARA QUE DEJE DE ESPERAR Y TERMINE EL HILO (HORRIBLE e.e)

	log_info(loggerDiscordiador,"SE EXPULSA AL TRIPULANTE %d",id_tripulante);
}

void iniciarPlanificacion(){
	void habilitarSemaforo(void* elemento){
		t_tripulante* tripulante = (t_tripulante*) elemento;
		sem_post(&tripulante->semaforoPlanificacion);
	}

	list_map(tripulantes,(void*) habilitarSemaforo);

	pthread_mutex_lock(&mutexActivarPlanificacion);
	planificacionActivada = true;
	planificacionFueActivadaAlgunaVez = true;
	pthread_mutex_unlock(&mutexActivarPlanificacion);

	log_info(loggerDiscordiador,"SE INICIA LA PLANIFICACIÓN");
}

void pausarPlanificacion(){
	void deshabilitarSemaforo(void* elemento){
		t_tripulante* tripulante = (t_tripulante*) elemento;
		sem_wait(&tripulante->semaforoPlanificacion);
	}

	list_map(tripulantes,(void*) deshabilitarSemaforo);

	pthread_mutex_lock(&mutexActivarPlanificacion);
	planificacionActivada = false;
	pthread_mutex_unlock(&mutexActivarPlanificacion);

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

void informarInicioDeTarea(int socketIMONGO, uint32_t tid, Tarea* tarea)
{
	t_paquete* paquete = malloc(sizeof(t_paquete));
	paquete->buffer = malloc(sizeof(t_buffer));
	int desplazamiento = 0;

	paquete->codigo_operacion = INICIO_TAREA;
	paquete->buffer->size = sizeof(uint32_t) + sizeof(tarea->nombre);
	paquete->buffer->stream = malloc(paquete->buffer->size);

	memcpy(paquete->buffer->stream + desplazamiento, &tid,sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);
	memcpy(paquete->buffer->stream + desplazamiento, &tarea->nombre,sizeof(tarea->nombre));

	enviar_paquete(paquete, socketIMONGO);
	eliminar_paquete(paquete);
}

void informarFinalizacionDeTarea(int socketIMONGO, uint32_t tid, Tarea* tarea)
{
	t_paquete* paquete = malloc(sizeof(t_paquete));
	paquete->buffer = malloc(sizeof(t_buffer));
	int desplazamiento = 0;

	paquete->codigo_operacion = FINALIZO_TAREA;
	paquete->buffer->size = sizeof(uint32_t) + sizeof(tarea->nombre);
	paquete->buffer->stream = malloc(paquete->buffer->size);

	memcpy(paquete->buffer->stream + desplazamiento, &tid,sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);
	memcpy(paquete->buffer->stream + desplazamiento, &tarea->nombre,sizeof(tarea->nombre));

	enviar_paquete(paquete, socketIMONGO);
	eliminar_paquete(paquete);
}

void informarTripulanteAtiendeSabotaje(int socketIMONGO, uint32_t tid)
{
	t_paquete* paquete = malloc(sizeof(t_paquete));
	paquete->buffer = malloc(sizeof(t_buffer));

	paquete->codigo_operacion = ATENDER_SABOTAJE;
	paquete->buffer->size = sizeof(uint32_t);
	paquete->buffer->stream = malloc(paquete->buffer->size);

	memcpy(paquete->buffer->stream,&tid,sizeof(uint32_t));

	enviar_paquete(paquete, socketIMONGO);
	eliminar_paquete(paquete);
}

void informarTripulanteResuelveSabotaje(int socketIMONGO, uint32_t tid)
{
	t_paquete* paquete = malloc(sizeof(t_paquete));
	paquete->buffer = malloc(sizeof(t_buffer));

	paquete->codigo_operacion = RESOLUCION_SABOTAJE;
	paquete->buffer->size = sizeof(uint32_t);
	paquete->buffer->stream = malloc(paquete->buffer->size);

	memcpy(paquete->buffer->stream,&tid,sizeof(uint32_t));

	enviar_paquete(paquete, socketIMONGO);
	eliminar_paquete(paquete);
}

/*
 * Comunicacion.c
 *
 *  Created on: 8 jun. 2021
 *      Author: utnso
 */

#include "Discordiador.h"

//PARA MANDÁRSELO A MI-RAM
char getEstadoComoCaracter(estado estado){
	return estado==NEW?'N':estado==READY?'R':estado==EXEC?'E':estado==EXIT?'F':'B';
}

//PARA IMPRIMIRLO CON LISTAR_TRIPULANTES
char* getEstadoComoCadena(estado estado){
	return estado==NEW?"NEW":estado==READY?"READY":estado==EXEC?"EXEC":estado==EXIT?"EXIT":estado==BLOCK_IO?"BLOCK I/O":"BLOCK EMERGENCIA";
}

void agregarAReady(t_tripulante* tripulante){
	pthread_mutex_lock(&mutexColaReady);
	list_add(colaReady,tripulante);
	pthread_mutex_unlock(&mutexColaReady);

	pthread_mutex_lock(&mutexTripulantes);
	tripulante->estado = READY;
	pthread_mutex_unlock(&mutexTripulantes);

	notificarCambioDeEstado(tripulante);
}

void agregarAExec(t_tripulante* tripulante){
	pthread_mutex_lock(&mutexColaExec);
	list_add(colaExec,tripulante);
	pthread_mutex_unlock(&mutexColaExec);

	pthread_mutex_lock(&mutexTripulantes);
	tripulante->estado = EXEC;
	pthread_mutex_unlock(&mutexTripulantes);

	notificarCambioDeEstado(tripulante);
}

void agregarAExit(t_tripulante* tripulante){
	pthread_mutex_lock(&mutexColaExit);
	list_add(colaExit,tripulante);
	pthread_mutex_unlock(&mutexColaExit);

	pthread_mutex_lock(&mutexTripulantes);
	tripulante->estado = EXIT;
	pthread_mutex_unlock(&mutexTripulantes);

	notificarCambioDeEstado(tripulante);
}

void agregarABlockIO(t_tripulante* tripulante){
	pthread_mutex_lock(&mutexColaBlockIO);
	list_add(colaBlockIO,tripulante);
	pthread_mutex_unlock(&mutexColaBlockIO);

	pthread_mutex_lock(&mutexTripulantes);
	tripulante->estado = BLOCK_IO;
	pthread_mutex_unlock(&mutexTripulantes);

	notificarCambioDeEstado(tripulante);
}

void sacarDeReady(t_tripulante* tripulante){
	bool buscarTripulante(void* elemento){
		return ((t_tripulante*) elemento)->tid == tripulante->tid;
	}

	pthread_mutex_lock(&mutexColaReady);
	list_remove_by_condition(colaReady,buscarTripulante);
	pthread_mutex_unlock(&mutexColaReady);
}

void sacarDeExec(t_tripulante* tripulante){
	bool buscarTripulante(void* elemento){
		return ((t_tripulante*) elemento)->tid == tripulante->tid;
	}

	pthread_mutex_lock(&mutexColaExec);
	list_remove_by_condition(colaExec,buscarTripulante);
	pthread_mutex_unlock(&mutexColaExec);
}

void sacarDeBlockIO(t_tripulante* tripulante){
	bool buscarTripulante(void* elemento){
		return ((t_tripulante*) elemento)->tid == tripulante->tid;
	}

	pthread_mutex_lock(&mutexColaBlockIO);
	list_remove_by_condition(colaBlockIO,buscarTripulante);
	pthread_mutex_unlock(&mutexColaBlockIO);
}

void sacarDeBlockEmergencia(t_tripulante* tripulante){
	bool buscarTripulante(void* elemento){
		return ((t_tripulante*) elemento)->tid == tripulante->tid;
	}

	pthread_mutex_lock(&mutexColaBlockSabotaje);
	list_remove_by_condition(colaBlockEmergencia,buscarTripulante);
	pthread_mutex_unlock(&mutexColaBlockSabotaje);
}

bool llegoALaPosicion(t_tripulante* tripulante, posicion* posicion){
	pthread_mutex_lock(&mutexTripulantes);
	bool retorno = (tripulante->posicion->posX == posicion->posX) && (tripulante->posicion->posY == posicion->posY);
	pthread_mutex_unlock(&mutexTripulantes);

	return retorno;
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

				liberarArray(coordenadas);
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
		tareas = fopen("/home/utnso/Tareas/tareasPatotaDefault.txt","r");
	}

	int MAX_PER_LINE = 50;
	char* buffer = malloc(sizeof(char) * MAX_PER_LINE);
	char* buffer_total = string_new();
	char* result_string;
	int linesize;

	while(fgets(buffer, MAX_PER_LINE, tareas) != NULL){
		linesize = strlen(buffer);
		if(linesize>0 && buffer[linesize - 1] == 10)
			buffer[linesize - 1] = 0;

		string_append(&buffer_total, buffer);
		string_append(&buffer_total, "|");
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

Tarea* solitarProximaTarea(int socket_cliente_MIRAM){
	Tarea* proximaTarea = malloc(sizeof(Tarea));

	tipo_mensaje mensaje = PROXIMA_TAREA;
	send(socket_cliente_MIRAM,&mensaje,sizeof(tipo_mensaje),0);

	uint32_t sizeTarea;
	uint32_t sizeBuffer;
	int op_code = recibir_operacion(socket_cliente_MIRAM);
	void* buffer = recibir_buffer(&sizeBuffer, socket_cliente_MIRAM);

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
		//["DESCARTAR_BASURA","0;3;1;7"]
		tareaSpliteada = string_split(stringCadena," ");
		char* parametros = tareaSpliteada[1];
		char** parametrosSpliteados = string_split(parametros,";");

		tiempo = atoi(parametrosSpliteados[3]); //SI NO LO HAGO ASI, proximaTarea->tiempo QUEDA IGUAL A 0 (NO SE POR QUÉ)
		proximaTarea->parametro = atoi(parametrosSpliteados[0]);
		proximaTarea->posicion.posX = atoi(parametrosSpliteados[1]);
		proximaTarea->posicion.posY = atoi(parametrosSpliteados[2]);
		proximaTarea->nombre = tareaSpliteada[0];
		proximaTarea->longNombre = strlen(proximaTarea->nombre) + 1;
		proximaTarea->tiempo = tiempo;
		proximaTarea->esDeEntradaSalida = true;

		liberarArray(parametrosSpliteados);
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

	proximaTarea->finalizada = false;
	proximaTarea->yaInicio = false;

	free(tareaSpliteada);
	free(buffer);
	free(stringCadena);

	return proximaTarea;
}

void notificarCambioDeEstado(t_tripulante* tripulante){
	tipo_mensaje op_code = CAMBIO_ESTADO;

	pthread_mutex_lock(&mutexTripulantes);
	char estadoTripulante = getEstadoComoCaracter(tripulante->estado);
	pthread_mutex_unlock(&mutexTripulantes);

	send(tripulante->socket_MIRAM,&op_code,sizeof(tipo_mensaje),0);
	send(tripulante->socket_MIRAM,&estadoTripulante,sizeof(char),0);
}

void notificarMovimientoMIRAM(t_tripulante* tripulante){
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

	enviar_paquete(paquete, tripulante->socket_MIRAM);
	eliminar_paquete(paquete);
}

void notificarMovimientoIMONGO(t_tripulante* tripulante, uint32_t posXAnterior, uint32_t posYAnterior){
	//Preparo paquete para enviar
	t_paquete* paquete = malloc(sizeof(t_paquete));
	paquete->buffer = malloc(sizeof(t_buffer));
	int desplazamiento = 0;

	paquete->codigo_operacion = INFORMAR_DESPLAZAMIENTO_FS;
	paquete->buffer->size = 4*sizeof(uint32_t);
	paquete->buffer->stream = malloc(paquete->buffer->size);

	memcpy(paquete->buffer->stream + desplazamiento, &(posXAnterior), sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);
	memcpy(paquete->buffer->stream + desplazamiento, &(posYAnterior), sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);
	memcpy(paquete->buffer->stream + desplazamiento, &(tripulante->posicion->posX), sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);
	memcpy(paquete->buffer->stream + desplazamiento, &(tripulante->posicion->posY), sizeof(uint32_t));

	fflush(stdout);

	enviar_paquete(paquete, tripulante->socket_MONGO);
	eliminar_paquete(paquete);
}

void notificarInicioDeTarea(t_tripulante* tripulante){
	t_paquete* paquete = malloc(sizeof(t_paquete));
	paquete->buffer = malloc(sizeof(t_buffer));
	int desplazamiento = 0;

	paquete->codigo_operacion = INICIO_TAREA;
	paquete->buffer->size = sizeof(uint32_t) + tripulante->proxTarea->longNombre;
	paquete->buffer->stream = malloc(paquete->buffer->size);

	memcpy(paquete->buffer->stream + desplazamiento, &(tripulante->proxTarea->longNombre),sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);
	memcpy(paquete->buffer->stream + desplazamiento, tripulante->proxTarea->nombre,tripulante->proxTarea->longNombre);

	enviar_paquete(paquete, tripulante->socket_MONGO);
	eliminar_paquete(paquete);
}

void notificarFinalizacionDeTarea(t_tripulante* tripulante){
	t_paquete* paquete = malloc(sizeof(t_paquete));
	paquete->buffer = malloc(sizeof(t_buffer));
	int desplazamiento = 0;

	paquete->codigo_operacion = FINALIZO_TAREA;
	paquete->buffer->size = sizeof(uint32_t) + tripulante->proxTarea->longNombre;
	paquete->buffer->stream = malloc(paquete->buffer->size);

	memcpy(paquete->buffer->stream + desplazamiento, &(tripulante->proxTarea->longNombre),sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);
	memcpy(paquete->buffer->stream + desplazamiento, tripulante->proxTarea->nombre,tripulante->proxTarea->longNombre);

	enviar_paquete(paquete, tripulante->socket_MONGO);
	eliminar_paquete(paquete);
}

void notificarAtencionSabotaje(t_tripulante* tripulante){
	tipo_mensaje op_code = ATENDER_SABOTAJE;
	send(tripulante->socket_MONGO,&op_code,sizeof(tipo_mensaje),0);
}

void notificarResolucionSabotaje(t_tripulante* tripulante){
	tipo_mensaje op_code = RESOLUCION_SABOTAJE;
	send(tripulante->socket_MONGO,&op_code,sizeof(tipo_mensaje),0);
}

void invocarFSCK(t_tripulante* tripulante){
	tipo_mensaje op_code = INVOCAR_FSCK;
	send(tripulante->socket_MONGO,&op_code,sizeof(tipo_mensaje),0);
}

float distancia(posicion* unaPosicion, posicion* otraPosicion){
	int cateto1, cateto2;
	float retorno;

	cateto1 = abs(unaPosicion->posX - otraPosicion->posX);
	cateto2 = abs(unaPosicion->posY - otraPosicion->posY);
	retorno = sqrt(pow(cateto1,2) + pow(cateto2,2));

	return retorno;
}

t_tripulante* tripulanteMasCercano(posicion* posicion){
	bool posicionMasCercana(void* unTripulante, void* otroTripulante){
		t_tripulante* tripulante1 = (t_tripulante*) unTripulante;
		t_tripulante* tripulante2 = (t_tripulante*) otroTripulante;

		return distancia(tripulante1->posicion,posicion) <= distancia(tripulante2->posicion,posicion);
	}

	t_list* ordenadosPorPosicion = list_sorted(colaBlockEmergencia,posicionMasCercana);

	t_tripulante* tripulante = (t_tripulante*) list_get(ordenadosPorPosicion,0);

	list_destroy(ordenadosPorPosicion);
	return tripulante;
}

void gestionarSabotaje(){
	pthread_mutex_lock(&mutexSituacionEmergencia);
	haySituacionDeEmergencia = true;
	pthread_mutex_unlock(&mutexSituacionEmergencia);

	//SI ESTÁ ACTIVADA, SE PAUSA LA PLANIFICACIÓN
	pthread_mutex_lock(&mutexActivarPlanificacion);
	if(planificacionActivada){
		pthread_mutex_unlock(&mutexActivarPlanificacion);
		pausarPlanificacion();
	}
	else
		pthread_mutex_unlock(&mutexActivarPlanificacion);

	bool ordenarPorId(void* unTripulante, void* otroTripulante){
		return ((t_tripulante*) unTripulante)->tid < ((t_tripulante*) otroTripulante)->tid;
	}

	void pasarABlockPorEmergencia(void* elemento){
		t_tripulante* tripulante = (t_tripulante*) elemento;

		pthread_mutex_lock(&mutexTripulantes);
		bool noEstabaBloqueado = tripulante->estado != BLOCK_IO;
		tripulante->estado = BLOCK_EMERGENCIA;
		pthread_mutex_unlock(&mutexTripulantes);

		if(noEstabaBloqueado)
			notificarCambioDeEstado(tripulante);
	}

	void pasarAReady(void* elemento){
		pthread_mutex_lock(&mutexTripulantes);
		((t_tripulante*) elemento)->estado = READY;
		pthread_mutex_unlock(&mutexTripulantes);

		notificarCambioDeEstado((t_tripulante*) elemento);
	}

	void pasarAExec(void* elemento){
		pthread_mutex_lock(&mutexTripulantes);
		((t_tripulante*) elemento)->estado = EXEC;
		pthread_mutex_unlock(&mutexTripulantes);

		notificarCambioDeEstado((t_tripulante*) elemento);
	}

	void pasarABlockIO(void* elemento){
		pthread_mutex_lock(&mutexTripulantes);
		((t_tripulante*) elemento)->estado = BLOCK_IO;
		pthread_mutex_unlock(&mutexTripulantes);
	}

	//SE ORDENAN LAS COLAS DE EXEC Y READY POR ID DE TRIPULANTE ASCENDENTE (1-2-3...)
	pthread_mutex_lock(&mutexColaExec);
	list_sort(colaExec,ordenarPorId);
	pthread_mutex_unlock(&mutexColaExec);

	pthread_mutex_lock(&mutexColaReady);
	list_sort(colaReady,ordenarPorId);
	pthread_mutex_unlock(&mutexColaReady);

	pthread_mutex_lock(&mutexColaBlockSabotaje);
	list_add_all(colaBlockEmergencia,colaExec);
	list_add_all(colaBlockEmergencia,colaReady);
	pthread_mutex_unlock(&mutexColaBlockSabotaje);

	pthread_mutex_lock(&mutexColaExec);
	list_clean(colaExec);
	pthread_mutex_unlock(&mutexColaExec);

	pthread_mutex_lock(&mutexColaReady);
	list_clean(colaReady);
	pthread_mutex_unlock(&mutexColaReady);

	pthread_mutex_lock(&mutexTripulantes);
	t_tripulante* tripulanteParaElSabotaje = tripulanteMasCercano(posicionSabotajeActual);
	bool estabaEnExec = tripulanteParaElSabotaje->estado == EXEC;
	pthread_mutex_unlock(&mutexTripulantes);

	//SI EL QUE LO RESUELVE ESTABA EN READY, LA VARIABLE GLOBAL QUEDA EN 0
	if(estabaEnExec){
		pthread_mutex_lock(&mutexIdTripulanteSabotaje);
		idTripulanteResolviendoSabotaje = tripulanteParaElSabotaje->tid;
		pthread_mutex_unlock(&mutexIdTripulanteSabotaje);
	}

	t_list* execYReadyPasadosAEmergencia = list_map(colaBlockEmergencia,(void*) pasarABlockPorEmergencia);
	t_list* blockIOPasadosAEmergencia = list_map(colaBlockIO,(void*) pasarABlockPorEmergencia); //NO LOS SACO DE LA DE I/O PORQUE NO CAMBIA EN NADA

	log_info(loggerDiscordiador,"[TRIPULANTE %d] ELEGIDO PARA RESOLVER EL SABOTAJE",tripulanteParaElSabotaje->tid);

	notificarAtencionSabotaje(tripulanteParaElSabotaje);

	//SE MUEVE A LA POSICION DEL SABOTAJE
	while(!llegoALaPosicion(tripulanteParaElSabotaje,posicionSabotajeActual)){
		moverTripulante(tripulanteParaElSabotaje,posicionSabotajeActual);
		sleep(RETARDO_CICLO_CPU);
	}

	//SE LO LLEVA AL FINAL DE LA COLA DE BLOQUEADOS
	sacarDeBlockEmergencia(tripulanteParaElSabotaje);

	pthread_mutex_lock(&mutexColaBlockSabotaje);
	list_add(colaBlockEmergencia,tripulanteParaElSabotaje);
	pthread_mutex_unlock(&mutexColaBlockSabotaje);

	invocarFSCK(tripulanteParaElSabotaje);

	//EMPIEZA A RESOLVERLO
	log_info(loggerDiscordiador,"[TRIPULANTE %d] RESOLVIENDO SABOTAJE...",tripulanteParaElSabotaje->tid);
	for(int i = 0; i<DURACION_SABOTAJE ;i++)
		sleep(RETARDO_CICLO_CPU);

	log_info(loggerDiscordiador,"[TRIPULANTE %d] RESOLVIÓ EL SABOTAJE",tripulanteParaElSabotaje->tid);

	notificarResolucionSabotaje(tripulanteParaElSabotaje);

	if(planificacionFueActivadaAlgunaVez){
		t_list* tripulantesEnExec;
		t_list* listaPasadosAExec;

		if(estabaEnExec)
			tripulantesEnExec = list_take_and_remove(colaBlockEmergencia,GRADO_MULTITAREA - 1);
		else
			tripulantesEnExec = list_take_and_remove(colaBlockEmergencia,GRADO_MULTITAREA);

		pthread_mutex_lock(&mutexColaExec);
		list_add_all(colaExec,tripulantesEnExec);
		pthread_mutex_unlock(&mutexColaExec);

		listaPasadosAExec = list_map(colaExec,(void*) pasarAExec);

		list_destroy(tripulantesEnExec);
		list_destroy(listaPasadosAExec);
	}

	pthread_mutex_lock(&mutexColaReady);
	list_add_all(colaReady,colaBlockEmergencia);
	pthread_mutex_unlock(&mutexColaReady);

	pthread_mutex_lock(&mutexColaBlockSabotaje);
	list_clean(colaBlockEmergencia);
	pthread_mutex_unlock(&mutexColaBlockSabotaje);

	t_list* listaPasadosAReady = list_map(colaReady,(void*) pasarAReady);
	t_list* listaPasadosABlockIO = list_map(colaBlockIO,(void*) pasarABlockIO);

	list_destroy(execYReadyPasadosAEmergencia);
	list_destroy(blockIOPasadosAEmergencia);
	list_destroy(listaPasadosAReady);
	list_destroy(listaPasadosABlockIO);

	pthread_mutex_lock(&mutexSituacionEmergencia);
	haySituacionDeEmergencia = false;
	pthread_mutex_unlock(&mutexSituacionEmergencia);

	//SI NO SE HABIA INICIADO NUNCA LA PLANIFICACION, ESPERAN A QUE SE INICIE POR CONSOLA (VER ESTO)
	if(planificacionFueActivadaAlgunaVez){
		pthread_mutex_lock(&mutexTripulantes);
		tripulanteParaElSabotaje->habilitado = false;
		pthread_mutex_unlock(&mutexTripulantes);

		iniciarPlanificacion();
		habilitarProximoAEjecutar();//SI EL QUE RESOLVIO EL SABOTAJE ESTABA EN EXEC, SE HABILITA AL PRIMERO QUE ESTABA EN READY (DESPUÉS DE HABER ORDENADO POR ID)
	}
}

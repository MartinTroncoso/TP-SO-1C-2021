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

void esperarSiHaySabotaje(t_tripulante* tripulante){
	pthread_mutex_lock(&mutexSituacionEmergencia);
	if(haySituacionDeEmergencia){
		pthread_mutex_unlock(&mutexSituacionEmergencia);
		sem_wait(&(tripulante->semaforoPlanificacion));
		sem_post(&(tripulante->semaforoPlanificacion));
	}
	else
		pthread_mutex_unlock(&mutexSituacionEmergencia);
}

void esperarParaEjecutar(t_tripulante* tripulante){
	pthread_mutex_lock(&mutexIdTripulanteSabotaje);
	idTripulanteResolviendoSabotaje = 0;
	pthread_mutex_unlock(&mutexIdTripulanteSabotaje);

	sem_wait(&(tripulante->puedeEjecutar));
	sacarDeReady(tripulante);
	agregarAExec(tripulante);

	if(tripulante->proxTarea->yaInicio){
		//SE MUEVE NUEVAMENTE A LA POSICIÓN DE LA TAREA (SI ES QUE
		while(!llegoALaPosicion(tripulante,&tripulante->proxTarea->posicion) && !tripulante->expulsado){
			sem_wait(&(tripulante->semaforoPlanificacion));
			moverTripulante(tripulante,&tripulante->proxTarea->posicion);
			sem_post(&(tripulante->semaforoPlanificacion));

			sleep(RETARDO_CICLO_CPU);
		}
	}

}

void moverXDelTripulante(t_tripulante* tripulante, posicion* posicion){
	uint32_t posXAnterior = tripulante->posicion->posX;
	if(tripulante->posicion->posX > posicion->posX){
		pthread_mutex_lock(&mutexTripulantes);
		tripulante->posicion->posX--;
		pthread_mutex_unlock(&mutexTripulantes);
	}
	else
	{
		if(tripulante->posicion->posX < posicion->posX){
			pthread_mutex_lock(&mutexTripulantes);
			tripulante->posicion->posX++;
			pthread_mutex_unlock(&mutexTripulantes);
		}
	}

	notificarMovimientoMIRAM(tripulante);
	notificarMovimientoIMONGO(tripulante,posXAnterior,tripulante->posicion->posY);
}

void moverYDelTripulante(t_tripulante* tripulante, posicion* posicion){
	uint32_t posYAnterior = tripulante->posicion->posY;

	if(tripulante->posicion->posY > posicion->posY){
		pthread_mutex_lock(&mutexTripulantes);
		tripulante->posicion->posY--;
		pthread_mutex_unlock(&mutexTripulantes);
	}
	else
	{
		if(tripulante->posicion->posY < posicion->posY){
			pthread_mutex_lock(&mutexTripulantes);
			tripulante->posicion->posY++;
			pthread_mutex_unlock(&mutexTripulantes);
		}
	}

	notificarMovimientoMIRAM(tripulante);
	notificarMovimientoIMONGO(tripulante,tripulante->posicion->posX,posYAnterior);
}

void moverTripulante(t_tripulante* tripulante,posicion* posicion){
	//PRIMERO SE MUEVE EN EL EJE X, DESPUÉS EN EL Y
	if(tripulante->posicion->posX != posicion->posX)
		moverXDelTripulante(tripulante,posicion);
	else
		moverYDelTripulante(tripulante,posicion);
}

//SOLO METO EN EL BUFFER EL PID, TID, EL ESTADO Y LA POSICION, QUE ES LO QUE NECESITA MI-RAM
void* serializar_tripulante(t_tripulante* tripulante){
	int bytes = 2*sizeof(uint32_t) + sizeof(char) + sizeof(posicion);
	void* magic = malloc(bytes);

	pthread_mutex_lock(&mutexTripulantes);
	char estado = getEstadoComoCaracter(tripulante->estado);
	pthread_mutex_unlock(&mutexTripulantes);

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

	pthread_mutex_lock(&mutexColaReady);
	t_tripulante* tripulante = (t_tripulante*) list_find(colaReady,primerTripulanteDeshabilitado);
	pthread_mutex_unlock(&mutexColaReady);

	//AGARRO AL PRIMER TRIPULANTE DE LA COLA DE READY QUE ESTÉ DESHABILITADO (QUE DEBERÍA SER SIEMPRE EL PRIMERO)
	//PERO PUEDE HABER PROBLEMAS DE CONCURRENCIA, VER MEJOR ESTO
	pthread_mutex_lock(&mutexColaExec);
	if(tripulante!=NULL && list_size(colaExec)<GRADO_MULTITAREA){
		pthread_mutex_lock(&mutexTripulantes);
		tripulante->habilitado = true;
		pthread_mutex_unlock(&mutexTripulantes);

		sem_post(&(tripulante->puedeEjecutar));
	}
	pthread_mutex_unlock(&mutexColaExec);
}

void habilitarSiCorresponde(t_tripulante* tripulante){
	bool buscarTripulante(void* elemento){
		return ((t_tripulante*) elemento)->tid == tripulante->tid;
	}

	pthread_mutex_lock(&mutexColaReady);
	t_list* tripulantesParaEjecutar = list_take(colaReady,GRADO_MULTITAREA);
	pthread_mutex_unlock(&mutexColaReady);

	pthread_mutex_lock(&mutexColaExec);
	if(list_any_satisfy(tripulantesParaEjecutar,buscarTripulante) && list_size(colaExec)<GRADO_MULTITAREA && !tripulante->habilitado){
		pthread_mutex_lock(&mutexTripulantes);
		tripulante->habilitado = true;
		pthread_mutex_unlock(&mutexTripulantes);

		sem_post(&(tripulante->puedeEjecutar));
	}
	pthread_mutex_unlock(&mutexColaExec);

	list_destroy(tripulantesParaEjecutar);
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

void realizarPeticionIO(t_tripulante* tripulante){
	sem_wait(&(tripulante->semaforoPlanificacion));

	tipo_mensaje op_code = PETICION_ENTRADA_SALIDA;
	send(tripulante->socket_MONGO,&op_code,sizeof(tipo_mensaje),0);

	if(tripulante->tid == idTripulanteResolviendoSabotaje)//SI JUSTO LLEGA EL SABOTAJE ANTES DE QUE LLEGUE A LA POSICION DE LA TAREA, ESPERA PARA EJECUTAR
		esperarParaEjecutar(tripulante);
	sem_post(&(tripulante->semaforoPlanificacion));

	sleep(RETARDO_CICLO_CPU);
}

void realizarAccionTareaIO(t_tripulante* tripulante){
	switch((int) dictionary_get(diccionarioTareas,tripulante->proxTarea->nombre)){
	case 1:{
		TAREA_IO tipoTarea = GENERAR_OXIGENO;
		uint32_t caracteresAGenerar = tripulante->proxTarea->parametro;
		send(tripulante->socket_MONGO,&tipoTarea,sizeof(TAREA_IO),0);
		send(tripulante->socket_MONGO,&caracteresAGenerar,sizeof(uint32_t),0);
		log_info(loggerDiscordiador,"[TRIPULANTE %d] SE GENERAN %d OXIGENOS",tripulante->tid,caracteresAGenerar);
		break;
	}
	case 2:{
		TAREA_IO tipoTarea = CONSUMIR_OXIGENO;
		tipo_mensaje respuesta;
		send(tripulante->socket_MONGO,&tipoTarea,sizeof(TAREA_IO),0);
		recv(tripulante->socket_MONGO,&respuesta,sizeof(tipo_mensaje),0);

		if(respuesta == EXISTE_EL_ARCHIVO){
			uint32_t caracteresABorrar = tripulante->proxTarea->parametro;
			send(tripulante->socket_MONGO,&caracteresABorrar,sizeof(uint32_t),0);
			log_info(loggerDiscordiador,"[TRIPULANTE %d] SE BORRAN %d OXIGENOS",tripulante->tid,caracteresABorrar);
		}
		else
			log_info(loggerDiscordiador,"[TRIPULANTE %d] EL ARCHIVO Oxigeno.ims NO EXISTE",tripulante->tid);
		break;
	}
	case 3:{
		TAREA_IO tipoTarea = GENERAR_COMIDA;
		uint32_t caracteresAGenerar = tripulante->proxTarea->parametro;
		send(tripulante->socket_MONGO,&tipoTarea,sizeof(TAREA_IO),0);
		send(tripulante->socket_MONGO,&caracteresAGenerar,sizeof(uint32_t),0);
		log_info(loggerDiscordiador,"[TRIPULANTE %d] SE GENERAN %d COMIDAS",tripulante->tid,caracteresAGenerar);
		break;
	}
	case 4:{
		TAREA_IO tipoTarea = CONSUMIR_COMIDA;
		tipo_mensaje respuesta;
		send(tripulante->socket_MONGO,&tipoTarea,sizeof(TAREA_IO),0);
		recv(tripulante->socket_MONGO,&respuesta,sizeof(tipo_mensaje),0);

		if(respuesta == EXISTE_EL_ARCHIVO){
			uint32_t caracteresABorrar = tripulante->proxTarea->parametro;
			send(tripulante->socket_MONGO,&caracteresABorrar,sizeof(uint32_t),0);
			log_info(loggerDiscordiador,"[TRIPULANTE %d] SE BORRAN %d COMIDAS",tripulante->tid,caracteresABorrar);
		}
		else
			log_info(loggerDiscordiador,"[TRIPULANTE %d] EL ARCHIVO Comida.ims NO EXISTE",tripulante->tid);
		break;
	}
	case 5:{
		TAREA_IO tipoTarea = GENERAR_BASURA;
		uint32_t caracteresAGenerar = tripulante->proxTarea->parametro;
		send(tripulante->socket_MONGO,&tipoTarea,sizeof(TAREA_IO),0);
		send(tripulante->socket_MONGO,&caracteresAGenerar,sizeof(uint32_t),0);
		log_info(loggerDiscordiador,"[TRIPULANTE %d] SE GENERAN %d BASURAS",tripulante->tid,caracteresAGenerar);
		break;
	}
	case 6:{
		TAREA_IO tipoTarea = DESCARTAR_BASURA;
		tipo_mensaje respuesta;
		send(tripulante->socket_MONGO,&tipoTarea,sizeof(TAREA_IO),0);
		recv(tripulante->socket_MONGO,&respuesta,sizeof(tipo_mensaje),0);

		if(respuesta == EXISTE_EL_ARCHIVO)
			log_info(loggerDiscordiador,"[TRIPULANTE %d] SE ELIMINA EL ARCHIVO Basura.ims",tripulante->tid);
		else
			log_info(loggerDiscordiador,"[TRIPULANTE %d] EL ARCHIVO Basura.ims NO EXISTE",tripulante->tid);
		break;
	}
	default:{
		log_warning(loggerDiscordiador,"SE LEYÓ MAL EL NOMBRE DE LA TAREA");
		break;
	}
	}
}

void ejecutarTareaFIFO(t_tripulante* tripulante){
	if(tripulante->proxTarea->esDeEntradaSalida){
		//LO HACE EN EXEC, DESPUÉS PASA A BLOCK
		realizarPeticionIO(tripulante);

		//SI JUSTO EL TRIPULANTE ES EXPULSADO DURANTE LA RÁFAGA EN LA QUE HACE LA PETICIÓN DE I/O, NO ENTRA ACÁ
		if(!tripulante->expulsado){
			esperarSiHaySabotaje(tripulante);

			realizarAccionTareaIO(tripulante);

			sacarDeExec(tripulante);

			agregarABlockIO(tripulante);

			pthread_mutex_lock(&mutexTripulantes);
			tripulante->habilitado = false;
			tripulante->proxTarea->yaInicio = true;
			pthread_mutex_unlock(&mutexTripulantes);

			habilitarProximoAEjecutar();

			log_info(loggerDiscordiador,"[TRIPULANTE %d] ME BLOQUEO POR I/O",tripulante->tid);

			notificarInicioDeTarea(tripulante);
		}

		pthread_mutex_lock(&mutexEjecutarIO);
		for(int i=0; i<tripulante->proxTarea->tiempo && !tripulante->expulsado ;i++){
			sem_wait(&(tripulante->semaforoPlanificacion));
			sem_post(&(tripulante->semaforoPlanificacion));

			sleep(RETARDO_CICLO_CPU);
		}
		pthread_mutex_unlock(&mutexEjecutarIO);

		//SI ES EXPULSADO MIENTRAS EJECUTA LA ENTRADA/SALIDA, NO ENTRA ACÁ
		if(!tripulante->expulsado){
			esperarSiHaySabotaje(tripulante);

			log_info(loggerDiscordiador,"[TRIPULANTE %d] TERMINÉ EL BLOQUEO POR I/O",tripulante->tid);
			sacarDeBlockIO(tripulante);

			notificarFinalizacionDeTarea(tripulante);

			pthread_mutex_lock(&mutexTripulantes);
			tripulante->proxTarea->finalizada = true;
			tripulante->tareasPendientes--;
			tripulante->habilitado = false;
			pthread_mutex_unlock(&mutexTripulantes);

			if(tripulante->tareasPendientes > 0)
				agregarAReady(tripulante);
		}
	}
	else
	{
		esperarSiHaySabotaje(tripulante);

		notificarInicioDeTarea(tripulante);
		log_info(loggerDiscordiador,"[TRIPULANTE %d] EMPIEZO A EJECUTAR %s",tripulante->tid,tripulante->proxTarea->nombre);

		pthread_mutex_lock(&mutexTripulantes);
		tripulante->proxTarea->yaInicio = true;
		pthread_mutex_unlock(&mutexTripulantes);

		//TODO VER SI HAY QUE EJECUTAR TODAS LAS RÁFAGAS DE NUEVO DESPUÉS DE UN SABOTAJE
		for(int i=0; i<tripulante->proxTarea->tiempo && !tripulante->expulsado ;i++){
			sem_wait(&(tripulante->semaforoPlanificacion));
			if(tripulante->tid == idTripulanteResolviendoSabotaje)
				esperarParaEjecutar(tripulante);
			sem_post(&(tripulante->semaforoPlanificacion));

			sleep(RETARDO_CICLO_CPU);
		}

		if(!tripulante->expulsado){
			log_info(loggerDiscordiador,"[TRIPULANTE %d] TERMINÉ DE EJECUTAR %s",tripulante->tid,tripulante->proxTarea->nombre);

			notificarFinalizacionDeTarea(tripulante);

			pthread_mutex_lock(&mutexTripulantes);
			tripulante->proxTarea->finalizada = true;
			tripulante->tareasPendientes--;
			tripulante->habilitado = false;
			pthread_mutex_unlock(&mutexTripulantes);

			esperarSiHaySabotaje(tripulante);

			if(tripulante->tareasPendientes > 0)
				sem_post(&(tripulante->puedeEjecutar));
			else
			{
				sacarDeExec(tripulante);

				habilitarProximoAEjecutar();
			}
		}
	}
}

void ejecutarTareaRR(t_tripulante* tripulante){
	if(tripulante->proxTarea->esDeEntradaSalida){
		//LO HACE EN EXEC, DESPUÉS PASA A BLOCK
		realizarPeticionIO(tripulante);

		//COMO ES RR NORMAL, EL NUEVO QUANTUM PARA LA SIGUIENTE EJECUCION ARRANCA EN 0 (DISTINTO DE VRR)
		pthread_mutex_lock(&mutexTripulantes);
		tripulante->quantum = 0;
		pthread_mutex_unlock(&mutexTripulantes);

		//SI JUSTO EL TRIPULANTE ES EXPULSADO DURANTE LA RÁFAGA EN LA QUE HACE LA PETICIÓN DE I/O, NO ENTRA ACÁ
		if(!tripulante->expulsado){
			esperarSiHaySabotaje(tripulante);

			realizarAccionTareaIO(tripulante);

			sacarDeExec(tripulante);

			agregarABlockIO(tripulante);

			pthread_mutex_lock(&mutexTripulantes);
			tripulante->habilitado = false;
			tripulante->proxTarea->yaInicio = true;
			pthread_mutex_unlock(&mutexTripulantes);

			habilitarProximoAEjecutar();

			log_info(loggerDiscordiador,"[TRIPULANTE %d] ME BLOQUEO POR I/O",tripulante->tid);

			notificarInicioDeTarea(tripulante);
		}

		pthread_mutex_lock(&mutexEjecutarIO);
		for(int i=0; i<tripulante->proxTarea->tiempo && !tripulante->expulsado ;i++){
			sem_wait(&(tripulante->semaforoPlanificacion));
			sem_post(&(tripulante->semaforoPlanificacion));
			sleep(RETARDO_CICLO_CPU);
		}
		pthread_mutex_unlock(&mutexEjecutarIO);

		//SI ES EXPULSADO MIENTRAS EJECUTA LA ENTRADA/SALIDA, NO ENTRA ACÁ
		if(!tripulante->expulsado){
			esperarSiHaySabotaje(tripulante);

			log_info(loggerDiscordiador,"[TRIPULANTE %d] TERMINÉ EL BLOQUEO POR I/O",tripulante->tid);
			sacarDeBlockIO(tripulante);

			notificarFinalizacionDeTarea(tripulante);

			pthread_mutex_lock(&mutexTripulantes);
			tripulante->proxTarea->finalizada = true;
			tripulante->tareasPendientes--;
			tripulante->habilitado = false;
			pthread_mutex_unlock(&mutexTripulantes);

			if(tripulante->tareasPendientes > 0)
				agregarAReady(tripulante);
		}
	}
	else
	{
		if(!tripulante->proxTarea->yaInicio){
			esperarSiHaySabotaje(tripulante);

			notificarInicioDeTarea(tripulante);
			log_info(loggerDiscordiador,"[TRIPULANTE %d] EMPIEZO A EJECUTAR %s",tripulante->tid,tripulante->proxTarea->nombre);

			pthread_mutex_lock(&mutexTripulantes);
			tripulante->proxTarea->yaInicio = true;
			pthread_mutex_unlock(&mutexTripulantes);
		}

		while(tripulante->quantum < QUANTUM && tripulante->proxTarea->tiempo > 0 && !tripulante->expulsado){
			sem_wait(&(tripulante->semaforoPlanificacion));
			if(tripulante->tid == idTripulanteResolviendoSabotaje)
				esperarParaEjecutar(tripulante);
			sem_post(&(tripulante->semaforoPlanificacion));

			sleep(RETARDO_CICLO_CPU);

			pthread_mutex_lock(&mutexTripulantes);
			tripulante->quantum++;
			tripulante->proxTarea->tiempo--;
			pthread_mutex_unlock(&mutexTripulantes);
		}

		//SI EL TRIPULANTE ES EXPULSADO MIENTRAS EJECUTA LA TAREA, NO ENTRA ACÁ
		if(!tripulante->expulsado){
			if(tripulante->proxTarea->tiempo == 0){
				log_info(loggerDiscordiador,"[TRIPULANTE %d] TERMINÉ DE EJECUTAR %s",tripulante->tid,tripulante->proxTarea->nombre);

				notificarFinalizacionDeTarea(tripulante);

				pthread_mutex_lock(&mutexTripulantes);
				tripulante->proxTarea->finalizada = true;
				tripulante->tareasPendientes--;
				pthread_mutex_unlock(&mutexTripulantes);

				esperarSiHaySabotaje(tripulante);

				if(tripulante->tareasPendientes > 0){
					if(tripulante->quantum == QUANTUM){
						log_info(loggerDiscordiador,"[TRIPULANTE %d] CONSUMÍ TODO EL QUANTUM. VUELVO A READY",tripulante->tid);

						sacarDeExec(tripulante);

						agregarAReady(tripulante);

						pthread_mutex_lock(&mutexTripulantes);
						tripulante->quantum = 0;
						tripulante->habilitado = false;
						pthread_mutex_unlock(&mutexTripulantes);

						habilitarProximoAEjecutar();
					}
					else
						sem_post(&(tripulante->puedeEjecutar));
				}
				else
				{
					sacarDeExec(tripulante);

					habilitarProximoAEjecutar();
				}
			}
			else
			{
				//ES EL ÚNICO CASO EN EL QUE SIGUE ADENTRO DEL 2DO WHILE EN 'planificarTripulanteRR',
				//ENTONCES LE SETEO EN FALSE QUE LA TAREA HAYA EMPEZADO PARA QUE VUELVA A EJECUTARLA DESDE 0
				if(haySituacionDeEmergencia){
					pthread_mutex_lock(&mutexTripulantes);
					tripulante->proxTarea->yaInicio = false;
					pthread_mutex_unlock(&mutexTripulantes);
				}

				esperarSiHaySabotaje(tripulante);

				log_info(loggerDiscordiador,"[TRIPULANTE %d] CONSUMÍ TODO EL QUANTUM. VUELVO A READY",tripulante->tid);

				sacarDeExec(tripulante);

				agregarAReady(tripulante);

				pthread_mutex_lock(&mutexTripulantes);
				tripulante->quantum = 0;
				tripulante->habilitado = false;
				pthread_mutex_unlock(&mutexTripulantes);

				habilitarProximoAEjecutar();
			}
		}
	}
}

void planificarTripulanteFIFO(t_tripulante* tripulante){
	pthread_mutex_lock(&mutexTripulantes);
	tripulante->proxTarea = solitarProximaTarea(tripulante->socket_MIRAM);
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
			sacarDeReady(tripulante);

			agregarAExec(tripulante);
			log_info(loggerDiscordiador,"[TRIPULANTE %d] EJECUTO...",tripulante->tid);
		}

		while(!llegoALaPosicion(tripulante,&tripulante->proxTarea->posicion) && !tripulante->expulsado){
			sem_wait(&(tripulante->semaforoPlanificacion));
			if(tripulante->tid == idTripulanteResolviendoSabotaje)
				esperarParaEjecutar(tripulante);

			moverTripulante(tripulante,&tripulante->proxTarea->posicion);
			sem_post(&(tripulante->semaforoPlanificacion));

			sleep(RETARDO_CICLO_CPU);
		}

		//SI ES EXPULSADO MIENTRAS SE MUEVE HACIA LA TAREA, DEJA DE EJECUTAR
		if(tripulante->expulsado)
			break;

		ejecutarTareaFIFO(tripulante);

		if(!tripulante->expulsado && tripulante->tareasPendientes > 0){
			pthread_mutex_lock(&mutexTripulantes);
			free(tripulante->proxTarea->nombre);
			free(tripulante->proxTarea);
			tripulante->proxTarea = solitarProximaTarea(tripulante->socket_MIRAM);
			pthread_mutex_unlock(&mutexTripulantes);

			habilitarSiCorresponde(tripulante);
		}
	}

	//SI EL TRIPULANTE ES EXPULSADO MIENTRAS EJECUTA, NO ENTRA ACÁ
	if(!tripulante->expulsado){
		log_info(loggerDiscordiador,"[TRIPULANTE %d] TERMINÉ",tripulante->tid);
		agregarAExit(tripulante);

		tipo_mensaje finalizar = EXPULSAR_TRIPULANTE;
		send(tripulante->socket_MONGO,&finalizar,sizeof(tipo_mensaje),0);

		pthread_mutex_lock(&mutexTripulantes);
		pthread_mutex_lock(&mutexColaExit);
		if(list_size(colaExit)==list_size(tripulantes)){
			log_info(loggerDiscordiador,"NO HAY MÁS TRIPULANTES PARA PLANIFICAR. SE CIERRA LA PLANIFICACIÓN");
			pthread_mutex_lock(&mutexActivarPlanificacion);
			planificacionActivada = false;
			planificacionFueActivadaAlgunaVez = false; //PARA QUE SI SE CREA UNA PATOTA, PASEN A READY Y NO QUEDEN EN NEW
			pthread_mutex_unlock(&mutexActivarPlanificacion);
		}
		pthread_mutex_unlock(&mutexColaExit);
		pthread_mutex_unlock(&mutexTripulantes);

		close(tripulante->socket_MIRAM);
		close(tripulante->socket_MONGO);
	}
}

void planificarTripulanteRR(t_tripulante* tripulante){
	pthread_mutex_lock(&mutexTripulantes);
	tripulante->proxTarea = solitarProximaTarea(tripulante->socket_MIRAM);
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
		sem_post(&(tripulante->puedeEjecutar));

		while(!tripulante->proxTarea->finalizada && !tripulante->expulsado){
			sem_wait(&(tripulante->puedeEjecutar));

			if(tripulante->estado == READY){
				sacarDeReady(tripulante);

				agregarAExec(tripulante);
				log_info(loggerDiscordiador,"[TRIPULANTE %d] EJECUTO...",tripulante->tid);
			}

			while(tripulante->quantum < QUANTUM && !llegoALaPosicion(tripulante,&tripulante->proxTarea->posicion) && !tripulante->expulsado){
				sem_wait(&(tripulante->semaforoPlanificacion));
				if(tripulante->tid == idTripulanteResolviendoSabotaje)
					esperarParaEjecutar(tripulante);

				moverTripulante(tripulante,&tripulante->proxTarea->posicion);
				sem_post(&(tripulante->semaforoPlanificacion));

				sleep(RETARDO_CICLO_CPU);

				pthread_mutex_lock(&mutexTripulantes);
				tripulante->quantum++;
				pthread_mutex_unlock(&mutexTripulantes);
			}

			//SI ES EXPULSADO MIENTRAS VA HACIA LA TAREA, NO ENTRA ACÁ
			if(!tripulante->expulsado){
				//SI EL QUANTUM DEL TRIPULANTE ES IGUAL AL MAXIMO, QUIERE DECIR QUE NO PUEDE EMPEZAR A EJECUTAR LA TAREA. VUELVE A READY
				if(tripulante->quantum == QUANTUM){
					esperarSiHaySabotaje(tripulante);

					log_info(loggerDiscordiador,"[TRIPULANTE %d] CONSUMÍ TODO EL QUANTUM. VUELVO A READY",tripulante->tid);

					sacarDeExec(tripulante);

					agregarAReady(tripulante);

					pthread_mutex_lock(&mutexTripulantes);
					tripulante->quantum = 0;
					tripulante->habilitado = false;
					pthread_mutex_unlock(&mutexTripulantes);

					habilitarProximoAEjecutar();
				}
				else
					ejecutarTareaRR(tripulante);
			}
		}

		if(!tripulante->expulsado && tripulante->tareasPendientes > 0){
			pthread_mutex_lock(&mutexTripulantes);
			free(tripulante->proxTarea->nombre);
			free(tripulante->proxTarea);
			tripulante->proxTarea = solitarProximaTarea(tripulante->socket_MIRAM);
			pthread_mutex_unlock(&mutexTripulantes);

			habilitarSiCorresponde(tripulante);
		}
	}

	//SI EL TRIPULANTE ES EXPULSADO MIENTRAS EJECUTA, NO ENTRA ACÁ
	if(!tripulante->expulsado){
		log_info(loggerDiscordiador,"[TRIPULANTE %d] TERMINÉ",tripulante->tid);
		agregarAExit(tripulante);

		tipo_mensaje finalizar = EXPULSAR_TRIPULANTE;
		send(tripulante->socket_MONGO,&finalizar,sizeof(tipo_mensaje),0);

		pthread_mutex_lock(&mutexTripulantes);
		pthread_mutex_lock(&mutexColaExit);
		if(list_size(colaExit)==list_size(tripulantes)){
			log_info(loggerDiscordiador,"NO HAY MÁS TRIPULANTES PARA PLANIFICAR. SE CIERRA LA PLANIFICACIÓN");
			pthread_mutex_lock(&mutexActivarPlanificacion);
			planificacionActivada = false;
			planificacionFueActivadaAlgunaVez = false; //PARA QUE SI SE CREA UNA PATOTA, PASEN A READY Y NO QUEDEN EN NEW
			pthread_mutex_unlock(&mutexActivarPlanificacion);
		}
		pthread_mutex_unlock(&mutexColaExit);
		pthread_mutex_unlock(&mutexTripulantes);

		close(tripulante->socket_MIRAM);
		close(tripulante->socket_MONGO);
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
		planificarTripulanteRR(tripulante);
		break;
	}
	default:{
		log_error(loggerDiscordiador,"Hubo un error con el algoritmo de planificación.");
		break;
	}
	}
}

void gestionarTripulante(t_tripulante* tripulante){
	pthread_mutex_lock(&mutexTripulantes);
	tripulante->socket_MIRAM = crearConexionCliente(IP_MI_RAM,PUERTO_MI_RAM);
	tripulante->socket_MONGO = crearConexionCliente(IP_I_MONGO_STORE,PUERTO_I_MONGO_STORE);
	pthread_mutex_unlock(&mutexTripulantes);

	t_paquete* paquete = malloc(sizeof(t_paquete));
	paquete->codigo_operacion = INICIAR_TRIPULANTE;
	paquete->buffer = malloc(sizeof(t_buffer));
	paquete->buffer->size = 2*sizeof(uint32_t) + sizeof(char) + sizeof(posicion);
	paquete->buffer->stream = serializar_tripulante(tripulante);
	enviar_paquete(paquete,tripulante->socket_MIRAM);
	eliminar_paquete(paquete);

	send(tripulante->socket_MONGO,&(tripulante->tid),sizeof(uint32_t),0);

	pthread_mutex_lock(&mutexActivarPlanificacion);
	if(planificacionFueActivadaAlgunaVez){
		pthread_mutex_unlock(&mutexActivarPlanificacion);
		if(!planificacionActivada){
			sem_wait(&(tripulante->semaforoPlanificacion));
			sem_post(&(tripulante->semaforoPlanificacion));
		}
		else
			sem_post(&(tripulante->semaforoPlanificacion));
	}
	else
		pthread_mutex_unlock(&mutexActivarPlanificacion);

	//SI ES EXPULSADO MIENTRAS ESTÁ EN NEW CON LA PLANIFICACIÓN PAUSADA, NO ES PLANIFICADO, PASA DIRECTO A EXIT
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
		pthread_mutex_lock(&mutexTripulantes);
		t_tripulante* tripulante = malloc(sizeof(t_tripulante));
		tripulante->idPatota = patota->pid;
		tripulante->tid = idTripulante;
		tripulante->estado = NEW;
		tripulante->quantum = 0;
		tripulante->posicion = list_get(estructura->coordenadasTripulantes,i);
		tripulante->tareasPendientes = patota->cantidadTareas;
		tripulante->habilitado = false;
		tripulante->expulsado = false;
		sem_init(&(tripulante->semaforoPlanificacion),0,0);
		sem_init(&(tripulante->puedeEjecutar),0,0);
		list_add(patota->tripulantes,tripulante);
		list_add(tripulantes,tripulante);
		sumarIdTripulante();
		pthread_mutex_unlock(&mutexTripulantes);

		pthread_t hiloTripulante;
		pthread_create(&hiloTripulante,NULL, (void*) gestionarTripulante, tripulante);
		pthread_detach(hiloTripulante);
	}

	if(patota->pid == 1 || list_size(colaExit)==list_size(tripulantes))
		log_info(loggerDiscordiador,"Discordiador LISTO PARA PLANIFICAR (%s)",ALGORITMO);
}

void listarTripulantes(){
	if(list_is_empty(tripulantes)){
		log_info(loggerDiscordiador,"NO HAY TRIPULANTES!");
	}
	else
	{
		char* fecha = temporal_get_string_time("%d/%m/%y %H:%M:%S");
		printf("------------------------------------------------------\n");
		printf("Estado de la Nave: %s\n",fecha);
		pthread_mutex_lock(&mutexTripulantes);
		for(int i=0; i<list_size(tripulantes); i++){
			t_tripulante* tripulante = (t_tripulante*) list_get(tripulantes,i);
			printf("Tripulante: %d    Patota: %d    Status: %s\n",tripulante->tid, tripulante->idPatota, getEstadoComoCadena(tripulante->estado));
		}
		pthread_mutex_unlock(&mutexTripulantes);
		printf("------------------------------------------------------\n");

		free(fecha);
	}
}

void expulsarTripulante(int id_tripulante){
	bool buscarTripulante(void* elemento){
		return ((t_tripulante*) elemento)->tid == id_tripulante;
	}

	pthread_mutex_lock(&mutexTripulantes);
	t_tripulante* tripulante = list_find(tripulantes,buscarTripulante);
	tripulante->expulsado = true;
	pthread_mutex_unlock(&mutexTripulantes);

	log_info(loggerDiscordiador,"SE EXPULSA AL TRIPULANTE %d",id_tripulante);

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
		sacarDeReady(tripulante);

		sem_post(&(tripulante->puedeEjecutar));
		sem_post(&(tripulante->semaforoPlanificacion));

		agregarAExit(tripulante);
		break;
	case EXEC:
		pthread_mutex_lock(&mutexColaExec);
		if(list_size(colaExec)==1){
			pthread_mutex_lock(&mutexActivarPlanificacion);
			planificacionActivada = false;
			planificacionFueActivadaAlgunaVez = false;
			pthread_mutex_unlock(&mutexActivarPlanificacion);

			log_info(loggerDiscordiador,"SE PAUSA LA PLANIFICACIÓN");
		}
		pthread_mutex_unlock(&mutexColaExec);

		sacarDeExec(tripulante);

		sem_post(&(tripulante->puedeEjecutar));

		agregarAExit(tripulante);
		break;
	case BLOCK_IO:
		sacarDeBlockIO(tripulante);

		sem_post(&(tripulante->puedeEjecutar));

		agregarAExit(tripulante);
		break;
//	case BLOCK_EMERGENCIA:
//		sacarDeBlockEmergencia(tripulante);
//
//		if(tripulante->habilitado){
//			pthread_mutex_lock(&mutexExpulsadosEnExec);
//			expulsadosEnExecDuranteSabotaje++;
//			pthread_mutex_unlock(&mutexExpulsadosEnExec);
//		}
//
//		sem_post(&(tripulante->puedeEjecutar));
//		sem_post(&(tripulante->semaforoPlanificacion));
//
//		agregarAExit(tripulante);
//		break;
	default:
		log_warning(loggerDiscordiador,"Hubo un error con el estado del Tripulante");
		break;
	}

	tipo_mensaje finalizar = EXPULSAR_TRIPULANTE;
	send(tripulante->socket_MIRAM,&finalizar,sizeof(tipo_mensaje),0);
	send(tripulante->socket_MONGO,&finalizar,sizeof(tipo_mensaje),0);

	close(tripulante->socket_MIRAM);
	close(tripulante->socket_MONGO);
}

void iniciarPlanificacion(){
	void habilitarSemaforo(void* elemento){
		t_tripulante* tripulante = (t_tripulante*) elemento;
		sem_post(&(tripulante->semaforoPlanificacion));
	}

	log_info(loggerDiscordiador,"SE INICIA LA PLANIFICACION");

	t_list* listaMapeada = list_map(tripulantes,(void*) habilitarSemaforo);

	pthread_mutex_lock(&mutexActivarPlanificacion);
	planificacionActivada = true;
	planificacionFueActivadaAlgunaVez = true;
	pthread_mutex_unlock(&mutexActivarPlanificacion);

	list_destroy(listaMapeada);
}

void pausarPlanificacion(){
	void deshabilitarSemaforo(void* elemento){
		t_tripulante* tripulante = (t_tripulante*) elemento;
		sem_wait(&(tripulante->semaforoPlanificacion));
	}

	log_info(loggerDiscordiador,"SE PAUSA LA PLANIFICACIÓN");

	t_list* listaMapeada = list_map(tripulantes,(void*) deshabilitarSemaforo);

	pthread_mutex_lock(&mutexActivarPlanificacion);
	planificacionActivada = false;
	pthread_mutex_unlock(&mutexActivarPlanificacion);

	list_destroy(listaMapeada);
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

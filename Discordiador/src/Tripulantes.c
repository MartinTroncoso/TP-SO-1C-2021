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

//LOS TRIPULANTES SE QUEDAN ESPERANDO SI EL SABOTAJE SE PRODUJO EN EL ÚLTIMO SLEEP DE SU EJECUCIÓN
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

//ESTA FUNCIÓN LA EJECUTA SOLO EL TRIPULANTE QUE RESOLVIÓ EL SABOTAJE.
void esperarParaEjecutar(t_tripulante* tripulante){
	pthread_mutex_lock(&mutexIdTripulanteSabotaje);
	idTripulanteResolviendoSabotaje = 0;
	pthread_mutex_unlock(&mutexIdTripulanteSabotaje);

	if(tripulante->estado == READY){
		sem_wait(&(tripulante->puedeEjecutar));
		sem_wait(&(tripulante->semaforoPlanificacion));
		sem_post(&(tripulante->semaforoPlanificacion));
		sacarDeReady(tripulante);
		agregarAExec(tripulante);
	}

	if(tripulante->proxTarea->yaInicio){
		//SE MUEVE NUEVAMENTE A LA POSICIÓN DE LA TAREA (SI ES QUE ES DISTINTA A LA DEL SABOTAJE)
		if(getAlgoritmoPlanificacion() == RR){
			while(!llegoALaPosicion(tripulante,&tripulante->proxTarea->posicion) && !tripulante->expulsado){
				pthread_mutex_lock(&mutexTripulantes);
				tripulante->quantum = 0;
				pthread_mutex_unlock(&mutexTripulantes);

				while(tripulante->quantum < QUANTUM && !tripulante->expulsado){
					sem_wait(&(tripulante->semaforoPlanificacion));
					sem_post(&(tripulante->semaforoPlanificacion));

					//ACÁ ENTRARÍA SI LE TOCA RESOLVER OTRA VEZ UN SABOJE
					if(tripulante->tid == idTripulanteResolviendoSabotaje && tripulante->estado == READY){
						pthread_mutex_lock(&mutexIdTripulanteSabotaje);
						idTripulanteResolviendoSabotaje = 0;
						pthread_mutex_unlock(&mutexIdTripulanteSabotaje);

						pthread_mutex_lock(&mutexTripulantes);
						tripulante->quantum = 0;
						pthread_mutex_unlock(&mutexTripulantes);

						sem_wait(&(tripulante->puedeEjecutar));
						sacarDeReady(tripulante);
						agregarAExec(tripulante);
					}

					moverTripulante(tripulante,&tripulante->proxTarea->posicion);

					pthread_mutex_lock(&mutexTripulantes);
					tripulante->quantum++;
					pthread_mutex_unlock(&mutexTripulantes);

					sleep(RETARDO_CICLO_CPU);

					if(llegoALaPosicion(tripulante,&tripulante->proxTarea->posicion)){
						esperarSiHaySabotaje(tripulante); //TODO PROBAR
						break;
					}
				}

				if(tripulante->quantum == QUANTUM && !tripulante->expulsado){
					log_info(loggerDiscordiador,"[TRIPULANTE %d] CONSUMÍ TODO EL QUANTUM.",tripulante->tid);

					esperarSiHaySabotaje(tripulante); //SI LLEGA EL SABOTAJE EN EL ÚLTIMO SLEEP, SE ESPERA PARA HABILITAR AL PRÓXIMO

					pthread_mutex_lock(&mutexTripulantes);
					tripulante->habilitado = false;
					pthread_mutex_unlock(&mutexTripulantes);

					if(tripulante->estado == EXEC){
						sacarDeExec(tripulante);

						agregarAReady(tripulante);

						habilitarProximoAEjecutar();
					}

					sem_wait(&(tripulante->puedeEjecutar));

					sacarDeReady(tripulante);
					agregarAExec(tripulante);
				}
			}
		}
		else
		{
			while(!llegoALaPosicion(tripulante,&tripulante->proxTarea->posicion) && !tripulante->expulsado){
				sem_wait(&(tripulante->semaforoPlanificacion));
				sem_post(&(tripulante->semaforoPlanificacion));

				//ACÁ ENTRARÍA SI LE TOCA RESOLVER OTRA VEZ UN SABOJE MIENTRAS VOLVÍA A DIRIGIRSE HACIA LA TAREA
				if(tripulante->tid == idTripulanteResolviendoSabotaje && tripulante->estado == READY){
					pthread_mutex_lock(&mutexIdTripulanteSabotaje);
					idTripulanteResolviendoSabotaje = 0;
					pthread_mutex_unlock(&mutexIdTripulanteSabotaje);

					sem_wait(&(tripulante->puedeEjecutar));
					sacarDeReady(tripulante);
					agregarAExec(tripulante);
				}

				moverTripulante(tripulante,&tripulante->proxTarea->posicion);

				sleep(RETARDO_CICLO_CPU);

				if(llegoALaPosicion(tripulante,&tripulante->proxTarea->posicion))
					esperarSiHaySabotaje(tripulante); //TODO PRINCIPALMENTE PARA QUE SI SE PRODUCE OTRO SABOTAJE EN EL ULTIMO SLEEP, SE VUELVA A MOVER
			}
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

	pthread_mutex_lock(&mutexTripulantes);
	tripulante->proxTarea->yaInicio = true;//SI JUSTO LLEGA EL SABOTAJE EN EL ÚLTIMO SLEEP CAMINO A LA TAREA, PROBABLEMENTE DEBA VOLVER A MOVERSE
	pthread_mutex_unlock(&mutexTripulantes);

	sem_post(&(tripulante->semaforoPlanificacion));

	if(tripulante->tid == idTripulanteResolviendoSabotaje)
		esperarParaEjecutar(tripulante); //TODO TAREA YA INICIADA

	//SI ES EXPULSADO ESTANDO EN 'esperarParaEjecutar()', NO REALIZA LA PETICIÓN DE I/O
	if(!tripulante->expulsado){
		tipo_mensaje op_code = PETICION_ENTRADA_SALIDA;
		send(tripulante->socket_MONGO,&op_code,sizeof(tipo_mensaje),0);

		sleep(RETARDO_CICLO_CPU);
	}
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
			realizarAccionTareaIO(tripulante);

			esperarSiHaySabotaje(tripulante);
			if(idTripulanteResolviendoSabotaje == tripulante->tid)
				esperarParaEjecutar(tripulante);

			log_info(loggerDiscordiador,"[TRIPULANTE %d] ME BLOQUEO POR I/O",tripulante->tid);

			sacarDeExec(tripulante);

			agregarABlockIO(tripulante);

			pthread_mutex_lock(&mutexTripulantes);
			tripulante->habilitado = false;
			pthread_mutex_unlock(&mutexTripulantes);

			habilitarProximoAEjecutar();

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

			if(tieneTareasPendientes(tripulante))
				agregarAReady(tripulante);
		}
	}
	else
	{
		notificarInicioDeTarea(tripulante);
		log_info(loggerDiscordiador,"[TRIPULANTE %d] EMPIEZO A EJECUTAR %s",tripulante->tid,tripulante->proxTarea->nombre);

		pthread_mutex_lock(&mutexTripulantes);
		tripulante->proxTarea->yaInicio = true;
		pthread_mutex_unlock(&mutexTripulantes);

		for(int i=0; i<tripulante->proxTarea->tiempo && !tripulante->expulsado ;i++){
			sem_wait(&(tripulante->semaforoPlanificacion));
			sem_post(&(tripulante->semaforoPlanificacion));
			if(tripulante->tid == idTripulanteResolviendoSabotaje){
				esperarParaEjecutar(tripulante); //TODO TAREA YA INICIADA
				i = 0; //PARA QUE VUELVA A EJECUTAR LA TAREA DESDE EL PRINCIPIO
			}

			sleep(RETARDO_CICLO_CPU);
		}

		if(!tripulante->expulsado){
			log_info(loggerDiscordiador,"[TRIPULANTE %d] TERMINÉ DE EJECUTAR %s",tripulante->tid,tripulante->proxTarea->nombre);

			notificarFinalizacionDeTarea(tripulante);

			pthread_mutex_lock(&mutexTripulantes);
			tripulante->proxTarea->finalizada = true;
			tripulante->tareasPendientes--;
			pthread_mutex_unlock(&mutexTripulantes);

			if(tieneTareasPendientes(tripulante)){
				esperarSiHaySabotaje(tripulante);

				//PREGUNTO SI ESTÁ EN EXEC PORQUE SI SE PRODUJO EL SABOTAJE EN EL ÚLTIMO SLEEP (Y ESTE TRIPULANTE LO RESOLVIÓ), PUEDE HABER QUEDADO EN READY.
				if(tripulante->estado == EXEC)
					sem_post(&(tripulante->puedeEjecutar));
			}
			else
			{
				sacarDeExec(tripulante);

				//TODO VER SI ACÁ VA esperarSiHaySabotaje(tripulante);

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
			realizarAccionTareaIO(tripulante);

			esperarSiHaySabotaje(tripulante);
			if(idTripulanteResolviendoSabotaje == tripulante->tid)
				esperarParaEjecutar(tripulante);

			log_info(loggerDiscordiador,"[TRIPULANTE %d] ME BLOQUEO POR I/O",tripulante->tid);

			sacarDeExec(tripulante);

			agregarABlockIO(tripulante);

			habilitarProximoAEjecutar();

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

			if(tieneTareasPendientes(tripulante))
				agregarAReady(tripulante);
		}
	}
	else
	{
		if(!tripulante->proxTarea->yaInicio){
			notificarInicioDeTarea(tripulante);
			log_info(loggerDiscordiador,"[TRIPULANTE %d] EMPIEZO A EJECUTAR %s",tripulante->tid,tripulante->proxTarea->nombre);

			pthread_mutex_lock(&mutexTripulantes);
			tripulante->proxTarea->yaInicio = true;
			pthread_mutex_unlock(&mutexTripulantes);
		}

		while(tripulante->quantum < QUANTUM && tripulante->proxTarea->tiempoEjecutado < tripulante->proxTarea->tiempo && !tripulante->expulsado){
			sem_wait(&(tripulante->semaforoPlanificacion));
			sem_post(&(tripulante->semaforoPlanificacion));
			if(tripulante->tid == idTripulanteResolviendoSabotaje){
				pthread_mutex_lock(&mutexTripulantes);
				tripulante->quantum = 0;
				tripulante->proxTarea->tiempoEjecutado = 0; //PARA REINICIAR LA EJECUCIÓN
				pthread_mutex_unlock(&mutexTripulantes);

				esperarParaEjecutar(tripulante);//TODO TAREA YA INICIADA
			}

			pthread_mutex_lock(&mutexTripulantes);
			tripulante->quantum++;
			tripulante->proxTarea->tiempoEjecutado++;
			pthread_mutex_unlock(&mutexTripulantes);

			sleep(RETARDO_CICLO_CPU);
		}

		//SI EL TRIPULANTE ES EXPULSADO MIENTRAS EJECUTA LA TAREA, NO ENTRA ACÁ
		if(!tripulante->expulsado){
			//SI EL TIEMPO EJECUTADO DE LA TAREA ES EL TIEMPO TOTAL, FINALIZA LA MISMA
			if(tripulante->proxTarea->tiempoEjecutado == tripulante->proxTarea->tiempo){
				log_info(loggerDiscordiador,"[TRIPULANTE %d] TERMINÉ DE EJECUTAR %s",tripulante->tid,tripulante->proxTarea->nombre);

				notificarFinalizacionDeTarea(tripulante);

				pthread_mutex_lock(&mutexTripulantes);
				tripulante->proxTarea->finalizada = true;
				tripulante->tareasPendientes--;
				pthread_mutex_unlock(&mutexTripulantes);

				if(tieneTareasPendientes(tripulante)){
					if(tripulante->quantum == QUANTUM){
						log_info(loggerDiscordiador,"[TRIPULANTE %d] CONSUMÍ TODO EL QUANTUM.",tripulante->tid);

						esperarSiHaySabotaje(tripulante);

						pthread_mutex_lock(&mutexTripulantes);
						tripulante->quantum = 0;
						tripulante->habilitado = false;
						pthread_mutex_unlock(&mutexTripulantes);

						//SI DESPUÉS DEL SABOTAJE QUEDARA EN READY (ES DECIR QUE LO RESUELVE ÉL), NO HABRÍA QUE SACARLO DE EXEC NI AGREGARLO A READY (PORQUE YA ESTÁ AHÍ :3)
						if(tripulante->estado == EXEC){
							sacarDeExec(tripulante);

							agregarAReady(tripulante);

							habilitarProximoAEjecutar();
						}
					}
					else
					{
						esperarSiHaySabotaje(tripulante);

						//SI SIGUE EN EXEC DESPUÉS DE RESOLVER EL SABOTAJE, CONTINÚA EJECUTANDO COMO SI NADA
						if(tripulante->estado == EXEC)
							sem_post(&(tripulante->puedeEjecutar));
						else
						{
							pthread_mutex_lock(&mutexTripulantes);
							tripulante->quantum = 0;
							pthread_mutex_unlock(&mutexTripulantes);
						}
					}
				}
				else
				{
					sacarDeExec(tripulante);

					//TODO VER SI ACÁ VA esperarSiHaySabotaje(tripulante);

					habilitarProximoAEjecutar();
				}
			}
			else
			{
				//SI EN EL ÚLTIMO SLEEP QUE EJECUTA SE PRODUCE UN SABOTAJE, Y TODAVÍA NO TERMINÓ DE EJECUTAR TODA LA TAREA,
				//SE LE SETEA EN 'false' QUE HAYA EMPEZADO LA TAREA PARA QUE VUELVA A EJECUTARLA DESDE 0 (SI ES EL QUE RESOLVIÓ EL SABOTAJE)
				if(tripulante->tid == idTripulanteResolviendoSabotaje){
					pthread_mutex_lock(&mutexTripulantes);
					tripulante->proxTarea->yaInicio = false;
					tripulante->proxTarea->tiempoEjecutado = 0;
					pthread_mutex_unlock(&mutexTripulantes);
				}

				log_info(loggerDiscordiador,"[TRIPULANTE %d] CONSUMÍ TODO EL QUANTUM.",tripulante->tid);

				esperarSiHaySabotaje(tripulante);

				pthread_mutex_lock(&mutexTripulantes);
				tripulante->quantum = 0;
				tripulante->habilitado = false;
				pthread_mutex_unlock(&mutexTripulantes);

				if(tripulante->estado == EXEC){
					sacarDeExec(tripulante);

					agregarAReady(tripulante);

					habilitarProximoAEjecutar();
				}
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

		//PREGUNTO SI ESTÁ EN READY PORQUE SI TERMINA DE EJECUTAR UNA TAREA COMÚN, SIGUE EN EXEC.
		if(tripulante->estado == READY){
			sacarDeReady(tripulante);

			agregarAExec(tripulante);
			log_info(loggerDiscordiador,"[TRIPULANTE %d] EJECUTO...",tripulante->tid);
		}

		//SE MUEVE A LA POSICIÓN DE LA TAREA
		while(!llegoALaPosicion(tripulante,&tripulante->proxTarea->posicion) && !tripulante->expulsado){
			sem_wait(&(tripulante->semaforoPlanificacion));
			sem_post(&(tripulante->semaforoPlanificacion));
			if(tripulante->tid == idTripulanteResolviendoSabotaje)
				esperarParaEjecutar(tripulante);

			moverTripulante(tripulante,&tripulante->proxTarea->posicion);

			sleep(RETARDO_CICLO_CPU);
		}

		//SI ES EXPULSADO MIENTRAS SE MUEVE HACIA LA TAREA, DEJA DE EJECUTAR
		if(tripulante->expulsado)
			break;

		ejecutarTareaFIFO(tripulante);

		if(tieneTareasPendientes(tripulante) && !tripulante->expulsado){
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

		//tipo_mensaje finalizar = EXPULSAR_TRIPULANTE;
		//send(tripulante->socket_MONGO,&finalizar,sizeof(tipo_mensaje),0); //LE AVISO A I-MONGO QUE TERMINÉ

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

//		close(tripulante->socket_MIRAM);
//		close(tripulante->socket_MONGO);
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
				sem_post(&(tripulante->semaforoPlanificacion));
				if(tripulante->tid == idTripulanteResolviendoSabotaje)
					esperarParaEjecutar(tripulante);

				moverTripulante(tripulante,&tripulante->proxTarea->posicion);

				pthread_mutex_lock(&mutexTripulantes);
				tripulante->quantum++;
				pthread_mutex_unlock(&mutexTripulantes);

				sleep(RETARDO_CICLO_CPU);
			}

			//SI ES EXPULSADO MIENTRAS VA HACIA LA TAREA, NO ENTRA ACÁ
			if(!tripulante->expulsado){
				//SI EL QUANTUM DEL TRIPULANTE ES IGUAL AL MAXIMO, QUIERE DECIR QUE NO PUEDE EMPEZAR A EJECUTAR LA TAREA. VUELVE A READY
				if(tripulante->quantum == QUANTUM){
					log_info(loggerDiscordiador,"[TRIPULANTE %d] CONSUMÍ TODO EL QUANTUM.",tripulante->tid);

					esperarSiHaySabotaje(tripulante);

					pthread_mutex_lock(&mutexTripulantes);
					tripulante->quantum = 0;
					tripulante->habilitado = false;
					pthread_mutex_unlock(&mutexTripulantes);

					if(tripulante->estado == EXEC){
						sacarDeExec(tripulante);

						agregarAReady(tripulante);

						habilitarProximoAEjecutar();
					}
				}
				else
					ejecutarTareaRR(tripulante);
			}
		}

		if(tieneTareasPendientes(tripulante) && !tripulante->expulsado){
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

		//tipo_mensaje finalizar = EXPULSAR_TRIPULANTE;
		//send(tripulante->socket_MONGO,&finalizar,sizeof(tipo_mensaje),0); //LE AVISO A I-MONGO QUE TERMINÉ

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

//		close(tripulante->socket_MIRAM);
//		close(tripulante->socket_MONGO);
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
	char* tareas = obtenerTareasComoCadena(estructura->rutaDeTareas);
	if(strcmp(tareas,"")==0){
		log_info(loggerDiscordiador,"EL ARCHIVO DE TAREAS INDICADO NO EXISTE");
		list_destroy_and_destroy_elements(estructura->coordenadasTripulantes,free);
		return;
	}

	int socket_cliente_MIRAM = crearConexionCliente(IP_MI_RAM,PUERTO_MI_RAM);
	t_patota* patota = malloc(sizeof(t_patota));
	patota->tripulantes = list_create();
	patota->pid = idPatota;
	patota->archivoTareas = estructura->rutaDeTareas;
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

	list_destroy(estructura->coordenadasTripulantes);
	free(estructura);

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
		log_info(loggerDiscordiador,"------------------------------------------------------");
		log_info(loggerDiscordiador,"Estado de la Nave: %s",fecha);
		pthread_mutex_lock(&mutexTripulantes);
		for(int i=0; i<list_size(tripulantes); i++){
			t_tripulante* tripulante = (t_tripulante*) list_get(tripulantes,i);
			log_info(loggerDiscordiador,"Tripulante: %d    Patota: %d    Status: %s",tripulante->tid, tripulante->idPatota, getEstadoComoCadena(tripulante->estado));
		}
		pthread_mutex_unlock(&mutexTripulantes);
		log_info(loggerDiscordiador,"------------------------------------------------------");

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
		sem_post(&(tripulante->semaforoPlanificacion));

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

	log_info(loggerDiscordiador,"SE EXPULSA AL TRIPULANTE %d",id_tripulante);

	tipo_mensaje finalizar = EXPULSAR_TRIPULANTE;
	send(tripulante->socket_MIRAM,&finalizar,sizeof(tipo_mensaje),0);

//	close(tripulante->socket_MIRAM);
//	close(tripulante->socket_MONGO);
}

void iniciarPlanificacion(){
	void habilitarSemaforo(void* elemento){
		t_tripulante* tripulante = (t_tripulante*) elemento;
		sem_post(&(tripulante->semaforoPlanificacion));
	}

	log_info(loggerDiscordiador,"SE INICIA LA PLANIFICACION");

	list_iterate(tripulantes,(void*) habilitarSemaforo);

	pthread_mutex_lock(&mutexActivarPlanificacion);
	planificacionActivada = true;
	planificacionFueActivadaAlgunaVez = true;
	pthread_mutex_unlock(&mutexActivarPlanificacion);
}

void pausarPlanificacion(){
	void deshabilitarSemaforo(void* elemento){
		t_tripulante* tripulante = (t_tripulante*) elemento;
		sem_wait(&(tripulante->semaforoPlanificacion));
	}

	log_info(loggerDiscordiador,"SE PAUSA LA PLANIFICACIÓN");

	list_iterate(tripulantes,(void*) deshabilitarSemaforo);

	pthread_mutex_lock(&mutexActivarPlanificacion);
	planificacionActivada = false;
	pthread_mutex_unlock(&mutexActivarPlanificacion);
}

void obtenerBitacora(int id_tripulante){
	bool buscarTripulante(void* elemento){
		return ((t_tripulante*) elemento)->tid == id_tripulante;
	}

	pthread_mutex_lock(&mutexTripulantes);
	t_tripulante* tripulante = (t_tripulante*) list_find(tripulantes,buscarTripulante);
	pthread_mutex_unlock(&mutexTripulantes);

	tipo_mensaje opCode = OBTENER_BITACORA;
	send(tripulante->socket_MONGO,&opCode,sizeof(tipo_mensaje),0);

	uint32_t size;
	char* bitacora = (char*) recibir_buffer(&size,tripulante->socket_MONGO);

	log_info(loggerDiscordiador,bitacora);
	free(bitacora);
}

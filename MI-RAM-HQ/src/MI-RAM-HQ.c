/*
 ============================================================================
 Name        : MI-RAM-HQ.c
 Author      : 
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include "MI-RAM-HQ.h"

static char calcular_identificador(uint32_t);



#define ASSERT_CREATE(nivel, id, err)                                                   \
	if(err) {                                                                           \
		nivel_destruir(nivel);                                                          \
		nivel_gui_terminar();                                                           \
		fprintf(stderr, "Error al crear '%c': %s\n", id, nivel_gui_string_error(err));  \
		return EXIT_FAILURE;                                                            \
	}

int main(void) {
//	signal(SIGUSR1,dumpMemoria);
	signal(SIGINT,terminar_programa);

	inicializarVariables();
	log_info(loggerSecundario,"PID MI-RAM HQ: %d",getpid());

	int socket_escucha = iniciarServidor(IP_MI_RAM,PUERTO_MI_RAM);
	log_info(loggerSecundario, "MI-RAM-HQ Listo para atender a los Tripulantes!");

	while(1) {
		int socket_cliente = esperar_cliente(socket_escucha);

		int tipo_msg = recibir_operacion(socket_cliente);

		pthread_t hilo_receptor;

		switch(tipo_msg) {
		case INICIAR_PATOTA:
			pthread_create(&hilo_receptor, NULL, (void*) recibir_datos_patota,(void*) socket_cliente);
			pthread_detach(hilo_receptor);
			break;
		case INICIAR_TRIPULANTE:
			pthread_create(&hilo_receptor, NULL,(void*) atenderTripulante, (void*) socket_cliente);
			pthread_detach(hilo_receptor);
			break;
		default:
			log_warning(loggerPrincipal, "Tipo de mensaje desconocido!!!. Cierro conexion con dicho cliente");
			close(socket_escucha);
			return -1;
			break;
		}
	}

	close(socket_escucha);
	return EXIT_SUCCESS;
}

void inicializarVariables(){

	configuracionMiRam = config_create("/home/utnso/workspace/tp-2021-1c-No-C-Aprueba-/MI-RAM-HQ/miram.config");
	loggerPrincipal = log_create("/home/utnso/workspace/tp-2021-1c-No-C-Aprueba-/MI-RAM-HQ/miramPrincipal.log", "MI-RAM-HQ", 1, LOG_LEVEL_INFO);
	loggerSecundario = log_create("/home/utnso/workspace/tp-2021-1c-No-C-Aprueba-/MI-RAM-HQ/miramSecundario.log", "MI-RAM-HQ", 0, LOG_LEVEL_INFO);
	TAMANIO_MEMORIA = config_get_int_value(configuracionMiRam,"TAMANIO_MEMORIA");
	TAMANIO_PAGINA = config_get_int_value(configuracionMiRam,"TAMANIO_PAGINA");
	TAMANIO_SWAP = config_get_int_value(configuracionMiRam,"TAMANIO_SWAP");
	IP_MI_RAM = config_get_string_value(configuracionMiRam,"IP_MI_RAM");
	PUERTO_MI_RAM = config_get_string_value(configuracionMiRam,"PUERTO");
	ESQUEMA_MEMORIA = config_get_string_value(configuracionMiRam,"ESQUEMA_MEMORIA");
	PATH_SWAP = config_get_string_value(configuracionMiRam,"PATH_SWAP");
	ALGORITMO_REEMPLAZO = config_get_string_value(configuracionMiRam,"ALGORITMO_REEMPLAZO");
	inicializarMapa();

	//Por el momento todx con lo basicooo
	inicializar_administrador(
			0,
			bas_inicializacion,
			bas_guardar_nueva_patota,
			bas_guardar_nuevo_tripulante,
			bas_obtener_patota,
			bas_obtener_tripulante,
			bas_actualizar_estado_tripulante,
			bas_actualizar_posicion_tripulante,
			bas_actualizar_instruccion_tripulante,
			bas_liberar_tripulante);
}

void atenderTripulante(void* _cliente) {
	log_info(loggerSecundario, "Se conectó un Tripulante!");
	log_info(loggerSecundario, "Primero recibo sus datos y armo TCB");

	int socket_tripulante = (int) _cliente;

	uint32_t tid;
	uint32_t pid;
	recibir_datos_tripulante(socket_tripulante, &tid, &pid);


	log_info(loggerSecundario, "[TRIPULANTE %d] Se ha creado TCB. Pertenece a la patota %d.", tid, pid);

	while(1){
		int tipo_msg = recibir_operacion(socket_tripulante);

		switch(tipo_msg){
		case PROXIMA_TAREA:
			enviar_proxima_tarea(tid, socket_tripulante);
			actualizar_instruccion_tripulante(tid);
			log_info(loggerSecundario, "[TRIPULANTE %d] Se envio proxima tarea para el tripulante", tid);
			break;
		case INFORMAR_MOVIMIENTO:
			recibir_movimiento_tripulante(tid, socket_tripulante);
			break;
		case CAMBIO_ESTADO:{
			datos_tripulante* tripulante = obtener_tripulante(tid);
			char nuevoEstado;
			recv(socket_tripulante,&nuevoEstado, sizeof(char), 0);
			log_info(loggerSecundario, "[TRIPULANTE %d] Pasa de estado %c a %c", tid, tripulante->estado, nuevoEstado);
			actualizar_estado_tripulante(tid, nuevoEstado);
			liberar_datos_tripulante(tripulante);

			if(nuevoEstado == 'F') {
				finalizar_tripulante(tid, socket_tripulante);
				return;
			}
			break;
		}
		case EXPULSAR_TRIPULANTE:
			finalizar_tripulante(tid, socket_tripulante);
			return;
			break;
		default:
			log_info(loggerPrincipal, "[TRIPULANTE %d] Tipo de mensaje desconocido!!!", tid);
			finalizar_tripulante(tid, socket_tripulante);
			return;
			break;
		}
	}

	close(socket_tripulante);
}

void recibir_datos_patota(void* _cliente) {

	int socket_cliente = (int) _cliente;
	void* buffer;
	uint32_t buffer_size;
	uint32_t desplazamiento = 0;
	uint32_t inst_len;

	datos_patota* datos_patota_nuevo = malloc(sizeof(datos_patota));

	log_info(loggerSecundario,"Me llegan los datos de una patota");

	buffer = recibir_buffer(&buffer_size, socket_cliente);

	//leo PID
	memcpy(&(datos_patota_nuevo->pid), buffer + desplazamiento, sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);

	//leo cant_tripulantes
	memcpy(&(datos_patota_nuevo->tripulantes), buffer + desplazamiento, sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);

	//leo longitud de instrucciones
	memcpy(&inst_len, buffer + desplazamiento, sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);

	//leo instrucciones, va sin &, porque tareas es un puntero
	datos_patota_nuevo->tareas = malloc(inst_len);
	memcpy(datos_patota_nuevo->tareas, buffer + desplazamiento, inst_len);

	//GUARDO LA PATOTA
	guardar_nueva_patota(datos_patota_nuevo);

	enviar_respuesta(OK, socket_cliente);
	log_info(loggerSecundario, "Se ha creado el PCB: %d\nSus instrucciones son: %s", datos_patota_nuevo->pid, datos_patota_nuevo->tareas);

	close(socket_cliente);
	liberar_datos_patota(datos_patota_nuevo);
	free(buffer);
}

void recibir_datos_tripulante(int socket_tripulante, uint32_t* tid, uint32_t* pid) {

	void* buffer;
	uint32_t buffer_size;
	uint32_t desplazamiento = 0;
	datos_tripulante* datos_trip_nuevo = malloc(sizeof(datos_tripulante));

	buffer = recibir_buffer(&buffer_size, socket_tripulante);

	memcpy(&(datos_trip_nuevo->pid), buffer + desplazamiento, sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);
	datos_trip_nuevo->proxInstruccion = NULL;

	memcpy(&(datos_trip_nuevo->tid), buffer + desplazamiento, sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);

	memcpy(&(datos_trip_nuevo->estado), buffer + desplazamiento, sizeof(char));
	desplazamiento += sizeof(char);

	memcpy(&(datos_trip_nuevo->posX), buffer + desplazamiento, sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);

	memcpy(&(datos_trip_nuevo->posY), buffer + desplazamiento, sizeof(uint32_t));

	//GUARDO EL TRIPULANTE
	guardar_nuevo_tripulante(datos_trip_nuevo);

	personaje_crear(nivel, calcular_identificador(datos_trip_nuevo->tid), datos_trip_nuevo->posX, datos_trip_nuevo->posY);
	nivel_gui_dibujar(nivel);

	*tid = datos_trip_nuevo->tid;
	*pid = datos_trip_nuevo->pid;
	liberar_datos_tripulante(datos_trip_nuevo);
	free(buffer);
}

void enviar_proxima_tarea(uint32_t tid, int socket_tripulante) {

	datos_tripulante* d_tripulante = obtener_tripulante(tid);

	int size_instruccion = strlen(d_tripulante->proxInstruccion) + 1;

	tipo_tarea cod_tarea;
	if(string_contains(d_tripulante->proxInstruccion," "))
		cod_tarea = ENTRADA_SALIDA;
	else
		cod_tarea = COMUN;

	//Preparo paquete para enviar
	t_buffer* buffer = malloc(sizeof(t_buffer));
	buffer->size = sizeof(uint32_t) + size_instruccion;
	buffer->stream = malloc(buffer->size);

	int desplazamiento = 0;

	memcpy(buffer->stream + desplazamiento, &size_instruccion, sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);
	memcpy(buffer->stream + desplazamiento, d_tripulante->proxInstruccion, size_instruccion);

	send(socket_tripulante,&cod_tarea,sizeof(int),0);
	enviar_buffer(buffer, socket_tripulante);

	liberar_datos_tripulante(d_tripulante);
	free(buffer->stream);
	free(buffer);
}

void recibir_movimiento_tripulante(uint32_t tid, int socket_tripulante) {

	void* buffer;
	uint32_t buffer_size;
	uint32_t desplazamiento = 0;
	uint32_t posX;
	uint32_t posY;

	buffer = recibir_buffer(&buffer_size, socket_tripulante);

	memcpy(&posX, buffer + desplazamiento, sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);

	memcpy(&posY, buffer + desplazamiento, sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);

	//ACTUALIZO POS TRIPULANTE
	actualizar_posicion_tripulante(tid, posX, posY);

	item_mover(nivel,calcular_identificador(tid),posX, posY);
	nivel_gui_dibujar(nivel);
	
//	log_info(loggerMiRam, "[TRIPULANTE %d] Se movió a %d|%d", tid, posX, posY);

	free(buffer);
}

void finalizar_tripulante(uint32_t tid, int socket_tripulante) {
	item_borrar(nivel, calcular_identificador(tid));
	nivel_gui_dibujar(nivel);
	liberar_tripulante(tid);
	log_info(loggerSecundario, "[TRIPULANTE %d] Se lo libera de memoria y elimina del mapa",tid);
	close(socket_tripulante);
}


static char calcular_identificador(uint32_t tid) {
	return tid + 64;
}

void inicializarMapa(){
	nivel = nivel_crear("AMONGASOO");
	int filas, columnas;

	nivel_gui_inicializar();

	nivel_gui_get_area_nivel(&columnas,&filas);
}

void terminar_programa(){
	config_destroy(configuracionMiRam);
	nivel_destruir(nivel);
	nivel_gui_terminar();
	log_info(loggerPrincipal,"Finaliza MI-RAM...");
	log_destroy(loggerSecundario);
	log_destroy(loggerPrincipal);
	exit(0);
}

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

static t_config* configuracionMiRam;
static t_log* logger_mi_ram;
static NIVEL* nivel;

static pthread_mutex_t mutex_mapa;
static pthread_mutex_t mutex_memoria;

static char calcular_identificador(uint32_t);

#define ASSERT_CREATE(nivel, id, err)                                                   \
	if(err) {                                                                           \
		nivel_destruir(nivel);                                                          \
		nivel_gui_terminar();                                                           \
		fprintf(stderr, "Error al crear '%c': %s\n", id, nivel_gui_string_error(err));  \
		return EXIT_FAILURE;                                                            \
	}

int main(void) {

	signal(SIGINT, terminar_programa);
	signal(SIGUSR1, iniciar_dump_memoria);
	signal(SIGUSR2, iniciar_accion_sigusr2);

	inicializar_variables();
	log_info(logger_mi_ram,"PID MI-RAM HQ: %d", getpid());

	int socket_escucha = iniciarServidor("127.0.0.1", PUERTO_MI_RAM);
	log_info(logger_mi_ram, "MI-RAM-HQ Listo para atender a los Tripulantes!");

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
			pthread_create(&hilo_receptor, NULL,(void*) atender_tripulante, (void*) socket_cliente);
			pthread_detach(hilo_receptor);
			break;
		default:
			log_warning(logger_mi_ram, "Tipo de mensaje desconocido!!!. Cierro conexion con dicho cliente");
			close(socket_escucha);
			return -1;
			break;
		}
	}

	close(socket_escucha);
	return EXIT_SUCCESS;
}

void inicializar_variables(){
	configuracionMiRam = config_create("/home/utnso/workspace/tp-2021-1c-No-C-Aprueba-/MI-RAM-HQ/miram.config");
	logger_mi_ram = log_create("/home/utnso/workspace/tp-2021-1c-No-C-Aprueba-/MI-RAM-HQ/miram.log", "MI-RAM-HQ", 0, LOG_LEVEL_INFO);
	TAMANIO_MEMORIA = config_get_int_value(configuracionMiRam,"TAMANIO_MEMORIA");
	TAMANIO_PAGINA = config_get_int_value(configuracionMiRam,"TAMANIO_PAGINA");
	TAMANIO_SWAP = config_get_int_value(configuracionMiRam,"TAMANIO_SWAP");
	PUERTO_MI_RAM = config_get_string_value(configuracionMiRam,"PUERTO");
	ESQUEMA_MEMORIA = config_get_string_value(configuracionMiRam,"ESQUEMA_MEMORIA");
	PATH_SWAP = config_get_string_value(configuracionMiRam,"PATH_SWAP");
	ALGORITMO_REEMPLAZO = config_get_string_value(configuracionMiRam,"ALGORITMO_REEMPLAZO");
	CRITERIO_SELECCION = config_get_string_value(configuracionMiRam,"CRITERIO_SELECCION");
	pthread_mutex_init(&mutex_memoria, NULL);
	inicializar_mapa();
	if(strcmp(ESQUEMA_MEMORIA, "SEGMENTACION") == 0) {
		inicializar_administrador(
				logger_mi_ram,
				seg_inicializacion,
				seg_guardar_nueva_patota,
				seg_guardar_nuevo_tripulante,
				seg_obtener_estado_tripulante,
				seg_obtener_prox_instruccion_tripulante,
				seg_actualizar_estado_tripulante,
				seg_actualizar_posicion_tripulante,
				seg_actualizar_instruccion_tripulante,
				seg_generar_dump_memoria,
				seg_receptor_sigusr2,
				seg_liberar_tripulante);
	}
	else {
	inicializar_administrador(
			logger_mi_ram,
			bas_inicializacion,
			bas_guardar_nueva_patota,
			bas_guardar_nuevo_tripulante,
			bas_obtener_estado_tripulante,
			bas_obtener_prox_instruccion_tripulante,
			bas_actualizar_estado_tripulante,
			bas_actualizar_posicion_tripulante,
			bas_actualizar_instruccion_tripulante,
			bas_generar_dump_memoria,
			NULL,
			bas_liberar_tripulante);
	}
}

void atender_tripulante(void* _cliente) {
	log_info(logger_mi_ram, "Se conectó un Tripulante!");
	log_info(logger_mi_ram, "Primero recibo sus datos y armo TCB");

	int socket_tripulante = (int) _cliente;

	uint32_t tid = recibir_datos_tripulante(socket_tripulante);

	while (1) {
		int tipo_msg = recibir_operacion(socket_tripulante);

		switch (tipo_msg) {
		case PROXIMA_TAREA:
			enviar_proxima_tarea(socket_tripulante, tid);
			pthread_mutex_lock(&mutex_memoria);
			mem_actualizar_instruccion_tripulante(tid);
			pthread_mutex_unlock(&mutex_memoria);
			break;
		case INFORMAR_MOVIMIENTO:
			recibir_movimiento_tripulante(socket_tripulante, tid);
			break;
		case CAMBIO_ESTADO: {
			char nuevo_estado = recibir_cambio_estado(socket_tripulante, tid);
			if (nuevo_estado == 'F') {
				finalizar_tripulante(socket_tripulante, tid);
				return;
			}
			break;
		}
		case EXPULSAR_TRIPULANTE:
			log_info(logger_mi_ram, "[TRIPULANTE %d] Se ha solicitado la expulsion del tripulante!!!", tid);
			finalizar_tripulante(socket_tripulante, tid);
			return;
			break;
		default:
			log_warning(logger_mi_ram, "[TRIPULANTE %d] Tipo de mensaje desconocido!!!", tid);
			finalizar_tripulante(socket_tripulante, tid);
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

	log_info(logger_mi_ram,"Me llegan los datos de una patota");

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
	pthread_mutex_lock(&mutex_memoria);
	mem_guardar_nueva_patota(datos_patota_nuevo);
	pthread_mutex_unlock(&mutex_memoria);

	enviar_respuesta(OK, socket_cliente);
	log_info(logger_mi_ram, "Se ha creado la patota: %d."
			" Tiene %d tripulante/s\nSus instrucciones son: %s", datos_patota_nuevo->pid, datos_patota_nuevo->tripulantes, datos_patota_nuevo->tareas);
	close(socket_cliente);
	liberar_datos_patota(datos_patota_nuevo);
	free(buffer);
}

uint32_t recibir_datos_tripulante(int socket_tripulante) {
	void* buffer;
	uint32_t buffer_size;
	uint32_t desplazamiento = 0;
	uint32_t tid;
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
	pthread_mutex_lock(&mutex_memoria);
	mem_guardar_nuevo_tripulante(datos_trip_nuevo);
	pthread_mutex_unlock(&mutex_memoria);

	pthread_mutex_lock(&mutex_mapa);
	personaje_crear(nivel, calcular_identificador(datos_trip_nuevo->tid), datos_trip_nuevo->posX, datos_trip_nuevo->posY);
	nivel_gui_dibujar(nivel);
	pthread_mutex_unlock(&mutex_mapa);

	tid = datos_trip_nuevo->tid;
	log_info(logger_mi_ram, "[TRIPULANTE %d] Se ha creado tripulante. Pertenece a la patota %d.", tid, datos_trip_nuevo->pid);

	liberar_datos_tripulante(datos_trip_nuevo);
	free(buffer);
	return tid;
}

void enviar_proxima_tarea(int socket_tripulante, uint32_t tid) {
	pthread_mutex_lock(&mutex_memoria);
	char* prox_instruccion = mem_obtener_prox_instruccion_tripulante(tid);
	pthread_mutex_unlock(&mutex_memoria);

	int size_instruccion = strlen(prox_instruccion) + 1;

	tipo_tarea cod_tarea;
	if(string_contains(prox_instruccion," "))
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
	memcpy(buffer->stream + desplazamiento, prox_instruccion, size_instruccion);

	send(socket_tripulante,&cod_tarea,sizeof(int),0);
	enviar_buffer(buffer, socket_tripulante);

	log_info(logger_mi_ram, "[TRIPULANTE %d] Se envio proxima tarea para el tripulante: %s", tid, prox_instruccion);

	free(prox_instruccion);
	free(buffer->stream);
	free(buffer);
}

void recibir_movimiento_tripulante(int socket_tripulante, uint32_t tid) {
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
	pthread_mutex_lock(&mutex_memoria);
	mem_actualizar_posicion_tripulante(tid, posX, posY);
	pthread_mutex_unlock(&mutex_memoria);

	pthread_mutex_lock(&mutex_mapa);
	item_mover(nivel,calcular_identificador(tid),posX, posY);
	nivel_gui_dibujar(nivel);
	pthread_mutex_unlock(&mutex_mapa);
	
	log_info(logger_mi_ram, "[TRIPULANTE %d] Se actualiza posicion a: %d|%d", tid, posX, posY);

	free(buffer);
}

char recibir_cambio_estado(int socket_tripulante, uint32_t tid) {
	pthread_mutex_lock(&mutex_memoria);
	char estado_actual = mem_obtener_estado_tripulante(tid);
	pthread_mutex_unlock(&mutex_memoria);
	char nuevo_estado;
	recv(socket_tripulante, &nuevo_estado, sizeof(char), 0);

	pthread_mutex_lock(&mutex_memoria);
	mem_actualizar_estado_tripulante(tid, nuevo_estado);
	pthread_mutex_unlock(&mutex_memoria);
	log_info(logger_mi_ram, "[TRIPULANTE %d] Pasa de estado %c a %c", tid, estado_actual, nuevo_estado);
	return nuevo_estado;
}

void finalizar_tripulante(int socket_tripulante, uint32_t tid) {
	pthread_mutex_lock(&mutex_mapa);
	item_borrar(nivel, calcular_identificador(tid));
	nivel_gui_dibujar(nivel);
	pthread_mutex_unlock(&mutex_mapa);
	pthread_mutex_lock(&mutex_memoria);
	mem_liberar_tripulante(tid);
	pthread_mutex_unlock(&mutex_memoria);
	log_info(logger_mi_ram, "[TRIPULANTE %d] Se lo libera de memoria y elimina del mapa",tid);
	close(socket_tripulante);
}

static char calcular_identificador(uint32_t tid) {
	char identificador;

	if(tid<=26)
		identificador = tid + 64; //A-B-C-D-E-F-G-H-I-J-K-L-M-N-O-P-Q-R-S-T-U-V-W-X-Y-Z
	else
		identificador = tid + 70; //a-b-c-d-e-f-g-h-i-j-k-l-m-n-o-p-q-r-s-t-u-v-w-x-y-z

	return identificador;
}

void inicializar_mapa(){
	nivel = nivel_crear("AMONGASOO");
	int filas, columnas;
	pthread_mutex_init(&mutex_mapa, NULL);
	nivel_gui_inicializar();
	nivel_gui_get_area_nivel(&columnas, &filas);
	nivel_gui_dibujar(nivel);
}

void iniciar_dump_memoria() {
	pthread_t hilo_dump;
	pthread_create(&hilo_dump, NULL, (void*) realizar_dump, NULL);
	pthread_detach(hilo_dump);
}

void iniciar_accion_sigusr2() {
	pthread_t hilo_sigusr2;
	pthread_create(&hilo_sigusr2, NULL, (void*) realizar_accion_sigusr2, NULL);
	pthread_detach(hilo_sigusr2);
}

void realizar_dump() {
	char* timestamp_file = temporal_get_string_time("%d%m%y-%H%M%S");
	char* timestamp_cabecera = temporal_get_string_time("%d/%m/%y %H:%M:%S");
	char* cadena_guion = "--------------------------------------------------------------------------\n";
	char* cabecera = string_from_format("Dump: %s\n", timestamp_cabecera);
	char* path_archivo = string_from_format("/home/utnso/workspace/tp-2021-1c-No-C-Aprueba-/MI-RAM-HQ/Dump_%s.dmp", timestamp_file);
	FILE* archivo_dump = txt_open_for_append(path_archivo);

	log_info(logger_mi_ram, "[DUMP] Se inicia del dump de la memoria - Timestamp: %s", timestamp_file);
	txt_write_in_file(archivo_dump, cadena_guion);
	txt_write_in_file(archivo_dump, cabecera);
	pthread_mutex_lock(&mutex_memoria);
	mem_generar_dump_memoria(archivo_dump);
	pthread_mutex_unlock(&mutex_memoria);
	txt_write_in_file(archivo_dump, cadena_guion);
	log_info(logger_mi_ram, "[DUMP] Se finaliza el dump de la memoria");

	txt_close_file(archivo_dump);
	free(timestamp_file);
	free(timestamp_cabecera);
	free(cabecera);
	free(path_archivo);
}

void realizar_accion_sigusr2() {
	pthread_mutex_lock(&mutex_memoria);
	mem_receptor_sigusr2();
	pthread_mutex_unlock(&mutex_memoria);
}


void terminar_programa() {
	config_destroy(configuracionMiRam);
	pthread_mutex_destroy(&mutex_mapa);
	pthread_mutex_destroy(&mutex_memoria);
	nivel_destruir(nivel);
	nivel_gui_terminar();
	finalizar_administrador();
	log_info(logger_mi_ram,"Finaliza MI-RAM...");
	log_destroy(logger_mi_ram);
	exit(0);
}

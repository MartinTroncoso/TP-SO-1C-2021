/*
 * utils.h
 *
 *  Created on: 24 abr. 2021
 *      Author: utnso
 */

#ifndef UTILS_H_
#define UTILS_H_

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <readline/readline.h>
#include <pthread.h>
#include <semaphore.h>
#include <commons/log.h>
#include <commons/string.h>
#include <commons/config.h>
#include <commons/collections/list.h>
#include <commons/collections/dictionary.h>
#include <commons/log.h>
#include <commons/txt.h>
#include <commons/bitarray.h>
#include <commons/temporal.h>

//void holiwis();
//void comoAndas();
//void bien();

t_log* logger;

typedef enum
{
	MENSAJE,
	PAQUETE
}op_code;

typedef struct
{
	int size;
	void* stream;
} t_buffer;

typedef struct
{
	op_code codigo_operacion;
	t_buffer* buffer;
} t_paquete;

typedef enum{
	INICIAR_PATOTA=1,
	EXPULSAR_TRIPULANTE=2,
	OBTENER_BITACORA=3,
	PROXIMA_TAREA=4,
	INFORMAR_MOVIMIENTO=5,
	FINALIZO_TAREA=6,
	INFORMAR_DESPLAZAMIENTO_FS=7,
	INICIO_TAREA=8,
	ATENDER_SABOTAJE=9,
	RESOLUCION_SABOTAJE=10,
	FINALIZA_FSCK=11,
	INVOCAR_FSCK=12,
	ESTADO_ALERTA=13
}tipo_mensaje;

int crearConexionCliente(char*,char*);
void enviar_mensaje(char* mensaje, int socket_cliente);
t_paquete* crear_paquete(void);
void* serializar_paquete(t_paquete*, int);
void* serializar_buffer(t_buffer* buffer, int bytes);
void agregar_a_paquete(t_paquete* paquete, void* valor, int tamanio);
void enviar_paquete(t_paquete* paquete, int socket_cliente);
void enviar_buffer(t_buffer* buffer, int socket_cliente);
void liberar_conexion(int socket_cliente);
void eliminar_paquete(t_paquete* paquete);

void* recibir_buffer(uint32_t*, int);

int iniciarServidor(char*,char*);
int esperar_cliente(int);
t_list* recibir_paquete(int);
void recibir_mensaje(int);
int recibir_operacion(int);

#endif /* UTILS_H_ */

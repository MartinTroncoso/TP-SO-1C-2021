/*
 * Discordiador.h
 *
 *  Created on: 25 abr. 2021
 *      Author: utnso
 */

#ifndef DISCORDIADOR_H_
#define DISCORDIADOR_H_

#include <utils.h>

char* IP_MI_RAM;
char* PUERTO_MI_RAM;
char* IP_I_MONGO_STORE;
char* PUERTO_I_MONGO_STORE;
int GRADO_MULTITAREA;
char* ALGORITMO;
int QUANTUM;
int DURACION_SABOTAJE;
int RETARDO_CICLO_CPU;

typedef struct{
	uint32_t cantidadTripulantes;
	char* rutaDeTareas;
	t_list* coordenadasTripulantes;
}t_iniciar_patota;

typedef struct{
	uint32_t pid;
	char* archivoTareas;
	t_list* tripulantes;
	int cantidadTareas;
}t_patota;

typedef enum{
	NEW,
	READY,
	EXEC,
	BLOCK,
	EXIT
}estado;

typedef struct{
	uint32_t tid;
	char estado;
	posicion* posicion;
	tarea* proxTarea;
	uint32_t tareasPendientes;
	uint32_t idPatota;
}t_tripulante;

typedef enum{
	FIFO,
	RR
}t_algoritmo;

t_config* configuracionDiscordiador;
t_log* loggerDiscordiador;
t_dictionary* diccionarioComandos;
t_dictionary* diccionarioTareas;

int socket_escucha_iMongo;

uint32_t idTripulante;
uint32_t idPatota;

t_list* patotas;
t_list* tripulantes;

void inicializarVariables();

void iniciarPatota(t_iniciar_patota*);
void listarTripulantes();
void expulsarTripulante(int);
void iniciarPlanificacion();
void pausarPlanificacion();
void obtenerBitacora(uint32_t);

void* serializar_tripulante(t_tripulante*);
void planificarTripulanteFIFO(t_tripulante*);
void gestionarTripulante(t_tripulante*);
void ingresar_comandos();
void terminar_programa();
void partirCadena(char**);
char* obtenerTareasComoCadena(char*);
void crearDiccionarioComandos(t_dictionary*);
void crearDiccionarioTareasEntradaSalida(t_dictionary*);
t_iniciar_patota* obtenerDatosPatota(char**);
int getCantidadTareasPatota(char*);
t_patota* buscarPatotaPorId(uint32_t);
bool tieneTareasPendientes(t_tripulante*);
tarea* solitarProximaTarea(int);
void moverXDelTripulante(t_tripulante*);
void moverYDelTripulante(t_tripulante*);
t_algoritmo getAlgoritmoPlanificacion();

#endif /* DISCORDIADOR_H_ */

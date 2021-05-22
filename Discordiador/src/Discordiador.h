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
	Tarea* proxTarea;
	uint32_t tareasPendientes;
	uint32_t idPatota;
	sem_t semaforoPlanificacion;
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

bool planificacionActivada;

t_list* patotas;
t_list* tripulantes;

t_list* colaReady;

sem_t multiprocesamiento;
sem_t mutexTripulantes;

void inicializarVariables();

void iniciarPatota(t_iniciar_patota*);
void listarTripulantes();
void expulsarTripulante(int);
void iniciarPlanificacion();
void pausarPlanificacion();
void obtenerBitacora(uint32_t);

void* serializar_tripulante(t_tripulante*);
void planificarTripulanteFIFO(t_tripulante*,int);
void planificarTripulanteRR(t_tripulante*,int);
void planificarTripulante(t_tripulante*,int);
void gestionarTripulante(t_tripulante*);
void ingresar_comandos();
void terminar_programa();
char* obtenerTareasComoCadena(char*);
void crearDiccionarioComandos(t_dictionary*);
void crearDiccionarioTareasEntradaSalida(t_dictionary*);
t_iniciar_patota* obtenerDatosPatota(char**);
int getCantidadTareasPatota(char*);
t_patota* buscarPatotaPorId(uint32_t);
bool tieneTareasPendientes(t_tripulante*);
Tarea* solitarProximaTarea(int,int);
void informarMovimiento(int,t_tripulante*);
void moverXDelTripulante(t_tripulante*);
void moverYDelTripulante(t_tripulante*);
t_algoritmo getAlgoritmoPlanificacion();
void agregarTripulanteAReady(t_tripulante*);

#endif /* DISCORDIADOR_H_ */

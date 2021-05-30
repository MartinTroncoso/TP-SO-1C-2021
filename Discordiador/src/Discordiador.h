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
char* IP_DISCORDIADOR;
char* PUERTO_DISCORDIADOR;
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
	BLOCK_IO,
	BLOCK_SABOTAJE,
	EXIT
}estado;

typedef struct{
	uint32_t tid;
	estado estado;
	int quantum; //PARA PLANIFICACIÓN RR
	posicion* posicion;
	Tarea* proxTarea;
	uint32_t tareasPendientes;
	uint32_t idPatota;
	sem_t semaforoPlanificacion;
	sem_t puedeEjecutar;
	bool habilitado;
	bool expulsado;
	int socket_MIRAM;
	int socket_MONGO;
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
bool planificacionFueActivadaAlgunaVez;

t_list* patotas;
t_list* tripulantes;

t_list* colaReady;
t_list* colaBlockIO;
t_list* colaExec;
t_list* colaExit;

pthread_mutex_t mutexTripulantes;
pthread_mutex_t mutexColaReady;
pthread_mutex_t mutexColaExec;
pthread_mutex_t mutexColaBlockIO;
pthread_mutex_t mutexColaExit;
pthread_mutex_t mutexActivarPlanificacion;
pthread_mutex_t mutexEjecutarIO;

void inicializarVariables();
void crearDiccionarioComandos(t_dictionary*);
void crearDiccionarioTareasEntradaSalida(t_dictionary*);
t_algoritmo getAlgoritmoPlanificacion();
void ingresar_comandos();
void destruirSemaforos();
void destruirListasYDiccionarios();
void terminar_programa();

void iniciarPatota(t_iniciar_patota*);
void listarTripulantes();
void expulsarTripulante(int);
void iniciarPlanificacion();
void pausarPlanificacion();
void obtenerBitacora(uint32_t);

char getEstadoComoCaracter(estado);
char* getEstadoComoCadena(estado);
void* serializar_tripulante(t_tripulante*);
void realizarPeticionIO(int);
bool llegoALaTarea(t_tripulante*);
void realizarAccionTareaIO(t_tripulante*);
void ejecutarTareaFIFO(t_tripulante*);
void ejecutarTareaRR(t_tripulante*);
void planificarTripulanteFIFO(t_tripulante*);
void planificarTripulanteRR(t_tripulante*);
void planificarTripulante(t_tripulante*);
void gestionarTripulante(t_tripulante*);
void habilitarProximoAEjecutar();
char* obtenerTareasComoCadena(char*);
t_iniciar_patota* obtenerDatosPatota(char**);
int getCantidadTareasPatota(char*);
bool existeElTripulante(uint32_t);
bool tieneTareasPendientes(t_tripulante*);
Tarea* solitarProximaTarea(int);
void moverXDelTripulante(t_tripulante*);
void moverYDelTripulante(t_tripulante*);
void moverTripulante(t_tripulante*);
void agregarAReady(t_tripulante*);
void agregarAExec(t_tripulante*);
void agregarAExit(t_tripulante*);
void agregarABlockIO(t_tripulante*);
void notificarCambioDeEstado(t_tripulante*);
void notificarMovimientoMIRAM(t_tripulante*);
void notificarMovimientoIMONGO(t_tripulante*,uint32_t,uint32_t);
void notificarInicioDeTarea(t_tripulante*);
void notificarFinalizacionDeTarea(t_tripulante*);
void notificarAtencionSabotaje(t_tripulante*);
void notificarResolucionSabotaje(t_tripulante*);

#endif /* DISCORDIADOR_H_ */

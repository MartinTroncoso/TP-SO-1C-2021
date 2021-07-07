/*
 * I-MONGO-STORE.h
 *
 *  Created on: 25 abr. 2021
 *      Author: utnso
 */

#ifndef I_MONGO_STORE_H_
#define I_MONGO_STORE_H_

#include <utils.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>

pthread_mutex_t mutexSincro;
pthread_mutex_t mutexBlocks;
pthread_mutex_t mutexBitMap;

char* PUNTO_MONTAJE;
char* IP_I_MONGO;
char* PUERTO_I_MONGO;
char* IP_DISCORDIADOR;
char* PUERTO_DISCORDIADOR;
char** POSICIONES_SABOTAJE;
int TIEMPO_SINCRONIZACION;

t_dictionary* caracterAsociadoATarea;
t_config* configuracionMongo;
t_log* loggerMongo;

int socket_servidor;
int socket_discordiador;

char** posicionSabotajeActual;
int sabotajesResueltos;

char* blocksMapOriginal;
char* blocksMap;
int fdArchivoBlocks;

uint32_t tamanioBlock;
uint32_t  cantidadDeBlocks;

void inicializarVariables();
void inicializarFileSystem();
void inicializarSuperBloque();
void inicializarBlocks();
void inicializarMapeoBlocks();
void forzarSincronizacionBlocks();
void inicializarDiccionario();
void inicializarSuperBloque();
t_bitarray* recuperarBitArray();
void atenderTripulante(void*);
void realizarTareaIO(int,uint32_t);
void guardarBitArray(t_bitarray*);
int posicionBlockLibre(t_bitarray*);
void recibirInformeDeDesplazamiento(int,uint32_t);
void recibirInicioDeTarea(int,uint32_t);
void enviarBitacora(int,uint32_t);
void recibirFinalizaTarea(int,uint32_t);
void recibirAtenderSabotaje(int,uint32_t);
void recibirResolucionSabotaje(int,uint32_t);
char* recuperarBitacora(uint32_t);
int byteExcedente(int, int);
void inicializarCarpetas();
void informarSabotaje();
void ejecutarFSCK();
void verificarSuperBloque();
void verificarFiles();
void escribirBitacora(char*,t_config*);
char** getSiguientePosicionSabotaje();
void escribirFile(char*, int);
char* obtenerMD5(char*);
char* bloqueRecuperado(int);
int ocuparBitVacio();
void escribirEnBlocks(int, char*);
void rellenarEnBlocks(int, char*, int);
void destruirConfig();
void terminar_programa();

#endif /* I_MONGO_STORE_H_ */

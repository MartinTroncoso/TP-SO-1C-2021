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
#include <dirent.h>

char* PUNTO_MONTAJE;
char* IP_I_MONGO;
char* PUERTO_I_MONGO;
char* IP_DISCORDIADOR;
char* PUERTO_DISCORDIADOR;
char** POSICIONES_SABOTAJE;
int TIEMPO_SINCRONIZACION;

typedef enum{
	SABOTAJE_EN_SUPERBLOQUE_CANTIDAD = 1,
	SABOTAJE_EN_SUPERBLOQUE_BITMAP = 2,
	SABOTAJE_EN_FILE_SIZE = 3,
	SABOTAJE_EN_FILE_BLOCK_COUNT = 4,
	SABOTAJE_EN_FILE_BLOCKS = 5,
	NO_HAY_SABOTAJES = 6
}casoDeSabotaje;

t_dictionary* caracterAsociadoATarea;
t_config* configuracionMongo;
t_log* loggerMongo;
t_bitarray* bitMapEnMemoria;

int socket_escucha;

char** posicionSabotajeActual;
int sabotajesResueltos;

char* blocksMapOriginal;
char* blocksMap;
int fdArchivoBlocks;

uint32_t tamanioBlock;
uint32_t  cantidadDeBlocks;

pthread_mutex_t mutexSincro;
pthread_mutex_t mutexBlocks;
pthread_mutex_t mutexBitMap;
pthread_mutex_t mutexFile;
pthread_mutex_t mutexMD5;
pthread_mutex_t mutexSabotaje;
pthread_mutex_t mutexPosicionSabotaje;

void sincronizarBlocks();
void atenderTripulantes();
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
void loggearAtencionSabotaje(uint32_t);
void loggearResolucionSabotaje(uint32_t);
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
void eliminarCaracterFile(char* , int);
bool existeArchivoRecurso(char*);
void liberarBloque(int);
void liberarBit(int);
char* bloqueRecuperado(int);
int ocuparBitVacio();
void escribirEnBlocks(int, char*);
void rellenarEnBlocks(int, char*, int);
void destruirConfig();
void terminar_programa();
void eliminarArchivoYLiberar(char*);
void asincronia();

//sabotajes
bool verificarCantidadBlocks();
bool verificarBitMap();
t_bitarray* bitmapDesdeBloques();
bool verificarSizeFile();
bool verificarMD5();
bool verificarBlockCount();
casoDeSabotaje casoSabotajeActual();
void resolverSabotajeBitMap();
void resolverSabotajeSizeFile();
void resolverSabotajeMD5();
void resolverSabotajeBlockCount();
void resolverSabotajeCantidadBlocks();

#endif /* I_MONGO_STORE_H_ */

/*
 * I-MONGO-STORE.h
 *
 *  Created on: 25 abr. 2021
 *      Author: utnso
 */

#ifndef I_MONGO_STORE_H_
#define I_MONGO_STORE_H_

#include <utils.h>

char* PUNTO_MONTAJE;
char* IP_I_MONGO;
char* PUERTO_I_MONGO;
int TIEMPO_SINCRONIZACION;

typedef struct{
	uint32_t tid;
}t_tripulante;

t_dictionary* caracterAsociadoATarea;
t_config* configuracionMongo;
t_log* loggerMongo;

int socket_servidor;
int socket_discordiador;

void inicializarVariables();
void inicializarFileSystem();
void inicializarSuperBloque();
void inicializarBlocks();
void inicializarDiccionario();
void inicializarSuperBloque();
t_bitarray recuperarBitArray();
void atenderTripulante(void*);
void guardarBitArray(t_bitarray*);
void recibirPeticionDeBitacora(int);
void terminar_programa();
void recibirInformeDeDesplazamiento(int socket);
void recibirInicioDeTarea(int socket);
void recibirFinalizaTarea(int socket);
void recibirAtenderSabotaje(int socket);
void recibirResolucionSabotaje(int socket);

#endif /* I_MONGO_STORE_H_ */

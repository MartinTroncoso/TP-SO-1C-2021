/*
 * I-MONGO-STORE.h
 *
 *  Created on: 25 abr. 2021
 *      Author: utnso
 */

#ifndef I_MONGO_STORE_H_
#define I_MONGO_STORE_H_

#include <utils.h>

typedef enum
{
	MOVIMIENTO_TRIPULANTE,
	COMIENZO_EJECUCION_DE_TAREA,
	FINALIZA_TAREA,
	CORRE_EN_PANICO_SABOTAJE,
	SABOTAJE_RESUELTO
}operacionBitacora;

char* PUNTO_MONTAJE;
char* PUERTO;
int TIEMPO_SINCRONIZACION;

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
void actualizarBitacora(int idTripulante, operacionBitacora idOperacion, char* stringParametros);
void inicializarSuperBloque();
t_bitarray recuperarBitArray();
void guardarBitArray(t_bitarray*);

#endif /* I_MONGO_STORE_H_ */

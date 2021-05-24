/*
 ============================================================================
 Name        : I-MONGO-STORE.c
 Author      : 
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include "I-MONGO-STORE.h"

int main(void) {
	inicializarVariables();

	int socket_escucha = iniciarServidor(IP_I_MONGO,PUERTO_I_MONGO);
	log_info(loggerMongo,"I-MONGO Listo para recibir a los Tripulantes!");

	while(1)
	{
		int socketCliente = esperar_cliente(socket_escucha);
		log_info(loggerMongo,"Se conectó un Tripulante!");

		pthread_t hilo_receptor;
		pthread_create(&hilo_receptor , NULL ,(void*) atenderTripulante, (void*) socketCliente);
		pthread_detach(hilo_receptor);
	}

	close(socket_escucha);
	terminar_programa();

	return EXIT_SUCCESS;
}

void inicializarVariables(){
	configuracionMongo = config_create("/home/utnso/workspace/tp-2021-1c-No-C-Aprueba-/I-MONGO-STORE/mongo.config");
	loggerMongo = log_create("/home/utnso/workspace/tp-2021-1c-No-C-Aprueba-/I-MONGO-STORE/mongo.log", "I-MONGO-STORE", 1, LOG_LEVEL_INFO);
	TIEMPO_SINCRONIZACION = config_get_int_value(configuracionMongo,"TIEMPO_SINCRONIZACION");
	PUNTO_MONTAJE = config_get_string_value(configuracionMongo,"PUNTO_MONTAJE");
	IP_I_MONGO = config_get_string_value(configuracionMongo,"IP_I_MONGO");
	PUERTO_I_MONGO = config_get_string_value(configuracionMongo,"PUERTO_I_MONGO");

//	actualizarBitacora(2, MOVIMIENTOTRIPULANTE, "1|2 3|4");
//	actualizarBitacora(2, COMIENZOEJECUCIONDETAREA, "GENERAR_OXIGENO");
//	actualizarBitacora(2, CORREENPANICOSABOTAJE, "");
	inicializarDiccionario();
	inicializarFileSystem();
	//socket_servidor = iniciarServidor("127.0.0.1",PUERTO);
	//log_info(loggerMongo, "I-MONGO-STORE listo para recibir al Discordiador");
	//socket_discordiador = esperar_cliente(socket_servidor);
	//printf("SE CONECTÓ EL DISCORDIADOR!\n");
}

void inicializarFileSystem()
{
	inicializarSuperBloque();
	//inicializarBlocks();
}

void inicializarSuperBloque()
{
	t_config* configuracionSuperBloque = config_create(string_from_format("%s/SuperBloque.ims",PUNTO_MONTAJE));
	uint32_t cantidadDeBloques = config_get_int_value(configuracionSuperBloque, "BLOCKS");
	printf("cantidad de bits a reservar: %d\n",(cantidadDeBloques/8));
	//printf("bool: %d\n",(int) bitsExcedentes(15));
	if(string_is_empty(config_get_string_value(configuracionSuperBloque,"BITMAP")))
	{
		printf("BITMAP ESTA VACIO \n");
		t_bitarray* bitArraySuperBloque = bitarray_create(malloc(cantidadDeBloques/8+cantidadDeBloques%8), cantidadDeBloques/8+cantidadDeBloques%8);
		printf("cantidad de bits en el bitarray %d\n",bitarray_get_max_bit(bitArraySuperBloque));

		for(int i=0;i<bitarray_get_max_bit(bitArraySuperBloque);i++)
		{
			printf("%d",bitarray_test_bit(bitArraySuperBloque,i));
		}
		printf("\n\n");

		//clean deja el bit en 0, set lo deja en 1
		for(int i=0;i<bitarray_get_max_bit(bitArraySuperBloque);i++)
		{
			bitarray_clean_bit(bitArraySuperBloque,i);
			//bitarray_set_bit(bitArraySuperBloque,i);
		}
		bitarray_set_bit(bitArraySuperBloque,0);
		for(int i=0;i<bitarray_get_max_bit(bitArraySuperBloque);i++)
		{
			printf("%d",bitarray_test_bit(bitArraySuperBloque,i));
		}
		printf("\n\nvalor del array:%s\n",bitArraySuperBloque->bitarray);


		config_set_value(configuracionSuperBloque,"BITMAP",bitArraySuperBloque->bitarray);
		config_save(configuracionSuperBloque);
		config_destroy(configuracionSuperBloque);
	}else
	{
		printf("BITMAP TIENE UN VALOR %s\n",config_get_string_value(configuracionSuperBloque,"BITMAP"));
		config_destroy(configuracionSuperBloque);
	}
}

t_bitarray recuperarBitArray()
{
	t_config* configuracionSuperBloque = config_create(string_from_format("%s/SuperBloque.ims",PUNTO_MONTAJE));
	uint32_t cantidadDeBloques = config_get_int_value(configuracionSuperBloque,"BLOCKS");
	t_bitarray* bitArrayARecuperar = bitarray_create(malloc(cantidadDeBloques/8),cantidadDeBloques);
	bitArrayARecuperar->bitarray = config_get_string_value(configuracionSuperBloque,"BITMAP");
	config_destroy(configuracionSuperBloque);

	return *bitArrayARecuperar;
}

void guardarBitArray(t_bitarray* arrayAGuardar)
{
	//Buscar puntero siguiente al tercer '='
	//si uso config_set_value y config_save cambia de posicion los valores dentro del archivo
}

void inicializarBlocks()
{
	char* puntoMontajeBlocks = PUNTO_MONTAJE;
	strcat(puntoMontajeBlocks,"/Blocks.ims");
	printf("%s\n",puntoMontajeBlocks);
	FILE* archivoBlocks = fopen(puntoMontajeBlocks,"r+");
	fseek(archivoBlocks,0,SEEK_END);
	if(ftell(archivoBlocks) == 0)
	{
		printf("No existe el archivo blocks\n");
		fclose(archivoBlocks);
	}else
	{
		printf("Existe el archivo blocks\n");
		fclose(archivoBlocks);
	}
	//mapeado a memoria y realizar copia cada bajada a disco
}

void inicializarDiccionario()
{
	caracterAsociadoATarea = dictionary_create();
	dictionary_put(caracterAsociadoATarea, "GENERAR_OXIGENO",(char*) "O");
	dictionary_put(caracterAsociadoATarea, "CONSUMIR_OXIGENO",(char*) "O");
	dictionary_put(caracterAsociadoATarea, "GENERAR_COMIDA",(char*) "C");
	dictionary_put(caracterAsociadoATarea, "CONSUMIR_COMIDA",(char*) "C");
	dictionary_put(caracterAsociadoATarea, "GENERAR_BASURA",(char*) "B");
	dictionary_put(caracterAsociadoATarea, "DESCARTAR_BASURA",(char*) "B");
}

void atenderTripulante(void* _cliente)
{
	int socket_tripulante = (int) _cliente;

	uint32_t idTripulante;
	recv(socket_tripulante,&idTripulante,sizeof(uint32_t),0);

	while(1){
		log_info(loggerMongo, "[TRIPULANTE %d] Espero proxima operacion del tripulante", idTripulante);
		int tipo_msg = recibir_operacion(socket_tripulante);

		switch(tipo_msg)
		{
		case INFORMAR_DESPLAZAMIENTO_FS:
			recibirInformeDeDesplazamiento(socket_tripulante);// recibe 4 uint32_t, los primero 2 son x y originales y los siguientes x' y' son a los que se desplaza
			break;
		case INICIO_TAREA: // PARAMETRO: "NOMBRETAREA"
			recibirInicioDeTarea(socket_tripulante);
			break;
		case FINALIZO_TAREA: //PARAMETRO "NOMBRETAREA"
			recibirFinalizaTarea(socket_tripulante);
			break;
		case ATENDER_SABOTAJE: //PARAMETRO INDISTINTO
			recibirAtenderSabotaje(socket_tripulante);
			break;
		case RESOLUCION_SABOTAJE: //PARAMETRO INDISTINTO
			recibirResolucionSabotaje(socket_tripulante);
			break;
		case OBTENER_BITACORA:
			recibirPeticionDeBitacora(socket_tripulante);
			break;
		default:
			break;
		}
	}
}

void recibirInformeDeDesplazamiento(int socketCliente)
{
	void* buffer;
	uint32_t tamanioBuffer;
	uint32_t tid;
	uint32_t coorXAnterior;
	uint32_t coorYAnterior;
	uint32_t coorXNueva;
	uint32_t coorYNueva;
	uint32_t desplazamiento = 0;

	buffer = recibir_buffer(&tamanioBuffer, socketCliente);
	memcpy(&tid, buffer+desplazamiento,sizeof(uint32_t));
	desplazamiento+=sizeof(uint32_t);
	memcpy(&coorXAnterior,buffer+desplazamiento,sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);
	memcpy(&coorYAnterior,buffer+desplazamiento,sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);
	memcpy(&coorXNueva,buffer+desplazamiento,sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);
	memcpy(&coorYNueva,buffer+desplazamiento,sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);

	FILE* bitacoraTripulante = txt_open_for_append(string_from_format("%s/Files/Bitacoras/Tripulante%d.ims",PUNTO_MONTAJE,tid));
	txt_write_in_file(bitacoraTripulante, string_from_format("Se mueve de %d|%d a %d|%d\n",coorXAnterior,coorYAnterior,coorXNueva,coorYNueva));
	free(buffer);
	txt_close_file(bitacoraTripulante);
}

void recibirInicioDeTarea(int socketCliente)
{
	void* buffer;
	uint32_t tamanioBuffer;
	uint32_t tid;
	uint32_t desplazamiento = 0;
	char* tarea;

	buffer = recibir_buffer(&tamanioBuffer, socketCliente);
	memcpy(&tid,buffer,sizeof(uint32_t));
	desplazamiento+=sizeof(uint32_t);
	memcpy(&tarea,buffer,tamanioBuffer-sizeof(uint32_t));
	FILE* bitacoraTripulante = txt_open_for_append(string_from_format("%s/Files/Bitacoras/Tripulante%d.ims",PUNTO_MONTAJE,tid));
	txt_write_in_file(bitacoraTripulante, string_from_format("Comienza ejecucion de tarea %s\n",tarea));
	free(buffer);
	txt_close_file(bitacoraTripulante);
}

void recibirFinalizaTarea(int socketCliente)
{
	void* buffer;
	uint32_t tamanioBuffer;
	uint32_t tid;
	uint32_t desplazamiento = 0;
	char* tarea;

	buffer = recibir_buffer(&tamanioBuffer, socketCliente);
	memcpy(&tid,buffer,sizeof(uint32_t));
	desplazamiento+=sizeof(uint32_t);
	memcpy(&tarea,buffer,tamanioBuffer-sizeof(uint32_t));
	FILE* bitacoraTripulante = txt_open_for_append(string_from_format("%s/Files/Bitacoras/Tripulante%d.ims",PUNTO_MONTAJE,tid));
	txt_write_in_file(bitacoraTripulante, string_from_format("Se finaliza la tarea %s\n",tarea));
	free(buffer);
	txt_close_file(bitacoraTripulante);
}

void recibirPeticionDeBitacora(int socketCliente)
{
	void* buffer;
	uint32_t tamanioBuffer;
	uint32_t tid;

	buffer = recibir_buffer(&tamanioBuffer,socketCliente);
	memcpy(&tid,buffer,sizeof(uint32_t));
	printf("PETICION DE BITACORA DEL TRIPULANTE: %d\n",tid);
	//enviar_respuesta(OK, socketCliente);
	free(buffer);
}

void recibirAtenderSabotaje(int socketCliente)
{
	void* buffer;
	uint32_t tamanioBuffer;
	uint32_t tid;

	buffer = recibir_buffer(&tamanioBuffer, socketCliente);
	memcpy(&tid, buffer,sizeof(uint32_t));

	FILE* bitacoraTripulante = txt_open_for_append(string_from_format("%s/Files/Bitacoras/Tripulante%d.ims",PUNTO_MONTAJE,tid));
	txt_write_in_file(bitacoraTripulante, string_from_format("Se corre en panico hacia la ubicacion del sabotaje\n"));
	free(buffer);
	txt_close_file(bitacoraTripulante);
}

void recibirResolucionSabotaje(int socketCliente)
{
	void* buffer;
	uint32_t tamanioBuffer;
	uint32_t tid;

	buffer = recibir_buffer(&tamanioBuffer, socketCliente);
	memcpy(&tid, buffer,sizeof(uint32_t));

	FILE* bitacoraTripulante = txt_open_for_append(string_from_format("%s/Files/Bitacoras/Tripulante%d.ims",PUNTO_MONTAJE,tid));
	txt_write_in_file(bitacoraTripulante, string_from_format("Se resuelve el sabotaje\n"));
	free(buffer);
	txt_close_file(bitacoraTripulante);
}
int bitsExcedentes(int cantidadDeBits)
{
	return (cantidadDeBits%8)>0;
}

void terminar_programa(){
	log_destroy(loggerMongo);
	config_destroy(configuracionMongo);
	dictionary_destroy(caracterAsociadoATarea);
}

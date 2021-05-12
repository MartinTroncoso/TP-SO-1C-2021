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

void inicializarVariables(){
	configuracionMongo = config_create("/home/utnso/workspace/tp-2021-1c-No-C-Aprueba-/I-MONGO-STORE/mongo.config");
	loggerMongo = log_create("/home/utnso/workspace/tp-2021-1c-No-C-Aprueba-/I-MONGO-STORE/mongo.log", "I-MONGO-STORE", 1, LOG_LEVEL_INFO);
	TIEMPO_SINCRONIZACION = config_get_int_value(configuracionMongo,"TIEMPO_SINCRONIZACION");
	PUNTO_MONTAJE = config_get_string_value(configuracionMongo,"PUNTO_MONTAJE");
	PUERTO = config_get_string_value(configuracionMongo,"PUERTO");

//	actualizarBitacora(2, MOVIMIENTOTRIPULANTE, "1|2 3|4");
//	actualizarBitacora(2, COMIENZOEJECUCIONDETAREA, "GENERAR_OXIGENO");
//	actualizarBitacora(2, CORREENPANICOSABOTAJE, "");
	inicializarDiccionario();
	inicializarFileSystem();
	socket_servidor = iniciarServidor("127.0.0.1",PUERTO);
	//log_info(loggerMongo, "I-MONGO-STORE listo para recibir al Discordiador");
	socket_discordiador = esperar_cliente(socket_servidor);
	printf("SE CONECTÓ EL DISCORDIADOR!\n");
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
//	config_set_value(configuracionSuperBloque,"BLOCKS","5555");
//	config_save(configuracionSuperBloque);
	config_destroy(configuracionSuperBloque);
	FILE* superBloque = fopen(string_from_format("%s/SuperBloque.ims",PUNTO_MONTAJE),"r+");
	if(superBloque!=NULL)
	{
		fseek(superBloque,-1,SEEK_END);//puntero del archivo apunta a la posicion anterior del EOF
		if(fgetc(superBloque)=='=') //si el caracter anterior al EOF es = entonces el campo de BITMAP esta vacio
		{

			void* memoriaArray = malloc(cantidadDeBloques/8);
			t_bitarray* bitArraySuperBloque = bitarray_create(memoriaArray, cantidadDeBloques);
			//fseek(superBloque,0,SEEK_END);
			txt_write_in_file(superBloque, bitArraySuperBloque->bitarray);
		}
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

void actualizarBitacora(int idTripulante, operacionBitacora idOperacion, char* stringParametros)
{
	FILE* bitacoraTripulante = txt_open_for_append(string_from_format("%s/Files/Bitacoras/Tripulante%d.ims",PUNTO_MONTAJE,idTripulante));
	char** parametros = string_split(stringParametros," ");

	switch(idOperacion)
	{
	case MOVIMIENTO_TRIPULANTE: //EN ESTE CASO EL PARAMETRO DEBE SER UN STRING DEL FORMATO "X|Y X'|Y' (ejemplo 1|2 a 2|2"
		txt_write_in_file(bitacoraTripulante, string_from_format("Se mueve de %s a %s\n",parametros[0],parametros[1]));
		free(parametros);
		break;
	case COMIENZO_EJECUCION_DE_TAREA: // PARAMETRO: "NOMBRETAREA"
		txt_write_in_file(bitacoraTripulante, string_from_format("Comienza ejecucion de tarea %s\n",parametros[0]));
		free(parametros);
		break;
	case FINALIZA_TAREA: //PARAMETRO "NOMBRETAREA"
		txt_write_in_file(bitacoraTripulante, string_from_format("Se finaliza la tarea %s\n",parametros[0]));
		free(parametros);
		break;
	case CORRE_EN_PANICO_SABOTAJE: //PARAMETRO INDISTINTO
		txt_write_in_file(bitacoraTripulante, "Se corre en panico hacia la ubicacion del sabotaje");
		free(parametros);
		break;
	case SABOTAJE_RESUELTO: //PARAMETRO INDISTINTO
		txt_write_in_file(bitacoraTripulante, "sSe resuelve el sabotaje");
		free(parametros);
		break;
	default:
		break;
	}

}
int main(void) {
	inicializarVariables();

	void iterator(char* value)
	{
		printf("%s\n", value);
	}

	t_list* lista;
	while(1)
	{
		int cod_op = recibir_operacion(socket_discordiador);
		switch(cod_op)
		{
		case MENSAJE:
			recibir_mensaje(socket_discordiador);
			break;
		case PAQUETE:
			lista = recibir_paquete(socket_discordiador);
			printf("ME LLEGARON LOS SIGUIENTES VALORES:\n");
			list_iterate(lista, (void*) iterator);
			break;
		case -1:
			log_error(loggerMongo, "SE DESCONECTÓ EL DISCORDIADOR. FINALIZO");
			return EXIT_FAILURE;
		default:
			log_warning(loggerMongo, "Operacion desconocida. No quieras meter la pata");
			break;
		}
	}

	close(socket_servidor);
	close(socket_discordiador);
	log_destroy(loggerMongo);
	config_destroy(configuracionMongo);
	dictionary_destroy(caracterAsociadoATarea);
	return EXIT_SUCCESS;
}

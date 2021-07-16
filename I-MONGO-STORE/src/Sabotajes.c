#include "I-MONGO-STORE.h"

bool verificarCantidadBlock() //true si hay un sabotaje
{
	bool respuesta;
	struct stat statCarpeta;
	char* directorioSuperBloque = string_from_format("%s/SuperBloque.ims",PUNTO_MONTAJE);
	char* directorioBlocks = string_from_format("%s/Blocks.ims",PUNTO_MONTAJE);
	t_config* datosConfig = config_create(directorioSuperBloque);
	stat(directorioBlocks,&statCarpeta);
	int cantidadBlocks = config_get_int_value(datosConfig,"BLOCKS");
	respuesta = statCarpeta.st_size/tamanioBlock != cantidadBlocks;
	free(directorioSuperBloque);
	free(directorioBlocks);
	config_destroy(datosConfig);
	return respuesta;
}

bool verificarBitMap()
{
	struct dirent *dir;
	bool resultado = false;
	char* direccionTripulantes = string_from_format("%s/Files/Bitacoras",PUNTO_MONTAJE);
	char* direccionFiles = string_from_format("%s/Files",PUNTO_MONTAJE);
	char* ubicacion;
	char** arrayBloques;
	int contador;
	t_config* configATrabajar;
	//stat(directorioBlocks,&statCarpeta);

	t_bitarray* bitMapRecuperadoDeArchivos = bitarray_create_with_mode(malloc((cantidadDeBlocks/8)+(cantidadDeBlocks%8)), (cantidadDeBlocks/8)+(cantidadDeBlocks%8), LSB_FIRST);
	for(int i = 0; i<bitarray_get_max_bit(bitMapRecuperadoDeArchivos);i++)
	{
		bitarray_clean_bit(bitMapRecuperadoDeArchivos,i);
	}
	DIR* directorio= opendir(direccionFiles);
	if(directorio == NULL)
	{
		log_info(loggerMongo,"Error en directorio FILES");
		exit(-1);
	}
	while((dir = readdir(directorio))!= NULL)
	{
		ubicacion = string_from_format("%s/%s",direccionFiles,dir->d_name);
		//archivo = fopen(ubicacion,"r");
		if((!strcmp(dir->d_name,".") || !strcmp(dir->d_name,"..") || !strcmp(dir->d_name,"AuxiliarFile.txt")|| !strcmp(dir->d_name,"AuxiliarMD5.txt")|| !strcmp(dir->d_name,"Bitacoras")))
		{

		}else //suponiendo que se filtraron todo lo que no son recursos
		{
			configATrabajar = config_create(ubicacion);
			arrayBloques = config_get_array_value(configATrabajar,"BLOCKS");
			contador = 0;
			while(arrayBloques[contador]!=NULL)
			{
				bitarray_set_bit(bitMapRecuperadoDeArchivos,atoi(arrayBloques[contador]));
				//log_info(loggerMongo, "Posicion bit ocupado: %d",atoi(arrayBloques[contador]));
				contador++;
			}
			config_destroy(configATrabajar);
		}
		free(ubicacion);
	}
	free(dir);
	closedir(directorio);
	//ahora  analiza los archivos tripulantes
	directorio = opendir(direccionTripulantes);
	if(directorio == NULL)
	{
		log_info(loggerMongo,"Error en directorio BITACORA");
		exit(-1);
	}
	while((dir = readdir(directorio))!=NULL)
	{
		ubicacion = string_from_format("%s/%s",direccionTripulantes,dir->d_name);
		if((!strcmp(dir->d_name,".") || !strcmp(dir->d_name,"..")))
		{
		}else
		{
			configATrabajar = config_create(ubicacion);
			arrayBloques = config_get_array_value(configATrabajar,"BLOCKS");
			contador = 0;
			while(arrayBloques[contador]!=NULL)
			{
				bitarray_set_bit(bitMapRecuperadoDeArchivos,atoi(arrayBloques[contador]));
				//log_info(loggerMongo, "Posicion bit ocupado: %d %d",atoi(arrayBloques[contador]),contador);
				contador++;
			}
			config_destroy(configATrabajar);
		}
		free(ubicacion);
	}
	free(dir);
	closedir(directorio);
	t_bitarray* bitArraySistema = recuperarBitArray();
	for(int i=0;i<bitarray_get_max_bit(bitMapRecuperadoDeArchivos);i++)
	{
		if(bitarray_test_bit(bitMapRecuperadoDeArchivos,i) != bitarray_test_bit(bitArraySistema,i))
		{
			log_info(loggerMongo,"BitArray en mal estado");
			resultado = true;
			break;
		}
	}
	bitarray_destroy(bitMapRecuperadoDeArchivos);
	bitarray_destroy(bitArraySistema);
	free(direccionFiles);
	free(direccionTripulantes);
	liberarArray(arrayBloques);
	return resultado;
}

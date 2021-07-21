#include "I-MONGO-STORE.h"

bool verificarCantidadBlock() //true si hay un sabotaje
{//
	//
	//pthread_mutex_lock(&mutexSabotaje);
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
	//pthread_mutex_unlock(&mutexSabotaje);
	return respuesta;
	//
}

bool verificarBitMap()
{
	//pthread_mutex_lock(&mutexSabotaje);
	pthread_mutex_lock(&mutexBitMap);
	bool resultado = false;
	t_bitarray* bitMapDesdeArchivos = bitmapDesdeBloques();
	//ahora  analiza los archivos tripulantes
	t_bitarray* bitArraySistema = recuperarBitArray();
	for(int i=0;i<cantidadDeBlocks;i++)
	{
		if(bitarray_test_bit(bitMapDesdeArchivos,i) != bitarray_test_bit(bitArraySistema,i))
		{
			log_info(loggerMongo,"BitArray en mal estado");
			resultado = true;
			break;
		}
		if(bitarray_test_bit(bitMapDesdeArchivos,i) != bitarray_test_bit(bitMapEnMemoria,i))
		{
			log_info(loggerMongo,"BitArray en memoria en mal estado");
			resultado = true;
			break;
		}
	}
	free(bitMapDesdeArchivos->bitarray);
	free(bitArraySistema->bitarray);
	bitarray_destroy(bitMapDesdeArchivos);
	bitarray_destroy(bitArraySistema);
	pthread_mutex_unlock(&mutexBitMap);
	//pthread_mutex_unlock(&mutexSabotaje);
	return resultado;

}

t_bitarray* bitmapDesdeBloques()
{

	struct dirent *dir;
	char* direccionTripulantes = string_from_format("%s/Files/Bitacoras",PUNTO_MONTAJE);
	char* direccionFiles = string_from_format("%s/Files",PUNTO_MONTAJE);
	char* ubicacion;
	char** arrayBloques;
	int contador;
	t_config* configATrabajar;
	//stat(directorioBlocks,&statCarpeta);

	t_bitarray* bitMapRecuperadoDeArchivos = bitarray_create_with_mode(malloc((cantidadDeBlocks/8)+byteExcedente(cantidadDeBlocks, 8)), (cantidadDeBlocks/8)+byteExcedente(cantidadDeBlocks, 8), LSB_FIRST);
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
				if(bitarray_test_bit(bitMapRecuperadoDeArchivos,atoi(arrayBloques[contador])))
				{
					log_warning(loggerMongo, "BIT OCUPADO ANTES DE SETEARLO");
				}
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
				if(bitarray_test_bit(bitMapRecuperadoDeArchivos,atoi(arrayBloques[contador])))
				{
					log_warning(loggerMongo, "BIT OCUPADO ANTES DE SETEARLO");
				}
				bitarray_set_bit(bitMapRecuperadoDeArchivos,atoi(arrayBloques[contador]));
				//log_info(loggerMongo, "Posicion bit ocupado: %d %d",atoi(arrayBloques[contador]),contador);
				contador++;
			}
			liberarArray(arrayBloques);
			config_destroy(configATrabajar);
		}
		free(ubicacion);
	}
	free(dir);
	closedir(directorio);
	free(direccionFiles);
	free(direccionTripulantes);

	return bitMapRecuperadoDeArchivos;
}

bool verificarSizeFile()
{
	//pthread_mutex_lock(&mutexSabotaje);
	bool resultado = false;
	struct dirent *dir;
	char* ubicacion;
	t_config* configuracionFile;
	char** arrayBloques;
	int contador;
	int marcador;
	char* recursoRecuperado;
	char* stringDeLlenado;
	char* bloqueRecuperadoFile;
	int tamanioFile;
	char caracterDeLlenado;
	char* direccionFiles = string_from_format("%s/Files",PUNTO_MONTAJE);
	DIR* directorio= opendir(direccionFiles);
	if(directorio == NULL)
	{
		log_info(loggerMongo,"Error en directorio FILES");
		exit(-1);
	}
	while((dir = readdir(directorio))!= NULL)
	{
		ubicacion = string_from_format("%s/%s",direccionFiles,dir->d_name);
		if((!strcmp(dir->d_name,".") || !strcmp(dir->d_name,"..") || !strcmp(dir->d_name,"AuxiliarFile.txt")|| !strcmp(dir->d_name,"AuxiliarMD5.txt")|| !strcmp(dir->d_name,"Bitacoras")))
		{

		}else //suponiendo que se filtraron todo lo que no son recursos
		{
			recursoRecuperado = string_new();
			configuracionFile = config_create(ubicacion);
			arrayBloques = config_get_array_value(configuracionFile,"BLOCKS");
			tamanioFile = config_get_int_value(configuracionFile,"SIZE");
			//cantidadBloquesUsados = config_get_int_value(configuracionFile,"BLOCK_COUNT");
			log_info(loggerMongo,"Tamanio file: %d",tamanioFile);
			stringDeLlenado = config_get_string_value(configuracionFile,"CARACTER_LLENADO");
			caracterDeLlenado = stringDeLlenado[0];
			log_info(loggerMongo,"caracter llenado: %c",caracterDeLlenado);
			contador = 0;
			while(arrayBloques[contador]!=NULL)
			{
				bloqueRecuperadoFile = bloqueRecuperado(atoi(arrayBloques[contador]));
				string_append(&recursoRecuperado, bloqueRecuperadoFile);
				contador++;
				free(bloqueRecuperadoFile);
			}

			//

			for(int i = 0; i<contador*tamanioBlock;i++)
			{
				marcador = i;
				if(recursoRecuperado[i]!=caracterDeLlenado)
				{
					marcador = i;
					//resultado = true;
					break;
				}

			}
//			log_warning(loggerMongo,"TamanioFile: %d", tamanioFile);
//			log_warning(loggerMongo,"El marcador es %d", marcador);
			if(marcador!=tamanioFile)
			{
				resultado = true;
			}

//			if(marcador != tamanioFile-1)
//			{
//				resultado = true;
//			}
			free(stringDeLlenado);
			free(recursoRecuperado);
			liberarArray(arrayBloques);
			//config_destroy(configuracionFile);
		}
		free(ubicacion);
	}
	log_info(loggerMongo,"Libero dir");
	free(dir);
	log_info(loggerMongo,"Se libero di y paso a closedir");
	closedir(directorio);  //por alguna razon crashean
	log_info(loggerMongo,"Se libero closedir");



	free(direccionFiles);
	//pthread_mutex_unlock(&mutexSabotaje);
	return resultado;
}

bool verificarMD5()
{
	//pthread_mutex_lock(&mutexSabotaje);
	bool resultado = false;
	struct dirent *dir;
	char* direccionFiles = string_from_format("%s/Files",PUNTO_MONTAJE);
	char* ubicacion;
	char** arrayBloques;
	int contador;
	t_config* configATrabajar;
	char* stringAuxiliar;
	char* recursoRecuperado;
	char* md5Config;
	char* valorMD5;
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
			recursoRecuperado = string_new();
			configATrabajar = config_create(ubicacion);
			arrayBloques = config_get_array_value(configATrabajar,"BLOCKS");
			contador = 0;
			while(arrayBloques[contador]!=NULL)
			{
				//log_info(loggerMongo,"Es un archivo :%s contador: %d",dir->d_name,atoi(arrayBloques[contador]));
				stringAuxiliar = bloqueRecuperado(atoi(arrayBloques[contador]));
				//log_info(loggerMongo,"String Recuperado: %s",stringAuxiliar);
				string_append(&recursoRecuperado, stringAuxiliar);
				contador++;
				free(stringAuxiliar);
			}
			//log_info(loggerMongo,"string recuperado de file: %s",recursoRecuperado);
			md5Config = config_get_string_value(configATrabajar,"MD5_ARCHIVO");
			valorMD5 = obtenerMD5(recursoRecuperado);
			//log_info(loggerMongo,"%s",md5Config);
			//log_info(loggerMongo,"%s",valorMD5);
			if(strcmp(md5Config,valorMD5))
			{
				log_info(loggerMongo,"Valores MD5 distintos");
				resultado = true;
			}
			config_destroy(configATrabajar);
//			free(valorMD5);
//			free(md5Config);
			free(recursoRecuperado);
			liberarArray(arrayBloques);

		}
		free(ubicacion);


	}
	free(dir);
	closedir(directorio);
	free(direccionFiles);


	//pthread_mutex_unlock(&mutexSabotaje);
	return resultado;
}

bool verificarBlockCount()
{
	//pthread_mutex_lock(&mutexSabotaje);
	bool resultado = false;
	char* direccion = string_from_format("%s/Files",PUNTO_MONTAJE);
	struct dirent *dir;
	char* ubicacion;
	DIR* directorio= opendir(direccion);
	t_config* configFile;
	int cantidadDeBloques;
	int contador;
	char** bloquesUtilizados;
	if(directorio == NULL)
	{
		log_info(loggerMongo,"Error en directorio FILES");
		exit(-1);
	}
	while((dir = readdir(directorio))!= NULL)
	{
		ubicacion = string_from_format("%s/%s",direccion,dir->d_name);
		//archivo = fopen(ubicacion,"r");
		if((!strcmp(dir->d_name,".") || !strcmp(dir->d_name,"..") || !strcmp(dir->d_name,"AuxiliarFile.txt")|| !strcmp(dir->d_name,"AuxiliarMD5.txt")|| !strcmp(dir->d_name,"Bitacoras")))
		{

		}else
		{
			configFile = config_create(ubicacion);
			bloquesUtilizados = config_get_array_value(configFile,"BLOCKS");
			cantidadDeBloques = config_get_int_value(configFile,"BLOCK_COUNT");
			contador = 0;
			while(bloquesUtilizados[contador]!=NULL)
			{
				contador++;
			}
			if(contador!=cantidadDeBloques)
			{
				resultado = true;
			}
			config_destroy(configFile);
			liberarArray(bloquesUtilizados);
		}
		free(ubicacion);
	}
	free(dir);
	closedir(directorio);
	free(direccion);
	log_info(loggerMongo,"Valor bool: %d",resultado);


	//pthread_mutex_unlock(&mutexSabotaje);
	return resultado;
}

casoDeSabotaje casoSabotajeActual()
{
	if(verificarCantidadBlock())
	{
		return SABOTAJE_EN_SUPERBLOQUE_CANTIDAD;
	}
	if(verificarBitMap())
	{
		return SABOTAJE_EN_SUPERBLOQUE_BITMAP;
	}
	if(verificarMD5())
	{
		return SABOTAJE_EN_FILE_BLOCKS;
	}
	if(verificarSizeFile())
	{
		return SABOTAJE_EN_FILE_SIZE;
	}
	if(verificarBlockCount())
	{
		return SABOTAJE_EN_FILE_BLOCK_COUNT;
	}


	return NO_HAY_SABOTAJES;
}

void resolverSabotajeBitMap()
{
	pthread_mutex_lock(&mutexBitMap);
	t_bitarray* bitMapBloquesFiles = bitmapDesdeBloques();
	t_bitarray* bitMapSistema = recuperarBitArray();
	for(int i = 0; i < bitarray_get_max_bit(bitMapSistema);i++)
	{
		if(bitarray_test_bit(bitMapBloquesFiles,i)!=bitarray_test_bit(bitMapSistema,i))
		{
			if(bitarray_test_bit(bitMapBloquesFiles,i)==0)
			{
				bitarray_clean_bit(bitMapSistema,i);
			}else
			{
				bitarray_set_bit(bitMapSistema,i);
			}
		}
		if(bitarray_test_bit(bitMapBloquesFiles,i)!=bitarray_test_bit(bitMapEnMemoria,i))
		{
			if(bitarray_test_bit(bitMapBloquesFiles,i)==0)
			{
				bitarray_clean_bit(bitMapEnMemoria,i);
			}else
			{
				bitarray_set_bit(bitMapEnMemoria,i);
			}
		}
	}
	guardarBitArray(bitMapBloquesFiles);
	free(bitMapSistema->bitarray);
	free(bitMapBloquesFiles->bitarray);
	bitarray_destroy(bitMapBloquesFiles);
	bitarray_destroy(bitMapSistema);
	pthread_mutex_unlock(&mutexBitMap);
}

void resolverSabotajeSizeFile()
{
	//pthread_mutex_lock(&mutexSabotaje);
	struct dirent *dir;
	char* ubicacion;
	t_config* configuracionFile;
	char** arrayBloques;
	int contador;
	int marcador;
	char* recursoRecuperado;
	char* stringDeLlenado;
	char* bloqueRecuperadoFile;
	int tamanioFile;
	char caracterDeLlenado;
	char* direccionFiles = string_from_format("%s/Files",PUNTO_MONTAJE);
	DIR* directorio= opendir(direccionFiles);
	if(directorio == NULL)
	{
		log_info(loggerMongo,"Error en directorio FILES");
		exit(-1);
	}
	while((dir = readdir(directorio))!= NULL)
	{
		ubicacion = string_from_format("%s/%s",direccionFiles,dir->d_name);
		if((!strcmp(dir->d_name,".") || !strcmp(dir->d_name,"..") || !strcmp(dir->d_name,"AuxiliarFile.txt")|| !strcmp(dir->d_name,"AuxiliarMD5.txt")|| !strcmp(dir->d_name,"Bitacoras")))
		{

		}else //suponiendo que se filtraron todo lo que no son recursos
		{
			recursoRecuperado = string_new();
			configuracionFile = config_create(ubicacion);
			arrayBloques = config_get_array_value(configuracionFile,"BLOCKS");
			tamanioFile = config_get_int_value(configuracionFile,"SIZE");
			//log_info(loggerMongo,"Tamanio file: %d",tamanioFile);
			stringDeLlenado = config_get_string_value(configuracionFile,"CARACTER_LLENADO");
			caracterDeLlenado = stringDeLlenado[0];
			//log_info(loggerMongo,"caracter llenado: %c",caracterDeLlenado);
			contador = 0;
			while(arrayBloques[contador]!=NULL)
			{
				bloqueRecuperadoFile = bloqueRecuperado(atoi(arrayBloques[contador]));
				string_append(&recursoRecuperado, bloqueRecuperadoFile);
				contador++;
				free(bloqueRecuperadoFile);
			}

			//
			for(int i = 0; i<contador*tamanioBlock;i++)
			{
				printf("%d",i);
				marcador = i;
				if(recursoRecuperado[i]!=caracterDeLlenado)
				{
					marcador = i;
					//resultado = true;
					break;
				}

			}

			if(marcador!=tamanioFile)
			{
				char* nuevoSize = string_itoa(marcador);
				config_set_value(configuracionFile,"SIZE",nuevoSize);
				config_save(configuracionFile);
			}
			//log_info(loggerMongo,"Recuperado de los bloques: %s",recursoRecuperado);
			//log_info(loggerMongo,"Tamanio: %d",string_length(recursoRecuperado));
			//log_info(loggerMongo,"Resultado : %d",resultado);
			free(stringDeLlenado);
			free(recursoRecuperado);
			liberarArray(arrayBloques);
			//config_destroy(configuracionFile);
		}
		free(ubicacion);
	}
	//log_info(loggerMongo,"Libero dir");
	free(dir);
	//log_info(loggerMongo,"Se libero di y paso a closedir");
	closedir(directorio);  //por alguna razon crashean
	//log_info(loggerMongo,"Se libero closedir");



	free(direccionFiles);
	//pthread_mutex_unlock(&mutexSabotaje);
}

void resolverSabotajeBlockCount()
{
	//pthread_mutex_lock(&mutexSabotaje);
	char* direccion = string_from_format("%s/Files",PUNTO_MONTAJE);
	struct dirent *dir;
	char* ubicacion;
	DIR* directorio= opendir(direccion);
	t_config* configFile;
	int cantidadDeBloques;
	int contador;
	char** bloquesUtilizados;
	if(directorio == NULL)
	{
		log_info(loggerMongo,"Error en directorio FILES");
		exit(-1);
	}
	while((dir = readdir(directorio))!= NULL)
	{
		ubicacion = string_from_format("%s/%s",direccion,dir->d_name);
		//archivo = fopen(ubicacion,"r");
		if((!strcmp(dir->d_name,".") || !strcmp(dir->d_name,"..") || !strcmp(dir->d_name,"AuxiliarFile.txt")|| !strcmp(dir->d_name,"AuxiliarMD5.txt")|| !strcmp(dir->d_name,"Bitacoras")))
		{

		}else
		{
			configFile = config_create(ubicacion);
			bloquesUtilizados = config_get_array_value(configFile,"BLOCKS");
			cantidadDeBloques = config_get_int_value(configFile,"BLOCK_COUNT");
			contador = 0;
			while(bloquesUtilizados[contador]!=NULL)
			{
				contador++;
			}
			if(contador!=cantidadDeBloques)
			{
				char* nuevoBlockCount = string_itoa(contador);
				config_set_value(configFile,"BLOCK_COUNT",nuevoBlockCount);
				config_save(configFile);
				free(nuevoBlockCount);
			}
			config_destroy(configFile);
			liberarArray(bloquesUtilizados);
		}
		free(ubicacion);
	}
	free(dir);
	closedir(directorio);
	free(direccion);
	//pthread_mutex_unlock(&mutexSabotaje);
}

void resolverSabotajeMD5()
{
	//pthread_mutex_lock(&mutexSabotaje);
	char* direccion = string_from_format("%s/Files",PUNTO_MONTAJE);
	struct dirent *dir;
	char* ubicacion;
	DIR* directorio= opendir(direccion);
	t_config* configATrabajar;
	int cantidadBloquesEnFile;
	int contador;
	char* md5Config;
	char* valorMD5;
	char** bloquesUtilizados;
	char* recursoRecuperado;
	char* stringAuxiliar;
	if(directorio == NULL)
	{
		log_info(loggerMongo,"Error en directorio FILES");
		exit(-1);
	}
	while((dir = readdir(directorio))!= NULL)
	{
		ubicacion = string_from_format("%s/%s",direccion,dir->d_name);
		//archivo = fopen(ubicacion,"r");
		if((!strcmp(dir->d_name,".") || !strcmp(dir->d_name,"..") || !strcmp(dir->d_name,"AuxiliarFile.txt")|| !strcmp(dir->d_name,"AuxiliarMD5.txt")|| !strcmp(dir->d_name,"Bitacoras")))
		{

		}else
		{
			recursoRecuperado = string_new();
			configATrabajar = config_create(ubicacion);
			bloquesUtilizados = config_get_array_value(configATrabajar,"BLOCKS");
			md5Config = config_get_string_value(configATrabajar,"MD5_ARCHIVO");
			cantidadBloquesEnFile = config_get_int_value(configATrabajar,"BLOCK_COUNT");
			contador = 0;
			while(bloquesUtilizados[contador]!=NULL)
			{
				log_info(loggerMongo,"Es un archivo :%s contador: %d",dir->d_name,atoi(bloquesUtilizados[contador]));
				stringAuxiliar = bloqueRecuperado(atoi(bloquesUtilizados[contador]));
				log_info(loggerMongo,"String Recuperado: %s",stringAuxiliar);
				string_append(&recursoRecuperado, stringAuxiliar);
				contador++;
				free(stringAuxiliar);
			}
			//log_info(loggerMongo,"string recuperado de file: %s",recursoRecuperado);
			valorMD5 = obtenerMD5(recursoRecuperado);
			//log_info(loggerMongo,"%s",md5Config);
			//log_info(loggerMongo,"%s",valorMD5);
			if(strcmp(md5Config,valorMD5))
			{
				//log_info(loggerMongo,"Valores MD5 distintos");
				char* stringRecurso = config_get_string_value(configATrabajar,"CARACTER_LLENADO");
				int tamanioFile = config_get_int_value(configATrabajar,"SIZE");
				char* stringParaBloques = string_repeat(' ', cantidadBloquesEnFile*tamanioBlock);
				char* recursosTotales = string_repeat(stringRecurso[0], tamanioFile);
				memcpy(stringParaBloques,recursosTotales,string_length(recursosTotales));
				char* stringPartido;
				for(int i= 0; i<cantidadBloquesEnFile;i++)
				{
					stringPartido = string_substring(stringParaBloques, i*tamanioBlock, tamanioBlock);
					escribirEnBlocks(atoi(bloquesUtilizados[i]), stringPartido);
					free(stringPartido);
				}
				free(stringRecurso);
				free(stringParaBloques);
				free(recursosTotales);
				//resultado = true;
			}
			//config_destroy(configATrabajar);
//			free(valorMD5);
//			free(md5Config);
			free(recursoRecuperado);
			liberarArray(bloquesUtilizados);
		}
		free(ubicacion);
	}
	free(dir);
	closedir(directorio);
	free(direccion);
	//pthread_mutex_unlock(&mutexSabotaje);
}

void resolverSabotajeCantidadBlocks()
{
	//pthread_mutex_lock(&mutexSabotaje);
	struct stat statCarpeta;
	char* direccionSuperBloque = string_from_format("%s/SuperBloque.ims",PUNTO_MONTAJE);
	char* directorioBlocks = string_from_format("%s/Blocks.ims",PUNTO_MONTAJE);
	t_config* datosConfig = config_create(direccionSuperBloque);
	stat(directorioBlocks,&statCarpeta);
	//int cantidadBlocks = config_get_int_value(datosConfig,"BLOCKS");
	//respuesta = statCarpeta.st_size/tamanioBlock != cantidadBlocks;
	t_bitarray* arrayAGuardar = recuperarBitArray();
	int cantidadDeBloquesReal = statCarpeta.st_size/tamanioBlock;
	char* nuevaData = string_from_format("BLOCK_SIZE=%d\nBLOCKS=%d\nBITMAP=",tamanioBlock,cantidadDeBloquesReal);
	stat(direccionSuperBloque,&statCarpeta);
	int archivo = open(direccionSuperBloque,O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
	if(statCarpeta.st_size != string_length(nuevaData)+arrayAGuardar->size)
	{
		ftruncate(archivo,string_length(nuevaData)+arrayAGuardar->size);
	}
	char* mapeoArchivo = mmap(NULL,string_length(nuevaData)+arrayAGuardar->size,PROT_READ | PROT_WRITE, MAP_SHARED, archivo,0);
	memcpy(mapeoArchivo,nuevaData,string_length(nuevaData));
	memcpy(mapeoArchivo+string_length(nuevaData),arrayAGuardar->bitarray,arrayAGuardar->size);
	msync(mapeoArchivo,string_length(nuevaData)+arrayAGuardar->size,MS_INVALIDATE);
	munmap(mapeoArchivo,string_length(nuevaData)+arrayAGuardar->size);
	free(direccionSuperBloque);
	free(directorioBlocks);
	config_destroy(datosConfig);
	free(arrayAGuardar->bitarray);
	bitarray_destroy(arrayAGuardar);
	free(nuevaData);
	//pthread_mutex_unlock(&mutexSabotaje);
}

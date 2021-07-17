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

int main(void){
	signal(SIGUSR1,informarSabotaje);
	signal(SIGINT,terminar_programa); //ctrl+C

	inicializarVariables();
	//escribirFile("Oxigeno", 39);
//	t_bitarray* unArray = recuperarBitArray();
//	bitarray_clean_bit(unArray,2);
//	guardarBitArray(unArray);
//	bitarray_destroy(unArray);
	//bool unvalor = verificarSizeFile();
	//resolverSabotajeSizeFile();
	//eliminarArchivoYLiberar("Oxigeno");
//	log_info(loggerMongo,"Caso sabotaje: %d",casoSabotajeActual());
//	log_info(loggerMongo,"Caso sabotaje: %d",casoSabotajeActual());
//	log_info(loggerMongo,"Caso sabotaje: %d",casoSabotajeActual());
//	resolverSabotajeBitMap();
//	resolverSabotajeBlockCount();
//	resolverSabotajeCantidadBlocks();
//	resolverSabotajeMD5();
//	resolverSabotajeSizeFile();
	log_info(loggerMongo,"Situacion de sabotaje %d",casoSabotajeActual());
	ejecutarFSCK();
	log_info(loggerMongo,"Situacion de sabotaje %d",casoSabotajeActual());
	log_info(loggerMongo,"PID DE I-MONGO-STORE: %d",getpid());

	int socket_escucha = iniciarServidor(IP_I_MONGO,PUERTO_I_MONGO);
	log_info(loggerMongo,"I-MONGO Listo para atender a los Tripulantes!");

	pthread_t hilo_sincro;
	pthread_create(&hilo_sincro,NULL,(void*) asincronia,NULL);
	pthread_detach(hilo_sincro);

	while(1){
		int socket_cliente = esperar_cliente(socket_escucha);
		log_info(loggerMongo,"Se conectó un Tripulante!");

		pthread_t hilo_receptor;
		pthread_create(&hilo_receptor , NULL ,(void*) atenderTripulante, (void*) socket_cliente);
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
	IP_DISCORDIADOR = config_get_string_value(configuracionMongo,"IP_DISCORDIADOR");
	PUERTO_DISCORDIADOR = config_get_string_value(configuracionMongo,"PUERTO_DISCORDIADOR");
	POSICIONES_SABOTAJE = config_get_array_value(configuracionMongo,"POSICIONES_SABOTAJE");
	posicionSabotajeActual = string_split(POSICIONES_SABOTAJE[0],"|");
	tamanioBlock = config_get_int_value(configuracionMongo,"BLOCK_SIZE");
	cantidadDeBlocks = config_get_int_value(configuracionMongo,"BLOCKS");
	sabotajesResueltos = 0;

	pthread_mutex_init(&mutexBitMap, NULL);
	pthread_mutex_init(&mutexSincro,NULL);
	pthread_mutex_init(&mutexMD5,NULL);
	pthread_mutex_init(&mutexFile,NULL);
	pthread_mutex_init(&mutexSabotaje,NULL);
//	actualizarBitacora(2, MOVIMIENTOTRIPULANTE, "1|2 3|4");
//	actualizarBitacora(2, COMIENZOEJECUCIONDETAREA, "GENERAR_OXIGENO");
//	actualizarBitacora(2, CORREENPANICOSABOTAJE, "");
	inicializarCarpetas();
	inicializarDiccionario();
	inicializarFileSystem();
	inicializarMapeoBlocks();
//	socket_servidor = iniciarServidor("127.0.0.1",PUERTO);
//	log_info(loggerMongo, "I-MONGO-STORE listo para recibir al Discordiador");
//	socket_discordiador = esperar_cliente(socket_servidor);
//	printf("SE CONECTÓ EL DISCORDIADOR!\n");
}

void inicializarDiccionario(){
	caracterAsociadoATarea = dictionary_create();
	dictionary_put(caracterAsociadoATarea, "GENERAR_OXIGENO",(char*) "Oxigeno");
	dictionary_put(caracterAsociadoATarea, "CONSUMIR_OXIGENO",(char*) "Oxigeno");
	dictionary_put(caracterAsociadoATarea, "GENERAR_COMIDA",(char*) "Comida");
	dictionary_put(caracterAsociadoATarea, "CONSUMIR_COMIDA",(char*) "Comida");
	dictionary_put(caracterAsociadoATarea, "GENERAR_BASURA",(char*) "Basura");
	dictionary_put(caracterAsociadoATarea, "DESCARTAR_BASURA",(char*) "Basura");
}

void inicializarFileSystem(){
	inicializarSuperBloque();
	inicializarBlocks();
}

void inicializarSuperBloque(){

	//calloc(cantidadDeBloques/8+cantidadDeBloques%8,1);
	//cantidadDeBlocks=1024;
	struct stat statCarpeta;
	char* direccionSuperBloque = string_from_format("%s/SuperBloque.ims",PUNTO_MONTAJE);
	if(stat(direccionSuperBloque,&statCarpeta)==-1)
	{

		log_info(loggerMongo,"No existe SupreBloque");
		t_bitarray* bitArray = bitarray_create_with_mode(malloc((cantidadDeBlocks/8)+(cantidadDeBlocks%8)), (cantidadDeBlocks/8)+(cantidadDeBlocks%8), LSB_FIRST);
		log_info(loggerMongo,"Tamanio struct: %d", sizeof(bitArray));
		log_info(loggerMongo,"Tamanio bitarray antes de poner en 1: %d",string_length(bitArray->bitarray));
		for(int i = 0; i<bitarray_get_max_bit(bitArray);i++)
		{
			bitarray_clean_bit(bitArray,i);
		}
//		bitarray_set_bit(bitArray,0);
//		bitarray_set_bit(bitArray,11);
//		bitarray_set_bit(bitArray,20);
//		bitarray_set_bit(bitArray,50);
		log_info(loggerMongo,"Tamanio struct post set: %d", sizeof(bitArray));
		log_info(loggerMongo,"Tamanio bitarray: %d",string_length(bitArray->bitarray));
		char* stringArchivo = string_from_format("BLOCK_SIZE=%d\nBLOCKS=%d\nBITMAP=",tamanioBlock,cantidadDeBlocks);
		int archivo = open(direccionSuperBloque,O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
		ftruncate(archivo,string_length(stringArchivo)+bitArray->size);
		struct stat caracteristicasArchivo;
		if(fstat(archivo,&caracteristicasArchivo)== -1)
		{
			log_info(loggerMongo,"No se pudo tener el tamaño del archivo.");
			exit(-4);
		}
		log_info(loggerMongo, "SuperBloque.ims creado");


//		for(int i=0;i<bitarray_get_max_bit(bitArray);i++)
//		{
//			//bitarray_set_bit(bitArray,i);
//			printf("%d",bitarray_test_bit(bitArray,i));
//		}
	//	memcpy(paraArchivo,&stringArchivo,strlen(stringArchivo));
	//	acumulador +=  strlen(stringArchivo);
	//	memcpy(&paraArchivo+acumulador,bitArray,sizeof(t_bitarray));
		char* mapeoArchivo = mmap(NULL,string_length(stringArchivo)+bitArray->size,PROT_READ | PROT_WRITE, MAP_SHARED, archivo,0);
		memcpy(mapeoArchivo,stringArchivo,string_length(stringArchivo));
		memcpy(mapeoArchivo+string_length(stringArchivo),bitArray->bitarray,bitArray->size);
		msync(mapeoArchivo,string_length(stringArchivo)+bitArray->size,MS_INVALIDATE);
		munmap(mapeoArchivo,string_length(stringArchivo)+bitArray->size);
		close(archivo);
		free(stringArchivo);
		free(bitArray->bitarray);
		bitarray_destroy(bitArray);
	}else
	{
		log_info(loggerMongo,"El archvio SuperBloque.ims ya existe");
		t_bitarray* recuperado = recuperarBitArray();
		log_info(loggerMongo,"Posicion del blocks libre: %d", posicionBlockLibre(recuperado));
		guardarBitArray(recuperado);
		free(recuperado->bitarray);
		bitarray_destroy(recuperado);
	}
	free(direccionSuperBloque);

}

t_bitarray* recuperarBitArray(){
	char* direccionSuperBloque = string_from_format("%s/SuperBloque.ims",PUNTO_MONTAJE);
	int archivo = open(direccionSuperBloque, O_RDWR, S_IRUSR | S_IWUSR);
	struct stat caracteristicasArchivo;
	if(fstat(archivo,&caracteristicasArchivo)== -1)
	{
		log_info(loggerMongo,"No se pudo tener el tamaño del archivo.");
	}
	char* archivoEnMemoria = mmap(NULL, caracteristicasArchivo.st_size, PROT_READ | PROT_WRITE, MAP_SHARED,archivo,0);
	int contador=0;
	int marcador = 0;
	while(contador <3)
	{
		if(archivoEnMemoria[marcador]=='=')
		{
			contador++;
		}
			marcador++;
	}
	//	for(marcador; marcador<caracteristicasArchivo2.st_size;marcador++)
	//	{
	//		printf("%c",archivoEnMemoria[marcador]);
	//	}
	void* punteroMemoria = calloc((cantidadDeBlocks/8)+(cantidadDeBlocks%8),1);
	t_bitarray* nuevoBitArray = bitarray_create_with_mode(punteroMemoria, cantidadDeBlocks/8+cantidadDeBlocks%8, LSB_FIRST);
	//char* charArray = malloc(caracteristicasArchivo2.st_size - marcador);
	for(int i = 0; i<bitarray_get_max_bit(nuevoBitArray);i++)
	{
		bitarray_clean_bit(nuevoBitArray,i);
	}
	if(caracteristicasArchivo.st_size == marcador)
	{
		log_info(loggerMongo,"BITARRAY VACIO");
	}else
	{
		memcpy((nuevoBitArray->bitarray),archivoEnMemoria+marcador,caracteristicasArchivo.st_size - marcador);
	}
	//free(punteroMemoria);
//	for(int i=0;i<bitarray_get_max_bit(nuevoBitArray);i++)
//	{
//		//bitarray_set_bit(bitArray,i);
//		printf("%d",bitarray_test_bit(nuevoBitArray,i));
//	}
	free(direccionSuperBloque);
	munmap(archivoEnMemoria,caracteristicasArchivo.st_size);
	return nuevoBitArray;
}

void guardarBitArray(t_bitarray* arrayAGuardar)
{
	//Buscar puntero siguiente al tercer '='
	//si uso config_set_value y config_save cambia de posicion los valores dentro del archivo
	char* direccionSuperBloque = string_from_format("%s/SuperBloque.ims",PUNTO_MONTAJE);
	int archivo = open(direccionSuperBloque, O_RDWR, S_IRUSR | S_IWUSR);
	struct stat caracteristicasArchivo;
	if(fstat(archivo,&caracteristicasArchivo)== -1)
	{
		log_info(loggerMongo,"No se pudo tener el tamaño del archivo.");
	}
	char* archivoEnMemoria = mmap(NULL, caracteristicasArchivo.st_size, PROT_READ | PROT_WRITE, MAP_SHARED,archivo,0);
	int contador=0;
	int marcador = 0;
	while(contador <3)
	{
		if(archivoEnMemoria[marcador]=='=')
		{
			contador++;
		}
			marcador++;
	}
	if(caracteristicasArchivo.st_size == marcador)
	{
		log_info(loggerMongo,"BITARRAY VACIO en guardar");
	}else
	{
		memcpy(archivoEnMemoria+marcador,arrayAGuardar->bitarray,caracteristicasArchivo.st_size - marcador);
	}

	if(msync(archivoEnMemoria,caracteristicasArchivo.st_size,MS_INVALIDATE)==0)
	{
		printf("\nSe actualizo el archivo\n");
	}
	munmap(archivoEnMemoria,caracteristicasArchivo.st_size);
	free(direccionSuperBloque);
	close(archivo);
}

int posicionBlockLibre(t_bitarray* bitMap)
{
	int posicion = 0;
	log_info(loggerMongo,"Tamanio bitmap: %d",bitarray_get_max_bit(bitMap));
	while(posicion < bitarray_get_max_bit(bitMap) )
	{
		if(!bitarray_test_bit(bitMap,posicion))
		{
			return posicion;
		}
		else
		{
			posicion++;
		}
	}
	if(posicion == bitarray_get_max_bit(bitMap))
	{
		log_info(loggerMongo,"ARCHIVO BLOCKS LLENO, TERMINANDO PROGRAMA");
		exit(-6);
	}
	return posicion;
}

void inicializarBlocks()
{
	char* direccionSuperBloque = string_from_format("%s/SuperBloque.ims",PUNTO_MONTAJE);
	char* direccionBlocks = string_from_format("%s/Blocks.ims",PUNTO_MONTAJE);
	t_config* configuracionBlocks = config_create(direccionSuperBloque);
	tamanioBlock = config_get_int_value(configuracionBlocks,"BLOCK_SIZE");
	cantidadDeBlocks = config_get_int_value(configuracionBlocks,"BLOCKS");
	config_destroy(configuracionBlocks);

	fdArchivoBlocks = open(direccionBlocks, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
	struct stat infoBlocks;
	if(fstat(fdArchivoBlocks,&infoBlocks)== -1)
	{
		log_info(loggerMongo, "No se pudo obtener stat de Blocks.ims");
	}
	if(infoBlocks.st_size != tamanioBlock*cantidadDeBlocks)
	{
		char* repeatVacio = string_repeat(' ', tamanioBlock*cantidadDeBlocks);
		if(ftruncate(fdArchivoBlocks,tamanioBlock*cantidadDeBlocks)==-1)
		{
			log_info(loggerMongo, "No se pudo crear archivo con el tamaño %dx%d",tamanioBlock,cantidadDeBlocks);
			write(fdArchivoBlocks,repeatVacio,tamanioBlock*cantidadDeBlocks);
		}
		write(fdArchivoBlocks,repeatVacio,tamanioBlock*cantidadDeBlocks);
		log_info(loggerMongo, "Se creo el archivo con el tamaño %dx%d",tamanioBlock,cantidadDeBlocks);
		free(repeatVacio);
	}
	log_info(loggerMongo, "Tamaño del archivo total es %d",infoBlocks.st_size);


	//struct stat infoBlocks;

	//log_info(loggerMongo,"%d",strlen(string_repeat(' ', tamanioBlock*cantidadDeBlocks)));

	if(fstat(fdArchivoBlocks,&infoBlocks)== -1)
	{
			log_info(loggerMongo, "No se pudo obtener stat de Blocks.ims");
	}
	log_info(loggerMongo, "Tamaño del archivo total es %d",infoBlocks.st_size);
	//
	free(direccionBlocks);
	free(direccionSuperBloque);
}

void inicializarMapeoBlocks(){
	struct stat infoBlocks;

	if(fstat(fdArchivoBlocks,&infoBlocks)== -1){
		log_info(loggerMongo, "No se pudo obtener stat de Blocks.ims");
		exit(-3);
	}

	if(infoBlocks.st_size == cantidadDeBlocks * tamanioBlock){
		blocksMap = malloc(cantidadDeBlocks * tamanioBlock);
		blocksMapOriginal = mmap(NULL, cantidadDeBlocks * tamanioBlock,PROT_READ | PROT_WRITE, MAP_SHARED,fdArchivoBlocks,0);
		memcpy(blocksMap,blocksMapOriginal,cantidadDeBlocks * tamanioBlock);
		//memcpy(blocksMap,"B",sizeof(char));
		//log_info(loggerMongo, "Mapeo original : %s",blocksMapOriginal);
		//log_info(loggerMongo, "Copia de mapeo : %s",blocksMap);
	}
}

void forzarSincronizacionBlocks(){
	memcpy(blocksMapOriginal, blocksMap, cantidadDeBlocks * tamanioBlock);
	if(msync(blocksMapOriginal,(cantidadDeBlocks*tamanioBlock),MS_SYNC)==0)
	log_info(loggerMongo, "BLOCK SINCRONIZADO");
}

void atenderTripulante(void* _cliente)
{
	int socket_tripulante = (int) _cliente;

	uint32_t idTripulante;
	recv(socket_tripulante,&idTripulante,sizeof(uint32_t),0);

	struct stat statCarpeta;
	char* directorioTripulante = string_from_format("%s/Files/Bitacoras/Tripulante%d.ims",PUNTO_MONTAJE,idTripulante);
	if(stat(directorioTripulante,&statCarpeta)==-1)
	{
		FILE* archivoBitacora = fopen(directorioTripulante,"w");
		txt_write_in_file(archivoBitacora, "SIZE=0\nBLOCKS=[]");
		log_info(loggerMongo, "Archivo Tripulante%d.ims creado",idTripulante);
	}
	free(directorioTripulante);
	//pthread_t idHilo = pthread_self();
	while(1){
		int op_code = recibir_operacion(socket_tripulante);

		switch(op_code)
		{
		case INFORMAR_DESPLAZAMIENTO_FS:
			recibirInformeDeDesplazamiento(socket_tripulante,idTripulante);
			break;
		case INICIO_TAREA:
			recibirInicioDeTarea(socket_tripulante,idTripulante);
			break;
		case FINALIZO_TAREA:
			recibirFinalizaTarea(socket_tripulante,idTripulante);
			break;
		case PETICION_ENTRADA_SALIDA:
			log_info(loggerMongo,"[TRIPULANTE %d] REALIZA PETICIÓN DE ENTRADA/SALIDA",idTripulante);
			realizarTareaIO(socket_tripulante,idTripulante);
			break;
		case ATENDER_SABOTAJE:
			loggearAtencionSabotaje(idTripulante);
			break;
		case RESOLUCION_SABOTAJE:
			loggearResolucionSabotaje(idTripulante);
			break;
		case INVOCAR_FSCK:
			log_info(loggerMongo,"Se ejecuta el FSCK. Por ahora no hace nada :D");
			ejecutarFSCK();
			break;
		case OBTENER_BITACORA:
			enviarBitacora(socket_tripulante,idTripulante);
			break;
		case EXPULSAR_TRIPULANTE:
			close(socket_tripulante);
			return;
			break;
		default:
			log_info(loggerMongo , "[TRIPULANTE %d] Tipo de mensaje desconocido!!!",idTripulante);
			close(socket_tripulante);
			return;
			break;
		}
	}
}

void realizarTareaIO(int socket_tripulante, uint32_t id_tripulante){
	int tipoTarea = recibir_operacion(socket_tripulante);

	switch(tipoTarea){
	case GENERAR_OXIGENO:{
		uint32_t caracteresAGenerar;
		recv(socket_tripulante,&caracteresAGenerar,sizeof(uint32_t),0);
		escribirFile("Oxigeno", caracteresAGenerar);
		printf("[TRIPULANTE %d] CARACTERES A GENERAR EN Oxigeno.ims: %d\n",id_tripulante,caracteresAGenerar);
		break;
	}
	case CONSUMIR_OXIGENO:{
		if(existeArchivoRecurso("Oxigeno"))
		{
			tipo_mensaje respuesta = EXISTE_EL_ARCHIVO; //HASTA TENER BIEN DEFINIDO LO DE LOS ARCHIVOS
			send(socket_tripulante,&respuesta,sizeof(tipo_mensaje),0);
			uint32_t caracteresABorrar;
			recv(socket_tripulante,&caracteresABorrar,sizeof(uint32_t),0);
			eliminarCaracterFile("Oxigeno", caracteresABorrar);
			printf("[TRIPULANTE %d] CARACTERES A BORRAR DE Oxigeno.ims: %d\n",id_tripulante,caracteresABorrar);
		}else
		{
			tipo_mensaje respuesta = NO_EXISTE_EL_ARCHIVO;
			send(socket_tripulante,&respuesta,sizeof(tipo_mensaje),0);
		}
		break;
	}
	case GENERAR_COMIDA:{
		uint32_t caracteresAGenerar;
		recv(socket_tripulante,&caracteresAGenerar,sizeof(uint32_t),0);
		escribirFile("Comida", caracteresAGenerar);
		printf("[TRIPULANTE %d] CARACTERES A GENERAR EN Comida.ims: %d\n",id_tripulante,caracteresAGenerar);
		break;
	}
	case CONSUMIR_COMIDA:{
		if(existeArchivoRecurso("Comida"))
		{
			tipo_mensaje respuesta = EXISTE_EL_ARCHIVO; //HASTA TENER BIEN DEFINIDO LO DE LOS ARCHIVOS
			send(socket_tripulante,&respuesta,sizeof(tipo_mensaje),0);
			uint32_t caracteresABorrar;
			recv(socket_tripulante,&caracteresABorrar,sizeof(uint32_t),0);
			eliminarCaracterFile("Comida", caracteresABorrar);
			printf("[TRIPULANTE %d] CARACTERES A BORRAR DE Comidas.ims: %d\n",id_tripulante,caracteresABorrar);
		}else
		{
			tipo_mensaje respuesta = NO_EXISTE_EL_ARCHIVO;
			send(socket_tripulante,&respuesta,sizeof(tipo_mensaje),0);
		}
		break;
	}
	case GENERAR_BASURA:{
		uint32_t caracteresAGenerar;
		recv(socket_tripulante,&caracteresAGenerar,sizeof(uint32_t),0);
		escribirFile("Basura", caracteresAGenerar);
		printf("[TRIPULANTE %d] CARACTERES A GENERAR EN Basura.ims: %d\n",id_tripulante,caracteresAGenerar);
		break;
	}
	case DESCARTAR_BASURA:{
		if(existeArchivoRecurso("Basura"))
		{
			tipo_mensaje respuesta = EXISTE_EL_ARCHIVO; //HASTA TENER BIEN DEFINIDO LO DE LOS ARCHIVOS
			send(socket_tripulante,&respuesta,sizeof(tipo_mensaje),0);
			//char* direccionBasura = string_from_format("%s/Files/Basura.ims",PUNTO_MONTAJE);
			//log_info(loggerMongo,"Direccion basura :%s",direccionBasura);
			eliminarArchivoYLiberar("Basura");
			//free(direccionBasura);
			printf("[TRIPULANTE %d] Se borra Basura.ims\n",id_tripulante);
		}else
		{
			tipo_mensaje respuesta = NO_EXISTE_EL_ARCHIVO;
			send(socket_tripulante,&respuesta,sizeof(tipo_mensaje),0);
		}
		break;
	}
	default:
		break;
	}
}

void recibirInformeDeDesplazamiento(int socket_tripulante, uint32_t id_tripulante)
{
	void* buffer;
	uint32_t sizeBuffer;
	uint32_t coorXAnterior;
	uint32_t coorYAnterior;
	uint32_t coorXNueva;
	uint32_t coorYNueva;
	uint32_t desplazamiento = 0;

	buffer = recibir_buffer(&sizeBuffer, socket_tripulante);

	memcpy(&coorXAnterior,buffer+desplazamiento,sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);
	memcpy(&coorYAnterior,buffer+desplazamiento,sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);
	memcpy(&coorXNueva,buffer+desplazamiento,sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);
	memcpy(&coorYNueva,buffer+desplazamiento,sizeof(uint32_t));

	log_info(loggerMongo,"[TRIPULANTE %d] Se mueve de %d|%d a %d|%d",id_tripulante,coorXAnterior,coorYAnterior,coorXNueva,coorYNueva);
	char* direccionBitacoraTripulante = string_from_format("%s/Files/Bitacoras/Tripulante%d.ims",PUNTO_MONTAJE,id_tripulante);
	t_config* configuracionTripulante = config_create(direccionBitacoraTripulante);
	char* stringBitacora = string_from_format("Se mueve de %d|%d a %d|%d;",coorXAnterior,coorYAnterior,coorXNueva,coorYNueva);
	escribirBitacora(stringBitacora, configuracionTripulante);
	config_save(configuracionTripulante);
	config_destroy(configuracionTripulante);
	free(stringBitacora);
	free(direccionBitacoraTripulante);
	free(buffer);
}

void recibirInicioDeTarea(int socket_tripulante, uint32_t id_tripulante)
{
	void* buffer;
	uint32_t sizeBuffer;
	uint32_t sizeTarea;

	buffer = recibir_buffer(&sizeBuffer, socket_tripulante);

	int desplazamiento = 0;

	memcpy(&sizeTarea, buffer + desplazamiento, sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);

	char* tarea = malloc(sizeTarea);
	memcpy(tarea, buffer + desplazamiento, sizeTarea);

	log_info(loggerMongo,"[TRIPULANTE %d] Inicia la tarea %s",id_tripulante,tarea);
	char* directorioTripulante = string_from_format("%s/Files/Bitacoras/Tripulante%d.ims",PUNTO_MONTAJE,id_tripulante);
	t_config* configuracionTripulante = config_create(directorioTripulante);

	char* stringBitacora = string_from_format("Inicia la tarea %s;",tarea);
	escribirBitacora(stringBitacora, configuracionTripulante);
	config_save(configuracionTripulante);
	config_destroy(configuracionTripulante);
	free(directorioTripulante);
	free(stringBitacora);
	free(tarea);
	free(buffer);
}

void recibirFinalizaTarea(int socket_tripulante, uint32_t id_tripulante){
	void* buffer;
	uint32_t sizeBuffer;
	uint32_t sizeTarea;

	buffer = recibir_buffer(&sizeBuffer, socket_tripulante);

	int desplazamiento = 0;

	memcpy(&sizeTarea, buffer + desplazamiento, sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);

	char* tarea = malloc(sizeTarea);
	memcpy(tarea, buffer + desplazamiento, sizeTarea);

	log_info(loggerMongo,"[TRIPULANTE %d] Finaliza la tarea %s",id_tripulante,tarea);
	char* directorioTripulante = string_from_format("%s/Files/Bitacoras/Tripulante%d.ims",PUNTO_MONTAJE,id_tripulante);
	t_config* configuracionTripulante = config_create(directorioTripulante);

	char* stringBitacora = string_from_format("Se finaliza la tarea %s;",tarea);
	escribirBitacora(stringBitacora, configuracionTripulante);
	config_save(configuracionTripulante);
	config_destroy(configuracionTripulante);

	free(directorioTripulante);
	free(stringBitacora);
	free(tarea);
	free(buffer);
}

void loggearAtencionSabotaje(uint32_t id_tripulante){
	log_info(loggerMongo,"[TRIPULANTE %d] ATIENDE EL SABOTAJE",id_tripulante);
	char* directorioTripulante = string_from_format("%s/Files/Bitacoras/Tripulante%d.ims",PUNTO_MONTAJE,id_tripulante);
	t_config* configuracionTripulante = config_create(directorioTripulante);

	char* stringBitacora = string_duplicate("Atiende el sabotaje");
	escribirBitacora(stringBitacora, configuracionTripulante);
	config_save(configuracionTripulante);
	config_destroy(configuracionTripulante);

	free(directorioTripulante);
	free(stringBitacora);
}

void loggearResolucionSabotaje(uint32_t id_tripulante){
	log_info(loggerMongo,"[TRIPULANTE %d] RESOLVIÓ EL SABOTAJE",id_tripulante);
	char* directorioTripulante = string_from_format("%s/Files/Bitacoras/Tripulante%d.ims",PUNTO_MONTAJE,id_tripulante);
	t_config* configuracionTripulante = config_create(directorioTripulante);

	char* stringBitacora = string_duplicate("Resolvio el sabotaje\n");
	escribirBitacora(stringBitacora, configuracionTripulante);
	config_save(configuracionTripulante);
	config_destroy(configuracionTripulante);

	liberarArray(posicionSabotajeActual);
	posicionSabotajeActual = getSiguientePosicionSabotaje();

	free(directorioTripulante);
	free(stringBitacora);
}

void enviarBitacora(int socket_tripulante, uint32_t id_tripulante){
	char* bitacora = recuperarBitacora(id_tripulante);
	t_buffer* buffer = malloc(sizeof(t_buffer));
	buffer->size = strlen(bitacora) + 1;
	buffer->stream = malloc(buffer->size);
	memcpy(buffer->stream,bitacora,buffer->size);
	enviar_buffer(buffer,socket_tripulante);
	free(buffer->stream);
	free(buffer);
	free(bitacora);
}

char* recuperarBitacora(uint32_t id_tripulante){
	struct stat statCarpeta;
	char* direccionBitacora = string_from_format("%s/Files/Bitacoras/Tripulante%d.ims",PUNTO_MONTAJE,id_tripulante);
	if(stat(direccionBitacora,&statCarpeta)==-1)
	{
		log_info(loggerMongo,"NO EXISTE LA BITACORA");
		return "NO EXISTE LA BITACORA";
	}
	t_config* configuracionTripulante = config_create(direccionBitacora);
	free(direccionBitacora);
	char** bloquesUtilizados = config_get_array_value(configuracionTripulante,"BLOCKS");
	int tamanioBitacora = config_get_int_value(configuracionTripulante,"SIZE");
	config_destroy(configuracionTripulante);
	if(bloquesUtilizados[0]==NULL)
	{
		log_info(loggerMongo,"LA BITACORA ESTA VACIA");
		return "BITACORA VACIA";
	}
	char* bitacora;
	char* bloqueRecuperado = calloc(tamanioBlock,1);
	int contador = 0;
	memcpy(bloqueRecuperado,blocksMap+atoi(bloquesUtilizados[contador])*tamanioBlock,tamanioBlock);
	bitacora = string_from_format("%s",bloqueRecuperado);
	contador++;
	tamanioBitacora-=tamanioBlock;
	while(bloquesUtilizados[contador]!=NULL)
	{
		memcpy(bloqueRecuperado,blocksMap+atoi(bloquesUtilizados[contador])*tamanioBlock,tamanioBlock);
		string_append_with_format(&bitacora, "%s",bloqueRecuperado);
		//bitacora = string_from_format("%s%s",bitacora,bloqueRecuperado);
		contador++;
	}
	for(int i = 0; i<string_length(bitacora);i++)
	{
		if(bitacora[i]==';')
		{
			bitacora[i]='\n';
		}
	}
	log_info(loggerMongo,"%s",bitacora);
	free(bloqueRecuperado);
	int i = 0;
	while(bloquesUtilizados[i]!=NULL)
	{
		free(bloquesUtilizados[i]);
		i++;
	}
	free(bloquesUtilizados);

	return bitacora;
}

int byteExcedente(int cantidadDeBits, int tamanio){
	return (cantidadDeBits%tamanio)>0;
}

void inicializarCarpetas()
{
	char* directorioFiles = string_from_format("%s/Files",PUNTO_MONTAJE);
	char* directorioBitacoras = string_from_format("%s/Files/Bitacoras",PUNTO_MONTAJE);
	struct stat statCarpeta;
	if (stat(PUNTO_MONTAJE,&statCarpeta)==-1){
		mkdir(PUNTO_MONTAJE,0700);
		log_info(loggerMongo,"Creada carpeta %s",PUNTO_MONTAJE);
	}
	else
	{
		log_info(loggerMongo,"Carpeta %s ya existe", PUNTO_MONTAJE);
	}
	if(stat(directorioFiles,&statCarpeta)==-1){
		mkdir(directorioFiles,0700);
		log_info(loggerMongo, "Creada carpeta %s/Files",PUNTO_MONTAJE);
	}
	else
	{
		log_info(loggerMongo, "La carpeta %s/Files ya existe",PUNTO_MONTAJE);
	}
	if(stat(directorioBitacoras,&statCarpeta)==-1){
		mkdir(directorioBitacoras,0700);
		log_info(loggerMongo, "Creada carpeta %s/Bitacoras",PUNTO_MONTAJE);
	}
	else
	{
		log_info(loggerMongo, "Carpeta %s/Bitacoras ya existe",PUNTO_MONTAJE);
	}
	free(directorioBitacoras);
	free(directorioFiles);
}

char** getSiguientePosicionSabotaje(){
	if(POSICIONES_SABOTAJE[sabotajesResueltos+1] != NULL){
		sabotajesResueltos++;
		return string_split(POSICIONES_SABOTAJE[sabotajesResueltos],"|");
	}
	else
		log_info(loggerMongo,"YA SE RESOLVIERON TODOS LOS SABOTAJES!");

	return posicionSabotajeActual;
}

void informarSabotaje(){
	int socket_cliente_discordiador = crearConexionCliente(IP_DISCORDIADOR,PUERTO_DISCORDIADOR);

	uint32_t posSabotajeX = atoi(posicionSabotajeActual[0]);
	uint32_t posSabotajeY = atoi(posicionSabotajeActual[1]);

	log_info(loggerMongo,"¡¡SE PRODUJO UN SABOTAJE EN %d|%d!!",posSabotajeX,posSabotajeY);

	send(socket_cliente_discordiador,&posSabotajeX,sizeof(uint32_t),0);
	send(socket_cliente_discordiador,&posSabotajeY,sizeof(uint32_t),0);

	close(socket_cliente_discordiador);
}

void ejecutarFSCK(){
	tipo_mensaje sabotaje= casoSabotajeActual();
	switch(sabotaje)
	{
	case SABOTAJE_EN_SUPERBLOQUE_CANTIDAD:
		resolverSabotajeCantidadBlocks();
		break;
	case SABOTAJE_EN_SUPERBLOQUE_BITMAP:
		resolverSabotajeBitMap();
		break;
	case SABOTAJE_EN_FILE_SIZE:
		resolverSabotajeSizeFile();
		break;
	case SABOTAJE_EN_FILE_BLOCK_COUNT:
		resolverSabotajeBlockCount();
		break;
	case SABOTAJE_EN_FILE_BLOCKS:
		resolverSabotajeMD5();
		break;
	case NO_HAY_SABOTAJES:
		log_info(loggerMongo,"NO HAY SABOTAJES");
		break;
	default:
		break;
	}
}

void verificarSuperBloque()
{
	t_config* configuracionSuperBloque = config_create(string_from_format("%s/SuperBloque.ims",PUNTO_MONTAJE));
	int cantidadDeSupuestosBloques = config_get_int_value(configuracionSuperBloque,"BLOCKS");
	if(cantidadDeSupuestosBloques == string_length(blocksMap)/tamanioBlock)
	{
		log_info(loggerMongo,"SUPERBLOQUE NO AFECTADO POR SABOTAJE");
	}
}
void escribirBitacora(char* string, t_config* configuracionTripulante){
	int tamanioString = string_length(string);
	char** listaBloques = config_get_array_value(configuracionTripulante,"BLOCKS");

	if(listaBloques[0]==NULL){
		//si no tiene ningun bloque entonces:
		int cantidadDeBytes = string_length(string);
		if(cantidadDeBytes <= tamanioBlock){
			//semaforo para modificar bitmap y blocks
			int posicion = ocuparBitVacio();
			escribirEnBlocks(posicion, string);//lo mismo que la linea de abajo pero con mutex para blocks
			//memcpy(blocksMap+(posicion*tamanioBlock),string,cantidadDeBytes);
			//forzarSincronizacionBlocks();
			//fin de semaforo
			config_set_value(configuracionTripulante,"BLOCKS",string_from_format("[%d]",posicion));
			tamanioString += config_get_int_value(configuracionTripulante,"SIZE");
			config_set_value(configuracionTripulante,"SIZE",string_from_format("%d",tamanioString));
		}
		else
		{
			int cantidadDeBloquesASolicitar = cantidadDeBytes/tamanioBlock + byteExcedente(cantidadDeBytes, tamanioBlock);
			char* bloquesSolicitados = string_new();
			//pthread_mutex_lock(&mutexBitMap);
			//t_bitarray* bitMap = recuperarBitArray();

			if(cantidadDeBloquesASolicitar > 2){
				int cantidad = 0;
				int posicionBloque = ocuparBitVacio();
				//int posicionBloque = posicionBlockLibre(bitMap);
				bloquesSolicitados = string_from_format("%d",posicionBloque);
				//log_info(loggerMongo,"tamanio string: %d", string_length(string));
				//log_info(loggerMongo,"cantidad de bloques a solicitar: %d",cantidadDeBloquesASolicitar);
				//log_info(loggerMongo,"String partido pre FOR: %s posicion: %d", string_substring(string, cantidad*tamanioBlock, tamanioBlock),cantidad*tamanioBlock);
				char* stringPartido = string_substring(string,cantidad*tamanioBlock,tamanioBlock);
				escribirEnBlocks(posicionBloque, stringPartido);
				free(stringPartido);
				//memcpy(blocksMap+(posicionBloque*tamanioBlock),string_substring(string, cantidad*tamanioBlock, tamanioBlock),tamanioBlock);
				cantidad++;
				for(; cantidad<cantidadDeBloquesASolicitar;cantidad++)
				{
					posicionBloque = ocuparBitVacio();
					//bitarray_set_bit(bitMap,posicionBloque);
					stringPartido = string_substring(string,cantidad*tamanioBlock,tamanioBlock);
					escribirEnBlocks(posicionBloque, stringPartido);
					//memcpy(blocksMap+(posicionBloque*tamanioBlock),string_substring(string, cantidad*tamanioBlock, tamanioBlock),string_length(string_substring(string, cantidad*tamanioBlock, tamanioBlock)));
					string_append_with_format(&bloquesSolicitados, ",%d",posicionBloque);
					//bloquesSolicitados = string_from_format("%s,%d",bloquesSolicitados,posicionBloque);
					free(stringPartido);

				}
				//log_info(loggerMongo,"cantidad: %d",cantidad);
				//log_info(loggerMongo,"String partido post FOR: %s posicion: %d", string_substring(string, cantidad, tamanioBlock),cantidad*tamanioBlock);
				//forzarSincronizacionBlocks();
				//pthread_mutex_unlock(&mutexBitMap);//semaforo cierre
				//log_info(loggerMongo,"Bloques pedidos: %s",bloquesSolicitados);
				char* bloquesConf = string_from_format("[%s]",bloquesSolicitados);
				config_set_value(configuracionTripulante,"BLOCKS",bloquesConf);
				tamanioString += config_get_int_value(configuracionTripulante,"SIZE");
				char* sizeConf = string_from_format("%d",tamanioString);
				config_set_value(configuracionTripulante,"SIZE",sizeConf);
				//free(stringPartido);
				free(bloquesSolicitados);
				free(sizeConf);
				free(bloquesConf);
			}
			else
			{
				char* stringPartido = string_substring(string, 0, tamanioBlock);
				int posicionBloque = ocuparBitVacio();
				bloquesSolicitados = string_from_format("%d",posicionBloque);
				escribirEnBlocks(posicionBloque,stringPartido);
				stringPartido = string_substring(string, tamanioBlock, tamanioBlock);
				posicionBloque = ocuparBitVacio();
				bloquesSolicitados = string_from_format("%d",posicionBloque);
				escribirEnBlocks(posicionBloque, stringPartido);
				config_set_value(configuracionTripulante,"BLOCKS",string_from_format("[%s]",bloquesSolicitados));
				tamanioString += config_get_int_value(configuracionTripulante,"SIZE");
				config_set_value(configuracionTripulante,"SIZE",string_from_format("%d",tamanioString));
				free(stringPartido);
				free(bloquesSolicitados);
			}
		}
	}
	else
	{
		char** bloquesUtilizados = config_get_array_value(configuracionTripulante,"BLOCKS");
		int bloquePosicion=0;
		int contador = 0;
		char* bloques;
		while(bloquesUtilizados[contador]!=NULL)
		{
			if(contador==0)
			{
				bloques = string_from_format("%s",bloquesUtilizados[0]);
			}
			else
			{
				string_append_with_format(&bloques, ",%s",bloquesUtilizados[contador]); //VER DESPUES
				//bloques = string_from_format("%s,%s",bloques,bloquesUtilizados[contador]);
			}
			bloquePosicion = atoi(bloquesUtilizados[contador]);
			contador++;
		}
		liberarArray(bloquesUtilizados);
		//log_info(loggerMongo,"Bloques utilizados: %s",bloques);
		//log_info(loggerMongo,"Cantidad de Bloques utilizados en total: %d", contador);
		//log_info(loggerMongo,"ULTIMO BLOQUE UTILIZADO: %d", bloquePosicion);
		//char* bloqueRecuperado = calloc(tamanioBlock,1);
		//memcpy(bloqueRecuperado,blocksMap+(bloquePosicion*tamanioBlock),tamanioBlock);
		int tamanioBitacora = config_get_int_value(configuracionTripulante,"SIZE");
		//log_info(loggerMongo,"Tamanio string: %d",string_length(bloqueRecuperado));
		//log_info(loggerMongo,"Tamanio en bytes del recuperado: %d",tamanioBitacora%tamanioBlock);
		//log_info(loggerMongo,"Contenido del ultimo Bloque utilizado: %s",string_substring(bloqueRecuperado,0,tamanioBitacora%tamanioBlock));

		if(tamanioString<=tamanioBlock)
		{
			if(tamanioBlock - tamanioBitacora%tamanioBlock >= tamanioString && tamanioBitacora%tamanioBlock != 0)
			{
				rellenarEnBlocks(bloquePosicion, string, tamanioBitacora);
				//forzarSincronizacionBlocks();//forzar la sincronizacion
				tamanioBitacora += tamanioString;
				config_set_value(configuracionTripulante,"SIZE",string_from_format("%d",tamanioBitacora));
			}else
			{
				int posicionString = 0;
				int nuevoBloque = ocuparBitVacio();
				int tamanioBitacoraBlock =tamanioBitacora;
				bloques = string_from_format("%s,%d",bloques,nuevoBloque);
				if(tamanioBitacora%tamanioBlock == 0)
				{
					escribirEnBlocks(nuevoBloque, string);
					tamanioBitacora += string_length(string);
				}
				else
				{
					char* stringPartido = string_substring(string, posicionString, tamanioBlock-tamanioBitacora%tamanioBlock);
					rellenarEnBlocks(bloquePosicion, stringPartido, tamanioBitacora);
					tamanioBitacora+=string_length(stringPartido);
					posicionString += tamanioBlock-tamanioBitacoraBlock%tamanioBlock;
					stringPartido = string_substring(string, posicionString, string_length(string)-posicionString);
					escribirEnBlocks(nuevoBloque, stringPartido);
					tamanioBitacora+=string_length(stringPartido);
					free(stringPartido);
				}
				//forzarSincronizacionBlocks();
				char* nuevosBloquesBitacora = string_from_format("[%s]",bloques);
				char* nuevoTamanioBitacora = string_from_format("%d",tamanioBitacora);
				config_set_value(configuracionTripulante,"BLOCKS",nuevosBloquesBitacora);
				config_set_value(configuracionTripulante,"SIZE",nuevoTamanioBitacora);
				free(nuevosBloquesBitacora);
				free(nuevoTamanioBitacora);
			}
		}
		else
		{
			int tamanioTotal = string_length(string);
			int cantidadDeBloquesASolicitar;// = tamanioTotal/tamanioBlock + byteExcedente(tamanioTotal, tamanioBlock);
//			while(bloquesUtilizados[contador]!=NULL)
//			{
//				if(contador==0)
//				{
//					bloques = string_from_format("%s",bloquesUtilizados[0]);
//				}
//				else
//				{
//					bloques = string_from_format("%s,%s",bloques,bloquesUtilizados[contador]);
//				}
//				bloquePosicion = atoi(bloquesUtilizados[contador]);
//				contador++;
//			}
			//char* bloqueRecuperado = calloc(tamanioBlock,1);
			//memcpy(bloqueRecuperado,blocksMap+(bloquePosicion*tamanioBlock),tamanioBlock);
			//int tamanioBitacora = config_get_int_value(configuracionTripulante,"SIZE");
			int posicionString = 0;
			char* stringPartido;
			if(tamanioBitacora%tamanioBlock!=0)
			{//Si hay q rellenar un bloque previamente utilizado
				stringPartido = string_substring(string,posicionString,tamanioBlock-tamanioBitacora%tamanioBlock);
				rellenarEnBlocks(bloquePosicion, stringPartido, tamanioBitacora);
				posicionString += string_length(stringPartido);
				int tamanioStringRestante = string_length(string) - string_length(stringPartido);
				cantidadDeBloquesASolicitar = tamanioStringRestante/tamanioBlock + byteExcedente(tamanioStringRestante, tamanioBlock);
				int nuevoBloque;
				free(stringPartido);
				for(int i = 0; i<cantidadDeBloquesASolicitar;i++)
				{
					nuevoBloque = ocuparBitVacio();
					stringPartido = string_substring(string, posicionString, tamanioBlock);
					posicionString += string_length(stringPartido);
					escribirEnBlocks(nuevoBloque, stringPartido);
					string_append_with_format(&bloques, ",%d",nuevoBloque);
					free(stringPartido);
					//bloques = string_from_format("%s,%d",bloques,nuevoBloque);
				}
				tamanioBitacora += string_length(string);
				char* bloquesConf = string_from_format("[%s]",bloques);
				char* sizeConf = string_from_format("%d",tamanioBitacora);
				config_set_value(configuracionTripulante,"BLOCKS",bloquesConf);
				config_set_value(configuracionTripulante,"SIZE",sizeConf);
				free(bloquesConf);
				free(sizeConf);
			}else
			{//si no hay q rellenar un bloque previamente utilizado
				cantidadDeBloquesASolicitar = tamanioTotal/tamanioBlock + byteExcedente(tamanioTotal, tamanioBlock);
				int nuevoBloque;
				for(int i=0;i<cantidadDeBloquesASolicitar;i++)
				{
					nuevoBloque = ocuparBitVacio();
					stringPartido = string_substring(string, posicionString, tamanioBlock);
					posicionString += string_length(stringPartido);
					escribirEnBlocks(nuevoBloque, stringPartido);
					string_append_with_format(&bloques, ",%d",nuevoBloque);
					free(stringPartido);
					//bloques = string_from_format("%s,%d",bloques,nuevoBloque);
				}
				tamanioBitacora += string_length(string);
				char* bloquesConf = string_from_format("[%s]",bloques);
				char* sizeConf = string_from_format("%d",tamanioBitacora);
				config_set_value(configuracionTripulante,"BLOCKS",bloquesConf);
				config_set_value(configuracionTripulante,"SIZE",sizeConf);
				free(bloquesConf);
				free(sizeConf);
			}
			//free(stringPartido);
		}
		free(bloques);
	}
	liberarArray(listaBloques);
}

int ocuparBitVacio()
{
	pthread_mutex_lock(&mutexBitMap);
	t_bitarray* bitMap = recuperarBitArray();
	int posicion = posicionBlockLibre(bitMap);
	bitarray_set_bit(bitMap,posicion);
	guardarBitArray(bitMap);
	free(bitMap->bitarray);
	bitarray_destroy(bitMap);
	pthread_mutex_unlock(&mutexBitMap);
	return posicion;
}

void escribirEnBlocks(int posicion, char* string)
{
	pthread_mutex_lock(&mutexBlocks);
	int tamanioString = string_length(string);
	memcpy(blocksMap+(posicion*tamanioBlock),string,tamanioString);
	//forzarSincronizacionBlocks();
	pthread_mutex_unlock(&mutexBlocks);
}

void rellenarEnBlocks(int posicion, char* string, int tamanioBitacora)
{
	pthread_mutex_lock(&mutexBlocks);
	int tamanioString = string_length(string);
	memcpy(blocksMap+(posicion*tamanioBlock)+tamanioBitacora%tamanioBlock,string,tamanioString);
	pthread_mutex_unlock(&mutexBlocks);
}

void escribirFile(char* recurso, int cantidad)
{
	struct stat statCarpeta;
	char* direccionArchivo = string_from_format("%s/Files/%s.ims",PUNTO_MONTAJE,recurso);
	//mutexFile lock
	pthread_mutex_lock(&mutexFile);
	if(stat(direccionArchivo,&statCarpeta)==-1)
	{
		FILE* archivoBitacora = fopen(string_from_format("%s/Files/%s.ims",PUNTO_MONTAJE,recurso),"w");
		char* textoAEscribir = string_from_format("SIZE=0\nBLOCK_COUNT=0\nBLOCKS=[]\nCARACTER_LLENADO=%c\nMD5_ARCHIVO=",recurso[0]);
		txt_write_in_file(archivoBitacora, textoAEscribir);
		fclose(archivoBitacora);
		log_info(loggerMongo, "File %s.ims creado",recurso);
		free(textoAEscribir);

	}
	t_config* configFile = config_create(direccionArchivo);
	char* fileCompleto = string_new();
	char* fileMD5;
	int tamanioFile = config_get_int_value(configFile,"SIZE");
	int cantidadDeBloques = config_get_int_value(configFile,"BLOCK_COUNT");
	char** bloquesUtilizados = config_get_array_value(configFile,"BLOCKS");
	char* stringRecurso = string_repeat(recurso[0], cantidad);
	char* recursosParaMD5 = string_repeat(' ',((tamanioFile+cantidad)/tamanioBlock + byteExcedente(tamanioFile+cantidad, tamanioBlock))*tamanioBlock);
	log_info(loggerMongo,"Tamanio stringRecurso: %d",string_length(stringRecurso));
	if(cantidadDeBloques==0)
	{//si no tiene ningun bloque
		if(cantidad<=tamanioBlock)
		{//si la cantidad es menor que un bloque de tamanio
			int posicion = ocuparBitVacio();
			//string_append(&fileCompleto, stringRecurso);
			char* posicionBit = string_from_format("[%d]",posicion);
			char* tamanioRecurso = string_itoa(cantidad);
			escribirEnBlocks(posicion, stringRecurso);
			memcpy(recursosParaMD5,stringRecurso,string_length(stringRecurso));
			fileMD5 = obtenerMD5(recursosParaMD5);
			//forzarSincronizacionBlocks();

			config_set_value(configFile,"MD5_ARCHIVO",fileMD5);
			config_set_value(configFile, "SIZE",tamanioRecurso);
			config_set_value(configFile,"BLOCK_COUNT","1");
			config_set_value(configFile,"BLOCKS",posicionBit);

//			config_save(configFile);
//			config_destroy(configFile);
			free(posicionBit);
			free(tamanioRecurso);
			//hashear en md5 y configsetvalue MD5
		}else
		{//si la cantidad es mayor a un bloque
			int cantidadDeBloquesASolicitar = cantidad/tamanioBlock + byteExcedente(cantidad, tamanioBlock);
			//log_info(loggerMongo,"Cantidad de bloques a solicitar %d",cantidadDeBloquesASolicitar);
			int contador = 0;
			int posicionBloque = ocuparBitVacio();
			char* bloquesSolicitados = string_from_format("%d",posicionBloque);
			char* stringPartido = string_substring(stringRecurso, contador*tamanioBlock, tamanioBlock);
			escribirEnBlocks(posicionBloque, stringPartido);
			contador++;
			free(stringPartido);
			cantidadDeBloques++;
			for(;contador<cantidadDeBloquesASolicitar;contador++)
			{
				posicionBloque = ocuparBitVacio();
				stringPartido = string_substring(stringRecurso, contador*tamanioBlock, tamanioBlock);
				//log_info(loggerMongo, "string_partido %s",stringPartido);
				escribirEnBlocks(posicionBloque, stringPartido);
				string_append_with_format(&bloquesSolicitados, ",%d",posicionBloque);
				free(stringPartido);
				cantidadDeBloques++;
			}
			memcpy(recursosParaMD5,stringRecurso,string_length(stringRecurso));
			fileMD5 = obtenerMD5(recursosParaMD5);
			//forzarSincronizacionBlocks();
			char* bloquesConf = string_from_format("[%s]",bloquesSolicitados);
			config_set_value(configFile,"BLOCKS",bloquesConf);
			char* tamanioRecurso = string_itoa(cantidad);
			char* cantidadBloquesConf = string_itoa(cantidadDeBloques);
			config_set_value(configFile,"MD5_ARCHIVO",fileMD5);
			config_set_value(configFile,"SIZE",tamanioRecurso);
			config_set_value(configFile,"BLOCK_COUNT",cantidadBloquesConf);
			free(bloquesSolicitados);
			free(bloquesConf);
			free(tamanioRecurso);
			free(cantidadBloquesConf);
		}
	}else
	{//si ya uso bloques, buscar y completar
		//char** bloquesUtilizados = config_get_array_value(configFile,"BLOCKS");
		int bloquePosicion = 0;
		int contador = 0;
		char* bloqueRecuperadoFile;
		//char* recursosParaMD5 = string_repeat(' ', ((tamanioFile+cantidad)/tamanioBlock + byteExcedente(tamanioFile+cantidad, tamanioBlock))*tamanioBlock);
		char* recursosEnFS = string_repeat(recurso[0], tamanioFile+cantidad);
		char* bloques;
		while(bloquesUtilizados[contador] != NULL)
		{
			if(contador == 0)
			{
				bloques = string_from_format("%s",bloquesUtilizados[contador]);
				bloqueRecuperadoFile = bloqueRecuperado(atoi(bloquesUtilizados[contador]));
				string_append(&fileCompleto, bloqueRecuperadoFile);
				free(bloqueRecuperadoFile);
			}else
			{
				string_append_with_format(&bloques, ",%s",bloquesUtilizados[contador]);
				bloqueRecuperadoFile = bloqueRecuperado(atoi(bloquesUtilizados[contador]));
				string_append(&fileCompleto, bloqueRecuperadoFile);
				free(bloqueRecuperadoFile);
			}
			bloquePosicion = atoi(bloquesUtilizados[contador]);
			contador++;
		}
		//string_append(&recursosEnFS, stringRecurso);
		memcpy(recursosParaMD5,recursosEnFS,string_length(recursosEnFS));
		log_info(loggerMongo,"Recursos para md5: %s",recursosParaMD5);
		fileMD5 = obtenerMD5(recursosParaMD5);
		liberarArray(bloquesUtilizados);
		if(cantidad<=tamanioBlock)
		{
			if(tamanioBlock - tamanioFile%tamanioBlock >= cantidad && tamanioFile%tamanioBlock != 0)
			{
				rellenarEnBlocks(bloquePosicion, stringRecurso, tamanioFile);
				//forzarSincronizacionBlocks();
				tamanioFile += cantidad;
				char* sizeConf = string_itoa(tamanioFile);
				config_set_value(configFile,"MD5_ARCHIVO",fileMD5);
				config_set_value(configFile,"SIZE",sizeConf);
				free(sizeConf);
			}else
			{
				int posicionString = 0;
				int nuevoBloque = ocuparBitVacio();
				string_append_with_format(&bloques, ",%d",nuevoBloque);
				if(tamanioFile%tamanioBlock == 0)
				{
					escribirEnBlocks(nuevoBloque, stringRecurso);
					tamanioFile += cantidad;
					cantidadDeBloques++;
				}else
				{
					char* stringPartido = string_substring(stringRecurso, posicionString, tamanioBlock-tamanioFile%tamanioBlock);
					rellenarEnBlocks(bloquePosicion,stringPartido,tamanioFile);
					posicionString += tamanioBlock - tamanioFile%tamanioBlock;
					tamanioFile += string_length(stringPartido);
					free(stringPartido);
					stringPartido = string_substring(stringRecurso, posicionString, string_length(stringRecurso)-posicionString);
					escribirEnBlocks(nuevoBloque, stringPartido);
					tamanioFile+=string_length(stringPartido);
					cantidadDeBloques++;
					free(stringPartido);
				}
				//forzarSincronizacionBlocks();
				char* blocksConf = string_from_format("[%s]",bloques);
				char* sizeConf = string_itoa(tamanioFile);
				char* cantidadConf = string_itoa(cantidadDeBloques);
				config_set_value(configFile,"MD5_ARCHIVO",fileMD5);
				config_set_value(configFile,"BLOCKS",blocksConf);
				config_set_value(configFile,"SIZE",sizeConf);
				config_set_value(configFile,"BLOCK_COUNT",cantidadConf);
				free(blocksConf);
				free(sizeConf);
				free(cantidadConf);
			}
		}else
		{
			//int tamanioTolta = string_length(stringRecurso);
			int cantidadDeBloquesASolicitar;
			int posicionString = 0;
			char* stringPartido;
			log_info(loggerMongo,"Resto: %d",tamanioFile%tamanioBlock);
			//sleep(5);
			if(tamanioFile%tamanioBlock!=0)
			{
				stringPartido = string_substring(stringRecurso, posicionString, tamanioBlock - tamanioFile%tamanioBlock);
				rellenarEnBlocks(bloquePosicion, stringPartido, tamanioFile);
				int tamanioStringRestante = cantidad - string_length(stringPartido);
				log_info(loggerMongo,"tamanio stringPartido en rellenar: %d",string_length(stringPartido));
				posicionString += string_length(stringPartido);
				cantidadDeBloquesASolicitar = tamanioStringRestante/tamanioBlock + byteExcedente(tamanioStringRestante, tamanioBlock);
				int nuevoBloque;
				free(stringPartido);
				for(int i = 0; i<cantidadDeBloquesASolicitar;i++)
				{
					nuevoBloque = ocuparBitVacio();
					stringPartido = string_substring(stringRecurso, posicionString, tamanioBlock);
					posicionString += string_length(stringPartido);
					escribirEnBlocks(nuevoBloque, stringPartido);
					string_append_with_format(&bloques, ",%d",nuevoBloque);
					cantidadDeBloques++;
					free(stringPartido);
				}
				tamanioFile += cantidad;
				char* blocksConf = string_from_format("[%s]",bloques);
				char* sizeConf = string_itoa(tamanioFile);
				char* cantidadConf = string_itoa(cantidadDeBloques);
				config_set_value(configFile,"MD5_ARCHIVO",fileMD5);
				config_set_value(configFile,"BLOCKS",blocksConf);
				config_set_value(configFile,"SIZE",sizeConf);
				config_set_value(configFile,"BLOCK_COUNT",cantidadConf);
				free(blocksConf);
				free(sizeConf);
				free(cantidadConf);
			}else
			{
				cantidadDeBloquesASolicitar = cantidad/tamanioBlock + byteExcedente(cantidad, tamanioBlock);
				int nuevoBloque;
				for(int i = 0; i<cantidadDeBloquesASolicitar;i++)
				{
					nuevoBloque = ocuparBitVacio();
					stringPartido = string_substring(stringRecurso, posicionString, tamanioBlock);
					posicionString += string_length(stringPartido);
					escribirEnBlocks(nuevoBloque, stringPartido);
					string_append_with_format(&bloques, ",%d",nuevoBloque);
					cantidadDeBloques++;
					free(stringPartido);
				}
				tamanioFile += cantidad;
				char* blocksConf = string_from_format("[%s]",bloques);
				char* sizeConf = string_itoa(tamanioFile);
				char* cantidadConf = string_itoa(cantidadDeBloques);
				config_set_value(configFile,"MD5_ARCHIVO",fileMD5);
				config_set_value(configFile,"BLOCKS",blocksConf);
				config_set_value(configFile,"SIZE",sizeConf);
				config_set_value(configFile,"BLOCK_COUNT",cantidadConf);
				free(blocksConf);
				free(sizeConf);
				free(cantidadConf);
			}
		}
	}
	free(recursosParaMD5);
	free(stringRecurso);
	config_save(configFile);
	config_destroy(configFile);
	pthread_mutex_unlock(&mutexFile);
}

void eliminarCaracterFile(char* recurso, int cantidad) //se trabaja suponiendo que existe el archivo
{
	//struct stat statCarpeta;
	char* direccionArchivo = string_from_format("%s/Files/%s.ims",PUNTO_MONTAJE,recurso);

	pthread_mutex_lock(&mutexFile);
	t_config* configFile = config_create(direccionArchivo);
	char* fileMD5;
	int tamanioFile = config_get_int_value(configFile,"SIZE");
	int cantidadDeBloques = config_get_int_value(configFile,"BLOCK_COUNT");
	char** bloquesUtilizados = config_get_array_value(configFile,"BLOCKS");
	int nuevoSize = tamanioFile - cantidad;
	//mutexFile lock
	if(cantidad >= tamanioFile)
	{
		int contador = 0;
		while(bloquesUtilizados[contador] != NULL)
		{
			liberarBloque(atoi(bloquesUtilizados[contador]));
			log_info(loggerMongo,"Bloque liberado: %s", bloquesUtilizados[contador]);
			contador++;
		}
		fileMD5 = obtenerMD5("");
		config_set_value(configFile,"SIZE","0");
		config_set_value(configFile,"BLOCK_COUNT","0");
		config_set_value(configFile,"BLOCKS","[]");
		config_set_value(configFile,"MD5_ARCHIVO",fileMD5);
		if(cantidad==tamanioFile)
		{
			log_info(loggerMongo,"Se quisieron eliminar mas caracteres de los existentes de %s",recurso);
		}

	}else
	{
		int cantidadDeBloquesNecesarios = nuevoSize/tamanioBlock + byteExcedente(nuevoSize, tamanioBlock);
		while(cantidadDeBloques != cantidadDeBloquesNecesarios)
		{
			liberarBloque(atoi(bloquesUtilizados[cantidadDeBloques-1]));
			log_info(loggerMongo,"Bloque liberado: %s", bloquesUtilizados[cantidadDeBloques-1]);
			cantidadDeBloques--;
		}
		if(nuevoSize%tamanioBlock != 0)
		{
			char* nuevoStringBloque = string_repeat(' ', tamanioBlock);
			char* nuevoStringRecurso = string_repeat(recurso[0],nuevoSize%tamanioBlock);
			log_info(loggerMongo,"Tamanio string recurso:%d string recurso: %s",string_length(nuevoStringRecurso),nuevoStringRecurso);
			memcpy(nuevoStringBloque,nuevoStringRecurso,string_length(nuevoStringRecurso));
			escribirEnBlocks(atoi(bloquesUtilizados[cantidadDeBloques-1]), nuevoStringBloque);
			free(nuevoStringBloque);
			free(nuevoStringRecurso);
		}
		char* nuevoArray = bloquesUtilizados[0];
		for(int i = 1 ; i<cantidadDeBloquesNecesarios;i++)
		{
			string_append_with_format(&nuevoArray, ",%s",bloquesUtilizados[i]);
		}
		char* recursoParaMD5 = string_repeat(' ',cantidadDeBloquesNecesarios*tamanioBlock);
		char* recursoTotal = string_repeat(recurso[0],nuevoSize);
		memcpy(recursoParaMD5,recursoTotal,string_length(recursoTotal));
		char* bloquesConf = string_from_format("[%s]",nuevoArray);
		char* sizeConf = string_itoa(nuevoSize);
		char* contadorConf = string_itoa(cantidadDeBloquesNecesarios);
		fileMD5 = obtenerMD5(recursoParaMD5);
		config_set_value(configFile,"SIZE",sizeConf);
		config_set_value(configFile,"BLOCK_COUNT",contadorConf);
		config_set_value(configFile,"BLOCKS",bloquesConf);
		config_set_value(configFile,"MD5_ARCHIVO",fileMD5);
		free(nuevoArray);
		free(recursoTotal);
		free(recursoParaMD5);
		free(bloquesConf);
		free(sizeConf);
		free(contadorConf);
	}
	config_save(configFile);
	pthread_mutex_unlock(&mutexFile);
	config_destroy(configFile);
	free(fileMD5);
	//liberarArray(bloquesUtilizados);
}

bool existeArchivoRecurso(char* recurso)
{
	struct stat statCarpeta;
	char* direccionArchivo = string_from_format("%s/Files/%s.ims",PUNTO_MONTAJE,recurso);
	return !(stat(direccionArchivo,&statCarpeta)==-1);
}

void liberarBloque(int bloqueALiberar)
{
	char* stringBloqueVacio = string_repeat(' ', tamanioBlock);
	escribirEnBlocks(bloqueALiberar, stringBloqueVacio);
	liberarBit(bloqueALiberar);
	free(stringBloqueVacio);
}

void liberarBit(int bit)
{
	pthread_mutex_lock(&mutexBitMap);
	t_bitarray* bitMap = recuperarBitArray();
	//int posicion = posicionBlockLibre(bitMap);
	bitarray_clean_bit(bitMap,bit);
	guardarBitArray(bitMap);
	free(bitMap->bitarray);
	bitarray_destroy(bitMap);
	pthread_mutex_unlock(&mutexBitMap);
}

char* obtenerMD5(char* bloquesRecuperados)
{
	pthread_mutex_lock(&mutexMD5);
	char* directorioAuxiliar = string_from_format("%s/Files/AuxiliarFile.txt",PUNTO_MONTAJE);
	char* directorioMD5 = string_from_format("%s/Files/AuxiliarMD5.txt",PUNTO_MONTAJE);
	FILE* archivoAuxiliar = fopen(directorioAuxiliar,"w");

	txt_write_in_file(archivoAuxiliar, bloquesRecuperados);
	txt_close_file(archivoAuxiliar);
	char* comando = string_from_format("md5sum %s > %s",directorioAuxiliar,directorioMD5);
	system(comando);
	int archivoMD5 = open(directorioMD5, O_RDWR, S_IRUSR | S_IWUSR);
	int contadorPosicion = 0;
	struct stat caracteristicasMD5;
	if(fstat(archivoMD5,&caracteristicasMD5)== -1)
	{
		log_info(loggerMongo, "No se pudo obtener stat de MD5");
	}
	//blocksMapOriginal = mmap(NULL, cantidadDeBlocks * tamanioBlock,PROT_READ | PROT_WRITE, MAP_SHARED,fdArchivoBlocks,0);

	char* mapeadoMD5 = mmap(NULL, caracteristicasMD5.st_size,PROT_READ | PROT_WRITE, MAP_SHARED,archivoMD5,0);
	while(mapeadoMD5[contadorPosicion]!=' ')
	{
		contadorPosicion++;
	}
	char* salidaMD5 = calloc(contadorPosicion,1);
	memcpy(salidaMD5,mapeadoMD5,contadorPosicion);
	munmap(mapeadoMD5,caracteristicasMD5.st_size);
	free(directorioAuxiliar);
	free(directorioMD5);
	free(comando);
	close(archivoMD5);
	pthread_mutex_unlock(&mutexMD5);
	return salidaMD5;
}

char* bloqueRecuperado(int posicionBloque)
{
	char* recuperado = calloc(tamanioBlock,1);
	memcpy(recuperado,blocksMap+posicionBloque*tamanioBlock,tamanioBlock);
	return recuperado;
}

void eliminarArchivoYLiberar(char* recurso)
{
	char* direccion = string_from_format("%s/Files/%s.ims",PUNTO_MONTAJE,recurso);
	t_config* configRecurso = config_create(direccion);
	char** bloquesUsados = config_get_array_value(configRecurso,"BLOCKS");
	int contador = 0;
	while(bloquesUsados[contador] != NULL)
	{
		liberarBloque(atoi(bloquesUsados[contador]));
		log_info(loggerMongo,"Bloque liberado: %s", bloquesUsados[contador]);
		contador++;
	}
	remove(direccion);
	free(direccion);
	liberarArray(bloquesUsados);
}

void asincronia()
{
	while(1)
	{
		sleep(TIEMPO_SINCRONIZACION);
		forzarSincronizacionBlocks();
	}
}

void destruirConfig(){
	free(PUNTO_MONTAJE);
	free(IP_I_MONGO);
	free(PUERTO_I_MONGO);
	free(IP_DISCORDIADOR);
	free(PUERTO_DISCORDIADOR);
	liberarArray(POSICIONES_SABOTAJE);
//	config_destroy(configuracionMongo); --> A VECES TIRA SEGFAULT
}

void terminar_programa(){
	log_info(loggerMongo,"Finaliza I-MONGO...");
	munmap(blocksMap,tamanioBlock*cantidadDeBlocks);
	munmap(blocksMapOriginal,tamanioBlock*cantidadDeBlocks);
	dictionary_destroy(caracterAsociadoATarea);
	destruirConfig();
	liberarArray(posicionSabotajeActual);
	log_destroy(loggerMongo);
	exit(0);
}

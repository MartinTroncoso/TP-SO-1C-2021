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
	recuperarBitacora(1);
	log_info(loggerMongo,"PID DE I-MONGO-STORE: %d",getpid());

	int socket_escucha = iniciarServidor(IP_I_MONGO,PUERTO_I_MONGO);
	log_info(loggerMongo,"I-MONGO Listo para atender a los Tripulantes!");

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
	sabotajesResueltos = 0;

	pthread_mutex_init(&mutexBitMap, NULL);
	pthread_mutex_init(&mutexSincro,NULL);
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
	cantidadDeBlocks=1024;
	struct stat statCarpeta;
	if(stat(string_from_format("%s/SuperBloque.ims",PUNTO_MONTAJE),&statCarpeta)==-1)
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
		char* stringArchivo = string_from_format("BLOCK_SIZE=8\nBLOCKS=1024\nBITMAP=");
		int archivo = open(string_from_format("%s/SuperBloque.ims",PUNTO_MONTAJE),O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
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
		bitarray_destroy(bitArray);
	}else
	{
		log_info(loggerMongo,"El archvio SuperBloque.ims ya existe");
		t_bitarray* recuperado = recuperarBitArray();
		log_info(loggerMongo,"Posicion del blocks libre: %d", posicionBlockLibre(recuperado));
		guardarBitArray(recuperado);
	}

}

t_bitarray* recuperarBitArray(){
	int archivo = open(string_from_format("%s/SuperBloque.ims",PUNTO_MONTAJE), O_RDWR, S_IRUSR | S_IWUSR);
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
	t_bitarray* nuevoBitArray = bitarray_create_with_mode(malloc((cantidadDeBlocks/8)+(cantidadDeBlocks%8)), cantidadDeBlocks/8+cantidadDeBlocks%8, LSB_FIRST);
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

	for(int i=0;i<bitarray_get_max_bit(nuevoBitArray);i++)
	{
		//bitarray_set_bit(bitArray,i);
		printf("%d",bitarray_test_bit(nuevoBitArray,i));
	}

	return nuevoBitArray;
}

void guardarBitArray(t_bitarray* arrayAGuardar)
{
	//Buscar puntero siguiente al tercer '='
	//si uso config_set_value y config_save cambia de posicion los valores dentro del archivo
	int archivo = open(string_from_format("%s/SuperBloque.ims",PUNTO_MONTAJE), O_RDWR, S_IRUSR | S_IWUSR);
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
	t_config* configuracionBlocks = config_create(string_from_format("%s/SuperBloque.ims",PUNTO_MONTAJE));
	tamanioBlock = config_get_int_value(configuracionBlocks,"BLOCK_SIZE");
	cantidadDeBlocks = config_get_int_value(configuracionBlocks,"BLOCKS");
	config_destroy(configuracionBlocks);

	fdArchivoBlocks = open(string_from_format("%s/Blocks.ims",PUNTO_MONTAJE), O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
	struct stat infoBlocks;
	if(fstat(fdArchivoBlocks,&infoBlocks)== -1)
	{
		log_info(loggerMongo, "No se pudo obtener stat de Blocks.ims");
	}
	if(infoBlocks.st_size != tamanioBlock*cantidadDeBlocks)
	{
		if(ftruncate(fdArchivoBlocks,tamanioBlock*cantidadDeBlocks)==-1)
		{
			log_info(loggerMongo, "No se pudo crear archivo con el tamaño %dx%d",tamanioBlock,cantidadDeBlocks);
			write(fdArchivoBlocks,string_repeat(' ', tamanioBlock*cantidadDeBlocks),tamanioBlock*cantidadDeBlocks);
		}
		write(fdArchivoBlocks,string_repeat(' ', tamanioBlock*cantidadDeBlocks),tamanioBlock*cantidadDeBlocks);
		log_info(loggerMongo, "Se creo el archivo con el tamaño %dx%d",tamanioBlock,cantidadDeBlocks);
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
	if(stat(string_from_format("%s/Files/Bitacoras/Tripulante%d.ims",PUNTO_MONTAJE,idTripulante),&statCarpeta)==-1)
	{
		FILE* archivoBitacora = fopen(string_from_format("%s/Files/Bitacoras/Tripulante%d.ims",PUNTO_MONTAJE,idTripulante),"w");
		txt_write_in_file(archivoBitacora, "SIZE=0\nBLOCKS=[]");
		log_info(loggerMongo, "Archivo Tripulante%d.ims creado",idTripulante);
	}

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
			log_info(loggerMongo,"[TRIPULANTE %d] ATIENDE EL SABOTAJE",idTripulante);
			break;
		case RESOLUCION_SABOTAJE:
			log_info(loggerMongo,"[TRIPULANTE %d] RESOLVIÓ EL SABOTAJE!",idTripulante);
			posicionSabotajeActual = getSiguientePosicionSabotaje();
			break;
		case INVOCAR_FSCK:
			log_info(loggerMongo,"Se ejecuta el FSCK. Por ahora no hace nada :D");
			ejecutarFSCK();
			break;
		case OBTENER_BITACORA:
			recibirPeticionDeBitacora(socket_tripulante,idTripulante);
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
		printf("[TRIPULANTE %d] CARACTERES A GENERAR EN Oxigeno.ims: %d\n",id_tripulante,caracteresAGenerar);
		break;
	}
	case CONSUMIR_OXIGENO:{
		tipo_mensaje respuesta = EXISTE_EL_ARCHIVO; //HASTA TENER BIEN DEFINIDO LO DE LOS ARCHIVOS
		send(socket_tripulante,&respuesta,sizeof(tipo_mensaje),0);
		uint32_t caracteresABorrar;
		recv(socket_tripulante,&caracteresABorrar,sizeof(uint32_t),0);
		printf("[TRIPULANTE %d] CARACTERES A BORRAR DE Oxigeno.ims: %d\n",id_tripulante,caracteresABorrar);
		break;
	}
	case GENERAR_COMIDA:{
		uint32_t caracteresAGenerar;
		recv(socket_tripulante,&caracteresAGenerar,sizeof(uint32_t),0);
		printf("[TRIPULANTE %d] CARACTERES A GENERAR EN Comida.ims: %d\n",id_tripulante,caracteresAGenerar);
		break;
	}
	case CONSUMIR_COMIDA:{
		tipo_mensaje respuesta = EXISTE_EL_ARCHIVO; //HASTA TENER BIEN DEFINIDO LO DE LOS ARCHIVOS
		send(socket_tripulante,&respuesta,sizeof(tipo_mensaje),0);
		uint32_t caracteresABorrar;
		recv(socket_tripulante,&caracteresABorrar,sizeof(uint32_t),0);
		printf("[TRIPULANTE %d] CARACTERES A BORRAR DE Comidas.ims: %d\n",id_tripulante,caracteresABorrar);
		break;
	}
	case GENERAR_BASURA:{
		uint32_t caracteresAGenerar;
		recv(socket_tripulante,&caracteresAGenerar,sizeof(uint32_t),0);
		printf("[TRIPULANTE %d] CARACTERES A GENERAR EN Basura.ims: %d\n",id_tripulante,caracteresAGenerar);
		break;
	}
	case DESCARTAR_BASURA:{
		tipo_mensaje respuesta = EXISTE_EL_ARCHIVO; //HASTA TENER BIEN DEFINIDO LO DE LOS ARCHIVOS
		send(socket_tripulante,&respuesta,sizeof(tipo_mensaje),0);
		printf("[TRIPULANTE %d] Se borra Basura.ims\n",id_tripulante);
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

	//Hasta tener bien definido lo de los archivos solo lo logeo
	log_info(loggerMongo,"[TRIPULANTE %d] Se mueve de %d|%d a %d|%d",id_tripulante,coorXAnterior,coorYAnterior,coorXNueva,coorYNueva);

	t_config* configuracionTripulante = config_create(string_from_format("%s/Files/Bitacoras/Tripulante%d.ims",PUNTO_MONTAJE,id_tripulante));
	char* stringBitacora = string_from_format("Se mueve de %d|%d a %d|%d;",coorXAnterior,coorYAnterior,coorXNueva,coorYNueva);

	escribirBitacora(stringBitacora, configuracionTripulante);
	config_save(configuracionTripulante);
	config_destroy(configuracionTripulante);
//	FILE* bitacoraTripulante = txt_open_for_append(string_from_format("%s/Files/Bitacoras/Tripulante%d.ims",PUNTO_MONTAJE,id_tripulante));
//	txt_write_in_file(bitacoraTripulante, string_from_format("Se mueve de %d|%d a %d|%d\n",coorXAnterior,coorYAnterior,coorXNueva,coorYNueva));
//	txt_close_file(bitacoraTripulante);

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

	//Hasta tener bien definido lo de los archivos solo lo logeo
	log_info(loggerMongo,"[TRIPULANTE %d] Inicia la tarea %s",id_tripulante,tarea);
	t_config* configuracionTripulante = config_create(string_from_format("%s/Files/Bitacoras/Tripulante%d.ims",PUNTO_MONTAJE,id_tripulante));

	char* stringBitacora = string_from_format("Comienza ejecucion de tarea %s;",tarea);
	escribirBitacora(stringBitacora, configuracionTripulante);
	config_save(configuracionTripulante);
	config_destroy(configuracionTripulante);
//	FILE* bitacoraTripulante = txt_open_for_append(string_from_format("%s/Files/Bitacoras/Tripulante%d.ims",PUNTO_MONTAJE,id_tripulante));
//	txt_write_in_file(bitacoraTripulante, string_from_format("Comienza ejecucion de tarea %s\n",tarea));
//	txt_close_file(bitacoraTripulante);
	free(buffer);
}

void recibirFinalizaTarea(int socket_tripulante, uint32_t id_tripulante)
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

	//Hasta tener bien definido lo de los archivos solo lo logeo
	log_info(loggerMongo,"[TRIPULANTE %d] Finaliza la tarea %s",id_tripulante,tarea);

	t_config* configuracionTripulante = config_create(string_from_format("%s/Files/Bitacoras/Tripulante%d.ims",PUNTO_MONTAJE,id_tripulante));

	char* stringBitacora = string_from_format("Se finaliza la tarea %s;",tarea);
	escribirBitacora(stringBitacora, configuracionTripulante);
	config_save(configuracionTripulante);
	config_destroy(configuracionTripulante);
//	FILE* bitacoraTripulante = txt_open_for_append(string_from_format("%s/Files/Bitacoras/Tripulante%d.ims",PUNTO_MONTAJE,id_tripulante));
//	txt_write_in_file(bitacoraTripulante, string_from_format("Se finaliza la tarea %s\n",tarea));
//	txt_close_file(bitacoraTripulante);

	free(buffer);
}

void recibirPeticionDeBitacora(int socket_tripulante, uint32_t id_tripulante)
{
	void* buffer;
	uint32_t sizeBuffer;

	buffer = recibir_buffer(&sizeBuffer,socket_tripulante);
	memcpy(&id_tripulante,buffer,sizeof(uint32_t));
	log_info(loggerMongo,"[TRIPULANTE %d] SOLICITÓ SU BITÁCORA",id_tripulante);
	//enviar_respuesta(OK, socketCliente);
	free(buffer);
}

char* recuperarBitacora(uint32_t id_tripulante)
{

	struct stat statCarpeta;
	if(stat(string_from_format("%s/Files/Bitacoras/Tripulante%d.ims",PUNTO_MONTAJE,id_tripulante),&statCarpeta)==-1)
	{
		log_info(loggerMongo,"NO EXISTE LA BITACORA");
		return "NO EXISTE LA BITACORA";
	}
	t_config* configuracionTripulante = config_create(string_from_format("%s/Files/Bitacoras/Tripulante%d.ims",PUNTO_MONTAJE,id_tripulante));
	char** bloquesUtilizados = config_get_array_value(configuracionTripulante,"BLOCKS");
	int tamanioBitacora = config_get_int_value(configuracionTripulante,"SIZE");
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
		bitacora = string_from_format("%s%s",bitacora,bloqueRecuperado);
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
	return bitacora;
}

int byteExcedente(int cantidadDeBits, int tamanio){
	return (cantidadDeBits%tamanio)>0;
}

void inicializarCarpetas()
{
	struct stat statCarpeta;
	if (stat(PUNTO_MONTAJE,&statCarpeta)==-1){
		mkdir(PUNTO_MONTAJE,0700);
		log_info(loggerMongo,"Creada carpeta %s",PUNTO_MONTAJE);
	}
	else
	{
		log_info(loggerMongo,"Carpeta %s ya existe", PUNTO_MONTAJE);
	}
	if(stat(string_from_format("%s/Files",PUNTO_MONTAJE),&statCarpeta)==-1){
		mkdir(string_from_format("%s/Files",PUNTO_MONTAJE),0700);
		log_info(loggerMongo, "Creada carpeta %s/Files",PUNTO_MONTAJE);
	}
	else
	{
		log_info(loggerMongo, "La carpeta %s/Files ya existe",PUNTO_MONTAJE);
	}
	if(stat(string_from_format("%s/Files/Bitacoras",PUNTO_MONTAJE),&statCarpeta)==-1){
		mkdir(string_from_format("%s/Files/Bitacoras",PUNTO_MONTAJE),0700);
		log_info(loggerMongo, "Creada carpeta %s/Bitacoras",PUNTO_MONTAJE);
	}
	else
	{
		log_info(loggerMongo, "Carpeta %s/Bitacoras ya existe",PUNTO_MONTAJE);
	}
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
	//???
	//???
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
void escribirBitacora(char* string, t_config* configuracionTripulante)
{
	int tamanioString = string_length(string);
	if(config_get_array_value(configuracionTripulante,"BLOCKS")[0]==NULL)
		{
		//si no tiene ningun bloque entonces:
		int cantidadDeBytes = string_length(string);
		if(cantidadDeBytes <= tamanioBlock)
		{
			//semaforo para modificar bitmap y blocks
			int posicion = ocuparBitVacio();
			escribirEnBlocks(posicion, string);//lo mismo que la linea de abajo pero con mutex para blocks
			//memcpy(blocksMap+(posicion*tamanioBlock),string,cantidadDeBytes);
			forzarSincronizacionBlocks();
			//fin de semaforo
			config_set_value(configuracionTripulante,"BLOCKS",string_from_format("[%d]",posicion));
			tamanioString += config_get_int_value(configuracionTripulante,"SIZE");
			config_set_value(configuracionTripulante,"SIZE",string_from_format("%d",tamanioString));
		}else
		{
			int cantidadDeBloquesASolicitar = cantidadDeBytes/tamanioBlock + byteExcedente(cantidadDeBytes, tamanioBlock);
			char* bloquesSolicitados= string_new();
			//pthread_mutex_lock(&mutexBitMap);
			//t_bitarray* bitMap = recuperarBitArray();

			if(cantidadDeBloquesASolicitar>2)
			{
				int cantidad = 0;
				int posicionBloque = ocuparBitVacio();
				//int posicionBloque = posicionBlockLibre(bitMap);
				bloquesSolicitados = string_from_format("%d",posicionBloque);
				//log_info(loggerMongo,"tamanio string: %d", string_length(string));
				//log_info(loggerMongo,"cantidad de bloques a solicitar: %d",cantidadDeBloquesASolicitar);
				//log_info(loggerMongo,"String partido pre FOR: %s posicion: %d", string_substring(string, cantidad*tamanioBlock, tamanioBlock),cantidad*tamanioBlock);
				char* stringPartido = string_substring(string,cantidad*tamanioBlock,tamanioBlock);
				escribirEnBlocks(posicionBloque, stringPartido);
				//memcpy(blocksMap+(posicionBloque*tamanioBlock),string_substring(string, cantidad*tamanioBlock, tamanioBlock),tamanioBlock);
				cantidad++;
				for(; cantidad<cantidadDeBloquesASolicitar;cantidad++)
				{
					log_info(loggerMongo,"String partido FOR: %s posicion: %d", string_substring(string, cantidad*tamanioBlock, tamanioBlock),cantidad*tamanioBlock);
					posicionBloque = ocuparBitVacio();
					//bitarray_set_bit(bitMap,posicionBloque);
					stringPartido = string_substring(string,cantidad*tamanioBlock,tamanioBlock);
					escribirEnBlocks(posicionBloque, stringPartido);
					//memcpy(blocksMap+(posicionBloque*tamanioBlock),string_substring(string, cantidad*tamanioBlock, tamanioBlock),string_length(string_substring(string, cantidad*tamanioBlock, tamanioBlock)));
					bloquesSolicitados = string_from_format("%s,%d",bloquesSolicitados,posicionBloque);

				}
				//log_info(loggerMongo,"cantidad: %d",cantidad);
				//log_info(loggerMongo,"String partido post FOR: %s posicion: %d", string_substring(string, cantidad, tamanioBlock),cantidad*tamanioBlock);
				forzarSincronizacionBlocks();
				//pthread_mutex_unlock(&mutexBitMap);//semaforo cierre
				//log_info(loggerMongo,"Bloques pedidos: %s",bloquesSolicitados);
				config_set_value(configuracionTripulante,"BLOCKS",string_from_format("[%s]",bloquesSolicitados));
				tamanioString += config_get_int_value(configuracionTripulante,"SIZE");
				config_set_value(configuracionTripulante,"SIZE",string_from_format("%d",tamanioString));
				free(stringPartido);
			}else
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
			}
		}

	}else
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
				bloques = string_from_format("%s,%s",bloques,bloquesUtilizados[contador]);
			}
			bloquePosicion = atoi(bloquesUtilizados[contador]);
			contador++;
		}
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
				forzarSincronizacionBlocks();//forzar la sincronizacion
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
				forzarSincronizacionBlocks();
				config_set_value(configuracionTripulante,"BLOCKS",string_from_format("[%s]",bloques));
				config_set_value(configuracionTripulante,"SIZE",string_from_format("%d",tamanioBitacora));
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
				for(int i = 0; i<cantidadDeBloquesASolicitar;i++)
				{
					nuevoBloque = ocuparBitVacio();
					stringPartido = string_substring(string, posicionString, tamanioBlock);
					posicionString += string_length(stringPartido);
					escribirEnBlocks(nuevoBloque, stringPartido);
					bloques = string_from_format("%s,%d",bloques,nuevoBloque);
				}
				tamanioBitacora += string_length(string);
				config_set_value(configuracionTripulante,"BLOCKS",string_from_format("[%s]",bloques));
				config_set_value(configuracionTripulante,"SIZE",string_from_format("%d",tamanioBitacora));
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
					bloques = string_from_format("%s,%d",bloques,nuevoBloque);
				}
				tamanioBitacora += string_length(string);
				config_set_value(configuracionTripulante,"BLOCKS",string_from_format("[%s]",bloques));
				config_set_value(configuracionTripulante,"SIZE",string_from_format("%d",tamanioBitacora));
			}
			free(stringPartido);
		}
	}
}

int ocuparBitVacio()
{
	pthread_mutex_lock(&mutexBitMap);
	t_bitarray* bitMap = recuperarBitArray();
	int posicion = posicionBlockLibre(bitMap);
	bitarray_set_bit(bitMap,posicion);
	guardarBitArray(bitMap);
	bitarray_destroy(bitMap);
	pthread_mutex_unlock(&mutexBitMap);
	return posicion;
}

void escribirEnBlocks(int posicion, char* string)
{
	pthread_mutex_lock(&mutexBlocks);
	int tamanioString = string_length(string);
	memcpy(blocksMap+(posicion*tamanioBlock),string,tamanioString);
	forzarSincronizacionBlocks();
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
	if(stat(direccionArchivo,&statCarpeta)==-1)
	{
		FILE* archivoBitacora = fopen(string_from_format("%s/Files/%s.ims",PUNTO_MONTAJE,recurso),"w");
		char* textoAEscribir = string_from_format("SIZE=0\nBLOCK_COUNT=0\nBLOCKS=[]\nCARACTER_LLENADO=%c\nMD5_ARCHIVO=",recurso[0]);
		txt_write_in_file(archivoBitacora, textoAEscribir);
		log_info(loggerMongo, "File %s.ims creado",recurso);
		free(textoAEscribir);
	}
	t_config* configFile = config_create(direccionArchivo);
	int cantidadDeBloques = config_get_int_value(configuracionMongo,"BLOCK_COUNT");
	if(cantidadDeBloques==0)
	{//si no tiene ningun bloque
		if(cantidad<=tamanioBlock)
		{//si la cantidad es menor que un bloque de tamanio

		}else
		{//si la cantidad es mayor a un bloque

		}
	}else
	{//si ya uso bloques, buscar y completar

	}
}

void destruirConfig(){
	free(PUNTO_MONTAJE);
	free(IP_I_MONGO);
	free(PUERTO_I_MONGO);
	free(IP_DISCORDIADOR);
	free(PUERTO_DISCORDIADOR);
	liberarArray(POSICIONES_SABOTAJE);
	config_destroy(configuracionMongo);
}

void terminar_programa(){
	log_info(loggerMongo,"Finaliza I-MONGO...");
	munmap(blocksMap,tamanioBlock*cantidadDeBlocks);
	munmap(blocksMapOriginal,tamanioBlock*cantidadDeBlocks);
	dictionary_destroy(caracterAsociadoATarea);
	destruirConfig();
//	log_destroy(loggerMongo);
	exit(0);
}

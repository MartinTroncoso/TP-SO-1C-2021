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
	signal(SIGINT,terminar_programa);

	inicializarVariables();
	log_info(loggerMongo,"PID I-MONGO-STORE: %d",getpid());

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
	dictionary_put(caracterAsociadoATarea, "GENERAR_OXIGENO",(char*) "O");
	dictionary_put(caracterAsociadoATarea, "CONSUMIR_OXIGENO",(char*) "O");
	dictionary_put(caracterAsociadoATarea, "GENERAR_COMIDA",(char*) "C");
	dictionary_put(caracterAsociadoATarea, "CONSUMIR_COMIDA",(char*) "C");
	dictionary_put(caracterAsociadoATarea, "GENERAR_BASURA",(char*) "B");
	dictionary_put(caracterAsociadoATarea, "DESCARTAR_BASURA",(char*) "B");
}

void inicializarFileSystem(){
	inicializarSuperBloque();
	inicializarBlocks();
}

void inicializarSuperBloque(){
	struct stat statCarpeta;
	if(stat(string_from_format("%s/SuperBloque.ims",PUNTO_MONTAJE),&statCarpeta)==-1)
	{
		FILE* archivoSuperBloque = fopen(string_from_format("%s/SuperBloque.ims",PUNTO_MONTAJE),"w");
		txt_write_in_file(archivoSuperBloque, "BLOCK_SIZE=20\nBLOCKS=40\nBITMAP=");
		log_info(loggerMongo, "Archivo SuperBloque.ims creado");
	}else
	{
		log_info(loggerMongo,"El archvio SuperBloque.ims ya existe");
	}
	t_config* configuracionSuperBloque = config_create(string_from_format("%s/SuperBloque.ims",PUNTO_MONTAJE));
	uint32_t cantidadDeBloques = config_get_int_value(configuracionSuperBloque, "BLOCKS");
	//printf("cantidad de bytes a reservar: %d\n",(cantidadDeBloques/8));
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
//		for(int i=0;i<bitarray_get_max_bit(bitArraySuperBloque);i++)
//		{
//			printf("%d",bitarray_test_bit(bitArraySuperBloque,i));
//		}
//		printf("\n\nvalor del array:%s\n",bitArraySuperBloque->bitarray);


		config_set_value(configuracionSuperBloque,"BITMAP",bitArraySuperBloque->bitarray);
		config_save(configuracionSuperBloque);
		config_destroy(configuracionSuperBloque);
	}else
	{
		//printf("BITMAP TIENE UN VALOR %s\n",config_get_string_value(configuracionSuperBloque,"BITMAP"));
		config_destroy(configuracionSuperBloque);
	}
}

t_bitarray recuperarBitArray(){
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
	log_info(loggerMongo, "Tamaño del archivo total es %d",infoBlocks.st_size);
	if(ftruncate(fdArchivoBlocks,tamanioBlock*cantidadDeBlocks)==-1)
	{
		log_info(loggerMongo, "No se pudo crear archivo con el tamaño %dx%d",tamanioBlock,cantidadDeBlocks);
	}
	log_info(loggerMongo, "Se creo el archivo con el tamaño %dx%d",tamanioBlock,cantidadDeBlocks);

	//struct stat infoBlocks;
	if(fstat(fdArchivoBlocks,&infoBlocks)== -1)
	{
			log_info(loggerMongo, "No se pudo obtener stat de Blocks.ims");
	}
	log_info(loggerMongo, "Tamaño del archivo total es %d",infoBlocks.st_size);
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
			recibirAtenderSabotaje(socket_tripulante,idTripulante);
			break;
		case RESOLUCION_SABOTAJE:
			recibirResolucionSabotaje(socket_tripulante,idTripulante);
			posicionSabotajeActual = getSiguientePosicionSabotaje();
			break;
		case INVOCAR_FSCK:
			log_info(loggerMongo,"Se ejecuta el FSCK. Por ahora no hace nada :D");
			ejecutarFSCK();
			break;
		case OBTENER_BITACORA:
			recibirPeticionDeBitacora(socket_tripulante,idTripulante);
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

void recibirAtenderSabotaje(int socket_tripulante, uint32_t id_tripulante)
{
	void* buffer;
	uint32_t sizeBuffer;

	buffer = recibir_buffer(&sizeBuffer, socket_tripulante);
	memcpy(&id_tripulante, buffer,sizeof(uint32_t));

	FILE* bitacoraTripulante = txt_open_for_append(string_from_format("%s/Files/Bitacoras/Tripulante%d.ims",PUNTO_MONTAJE,id_tripulante));
	txt_write_in_file(bitacoraTripulante, string_from_format("Se corre en panico hacia la ubicacion del sabotaje\n"));
	free(buffer);
	txt_close_file(bitacoraTripulante);
}

void recibirResolucionSabotaje(int socket_tripulante, uint32_t id_tripulante)
{
	void* buffer;
	uint32_t sizeBuffer;

	buffer = recibir_buffer(&sizeBuffer, socket_tripulante);
	memcpy(&id_tripulante, buffer,sizeof(uint32_t));

	FILE* bitacoraTripulante = txt_open_for_append(string_from_format("%s/Files/Bitacoras/Tripulante%d.ims",PUNTO_MONTAJE,id_tripulante));
	txt_write_in_file(bitacoraTripulante, string_from_format("Se resuelve el sabotaje\n"));
	free(buffer);
	txt_close_file(bitacoraTripulante);
}

int bitsExcedentes(int cantidadDeBits){
	return (cantidadDeBits%8)>0;
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
	dictionary_destroy(caracterAsociadoATarea);
	destruirConfig();
//	log_destroy(loggerMongo);
	exit(0);
}

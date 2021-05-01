/*
 ============================================================================
 Name        : Discordiador.c
 Author      : 
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include "Discordiador.h"

void inicializarVariables(){
	configuracionDiscordiador = config_create("/home/utnso/workspace/tp-2021-1c-No-C-Aprueba-/Discordiador/discordiador.config");
	loggerDiscordiador = log_create("/home/utnso/workspace/tp-2021-1c-No-C-Aprueba-/Discordiador/discordiador.log","Discordiador",1,LOG_LEVEL_INFO);
	diccionarioDiscordiador = dictionary_create();

	IP_MI_RAM = config_get_string_value(configuracionDiscordiador,"IP_MI_RAM_HQ");
	IP_I_MONGO_STORE = config_get_string_value(configuracionDiscordiador,"IP_I_MONGO_STORE");
	ALGORITMO = config_get_string_value(configuracionDiscordiador,"ALGORITMO");
	PUERTO_MI_RAM = config_get_string_value(configuracionDiscordiador,"PUERTO_MI_RAM_HQ");
	PUERTO_I_MONGO_STORE = config_get_string_value(configuracionDiscordiador,"PUERTO_I_MONGO_STORE");
	GRADO_MULTITAREA = config_get_int_value(configuracionDiscordiador,"GRADO_MULTITAREA");
	QUANTUM = config_get_int_value(configuracionDiscordiador,"QUANTUM");
	DURACION_SABOTAJE = config_get_int_value(configuracionDiscordiador,"DURACION_SABOTAJE");
	RETARDO_CICLO_CPU = config_get_int_value(configuracionDiscordiador,"RETARDO_CICLO_CPU");
	crearDiccionarioComandos(diccionarioDiscordiador);
	id_tripulante = 1;

	socket_cliente_miRam = crearConexionCliente(IP_MI_RAM, PUERTO_MI_RAM);
	socket_cliente_iMongo = crearConexionCliente(IP_I_MONGO_STORE, PUERTO_I_MONGO_STORE);
}

void crearDiccionarioComandos(t_dictionary* diccionario)
{
	dictionary_put(diccionario,"INICIAR_PATOTA",(int*) 1);
	dictionary_put(diccionario,"LISTAR_TRIPULANTES",(int*) 2);
	dictionary_put(diccionario,"EXPULSAR_TRIPULANTE",(int*) 3);
	dictionary_put(diccionario,"INICIAR_PLANIFICACION",(int*) 4);
	dictionary_put(diccionario,"PAUSAR_PLANIFICACION",(int*) 5);
	dictionary_put(diccionario,"OBTENER_BITACORA",(int*) 6);

	//printf("diccionario %d\n", (int) dictionary_get(diccionario,"OBTENER_BITACORA"));
}

void iniciarPatota(t_iniciar_patota estructura){
	//crea una patota
	//crea estructura->cantidadTripulantes tripulantes
	//le asigna a cada uno su posición
	//cada tripulante arranca en estado NEW
	//ir asignadole a cada tripulante su id
	//mandarle a Mi-RAM todos los tripulantes para que los ponga en memoria
}

void listarTripulantes(){
	//imprime todos los tripulantes
	/*
	 Por ejemplo

	 Estado de la Nave: 30/06/2021 10:30:00
	 Tripulante: 1		Patota: 1		Status: EXEC
	 Tripulante: 2		Patota: 2		Status: EXEC
	 Tripulante: 3		Patota: 2		Status: BLOCK I/O
	 Tripulante: 4		Patota: 3		Status: READY
	*/
}

void expulsarTripulante(int idTripulante){
	//se finaliza un tripulante
	//avisarle a MI-RAM que lo borre del mapa
	//en caso de ser necesario tambien elimina su segmento de tareas
}

void iniciarPlanificacion(){
	//se inicia la planificacion
	//hasta este punto se supone que no hubo movimientos entre colas de planificacion ni de los tripulantes
}

void pausarPlanificacion(){
	//se pausa la planificacion
}

void obtenerBitacora(int idTripulante){
	//se manda un mensaje a I-MONGO solicitando la bitacora de un tripulante
}

void leer_consola(t_dictionary* diccionario,t_log* logger)
{
	char* palabras;
	char* comando = readline(">");

	while(strcmp(comando,"")!=0){
		log_info(logger,comando);

		switch((int) dictionary_get(diccionario,strtok(comando," ")))
		{
		case 1:{
			//INICIAR_PATOTA [cant_tripulantes] [path] [pos1] [pos2] ...
			//PRIMER COMANDO A MANDAR
			palabras = comando;
			partirCadena(palabras);
			//iniciarPatota(t_iniciar_patota);
			break;
		}
		case 2:{
			//LISTAR_TRIPULANTES
			listarTripulantes();
			break;
		}
		case 3:{
			//EXPULSAR_TRIPULANTE 'id'
			int idTripulante = atoi(strtok(NULL," "));
			expulsarTripulante(idTripulante);
			break;
		}
		case 4:{
			//INICIAR_PLANIFICACION
			iniciarPlanificacion();
			break;
		}
		case 5:{
			//PAUSAR_PLANIFICACION
			pausarPlanificacion();
			break;
		}
		case 6:{
			//OBTENER_BITACORA [idTripulante]
			int idTripulante = atoi(strtok(NULL," "));
			obtenerBitacora(idTripulante);
			break;
		}
		default:
			printf("Comando no reconocido\n");
			break;
		}
		free(comando);
		comando = readline(">");
	}

	free(comando);
}

void paquete(int socket_cliente, int otro_socket_cliente)
{
	char* leido;
	t_paquete* paquete = crear_paquete();
	leido = readline(">");
	while(strcmp(leido,"")!=0){
		agregar_a_paquete(paquete,leido,strlen(leido) + 1);
		free(leido);
		leido = readline(">");
	}

	free(leido);
	enviar_paquete(paquete,socket_cliente);
	enviar_paquete(paquete,otro_socket_cliente);

	eliminar_paquete(paquete);
}

void terminar_programa()
{
	close(socket_cliente_iMongo);
	close(socket_cliente_miRam);
	log_destroy(loggerDiscordiador);
	config_destroy(configuracionDiscordiador);
}

void partirCadena(char* cadena)
{
	t_iniciar_patota* parametrosPatota = malloc(sizeof(t_iniciar_patota));
	t_list* listaCoordenadas = list_create();
	char* token = strtok(NULL," ");
	if(atoi(token)!=0){
		parametrosPatota->cantidadTripulantes = atoi(token);
	}
	else
	{
		printf("Error leyendo cantidadTripulantes\n");
		token = NULL;
	}
	//parametrosPatota->cantidadTripulantes = atoi(token);
	token = strtok(NULL," ");
	if(token!=NULL)
	{
		parametrosPatota->rutaDeTareas = token;
		token = strtok(NULL," |");
		printf("%d\n",parametrosPatota->cantidadTripulantes);
		printf("%s\n",parametrosPatota->rutaDeTareas);
		for(int i = 0;i<parametrosPatota->cantidadTripulantes;i++)
		{
			if(token!=NULL)
			{
				coordenadasTripulante* posicionTripulante = malloc(sizeof(coordenadasTripulante));
				if(strcmp(token,"0")==0)
				{
					posicionTripulante->coordenadaX = 0;
					token = strtok(NULL," |");
				}
				else
					if(atoi(token)!=0){
						posicionTripulante->coordenadaX = atoi(token);
						token = strtok(NULL," |");
					}else
					{
						printf("Coordenadas mal ingresadas X\n");
					}
					/*posicionTripulante->coordenadaX = atoi(token);
					token = strtok(NULL," |");*/
					if(strcmp(token,"0")==0)
					{
						posicionTripulante->coordenadaY = 0;
						token = strtok(NULL," |");
					}else if(atoi(token)!=0)
					{
						posicionTripulante->coordenadaY = atoi(token);
						token = strtok(NULL," |");
					}else
					{
						printf("Coordenadas mal ingresadas Y\n");
					}
					printf("Posicion x e y %d %d\n",posicionTripulante->coordenadaX,posicionTripulante->coordenadaY);
					list_add(listaCoordenadas,posicionTripulante);
			}else
			{
				coordenadasTripulante* posicionTripulante = malloc(sizeof(coordenadasTripulante));
				posicionTripulante->coordenadaX=0;
				posicionTripulante->coordenadaY=0;
				printf("Posicion 0|0 es %d %d\n",posicionTripulante->coordenadaX,posicionTripulante->coordenadaY);
				list_add(listaCoordenadas,posicionTripulante);
			}
		}
	}
}

int main(void)
{
	inicializarVariables();

	char palabras[] = "COMANDO 5 1|4 1|7";
	char* token = strtok(palabras," |");
	while(token!=NULL)
	{
		printf("%s\n",token);
		token = strtok(NULL," |");
	}

	leer_consola(diccionarioDiscordiador, loggerDiscordiador);

	//SE ENVIA EL ALGORITMO A LOS 2 PROCESOS
	enviar_mensaje(ALGORITMO,socket_cliente_miRam);
	enviar_mensaje(ALGORITMO,socket_cliente_iMongo);

	//SE ARMA UN PAQUETE CON LOS MENSAJES QUE SE ESCRIBAN EN CONSOLA Y CUANDO SE APRIETA 'ENTER' SE MANDA TODO JUNTO
	paquete(socket_cliente_miRam, socket_cliente_iMongo);

	terminar_programa();

	return EXIT_SUCCESS;
}

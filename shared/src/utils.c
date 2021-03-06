/*
 * utils.c
 *
 *  Created on: 24 abr. 2021
 *      Author: utnso
 */

#include "utils.h"

int iniciarServidor(char* ip, char* puerto)
{
	int socket_servidor;

    struct addrinfo hints;
    struct addrinfo *servinfo;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    getaddrinfo(ip, puerto, &hints, &servinfo);

    socket_servidor = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);

    int activado = 1;
    setsockopt(socket_servidor, SOL_SOCKET, SO_REUSEADDR, &activado, sizeof(activado));

    bind(socket_servidor, servinfo->ai_addr, servinfo->ai_addrlen);

	listen(socket_servidor, SOMAXCONN);

    freeaddrinfo(servinfo);

    return socket_servidor;
}

int crearConexionCliente(char *ip, char* puerto)
{
	struct addrinfo hints;
	struct addrinfo *server_info;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	getaddrinfo(ip, puerto, &hints, &server_info);

	int socket_cliente = socket(server_info->ai_family, server_info->ai_socktype, server_info->ai_protocol);

	int status = connect(socket_cliente, server_info->ai_addr, server_info->ai_addrlen);

	freeaddrinfo(server_info);

	if(status == -1)
		return status;

	return socket_cliente;
}

void enviar_mensaje(char* mensaje, int socket_cliente)
{
	t_paquete* paquete = malloc(sizeof(t_paquete));

	paquete->codigo_operacion = INICIAR_PATOTA;
	paquete->buffer = malloc(sizeof(t_buffer));
	paquete->buffer->size = strlen(mensaje) + 1;
	paquete->buffer->stream = malloc(paquete->buffer->size);
	memcpy(paquete->buffer->stream, mensaje, paquete->buffer->size);

	int bytes = paquete->buffer->size + 2*sizeof(int);

	void* a_enviar = serializar_paquete(paquete, bytes);

	send(socket_cliente, a_enviar, bytes, 0);

	free(a_enviar);
	eliminar_paquete(paquete);
}

void agregar_a_paquete(t_paquete* paquete, void* valor, int tamanio)
{
	paquete->buffer->stream = realloc(paquete->buffer->stream, paquete->buffer->size + tamanio + sizeof(int));

	memcpy(paquete->buffer->stream + paquete->buffer->size, &tamanio, sizeof(int));
	memcpy(paquete->buffer->stream + paquete->buffer->size + sizeof(int), valor, tamanio);

	paquete->buffer->size += tamanio + sizeof(int);
}

void* serializar_paquete(t_paquete* paquete, int bytes)
{
	void* magic = malloc(bytes);
	int desplazamiento = 0;

	memcpy(magic + desplazamiento, &(paquete->codigo_operacion), sizeof(int));
	desplazamiento+= sizeof(int);
	memcpy(magic + desplazamiento, &(paquete->buffer->size), sizeof(int));
	desplazamiento += sizeof(int);
	memcpy(magic + desplazamiento, paquete->buffer->stream, paquete->buffer->size);

	return magic;
}

void* serializar_buffer(t_buffer* buffer, int bytes){
	void* magic = malloc(bytes);
	int desplazamiento = 0;

	memcpy(magic + desplazamiento, &(buffer->size), sizeof(int));
	desplazamiento+= sizeof(int);
	memcpy(magic + desplazamiento, buffer->stream, buffer->size);

	return magic;
}

void* serializar_tarea(Tarea* tarea, int bytes){
	void* magic = malloc(bytes);
	int desplazamiento = 0;

	memcpy(magic + desplazamiento, &(tarea->longNombre), sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);
	memcpy(magic + desplazamiento, tarea->nombre, tarea->longNombre);
	desplazamiento += tarea->longNombre;
	memcpy(magic + desplazamiento, &(tarea->parametro), sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);
	memcpy(magic + desplazamiento, &(tarea->posicion.posX), sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);
	memcpy(magic + desplazamiento, &(tarea->posicion.posY), sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);
	memcpy(magic + desplazamiento, &(tarea->tiempo), sizeof(uint32_t));

	return magic;
}

void enviar_paquete(t_paquete* paquete, int socket_cliente)
{
	int bytes = paquete->buffer->size + 2*sizeof(int);
	void* a_enviar = serializar_paquete(paquete, bytes);

	send(socket_cliente, a_enviar, bytes, 0);

	free(a_enviar);
}

void enviar_buffer(t_buffer* buffer, int socket_cliente){
	int bytes = buffer->size + sizeof(int);
	void* a_enviar = serializar_buffer(buffer, bytes);

	send(socket_cliente, a_enviar, bytes, 0);

	free(a_enviar);
}

void eliminar_paquete(t_paquete* paquete)
{
	free(paquete->buffer->stream);
	free(paquete->buffer);
	free(paquete);
}

int esperar_cliente(int socket_servidor)
{
	struct sockaddr_in addr;
	socklen_t tam_direccion = sizeof(addr);

	int socket_cliente = accept(socket_servidor, (struct sockaddr*) &addr, &tam_direccion);

	return socket_cliente;
}

int recibir_operacion(int socket_cliente)
{
	int cod_op;
	if(recv(socket_cliente, &cod_op, sizeof(int), MSG_WAITALL) != 0)
		return cod_op;
	else
	{
		close(socket_cliente);
		return -1;
	}
}

void enviar_respuesta(tipo_respuesta cod_respuesta, int socket_cliente) {
	void* a_enviar = malloc(sizeof(tipo_respuesta));
	memcpy(a_enviar,&cod_respuesta,sizeof(tipo_respuesta));
	send(socket_cliente, a_enviar, sizeof(tipo_respuesta), 0);
	free(a_enviar);
}

tipo_respuesta recibir_respuesta(int socket_servidor)
{
	tipo_respuesta cod_respuesta;
	if(recv(socket_servidor, &cod_respuesta, sizeof(int), MSG_WAITALL) != 0)
		return cod_respuesta;
	else
	{
		close(socket_servidor);
		return -1;
	}
}

void* recibir_buffer(uint32_t* size, int socket_cliente)
{
	void* buffer;

	recv(socket_cliente, size, sizeof(uint32_t), MSG_WAITALL);
	buffer = malloc(*size);
	recv(socket_cliente, buffer, *size, MSG_WAITALL);

	return buffer;
}

void recibir_mensaje(int socket_cliente)
{
	uint32_t size;
	char* buffer = recibir_buffer(&size, socket_cliente);
	printf("%s\n",buffer);
	free(buffer);
}

void liberarArray(char** array){
	for(int i=0; array[i]!=NULL ;i++){
		free(array[i]);
	}
	free(array);
}

int getTamanioArray(char** array){
	int i;
	for(i=0; array[i]!=NULL ;i++);

	return i;
}

char* string_substring_until_char(char* text, char separator) {
	char* new_string;
	char* sep_pos = strchr(text, separator);

	if(sep_pos != NULL) {
		new_string = string_substring(text, 0, sep_pos - text);
	}
	else {
		new_string = string_duplicate(text);
	}
	return new_string;
}

//podemos usar la lista de valores para poder hablar del for y de como recorrer la lista
t_list* recibir_paquete(int socket_cliente)
{
	uint32_t size;
	int desplazamiento = 0;
	void * buffer;
	t_list* valores = list_create();
	int tamanio;

	buffer = recibir_buffer(&size, socket_cliente);
	while(desplazamiento < size)
	{
		memcpy(&tamanio, buffer + desplazamiento, sizeof(int));
		desplazamiento += sizeof(int);
		char* valor = malloc(tamanio);
		memcpy(valor, buffer+desplazamiento, tamanio);
		desplazamiento += tamanio;
		list_add(valores, valor);
	}

	free(buffer);
	return valores;
	return NULL;
}

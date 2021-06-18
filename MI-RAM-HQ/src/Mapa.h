/*
 * Mapa.h
 *
 *  Created on: 18 jun. 2021
 *      Author: utnso
 */

#ifndef MAPA_H_
#define MAPA_H_

#include <stdlib.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <curses.h>
#include <stdbool.h>
#include <commons/collections/list.h>
#include <commons/string.h>

/*------------------------------------nivel-gui.h------------------------------------*/
	#define PERSONAJE_ITEM_TYPE 2
	#define CAJA_ITEM_TYPE      3
	#define ENEMIGO_ITEM_TYPE   4

	typedef struct nivel {
		char* nombre;
		t_list* items;
	} NIVEL;

	typedef struct item {
		char id;
		int posx;
		int posy;
		int item_type; // PERSONAJE, ENEMIGO o CAJA
		int quantity;
	} ITEM_NIVEL;

	/*
	* @NAME: nivel_gui_inicializar
	* @DESC: Inicializa el espacio de dibujo
	* @ERRORS:
	*     NGUI_ALREADY_INIT
	*/
	int nivel_gui_inicializar(void);

	/*
	* @NAME: nivel_gui_dibujar
	* @DESC: Dibuja cada entidad en la lista de items
	* @PARAMS:
	*       nivel - nivel a dibujar
	* @ERRORS:
	*     NGUI_NO_INIT
	*/
	int nivel_gui_dibujar(NIVEL* nivel);

	/*
	* @NAME: nivel_gui_terminar
	* @DESC: Termina el nivel de forma prolija
	* @ERRORS:
	*     NGUI_NO_INIT
	*/
	int nivel_gui_terminar(void);

	/*
	* @NAME: nivel_gui_get_area_nivel
	* @DESC: Devuelve el tamanio usable de la pantalla
	* @PARAMS:
	*       columnas - valor de retorno de columnas
	*       filas    - valor de retorno de filas
	* @ERRORS:
	*     NGUI_NO_INIT
	*     NGUI_TERM_SIZE_FAIL
	*/
	int nivel_gui_get_area_nivel(int * columnas, int * filas);

	/*
	* @NAME: nivel_gui_string_error
	* @DESC: Devuelve un string según el código de error recibido
	* @PARAMS:
	*     errnum - código de error
	*/
	char* nivel_gui_string_error(int errnum);

	/*
	* @NAME: NGUI_SUCCESS
	* @DESC: Operacion exitosa.
	*/
	#define NGUI_SUCCESS                0
	/*
	* @NAME: NGUI_ITEM_NOT_FOUND
	* @DESC: No se encontro el item.
	*/
	#define NGUI_ITEM_NOT_FOUND        -1
	/*
	* @NAME: NGUI_ITEM_ALREADY_EXISTS
	* @DESC: El item ya existe.
	*/
	#define NGUI_ITEM_ALREADY_EXISTS   -2
	/*
	* @NAME: NGUI_ITEM_NOT_A_BOX
	* @DESC: El item no es una caja de recursos.
	*/
	#define NGUI_ITEM_NOT_A_BOX        -3
	/*
	* @NAME: NGUI_ITEM_INVALID_POSITION
	* @DESC: La posicion se encuentra fuera del tablero.
	*/
	#define NGUI_ITEM_INVALID_POSITION -4
	/*
	* @NAME: NGUI_ITEM_INVALID_SRCS
	* @DESC: La cantidad de recursos recibida no es valida.
	*/
	#define NGUI_ITEM_INVALID_SRCS     -5
	/*
	* @NAME: NGUI_ITEM_EMPTY_BOX
	* @DESC: "La caja de recursos se encuentra vacia."
	*/
	#define NGUI_ITEM_EMPTY_BOX        -6
	/*
	* @NAME: NGUI_NO_INIT
	* @DESC: Library no inicializada.
	*/
	#define NGUI_NO_INIT               -7
	/*
	* @NAME: NGUI_ALREADY_INIT
	* @DESC: Library ya inicializada.
	*/
	#define NGUI_ALREADY_INIT          -8
	/*
	* @NAME: NGUI_TERM_SIZE_FAIL
	* @DESC: Error al obtener el tamaño de la terminal.
	*/
	#define NGUI_TERM_SIZE_FAIL        -9


/*------------------------------------tad_nivel.h------------------------------------*/
/*
* @NAME: nivel_crear
* @DESC: Crea una instancia de estructura nivel.
* @PARAMS:
*     nombre - nombre del nivel
*/
NIVEL* nivel_crear(char* nombre);

/*
* @NAME: nivel_destruir
* @DESC: Libera la memoria ocupada por una estructura nivel
* y sus items.
*/
void nivel_destruir(NIVEL* nivel);

/*
* @NAME: personaje_crear
* @DESC: Crea una instancia de personaje que se mostrará
* en el nivel con el caracter que lo identifica en color
* blanco y fondo en negro.
* @PARAMS:
*     nivel - nivel donde se encontrará el personaje
*     id    - identificador único de acceso del item
*     x     - posición inicial en x
*     y     - posición inicial en y
* @ERRORS:
*     NGUI_ITEM_ALREADY_EXISTS
*     NGUI_ITEM_INVALID_POSITION
*/
int personaje_crear(NIVEL* nivel, char id, int x , int y);

/*
* @NAME: enemigo_crear
* @DESC: Crea una instancia de enemigo que se mostrará
* en el nivel con el caracter '*' en color negro y fondo
* en azul.
* @PARAMS:
*     nivel - nivel donde se encontrará el enemigo
*     id    - identificador único de acceso del item
*     x     - posición inicial en x
*     y     - posición inicial en y
* @ERRORS:
*     NGUI_ITEM_ALREADY_EXISTS
*     NGUI_ITEM_INVALID_POSITION
*/
int enemigo_crear(NIVEL* nivel, char id, int x, int y);

/*
* @NAME: caja_crear
* @DESC: Crea una instancia de caja que se mostrará en
* el nivel con el caracter que lo identifica en color negro
* y fondo en amarillo.
* @PARAMS:
*     nivel - nivel donde se encontrará la caja
*     id    - identificador único de acceso del item
*     x     - posición inicial en x
*     y     - posición inicial en y
*     srcs  - cantidad inicial de recursos contenidos en la caja
* @ERRORS:
*     NGUI_ITEM_ALREADY_EXISTS
*     NGUI_ITEM_INVALID_POSITION
*     NGUI_ITEM_INVALID_SRCS
*/
int caja_crear(NIVEL* nivel, char id, int x, int y, int srcs);

/*
* @NAME: item_borrar
* @DESC: Elimina un item que se encuentre en el nivel.
* @PARAMS:
*     nivel - nivel donde se encuentra el item
*     id    - identificador único de acceso del item
* @ERRORS:
*     NGUI_ITEM_NOT_FOUND
*/
int item_borrar(NIVEL* nivel, char id);

/*
* @NAME: item_mover
* @DESC: Mueve el item a la posición indicada. En caso de
* moverse fuera de la pantalla, no realiza ninguna acción.
* @PARAMS:
*     nivel - nivel donde se encuentra el item
*     id    - identificador único de acceso del item
*     x     - nueva posición en x
*     y     - nueva posición en y
* @ERRORS:
*     NGUI_ITEM_NOT_FOUND
*     NGUI_ITEM_INVALID_POSITION
*/
int item_mover(NIVEL* nivel, char id, int x, int y);

/*
* @NAME: item_desplazar
* @DESC: Desplaza el item tomando a su posición actual
* como pivote. En caso de desplazarse fuera de la pantalla,
* no realiza ninguna acción.
* @PARAMS:
*     nivel    - nivel donde se encuentra el item
*     id       - identificador único de acceso del item
*     offset_x - desplazamiento en x
*     offset_y - desplazamiento en y
* @ERRORS:
*     NGUI_ITEM_NOT_FOUND
*     NGUI_ITEM_INVALID_POSITION
*/
int item_desplazar(NIVEL* nivel, char id, int offset_x, int offset_y);

/*
* @NAME: caja_quitar_recurso
* @DESC: Quita un recurso de la caja. Si la caja está
* vacía, no realiza ninguna acción.
* @PARAMS:
*     nivel - nivel donde se encuentra el item
*     id    - identificador único de acceso del item
* @ERRORS:
*     NGUI_ITEM_NOT_FOUND
*     NGUI_ITEM_NOT_A_BOX
*     NGUI_ITEM_EMPTY_BOX
*/
int caja_quitar_recurso(NIVEL* nivel, char id);

/*
* @NAME: caja_agregar_recurso
* @DESC: Agrega un recurso a la caja.
* @PARAMS:
*     nivel - nivel donde se encuentra el item
*     id    - identificador único de acceso del item
* @ERRORS:
*     NGUI_ITEM_NOT_FOUND
*     NGUI_ITEM_NOT_A_BOX
*/
int caja_agregar_recurso(NIVEL* nivel, char id);

/*
* @NAME: items_chocan
* @DESC: Devuelve true si ambos items se encuentran
* en la misma posición
* @PARAMS:
*     nivel - nivel donde se encuentra el item
*     id1   - identificador único de acceso del ítem 1
*     id2   - identificador único de acceso del ítem 2
*/
bool items_chocan(NIVEL* nivel, char id1, char id2);

#endif /* MAPA_H_ */

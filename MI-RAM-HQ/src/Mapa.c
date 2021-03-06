/*
 * Mapa.c
 *
 *  Created on: 18 jun. 2021
 *      Author: utnso
 */

#include "Mapa.h"

	/*
	 * @NAME: rnd
	 * @DESC: Retorna un entero en el rango [-1, 1]
	 */
	int rnd() {
		return (rand() % 3) - 1;
	}

/*------------------------------------nivel-gui.c------------------------------------*/
static WINDOW * secwin;
static WINDOW * mainwin;
static int rows, cols;
static int inicializado = 0;

// ------ Prototipos de Funciones utilitarias ------ //

/*
 * @NAME: nivel_gui_get_term_size
 * @DESC: Devuelve el tamanio total de la pantalla
 * @PARAMS:
 *      filas    - valor de retorno de filas
 *      columnas - valor de retorno de columnas
 */
int nivel_gui_get_term_size(int * filas, int * columnas);

/*
 * @NAME: nivel_gui_int_validar_inicializado
 * @DESC: Informa si se inicializo el nivel corectamente
 */
int nivel_gui_int_validar_inicializado(void);

/*
 * @NAME: nivel_gui_item_show
 * @DESC: Devuelve el caracter a mostrarse en pantalla
 * segun el item
 */
char nivel_gui_item_show(ITEM_NIVEL* item);

// ------------------------------------------------------

int nivel_gui_inicializar(void) {

	if (nivel_gui_int_validar_inicializado()) {
		return NGUI_ALREADY_INIT;
	}

	mainwin = initscr();
	keypad(stdscr, TRUE);
	noecho();
	start_color();
	init_pair(1                  , COLOR_GREEN, COLOR_BLACK);
	init_pair(PERSONAJE_ITEM_TYPE, COLOR_WHITE, COLOR_BLACK);
	init_pair(CAJA_ITEM_TYPE     , COLOR_BLACK, COLOR_YELLOW);
	init_pair(ENEMIGO_ITEM_TYPE  , COLOR_BLACK, COLOR_BLUE);
	box(stdscr, 0, 0);
	refresh();

	nivel_gui_get_term_size(&rows, &cols);
	secwin = newwin(rows - 3, cols, 0, 0);
	box(secwin, 0, 0);
	wrefresh(secwin);

	inicializado = 1;

	return NGUI_SUCCESS;

}

int nivel_gui_dibujar(NIVEL* nivel) {

	if (!nivel_gui_int_validar_inicializado()) {
		return NGUI_NO_INIT;
	}

	char* srcs_text = string_duplicate("Recursos: ");

	werase(secwin);
	box(secwin, 0, 0);
	wbkgd(secwin, COLOR_PAIR(1));

	void _draw_element(ITEM_NIVEL* item) {
		wmove(secwin, item->posy + 1, item->posx + 1);
		waddch(secwin, nivel_gui_item_show(item) | COLOR_PAIR(item->item_type));
		if (item->item_type == CAJA_ITEM_TYPE) {
			string_append_with_format(&srcs_text, "%c: %d - ", item->id, item->quantity);
		}
	}

	list_iterate(nivel->items, (void*) _draw_element);

	move(rows - 3, 2);
	printw("Nivel: %s - Tamanio: %dx%d", nivel->nombre, cols - 2, rows - 5);
	move(rows - 2, 2);
	printw(srcs_text);

	wrefresh(secwin);
	wrefresh(mainwin);

	free(srcs_text);

	return NGUI_SUCCESS;

}

int nivel_gui_terminar(void) {

	if (!nivel_gui_int_validar_inicializado()) {
		return NGUI_NO_INIT;
	}

	delwin(mainwin);
	delwin(secwin);
	endwin();
	refresh();

	return NGUI_SUCCESS;

}

int nivel_gui_get_area_nivel(int * cols, int * rows) {

	if (!nivel_gui_int_validar_inicializado()) {
		return NGUI_NO_INIT;
	}

	if(nivel_gui_get_term_size(rows, cols)) {
		return NGUI_TERM_SIZE_FAIL;
	}

	if(rows) *rows = *rows - 5;
	if(cols) *cols = *cols - 2;

	return NGUI_SUCCESS;
}

char* nivel_gui_string_error(int errnum) {
	switch ( errnum ) {
		case NGUI_SUCCESS:
			return "Operacion exitosa.";
		case NGUI_ITEM_NOT_FOUND:
			return "No se encontro el item.";
		case NGUI_ITEM_ALREADY_EXISTS:
			return "El item ya existe.";
		case NGUI_ITEM_NOT_A_BOX:
			return "El item no es una caja de recursos.";
		case NGUI_ITEM_INVALID_POSITION:
			return "La posicion se encuentra fuera del tablero.";
		case NGUI_ITEM_INVALID_SRCS:
			return "La cantidad de recursos recibida no es valida.";
		case NGUI_ITEM_EMPTY_BOX:
			return "La caja de recursos se encuentra vacia.";
		case NGUI_NO_INIT:
			return "Library no inicializada.";
		case NGUI_ALREADY_INIT:
			return "Library ya inicializada.";
		case NGUI_TERM_SIZE_FAIL:
			return "Error al obtener el tamanio de la terminal.";
		default:
			return "Error desconocido.";
	}
}

/*---------------- Funciones utilitarias ----------------*/

int nivel_gui_get_term_size(int * rows, int * cols) {

	struct winsize ws;

	if ( ioctl(0, TIOCGWINSZ, &ws) < 0 ) {
		return NGUI_TERM_SIZE_FAIL;
	}

	if(rows) *rows = ws.ws_row;
	if(cols) *cols = ws.ws_col;

	return NGUI_SUCCESS;
}

int nivel_gui_int_validar_inicializado(void) {
	return inicializado;
}

char nivel_gui_item_show(ITEM_NIVEL* item) {
	return item->item_type == ENEMIGO_ITEM_TYPE ? '*' : item->id;
}
/*-----------------------------------------------------------------------------------*/


//TODO


/*------------------------------------tad_nivel.c------------------------------------*/
int _crear_item(NIVEL* nivel, char id, int x, int y, int tipo, int cant);
ITEM_NIVEL* _search_item_by_id(NIVEL* nivel, char id);
int _cambiar_posicion(ITEM_NIVEL* item, int x, int y);
bool _validar_posicion(int x, int y);

NIVEL* nivel_crear(char* nombre) {
	NIVEL* nivel = malloc(sizeof(NIVEL));

	nivel->nombre = string_duplicate(nombre);
	nivel->items = list_create();

	return nivel;
}

void nivel_destruir(NIVEL* nivel) {
	list_destroy_and_destroy_elements(nivel->items, (void*) free);
	free(nivel->nombre);
	free(nivel);
}

int personaje_crear(NIVEL* nivel, char id, int x , int y) {
	return _crear_item(nivel, id, x, y, PERSONAJE_ITEM_TYPE, 0);
}

int enemigo_crear(NIVEL* nivel, char id, int x , int y) {
	return _crear_item(nivel, id, x, y, ENEMIGO_ITEM_TYPE, 0);
}

int caja_crear(NIVEL* nivel, char id, int x , int y, int cant) {
	if(cant < 0) {
		return NGUI_ITEM_INVALID_SRCS;
	}

	return _crear_item(nivel, id, x, y, CAJA_ITEM_TYPE, cant);
}

int item_borrar(NIVEL* nivel, char id) {
	bool found = false;
	bool _search_by_id(ITEM_NIVEL* item) {
		found = item->id == id;
		return found;
	}
	list_remove_and_destroy_by_condition(nivel->items, (void*) _search_by_id, (void*) free);

	return found ? NGUI_SUCCESS : NGUI_ITEM_NOT_FOUND;
}

int item_mover(NIVEL* nivel, char id, int x, int y) {
	ITEM_NIVEL* item = _search_item_by_id(nivel, id);

	if (item == NULL) {
		return NGUI_ITEM_NOT_FOUND;
	}

	return _cambiar_posicion(item, x, y);
}

int item_desplazar(NIVEL* nivel, char id, int offset_x, int offset_y) {
	ITEM_NIVEL* item = _search_item_by_id(nivel, id);

	if (item == NULL) {
		return NGUI_ITEM_NOT_FOUND;
	}

	return _cambiar_posicion(item, item->posx + offset_x, item->posy + offset_y);
}

int caja_quitar_recurso(NIVEL* nivel, char id) {
	ITEM_NIVEL* item = _search_item_by_id(nivel, id);

	if (item == NULL) {
		return NGUI_ITEM_NOT_FOUND;
	}

	if(item->item_type != CAJA_ITEM_TYPE) {
		return NGUI_ITEM_NOT_A_BOX;
	}

	if(item->quantity == 0) {
		return NGUI_ITEM_EMPTY_BOX;
	}

	item->quantity--;

	return NGUI_SUCCESS;
}

int caja_agregar_recurso(NIVEL* nivel, char id) {
	ITEM_NIVEL* item = _search_item_by_id(nivel, id);

	if (item == NULL) {
		return NGUI_ITEM_NOT_FOUND;
	}

	if(item->item_type != CAJA_ITEM_TYPE) {
		return NGUI_ITEM_NOT_A_BOX;
	}

	item->quantity++;

	return NGUI_SUCCESS;
}

bool items_chocan(NIVEL* nivel, char id1, char id2) {
	ITEM_NIVEL* item1 = _search_item_by_id(nivel, id1);
	ITEM_NIVEL* item2 = _search_item_by_id(nivel, id2);
	if(item1 == NULL || item2 == NULL) {
		return false;
	} else {
		return (item1->posx == item2->posx) && (item1->posy == item2->posy);
	}
}

int _crear_item(NIVEL* nivel, char id, int x , int y, int tipo, int cant_rec) {
	if(!_validar_posicion(x, y)) {
		return NGUI_ITEM_INVALID_POSITION;
	}

	if (_search_item_by_id(nivel, id) != NULL) {
		return NGUI_ITEM_ALREADY_EXISTS;
	}

	ITEM_NIVEL* item = malloc(sizeof(ITEM_NIVEL));

	item->id = id;
	item->posx=x;
	item->posy=y;
	item->item_type = tipo;
	item->quantity = cant_rec;

	list_add(nivel->items, item);

	return NGUI_SUCCESS;
}

ITEM_NIVEL* _search_item_by_id(NIVEL* nivel, char id) {
	bool _search_by_id(ITEM_NIVEL* item) {
		return item->id == id;
	}

	return list_find(nivel->items, (void*) _search_by_id);
}

int _cambiar_posicion(ITEM_NIVEL* item, int x, int y) {
	if (!_validar_posicion(x, y)) {
		return NGUI_ITEM_INVALID_POSITION;
	}

	item->posx = x;
	item->posy = y;

	return NGUI_SUCCESS;
}

bool _validar_posicion(int x, int y) {
	if(x < 0 || y < 0) {
		return false;
	}

	int columnas, filas;
	nivel_gui_get_area_nivel(&columnas, &filas);

	if(x >= columnas || y >= filas) {
		return false;
	}

	return true;
}
/*-----------------------------------------------------------------------------------*/

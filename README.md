# tp-2021-1c-No-C-Aprueba-+

### :point_right: CLONAR EL REPO PARADOS EN /home/utnso/workspace     
        
        git clone https://github.com/sisoputnfrba/tp-2021-1c-No-C-Aprueba-

### :point_right: File > Import > General > Existing Projects Into Workspace > tp-2021-1c-No-C-Aprueba-

### :point_right: SIEMPRE BUILDEAR PRIMERO LA CARPETA 'shared' Y DESPUÉS LOS 3 PROYECTOS PORQUE SINO NO RECONOCEN LAS LIBRERIAS

### :point_right: LEVANTAR PROCESO EN LA CONSOLA (CON LAS SHARED LIBRARY)
        
        LD_LIBRARY_PATH="/home/utnso/workspace/tp-2021-1c-No-C-Aprueba-/shared/Debug" ./Debug/'Proceso'

### :point_right: La función send() manda todo el bloque completo. La función recv() agarra el buffer que llegó y agarra los primeros 'n' bytes que se le definan, entonces puede haber varios recv() para un solo send()

### :point_right: Diagrama de flujo: https://lucid.app/lucidchart/08070e2d-21af-4143-b078-d4927ef03d9b/edit?shared=true&page=0_0#

### :policeman: MÓDULO DISCORDIADOR

Es el encargado de organizar, planificar y ordenar la ejecución de las tareas de los tripulantes. Se usan los conceptos de procesos e hilos. Tiene su propia consola, en la que se pueden mandar los mensajes INICIAR_PATOTA, INICIAR_PLANIFICACION, PAUSAR_PLANIFICACION, LISTAR_TRIPULANTES, EXPULSAR_TRIPULANTES y OBTENER_BITACORA. Después están los del Anexo II que no se quien los mandaría.

Los tripulantes se modelarían como hilos, osea cada vez que se crea un tripulante nuevo (el TCB), se crea un hilo para que puedan ser atendidos todos al mismo tiempo.

### :floppy_disk: MÓDULO MI-RAM-HQ

Es el encargado de almacenar toda la información relacionada a las instrucciones, PCB’s (Patota Control Block) y TCB’s (Tripulante Control Block). A su vez, se encargará de mantener un mapa actualizado con las posiciones de cada tripulante durante la ejecución de los mismos.

Al momento de inicializar el proceso se realizarán las siguientes operaciones:

        #Reservar el espacio de memoria indicado por el archivo de configuración.
        
        #Dibujar el mapa inicial vacío, dado de que al inicio no se encontrará ningún tripulante conectado.
        
        #Iniciar el servidor, al cual se conectarán los tripulantes para comenzar a ejecutar sus instrucciones.



### :file_folder: MÓDULO I-MONGO-STORE

Es el encargado de almacenar toda la información de manera persistente, análogamente a como funciona el File System de un sistema operativo real. Por lo tanto, será capaz de finalizar su ejecución y volver a reanudarse manteniendo la información.

La estructura básica del FS se basa en una estructura de árbol de directorios para representar la información administrativa y los datos en formato de archivos. El árbol de directorios tomará su punto de partida del punto de montaje del archivo de configuración.
Durante las pruebas no se proveerán archivos que tengan estados inconsistentes respecto del trabajo práctico, por lo que no es necesario tomar en cuenta dichos casos.
Para este file system, como lo que se busca es generar los recursos que se van a utilizar en la nave, cada archivo va a tener un caracter de llenado el cual va a ser el único que deba encontrarse dentro de un archivo y el cual se van a incrementar o decrementar por medio de las tareas ejecutadas por los tripulantes. Ejemplo: En el caso de tener 5 oxígenos el archivo deberá tener “OOOOO” en su contenido.

![Arquitectura TP](https://user-images.githubusercontent.com/49170861/115929112-9a259480-a45d-11eb-854d-e409a32410a9.jpg)

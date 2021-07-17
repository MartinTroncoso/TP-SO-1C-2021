# tp-2021-1c-No-C-Aprueba-+

* Adri: I-MONGO-STORE
* Paul: MI-RAM-HQ
* Martín: Discordiador
* Gastón: I-MONGO-STORE y Discordiador
* Lean: MI-RAM-HQ

### :point_right: DEPLOY
* Crear en el directorio /home/utnso la carpeta 'workspace'.
* Hacer 'cd workspace' y clonar tanto este repo como el de las commons (https://github.com/sisoputnfrba/so-commons-library).
* Una vez clonado todo, hacer 'cd so-commons-library'. Después hacer 'sudo make install'
* El que corra el Discordiador tiene que clonar las tareas (https://github.com/sisoputnfrba/a-mongos-pruebas)
* Cada uno se para en el Debug de un proceso diferente (Discordiador/MI-RAM/I-MONGO) y hacer 'make all' para compilarlo
* Escribir 'export LD_LIBRARY_PATH=/home/utnso/workspace/tp-2021-1c-No-C-Aprueba-/shared/Debug' (para la shared)
* Correr cada uno su proceso y que rompa todo :trollface:

![Arquitectura TP](https://user-images.githubusercontent.com/49170861/115929112-9a259480-a45d-11eb-854d-e409a32410a9.jpg)

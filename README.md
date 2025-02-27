# Proyecto Buscador de Películas
### El link del video del proyecto se encuentra al final de este README
## Descripcion General

Este proyecto es una plataforma de streaming desarrollada en C++ que permite a los usuarios buscar películas, gestionar listas personalizadas (como “Ver más tarde” y “Películas a las que di Like”), y obtener recomendaciones basadas en sus gustos. Para lograr una experiencia fluida y eficiente, se han implementado diversas técnicas y patrones de diseño, entre ellos un árbol de sufijos (con algoritmo de Ukkonen), Singleton, Observer, Memento y Strategy. Además, se utiliza programación concurrente para acelerar ciertos procesos de búsqueda.

## Indice de Contenidos

1. Árbol de Sufijos: ¿Por qué y cómo?
2. Clases Importantes: Función y Diseño
3. Patrones de Diseño Utilizados
4. Programación Concurrente
5. Requerimientos del Sistema
6. Bibliografía y Recursos Recomendados
7. Interfaz principal del programa

## Árbol de Sufijos: ¿Por qué y cómo?

### ¿Por qué se utiliza?

- **Busqueda Eficiente:** El árbol de sufijos permite realizar búsquedas de subcadenas en tiempo casi lineal respecto al tamaño de la cadena, lo cual es crucial al buscar coincidencias en grandes volúmenes de texto (títulos y sinopsis de películas).
- **Indexacion Global:** Al concatenar los títulos y sinopsis de todas las películas en un único texto global (separado por un caracter especial, como #), se puede indexar y mapear rápidamente cada posición a la película correspondiente mediante un vector de mapeo.

### ¿Cómo se implementa?

- **Algoritmo de Ukkonen:** Se emplea una versión simplificada del algoritmo de Ukkonen para construir el árbol de sufijos de la cadena global. Esto incluye:
    - **Extensión Incremental:** Se agrega cada nuevo carácter y se actualizan los nodos del árbol.
    - **Uso de Sufijos Pendientes y Enlaces Sufijos:** Permite optimizar la construcción y reducir la complejidad.
    - **Propagación de Índices:** Cada nodo almacena (y luego propaga) un conjunto de índices que indica a qué películas corresponde el fragmento de texto representado en ese nodo.

Esta estructura no solo mejora la rapidez en las búsquedas, sino que también permite hacer búsquedas "inteligentes" (por ejemplo, extrayendo fragmentos relevantes de la sinopsis cuando se encuentra una coincidencia).


[![](https://mermaid.ink/img/pako:eNptks9ugkAQxl_F7EkTNOwfKNKkSdkFe-ml9dTSw0ZWJREwFJKq8ZH6FH2xjow22U05EPa3k-_7mJkTWTWFITHZtHq_HS3VfV6P4Hkcj1_0z_dxMhlNpw-j5D0ny5x8XC8HJoGVDlPAOoelwLTDMmC1wxb_6D0BWw0MaYJpqFWaUKTMMk8YUm7ZJxypsAIkAmlg6wZIw78QF4pviS2glqXEIJJZlhKDSG5ZSgwihWUpMYgMnP9W2F1qCSu0U8wSVminuCWs0E4JRzjFEVFLIkXhlFkSKQqn3JHIcKL2TDKUyJhTvMBR0ysmHqlMW-mygBU8XUqgn1tTmZzE8FmYte530OC8PkOp7rvm9VCvSNy1vfFI2_Sb7e3Q7wvdGVVq2OTqBve6fmsaOK717hPOpii7pn3GlR82f6gh8Yl8kXhK7yI282kkwnDOhE8Z98gBOI9mwmeCz--Aw0149shx0OUzGjLGA5_RYE5pFAbnX0Oh2e0?type=png)](https://mermaid.live/edit#pako:eNptks9ugkAQxl_F7EkTNOwfKNKkSdkFe-ml9dTSw0ZWJREwFJKq8ZH6FH2xjow22U05EPa3k-_7mJkTWTWFITHZtHq_HS3VfV6P4Hkcj1_0z_dxMhlNpw-j5D0ny5x8XC8HJoGVDlPAOoelwLTDMmC1wxb_6D0BWw0MaYJpqFWaUKTMMk8YUm7ZJxypsAIkAmlg6wZIw78QF4pviS2glqXEIJJZlhKDSG5ZSgwihWUpMYgMnP9W2F1qCSu0U8wSVminuCWs0E4JRzjFEVFLIkXhlFkSKQqn3JHIcKL2TDKUyJhTvMBR0ysmHqlMW-mygBU8XUqgn1tTmZzE8FmYte530OC8PkOp7rvm9VCvSNy1vfFI2_Sb7e3Q7wvdGVVq2OTqBve6fmsaOK717hPOpii7pn3GlR82f6gh8Yl8kXhK7yI282kkwnDOhE8Z98gBOI9mwmeCz--Aw0149shx0OUzGjLGA5_RYE5pFAbnX0Oh2e0)
*Imagen referencial sobre como se forma el árbol de sufijos*

## Clases Importantes: Función y Diseño

### 1. Pelicula

- **Función:** Representa la información de cada película, incluyendo título, sinopsis, etiquetas, fuente y un contador de "likes".
- **Por qué y cómo:** Se utiliza como unidad básica de datos, facilitando la manipulación y visualización de información en el sistema.

### 2. BaseDeDatos (Patrón Singleton – Clase Plantilla)

- **Función:** Gestiona la carga y almacenamiento de la base de datos de películas desde un archivo CSV.
- **¿Por qué?:**
    - **Singleton:** Garantiza que solo exista una única instancia de la base de datos en toda la aplicación, evitando inconsistencias.
    - **Clase Plantilla:** Hace que la clase sea genérica y reutilizable para diferentes tipos de datos (por ejemplo, películas, series, usuarios, etc.).
- **¿Cómo?:**
    - Se define la clase como template (**template<typename T>**) y se utiliza un puntero estático para almacenar la única instancia.
    - El constructor es privado y se accede a la instancia mediante un método estático **obtenerInstancia()**.
    - Se utiliza un puntero a función para cargar datos genéricos desde un archivo, permitiendo que la clase funcione con cualquier tipo de dato.

### 3. EstrategiaBusqueda y Subclases (Strategy)

- **Función:** Permite implementar distintas estrategias de búsqueda (por ejemplo, búsqueda en título y sinopsis versus búsqueda por etiquetas).
- **Por qué y cómo:**
    - **Por qué:** Para separar y encapsular algoritmos de búsqueda que pueden cambiar o extenderse sin modificar el código cliente.
    - **Cómo:** Se define una clase abstracta **EstrategiaBusqueda** con un método virtual ****buscar()**, y se implementan dos estrategias concretas:
        - **EstrategiaTituloSinopsis:** Utiliza el árbol de sufijos y un fallback a búsqueda lineal para encontrar coincidencias en títulos y sinopsis.
        - **EstrategiaEtiqueta:** Paraleliza la búsqueda en etiquetas usando **async** para aprovechar múltiples hilos.

### ObservadorRecomendacion (Patrón Observer)

- **Función:** Actualiza automáticamente la lista de recomendaciones cuando se producen cambios en la lista de películas gustadas.
- **Por qué y cómo:**
    - **Por qué:** Para mantener actualizadas las sugerencias basadas en las interacciones del usuario (por ejemplo, al dar "like" a una película).
    - **Cómo:** Implementa la interfaz **Observador** y se registra para que, tras cada acción del usuario, se invoque el método **actualizar()** que recalcula las recomendaciones.
    
## Patrones de Diseño Utilizados

- **Singleton:**
    - Clase: **BaseDeDatos**
    - *Proposito:* Garantizar una única instancia de la base de datos durante la vida del programa, centralizando el acceso a la información.
- **Observer:** 
    - Clase: **ObservadorRecomendacion**
    - *Proposito:* Permitir la actualización automática de recomendaciones cada vez que se modifica la lista de películas gustadas, facilitando una respuesta dinámica ante cambios.
- **Memento:**
    - Clases: **MementoBusqueda** y **CuidadorHistorialBusquedas**
    - *Proposito:* Registrar el historial de búsquedas del usuario sin violar la encapsulación, permitiendo una gestión flexible del mismo.
- **Strategy:** 
    - Clases: **EstrategiaBusqueda**, **EstrategiaTituloSinopsis** y **EstrategiaEtiqueta**
    - *Proposito:* Ofrecer múltiples métodos de búsqueda (por contenido textual o por etiquetas) que pueden cambiarse o extenderse sin alterar la lógica del programa.

## Programación Concurrente

### ¿Dónde se utiliza?

- **Búsqueda por Etiquetas:** En la implementación de la estrategia de búsqueda por etiquetas se emplea async para dividir la tarea entre varios hilos. Esto se hace para:
    - Mejorar el rendimiento en búsquedas sobre grandes volúmenes de datos.
    - Procesar bloques de películas en paralelo (definidos por la constante **NUM_HILOS**).

### ¿Por qué y cómo?

- **Por qué:** La búsqueda por etiquetas puede ser costosa en términos computacionales cuando el dataset es extenso. Al dividir la carga entre varios hilos, se reduce el tiempo total de procesamiento.
- **Cómo:** 
    - Se calcula el tamaño de bloque dividiendo el total de películas por el número de hilos.
    - Cada hilo procesa su bloque y devuelve un conjunto parcial de resultados mediante **future**.
    - Los resultados parciales se combinan y se ordenan según la relevancia (puntaje).

**Tabla 1:** Comparación de tiempos al usar distintos números de hilos en la “**Búsqueda por Etiquetas**”

Se buscó la película: “**Kung Fu Panda**” por sus etiquetas: “**comedy, cute, violence, clever, good versus evil, flashback, absurd, action, entertaining**”

| **Número de hilos**  | **Tiempo** |
| -------- | ------- |
| 1  | 0.0192272 segundos |
| 2 | 0.0180095 segundos |
| 4 **(ideal)-Utilizado en la versión final del código**    | 0.0094404 segundos    |
| 8 | 0.0086066 segundos. |

**Búsqueda por Título y Sinopsis:** Aunque es posible paralelizarla, se optó por no hacerlo porque el árbol de sufijos ya optimiza significativamente la búsqueda, evitando complicaciones de sincronización y reduciendo beneficios adicionales.

## Requerimientos del Sistema

### Hardware Recomendado

- **Procesador:** Los CPU recomendados minimo son; para Windows y Linux Intel una I5 y para Ryzen es recomendado una Rizen 5; para macOS es recomendado una M1 (mínimo 2 núcleos, 4 núcleos o más recomendado para aprovechar la concurrencia).
- **Memoria RAM:**  8 GB o más para grandes volúmenes de datos.
- **Almacenamiento:** Espacio suficiente para almacenar el archivo CSV y cualquier archivo de salida.

### Software

- **Sistema Operativo:** Linux, Windows o macOS.

- **IDE y Herramientas de Desarrollo:**
    - **CLion** (recomendado, versión 2021.3 o superior) o cualquier otro IDE compatible con C++.
    - **Visual Code Studio** C++ version 1.23.6, compiler g++
    - Compilador compatible con C++11 (o superior), como GCC (v9.0+), Clang o MSVC.

## Bibliografía y Recursos Recomendados

- MIT OpenCourseWare, Erik Demaine, 2012. [MIT OpenCourseWare 16. Strings](https://www.youtube.com/watch?v=NinWEPPrkDQ).
- Wikipedia contributors. (2025, 24 enero). [Wikipedia Suffix tree](https://en.wikipedia.org/wiki/Suffix_tree)
- Ukkonen, E. (1995), 14(3), 249-260. [On-line construction of suffix trees. Algorithmica](https://doi.org/10.1007/bf01206331)

## Interfaz principal del programa

Una vez compilado, se puede ejecutar el programa desde la terminal. El menú principal ofrece las siguientes opciones:

1. Buscar películas.
2. Ver recomendaciones.
3. Ver lista de “Ver más tarde”.
4. Ver películas a las que se dio “Like”.
5. Ver historial de búsquedas.
6. Salir del programa.

## Conclusiones

Este proyecto demuestra el uso combinado de algoritmos avanzados, patrones de diseño y programación concurrente para crear una plataforma eficiente y modular. La integración del árbol de sufijos permite búsquedas rápidas en grandes volúmenes de texto, mientras que los patrones como Singleton, Observer, Memento y Strategy facilitan un diseño robusto y flexible.
Por último, en caso estés interesado en ampliar o modificar el comportamiento del sistema, lo más recomendado sería explorar en detalle cada uno de estos patrones y técnicas usadas en el programa, y dicho sea de paso, consultar la bibliografía adjunta para una mayor comprensión.


# Link del video (solo admite a personas que pertenezcan a la institución de UTEC): https://drive.google.com/drive/folders/1vyGaaT1sVQD2-Le7CsoUV6qiUdd3PI47?usp=sharing
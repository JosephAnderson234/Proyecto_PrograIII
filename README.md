# Proyecto Buscador de Películas

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
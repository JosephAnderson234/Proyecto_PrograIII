#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <unordered_map>
#include <set>
#include <algorithm>
#include <cctype>
#include <limits>
#include <future>
#include <chrono>
using namespace std;

// -------------------- CONSTANTES GLOBALES --------------------
const int ANCHO = 80;           // Ancho para el recuadro de la sinopsis
const int ANCHO_TITULO = 60;      // Ancho para justificar los titulos
int modoBusquedaGlobal = 1;       // 1: Titulo y sinopsis, 2: Etiqueta
const int NUM_HILOS = 4;        // Numero de hilos para busquedas paralelas (modificable manualmente)

// -------------------- DECLARACION ANTICIPADA --------------------
class ArbolSufijosUkkonen;   // Forward declaration

// -------------------- ESTRUCTURA PELICULA --------------------
struct Pelicula {
    string titulo;
    string sinopsis;
    vector<string> etiquetas;
    string fuente;     // Fuente de la sinopsis (columna 6 del CSV)
    int likes = 0;     // 0 o 1; se permite un unico like.
};

// -------------------- DECLARACIONES DE FUNCIONES AUXILIARES --------------------
string aMinusculas(const string &s);
string recortar(const string &s);
int contarComillas(const string &s);
vector<string> parsearLineaCSV(const string &linea);
vector<string> tokenizar(const string &s);
string normalizarEspacios(const string &s);
vector<string> justificarTexto(const string &texto, int ancho);
void imprimirCuadro(const string &texto, int ancho);
void imprimirTituloJustificado(const string &titulo);
string extraerFragmento(const string &sinopsis, const string &consulta);


// -------------------- DECLARACIONES DE FUNCIONES DE CARGA E INDICES --------------------
vector<Pelicula> cargarPeliculas(const string &nombreArchivo);
unordered_map<string, set<Pelicula*>> construirIndice(const vector<Pelicula>& peliculas);
unordered_map<string, set<Pelicula*>> construirIndiceEtiquetas(const vector<Pelicula>& peliculas);
vector<Pelicula*> recomendarPeliculas(const vector<Pelicula>& peliculas, const vector<Pelicula*>& gustadas);

// -------------------- DECLARACIONES DE FUNCIONES DE IMPRESION Y MENU --------------------

void mostrarListaTitulos(const vector<Pelicula*>& lista);
// Ahora, el submenú de pelicula retorna un bool: true si el usuario desea volver directamente al menu principal.
bool submenuPelicula(Pelicula* seleccionada, vector<Pelicula*>& gustadas, vector<Pelicula*>& verMasTarde);
void manejarLista(const vector<Pelicula*>& lista, const string &nombreLista, vector<Pelicula*>& gustadas, vector<Pelicula*>& verMasTarde);
void manejarBusqueda(vector<Pelicula>& peliculas,
                    unordered_map<string, set<Pelicula*>> &indiceMode1,
                    unordered_map<string, set<Pelicula*>> &indiceEtiqueta,
                    vector<Pelicula*>& gustadas,
                    vector<Pelicula*>& verMasTarde);
void manejarHistorialBusquedas();


// -------------------- PATRON MEMENTO: HISTORIAL DE BUSQUEDAS --------------------
class MementoBusqueda {
    public:
    string consulta;
    int modoBusqueda;
    MementoBusqueda(const string &c, int m) : consulta(c), modoBusqueda(m) {}
};

class CuidadorHistorialBusquedas {
private:
    vector<MementoBusqueda> historial;
public:
    void agregarMemento(const MementoBusqueda &m) {
        historial.push_back(m);
    }
    const vector<MementoBusqueda>& obtenerHistorial() const {
        return historial;
    }
    void eliminarMemento(int indice) {
        if (indice >= 0 && indice < (int)historial.size())
            historial.erase(historial.begin() + indice);
    }
    void borrarHistorial() {
        historial.clear();
    }
    void mostrarHistorial() const {
        if (historial.empty()){
            cout << "\nNo hay historial de busquedas." << endl;
            return;
        }
        cout << "\n--- Historial de Busquedas ---" << endl;
        for (int i = 0; i < historial.size(); i++){
            cout << i+1 << ". Consulta: \"" << historial[i].consulta
                 << "\" (Modo: " << (historial[i].modoBusqueda==1 ? "Titulo y sinopsis" : "Etiqueta") << ")" << endl;
        }
    }
};

CuidadorHistorialBusquedas cuidadorHistorial;

// -------------------- PATRON SINGLETON: BASE DE DATOS --------------------
class BaseDeDatos {
    private:
        static BaseDeDatos* instancia;
        vector<Pelicula> peliculas;
        BaseDeDatos(const string &archivo) {
            peliculas = cargarPeliculas(archivo);
        }
    public:
        static BaseDeDatos* obtenerInstancia(const string &archivo) {
            if (!instancia)
                instancia = new BaseDeDatos(archivo);
            return instancia;
        }
        vector<Pelicula>& obtenerPeliculas() {
            return peliculas;
        }
    };
    BaseDeDatos* BaseDeDatos::instancia = nullptr;

// -------------------- FUNCIONES AUXILIARES --------------------
// Convierte una cadena a minusculas.
string aMinusculas(const string &s) {
    string out = s;
    transform(out.begin(), out.end(), out.begin(), ::tolower);
    return out;
}

// Elimina espacios y comillas al inicio y al final.
string recortar(const string &s) {
    size_t inicio = s.find_first_not_of(" \"");
    size_t fin = s.find_last_not_of(" \"");
    if (inicio == string::npos || fin == string::npos)
        return "";
    return s.substr(inicio, fin - inicio + 1);
}

// Cuenta el numero de comillas (") en una cadena.
int contarComillas(const string &s) {
    int cuenta = 0;
    for (char c : s)
        if (c == '\"')
            cuenta++;
    return cuenta;
}

// Parsea una linea de CSV considerando comillas.
vector<string> parsearLineaCSV(const string &linea) {
    vector<string> resultado;
    string campo;
    bool enComillas = false;
    for (size_t i = 0; i < linea.size(); i++) {
        char c = linea[i];
        if (c == '\"')
            enComillas = !enComillas;
        else if (c == ',' && !enComillas) {
            resultado.push_back(recortar(campo));
            campo.clear();
        } else {
            campo.push_back(c);
        }
    }
    resultado.push_back(recortar(campo));
    return resultado;
}

// Tokeniza una cadena en palabras.
// Se conservan el guion '-' y, ahora, los caracteres ':' y '/' cuando esten entre digitos.
vector<string> tokenizar(const string &s) {
    vector<string> tokens;
    string token;
    for (size_t i = 0; i < s.size(); i++) {
        char c = s[i];
        if ((c == ':' || c == '/') && i > 0 && i < s.size()-1 &&
            isdigit(s[i-1]) && isdigit(s[i+1])) {
            token.push_back(c);
        }
        else if (isalnum(static_cast<unsigned char>(c)) || c == '-') {
            token.push_back(tolower(c));
        } else {
            if (!token.empty()) {
                tokens.push_back(token);
                token.clear();
            }
        }
    }
    if (!token.empty())
        tokens.push_back(token);
    return tokens;
}

// Normaliza los espacios en una cadena: deja solo un espacio entre palabras.
string normalizarEspacios(const string &s) {
    istringstream iss(s);
    string palabra, resultado;
    while (iss >> palabra) {
        if (!resultado.empty())
            resultado += " ";
        resultado += palabra;
    }
    return resultado;
}

// -------------------- FUNCION PARA CARGAR PELICULAS --------------------
// Carga las peliculas desde un CSV (respetando registros multilinea).
// Se separan los etiquetas usando coma como delimitador para conservar etiquetas compuestos.
vector<Pelicula> cargarPeliculas(const string &nombreArchivo) {
    vector<Pelicula> peliculas;
    ifstream archivo(nombreArchivo);
    string linea;
    if (!archivo.is_open()) {
        cerr << "Error al abrir el archivo." << endl;
        return peliculas;
    }
    getline(archivo, linea); // Descartar encabezado.
    while (getline(archivo, linea)) {
        while (contarComillas(linea) % 2 != 0 && !archivo.eof()) {
            string siguienteLinea;
            getline(archivo, siguienteLinea);
            linea += "\n" + siguienteLinea;
        }
        vector<string> columnas = parsearLineaCSV(linea);
        if (columnas.size() < 6)
            continue;
        Pelicula pelicula;
        pelicula.titulo = columnas[1];
        pelicula.sinopsis = columnas[2];
        {
            // Separar la columna de etiquetas usando coma como delimitador.
            istringstream iss(columnas[3]);
            string etiquetas;
            while (getline(iss, etiquetas, ',')) {
                etiquetas = recortar(etiquetas);
                if (!etiquetas.empty())
                    pelicula.etiquetas.push_back(etiquetas);
            }
        }
        pelicula.fuente = columnas[5]; // Fuente de la sinopsis.
        peliculas.push_back(pelicula);
    }
    archivo.close();
    return peliculas;
}

// Construye un indice invertido para busquedas por titulo y sinopsis.
unordered_map<string, set<Pelicula*>> construirIndice(const vector<Pelicula>& peliculas) {
    unordered_map<string, set<Pelicula*>> indice;
    for (const auto &pelicula : peliculas) {
        vector<string> tokensTitulo = tokenizar(pelicula.titulo);
        vector<string> tokensSinopsis = tokenizar(pelicula.sinopsis);
        for (const auto &palabra : tokensTitulo)
            indice[palabra].insert(const_cast<Pelicula*>(&pelicula));
        for (const auto &palabra : tokensSinopsis)
            indice[palabra].insert(const_cast<Pelicula*>(&pelicula));
    }
    return indice;
}

// Construye un indice para busquedas por etiqueta.
unordered_map<string, set<Pelicula*>> construirIndiceEtiquetas(const vector<Pelicula>& peliculas) {
    unordered_map<string, set<Pelicula*>> indiceEtiquetas;
    for (const auto &pelicula : peliculas) {
        for (const auto &etiqueta : pelicula.etiquetas) {
            string etiquetaLower = aMinusculas(etiqueta);
            indiceEtiquetas[etiquetaLower].insert(const_cast<Pelicula*>(&pelicula));
        }
    }
    return indiceEtiquetas;
}

// Justifica un texto en un ancho fijo y retorna un vector de lineas.
vector<string> justificarTexto(const string &texto, int ancho) {
    istringstream iss(texto);
    vector<string> palabras;
    string palabra;
    while (iss >> palabra)
        palabras.push_back(palabra);
    vector<string> lineas;
    int n = palabras.size();
    int i = 0;
    while (i < n) {
        int j = i;
        int suma = palabras[j].size();
        while (j + 1 < n && suma + 1 + palabras[j+1].size() <= ancho) {
            j++;
            suma += 1 + palabras[j].size();
        }
        int numPalabras = j - i + 1;
        string linea;
        if (numPalabras == 1 || j == n - 1) {
            linea = palabras[i];
            for (int k = i + 1; k <= j; k++)
                linea += " " + palabras[k];
            while ((int)linea.size() < ancho)
                linea += " ";
        } else {
            int totalLetras = 0;
            for (int k = i; k <= j; k++)
                totalLetras += palabras[k].size();
            int totalEspacios = ancho - totalLetras;
            int gaps = numPalabras - 1;
            int espacioBase = totalEspacios / gaps;
            int extra = totalEspacios % gaps;
            for (int k = i; k < j; k++) {
                linea += palabras[k];
                int espacios = espacioBase + ((k - i) < extra ? 1 : 0);
                linea += string(espacios, ' ');
            }
            linea += palabras[j];
        }
        lineas.push_back(linea);
        i = j + 1;
    }
    return lineas;
}

// Imprime un recuadro con el texto justificado en un ancho fijo.
void imprimirCuadro(const string &texto, int ancho) {
    vector<string> lineas = justificarTexto(texto, ancho);
    cout << "+" << string(ancho, '-') << "+" << endl;
    for (const auto &linea : lineas)
        cout << "|" << linea << "|" << endl;
    cout << "+" << string(ancho, '-') << "+" << endl;
}

// Imprime un titulo justificado en un ancho fijo (sin recuadro).
void imprimirTituloJustificado(const string &titulo) {
    vector<string> lineas = justificarTexto(titulo, ANCHO_TITULO);
    for (const auto &linea : lineas)
        cout << linea << endl;
}

// Extrae un snippet (10 palabras) de la sinopsis, a partir de la primera ocurrencia de la consulta.
string extraerFragmento(const string &sinopsis, const string &consulta) {
    string sinopsisLower = aMinusculas(sinopsis);
    string consultaLower = aMinusculas(consulta);
    size_t pos = sinopsisLower.find(consultaLower);
    if (pos == string::npos)
        return "";
    string sub = sinopsis.substr(pos);
    istringstream iss(sub);
    string snippet, palabra;
    int cuenta = 0;
    while (iss >> palabra && cuenta < 10) {
         snippet += palabra + " ";
         cuenta++;
    }
    if (!snippet.empty())
         snippet.pop_back();
    return snippet;
}


// Genera recomendaciones basadas en los gustados.
// Se omiten las peliculas ya gustadas y se ordenan por puntaje segun coincidencia de etiquetas.
vector<Pelicula*> recomendarPeliculas(const vector<Pelicula>& peliculas, const vector<Pelicula*>& gustadas) {
    set<string> etiquetasGustadas;
    for (auto pelicula : gustadas)
        for (auto &etiqueta : pelicula->etiquetas)
            etiquetasGustadas.insert(etiqueta);
    vector<pair<Pelicula*, int>> puntajes;
    for (auto &pelicula : peliculas) {
        bool yaGustada = false;
        for (auto pelGustada : gustadas) {
            if (pelGustada == &pelicula) {
                yaGustada = true;
                break;
            }
        }
        if (yaGustada)
            continue;
        int puntaje = 0;
        for (auto &etiqueta : pelicula.etiquetas)
            if (etiquetasGustadas.count(etiqueta))
                puntaje++;
        if (puntaje > 0)
            puntajes.push_back(make_pair(const_cast<Pelicula*>(&pelicula), puntaje));
    }
    sort(puntajes.begin(), puntajes.end(), [](auto &a, auto &b){ return a.second > b.second; });
    vector<Pelicula*> recomendadas;
    for (size_t i = 0; i < puntajes.size() && i < 5; i++)
        recomendadas.push_back(puntajes[i].first);
    return recomendadas;
}


// -------------------- ARBOL DE SUFIJOS CON ALGORITMO DE UKKONEN --------------------
// Nota: Esta implementacion es una version simplificada.
class ArbolSufijosUkkonen {
    public:
    struct Nodo {
        unordered_map<char, Nodo*> hijos;
        int inicio;
        int* fin;
        Nodo* enlaceSufijo;
        set<int> indicesPeliculas;
        int indiceSufijo;
        Nodo(int inicio, int* fin) : inicio(inicio), fin(fin), enlaceSufijo(nullptr), indiceSufijo(-1) {}
    };

    string texto;
    Nodo* raiz;
    Nodo* ultimoNodoNuevo;
    Nodo* nodoActivo;
    int aristaActiva;
    int longitudActiva;
    int sufijosPendientes;
    int finHoja;
    int tamano; // Longitud de texto
    vector<int> mapeoPosPeliculas;

    ArbolSufijosUkkonen(const string& txt, const vector<int>& mapeo) : texto(txt), mapeoPosPeliculas(mapeo) {
        tamano = texto.size();
        raiz = new Nodo(-1, new int(-1));
        nodoActivo = raiz;
        aristaActiva = -1;
        longitudActiva = 0;
        sufijosPendientes = 0;
        finHoja = -1;
        ultimoNodoNuevo = nullptr;
        construirArbol();
        asignarIndiceSufijoDFS(raiz, 0);
        propagarIndices(raiz);
    }

    void construirArbol() {
        for (int i = 0; i < tamano; i++) {
            extenderArbol(i);
        }
    }

    void extenderArbol(int pos) {
        finHoja = pos;
        sufijosPendientes++;
        ultimoNodoNuevo = nullptr;
        while (sufijosPendientes > 0) {
            if (longitudActiva == 0)
                aristaActiva = pos;
            char cAct = texto[aristaActiva];
            if (nodoActivo->hijos.find(cAct) == nodoActivo->hijos.end()) {
                nodoActivo->hijos[cAct] = new Nodo(pos, new int(finHoja));
                nodoActivo->hijos[cAct]->indicesPeliculas.insert(mapeoPosPeliculas[pos]);
                if (ultimoNodoNuevo != nullptr) {
                    ultimoNodoNuevo->enlaceSufijo = nodoActivo;
                    ultimoNodoNuevo = nullptr;
                }
            } else {
                Nodo* siguiente = nodoActivo->hijos[cAct];
                int largoArista = *(siguiente->fin) - siguiente->inicio + 1;
                if (longitudActiva >= largoArista) {
                    aristaActiva += largoArista;
                    longitudActiva -= largoArista;
                    nodoActivo = siguiente;
                    continue;
                }
                if (texto[siguiente->inicio + longitudActiva] == texto[pos]) {
                    if (ultimoNodoNuevo != nullptr && nodoActivo != raiz) {
                        ultimoNodoNuevo->enlaceSufijo = nodoActivo;
                        ultimoNodoNuevo = nullptr;
                    }
                    longitudActiva++;
                    break;
                }
                int* finDividir = new int(siguiente->inicio + longitudActiva - 1);
                Nodo* nodoDividir = new Nodo(siguiente->inicio, finDividir);
                nodoActivo->hijos[cAct] = nodoDividir;
                nodoDividir->hijos[texto[pos]] = new Nodo(pos, new int(finHoja));
                nodoDividir->hijos[texto[pos]]->indicesPeliculas.insert(mapeoPosPeliculas[pos]);
                siguiente->inicio += longitudActiva;
                nodoDividir->hijos[texto[siguiente->inicio]] = siguiente;
                if (ultimoNodoNuevo != nullptr) {
                    ultimoNodoNuevo->enlaceSufijo = nodoDividir;
                }
                ultimoNodoNuevo = nodoDividir;
            }
            sufijosPendientes--;
            if (nodoActivo == raiz && longitudActiva > 0) {
                longitudActiva--;
                aristaActiva = pos - sufijosPendientes + 1;
            } else if (nodoActivo != raiz) {
                nodoActivo = nodoActivo->enlaceSufijo ? nodoActivo->enlaceSufijo : raiz;
            }
        }
    }

    void asignarIndiceSufijoDFS(Nodo* n, int alturaEtiqueta) {
        if (n == nullptr) return;
        bool esHoja = true;
        for (auto &par : n->hijos) {
            esHoja = false;
            asignarIndiceSufijoDFS(par.second, alturaEtiqueta + *(par.second->fin) - par.second->inicio + 1);
        }
        if (esHoja) {
            n->indiceSufijo = tamano - alturaEtiqueta;
            if(n->indiceSufijo >= 0 && n->indiceSufijo < mapeoPosPeliculas.size())
                n->indicesPeliculas.insert(mapeoPosPeliculas[n->indiceSufijo]);
        }
    }

    void propagarIndices(Nodo* n) {
        if (n == nullptr) return;
        for (auto &par : n->hijos) {
            propagarIndices(par.second);
            n->indicesPeliculas.insert(par.second->indicesPeliculas.begin(), par.second->indicesPeliculas.end());
        }
    }

    // Busqueda: recorre el arbol y devuelve el conjunto de indices de peliculas.
    set<int> buscar(const string& patron) {
        Nodo* nAct = raiz;
        int i = 0;
        while (i < patron.size()) {
            if (nAct->hijos.find(patron[i]) == nAct->hijos.end())
                return {};
            Nodo* sig = nAct->hijos[patron[i]];
            int largoArista = *(sig->fin) - sig->inicio + 1;
            int j = 0;
            while (j < largoArista && i < patron.size()) {
                if (texto[sig->inicio + j] != patron[i])
                    return {};
                i++; j++;
            }
            nAct = sig;
        }
        return nAct->indicesPeliculas;
    }
};

// -------------------- PATRON STRATEGY: ESTRATEGIA DE BUSQUEDA --------------------
class EstrategiaBusqueda {
public:
    virtual vector<pair<Pelicula*, int>> buscar(vector<Pelicula>& peliculas, const string &consulta) = 0;
    virtual ~EstrategiaBusqueda() {}
};

class EstrategiaTituloSinopsis : public EstrategiaBusqueda {
public:
    vector<pair<Pelicula*, int>> buscar(vector<Pelicula>& peliculas, const string &consulta) override {
        string consultaLower = aMinusculas(consulta);
        string consultaNorm = normalizarEspacios(consultaLower);
        // Uso del arbol de sufijos
        extern ArbolSufijosUkkonen* arbolSufijosGlobal;
        set<int> indicesCoincidentes = arbolSufijosGlobal->buscar(consultaLower);
        // Fallback: union con busqueda lineal
        for (int i = 0; i < peliculas.size(); i++) {
            string titLower = aMinusculas(peliculas[i].titulo);
            string sinopLower = aMinusculas(peliculas[i].sinopsis);
            if (titLower.find(consultaLower) != string::npos || sinopLower.find(consultaLower) != string::npos)
                indicesCoincidentes.insert(i);
        }
        vector<pair<Pelicula*, int>> resultados;
        for (int idx : indicesCoincidentes) {
            Pelicula &pel = peliculas[idx];
            string titLower = aMinusculas(pel.titulo);
            string titNorm = normalizarEspacios(titLower);
            string sinopLower = aMinusculas(pel.sinopsis);
            int puntaje = 0;
            if (titLower.find(consultaLower) != string::npos) {
                puntaje += 3;
                if (titNorm == consultaNorm)
                    puntaje += 50;
            }
            if (sinopLower.find(consultaLower) != string::npos)
                puntaje += 2;
            if (puntaje > 0)
                resultados.push_back(make_pair(&pel, puntaje));
        }
        sort(resultados.begin(), resultados.end(), [](auto &a, auto &b){ return a.second > b.second; });
        return resultados;
    }
};

class EstrategiaEtiqueta : public EstrategiaBusqueda {
public:
    vector<pair<Pelicula*, int>> buscar(vector<Pelicula>& peliculas, const string &consulta) override {
        vector<string> etiquetasConsulta;
        if (consulta.find(',') != string::npos) {
            istringstream iss(consulta);
            string token;
            while(getline(iss, token, ',')) {
                token = recortar(token);
                if (!token.empty())
                    etiquetasConsulta.push_back(aMinusculas(token));
            }
        } else {
            etiquetasConsulta = tokenizar(consulta);
        }
        vector<future<vector<pair<Pelicula*, int>>>> futuros;
        size_t total = peliculas.size();
        size_t numHilos = NUM_HILOS;
        size_t tamBloque = total / numHilos;
        for (size_t i = 0; i < numHilos; i++) {
            size_t inicio = i * tamBloque;
            size_t fin = (i == numHilos - 1) ? total : (i + 1) * tamBloque;
            futuros.push_back(async(launch::async, [&, inicio, fin, etiquetasConsulta]() {
                vector<pair<Pelicula*, int>> parcial;
                for (size_t j = inicio; j < fin; j++) {
                    Pelicula& pel = peliculas[j];
                    bool valido = true;
                    for (auto &qt : etiquetasConsulta) {
                        bool encontrado = false;
                        for (auto &etiqueta : pel.etiquetas) {
                            if (aMinusculas(etiqueta).find(qt) != string::npos) {
                                encontrado = true;
                                break;
                            }
                        }
                        if (!encontrado) {
                            valido = false;
                            break;
                        }
                    }
                    if (valido)
                        parcial.push_back(make_pair(&pel, 5));
                }
                return parcial;
            }));
        }
        vector<pair<Pelicula*, int>> resultados;
        for (auto &fut : futuros) {
            vector<pair<Pelicula*, int>> parcial = fut.get();
            resultados.insert(resultados.end(), parcial.begin(), parcial.end());
        }
        sort(resultados.begin(), resultados.end(), [](auto &a, auto &b){ return a.second > b.second; });
        return resultados;
    }
};

// -------------------- VARIABLE GLOBAL PARA ARBOL DE SUFIJOS --------------------
ArbolSufijosUkkonen* arbolSufijosGlobal = nullptr;

// Muestra solo los titulos de una lista, justificados.
void mostrarListaTitulos(const vector<Pelicula*>& lista) {
    for (size_t i = 0; i < lista.size(); i++) {
        cout << i + 1 << ". " << endl;
        imprimirTituloJustificado(lista[i]->titulo);
    }
}

// Submenu para la pelicula seleccionada.
// Se imprime el titulo, los etiquetas (separados por coma), la sinopsis en recuadro y la fuente.
// Se agrego la opcion 4 para volver directamente al menu principal.
// La funcion retorna true si el usuario eligio volver al menu principal, false en caso contrario.
bool submenuPelicula(Pelicula* seleccionada, vector<Pelicula*>& gustadas, vector<Pelicula*>& verMasTarde) {
    while (true) {
        cout << "\n========================================" << endl;
        cout << "       DETALLES DE LA PELICULA" << endl;
        cout << "Titulo: " << seleccionada->titulo << endl;
        cout << "Etiquetas: ";
        for (size_t i = 0; i < seleccionada->etiquetas.size(); i++){
            cout << seleccionada->etiquetas[i];
            if (i < seleccionada->etiquetas.size() - 1)
                cout << ", ";
        }
        cout << endl;
        cout << "Sinopsis:" << endl;
        imprimirCuadro(seleccionada->sinopsis, ANCHO);
        cout << "Fuente de la sinopsis: " << seleccionada->fuente << endl;
        cout << "========================================" << endl;
        cout << "\nOpciones:" << endl;
        cout << "1. " << (find(gustadas.begin(), gustadas.end(), seleccionada) == gustadas.end() ? "Dar Like" : "Quitar Like") << endl;
        cout << "2. " << (find(verMasTarde.begin(), verMasTarde.end(), seleccionada) == verMasTarde.end() ? "Agregar a Ver mas tarde" : "Quitar de Ver mas tarde") << endl;
        cout << "3. Regresar al submenu" << endl;
        cout << "4. Volver al Menu Principal" << endl;
        int op;
        while (true) {
            cout << "Seleccione una opcion (1-4): " << flush;
            cin >> op;
            if (cin.fail() || op < 1 || op > 4) {
                cin.clear();
                cin.ignore(numeric_limits<streamsize>::max(), '\n');
                cout << "Ingrese un numero entre 1 y 4." << endl;
            } else {
                cin.ignore(numeric_limits<streamsize>::max(), '\n');
                break;
            }
        }
        if (op == 4)
            return true;
        if (op == 3)
            return false;
        if (op == 1) {
            if (find(gustadas.begin(), gustadas.end(), seleccionada) == gustadas.end()){
                seleccionada->likes = 1;
                gustadas.push_back(seleccionada);
                cout << "Like anadido a " << seleccionada->titulo << "!" << endl;
            } else {
                gustadas.erase(remove(gustadas.begin(), gustadas.end(), seleccionada), gustadas.end());
                seleccionada->likes = 0;
                cout << "Like removido de " << seleccionada->titulo << "." << endl;
            }
        }
        if (op == 2) {
            if (find(verMasTarde.begin(), verMasTarde.end(), seleccionada) == verMasTarde.end()){
                verMasTarde.push_back(seleccionada);
                cout << seleccionada->titulo << " agregada a Ver mas tarde." << endl;
            } else {
                verMasTarde.erase(remove(verMasTarde.begin(), verMasTarde.end(), seleccionada), verMasTarde.end());
                cout << seleccionada->titulo << " removida de Ver mas tarde." << endl;
            }
        }
    }
}

// Funcion para manejar listas (como "Ver mas tarde", "gustadas", "Recomendaciones").
// Se muestran solo los titulos justificados y, al seleccionar una pelicula, se abre el submenu.
void manejarLista(const vector<Pelicula*>& lista, const string &nombreLista, vector<Pelicula*>& gustadas, vector<Pelicula*>& verMasTarde) {
    if (lista.empty()) {
        cout << "\nNo hay peliculas en " << nombreLista << "." << endl;
        return;
    }
    cout << "\n--- " << nombreLista << " (solo titulos) ---" << endl;
    mostrarListaTitulos(lista);
    int indiceSel;
    while (true) {
        cout << "Seleccione el numero de la pelicula para ver detalles (0 para regresar): " << flush;
        cin >> indiceSel;
        if (cin.fail() || indiceSel < 0 || indiceSel > (int)lista.size()) {
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            cout << "Por favor, ingrese un numero valido." << endl;
        } else {
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            break;
        }
    }
    if (indiceSel == 0)
        return;
    Pelicula* seleccionada = lista[indiceSel - 1];
    // Si el submenú retorna true, se desea volver directamente al menú principal.
    bool volverAlMenuPrincipal = submenuPelicula(seleccionada, gustadas, verMasTarde);
    if (volverAlMenuPrincipal)
        return;
}

// -------------------- PATRON STRATEGY: MANEJO DE BUSQUEDA --------------------
void manejarBusqueda(vector<Pelicula>& peliculas,
                     unordered_map<string, set<Pelicula*>> &indiceModo1,
                     unordered_map<string, set<Pelicula*>> &indiceEtiqueta,
                     vector<Pelicula*>& gustadas,
                     vector<Pelicula*>& verMasTarde) {
    cout << "\n--- Busqueda de Peliculas ---" << endl;
    cout << "Modo actual: " << (modoBusquedaGlobal == 1 ? "Titulo y sinopsis" : "Etiqueta") << endl;
    cout << "Ingrese 'modo' para cambiar el modo de busqueda, o ingrese su consulta: " << flush;
    string consulta;
    getline(cin, consulta);
    if (aMinusculas(consulta) == "modo") {
        cout << "Seleccione modo (1: Titulo y sinopsis, 2: Etiqueta): " << flush;
        cin >> modoBusquedaGlobal;
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
        cout << "Modo actualizado a: " << (modoBusquedaGlobal == 1 ? "Titulo y sinopsis" : "Etiqueta") << endl;
        cout << "Ingrese su consulta: " << flush;
        getline(cin, consulta);
    }
    // Guardar la busqueda en el historial (Memento)
    cuidadorHistorial.agregarMemento(MementoBusqueda(consulta, modoBusquedaGlobal));
    // Medir tiempo de busqueda
    auto inicioBusq = chrono::high_resolution_clock::now();
    vector<pair<Pelicula*, int>> resultados;
    if (modoBusquedaGlobal == 1) {
        string consultaLower = aMinusculas(consulta);
        string consultaNorm = normalizarEspacios(consultaLower);
        set<int> indicesCoincidentes = arbolSufijosGlobal->buscar(consultaLower);
        // Fallback: union con busqueda lineal
        for (int i = 0; i < peliculas.size(); i++) {
            string titLower = aMinusculas(peliculas[i].titulo);
            string sinopLower = aMinusculas(peliculas[i].sinopsis);
            if (titLower.find(consultaLower) != string::npos || sinopLower.find(consultaLower) != string::npos)
                indicesCoincidentes.insert(i);
        }
        for (int idx : indicesCoincidentes) {
            Pelicula &pel = peliculas[idx];
            string titLower = aMinusculas(pel.titulo);
            string titNorm = normalizarEspacios(titLower);
            string sinopLower = aMinusculas(pel.sinopsis);
            int puntaje = 0;
            if (titLower.find(consultaLower) != string::npos) {
                puntaje += 3;
                if (titNorm == consultaNorm)
                    puntaje += 50;
            }
            if (sinopLower.find(consultaLower) != string::npos)
                puntaje += 2;
            if (puntaje > 0)
                resultados.push_back(make_pair(&pel, puntaje));
        }
    } else {
        vector<string> etiquetasConsulta;
        if (consulta.find(',') != string::npos) {
            istringstream iss(consulta);
            string token;
            while(getline(iss, token, ',')) {
                token = recortar(token);
                if (!token.empty())
                    etiquetasConsulta.push_back(aMinusculas(token));
            }
        } else {
            etiquetasConsulta = tokenizar(consulta);
        }
        vector<future<vector<pair<Pelicula*, int>>>> futuros;
        size_t total = peliculas.size();
        size_t numHilos = NUM_HILOS;
        size_t tamBloque = total / numHilos;
        for (size_t i = 0; i < numHilos; i++) {
            size_t inicio = i * tamBloque;
            size_t fin = (i == numHilos - 1) ? total : (i + 1) * tamBloque;
            futuros.push_back(async(launch::async, [&, inicio, fin, etiquetasConsulta]() {
                vector<pair<Pelicula*, int>> parcial;
                for (size_t j = inicio; j < fin; j++) {
                    Pelicula& pel = peliculas[j];
                    bool valido = true;
                    for (auto &qt : etiquetasConsulta) {
                        bool encontrado = false;
                        for (auto &etiqueta : pel.etiquetas) {
                            if (aMinusculas(etiqueta).find(qt) != string::npos) {
                                encontrado = true;
                                break;
                            }
                        }
                        if (!encontrado) {
                            valido = false;
                            break;
                        }
                    }
                    if (valido)
                        parcial.push_back(make_pair(&pel, 5));
                }
                return parcial;
            }));
        }
        for (auto &fut : futuros) {
            vector<pair<Pelicula*, int>> parcial = fut.get();
            resultados.insert(resultados.end(), parcial.begin(), parcial.end());
        }
    }
    auto finBusq = chrono::high_resolution_clock::now();
    chrono::duration<double> tiempoBusq = finBusq - inicioBusq;
    cout << "Tiempo de busqueda: " << tiempoBusq.count() << " segundos." << endl;
    if (resultados.empty()) {
        cout << "\nNo se encontraron peliculas para la consulta." << endl;
        return;
    }
    sort(resultados.begin(), resultados.end(), [](auto &a, auto &b){ return a.second > b.second; });
    int totalPaginas = (resultados.size() + 4) / 5;
    int paginaActual = 0;
    string consultaLower = aMinusculas(consulta);
    while (true) {
        cout << "\n--- Resultados (pagina " << (paginaActual + 1) << " de " << totalPaginas << ") ---" << endl;
        for (int i = paginaActual * 5; i < min((int)resultados.size(), (paginaActual + 1) * 5); i++) {
            Pelicula* p = resultados[i].first;
            string salida = p->titulo;
            bool encTit = (aMinusculas(p->titulo).find(consultaLower) != string::npos);
            bool encSin = (aMinusculas(p->sinopsis).find(consultaLower) != string::npos);
            if (encTit && encSin)
                salida += " [Encontrado en titulo y sinopsis]";
            else if (encTit)
                salida += " [Encontrado solo en titulo]";
            else if (encSin)
                salida += " [Encontrado solo en sinopsis]";
            if (!encTit && encSin) {
                string frag = extraerFragmento(p->sinopsis, consultaLower);
                salida += " - " + frag;
            }
            cout << to_string(i + 1) << ". " << endl;
            imprimirTituloJustificado(salida);
        }
        cout << "\nOpciones:" << endl;
        cout << "1. Seleccionar pelicula" << endl;
        cout << "2. Pagina anterior" << endl;
        cout << "3. Siguiente pagina" << endl;
        cout << "4. Regresar al menu principal" << endl;
        int op;
        while (true) {
            cout << "Seleccione una opcion (1-4): " << flush;
            cin >> op;
            if (cin.fail() || op < 1 || op > 4) {
                cin.clear();
                cin.ignore(numeric_limits<streamsize>::max(), '\n');
                cout << "Ingrese un numero entre 1 y 4." << endl;
            } else {
                cin.ignore(numeric_limits<streamsize>::max(), '\n');
                break;
            }
        }
        if (op == 4)
            break;
        else if (op == 2) {
            if (paginaActual > 0)
                paginaActual--;
            else
                cout << "No hay pagina anterior." << endl;
        }
        else if (op == 3) {
            if (paginaActual < totalPaginas - 1)
                paginaActual++;
            else
                cout << "No hay mas paginas." << endl;
        }
        else if (op == 1) {
            cout << "Seleccione el numero de la pelicula (segun el listado mostrado): " << flush;
            int indiceSel;
            cin >> indiceSel;
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            int indiceGlobal = paginaActual * 5 + indiceSel;
            if (indiceSel < 1 || indiceSel > 5 || indiceGlobal > (int)resultados.size()) {
                cout << "Seleccion invalida." << endl;
            } else {
                bool volverMenu = submenuPelicula(resultados[indiceGlobal - 1].first, gustadas, verMasTarde);
                if (volverMenu)
                    return;
            }
        }
    }
}

// -------------------- MANEJO DEL HISTORIAL DE BUSQUEDAS --------------------
void manejarHistorialBusquedas() {
    while (true) {
        cuidadorHistorial.mostrarHistorial();
        cout << "\nOpciones:" << endl;
        cout << "1. Eliminar una entrada" << endl;
        cout << "2. Borrar todo el historial" << endl;
        cout << "3. Regresar al menu principal" << endl;
        int op;
        while (true) {
            cout << "Seleccione una opcion (1-3): " << flush;
            cin >> op;
            if (cin.fail() || op < 1 || op > 3) {
                cin.clear();
                cin.ignore(numeric_limits<streamsize>::max(), '\n');
                cout << "Ingrese un numero entre 1 y 3." << endl;
            } else {
                cin.ignore(numeric_limits<streamsize>::max(), '\n');
                break;
            }
        }
        if (op == 3)
            break;
        else if (op == 1) {
            cout << "Ingrese el numero de la entrada a eliminar: " << flush;
            int num;
            cin >> num;
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            if (num < 1 || num > (int)cuidadorHistorial.obtenerHistorial().size()) {
                cout << "Numero invalido." << endl;
            } else {
                cuidadorHistorial.eliminarMemento(num - 1);
                cout << "Entrada eliminada." << endl;
            }
        }
        else if (op == 2) {
            cuidadorHistorial.borrarHistorial();
            cout << "Historial borrado." << endl;
            break;
        }
    }
}

// -------------------- MENU PRINCIPAL --------------------
int main() {
    cout << "========================================" << endl;
    cout << " BIENVENIDO A LA PLATAFORMA DE STREAMING" << endl;
    cout << "========================================" << endl;

    BaseDeDatos* bd = BaseDeDatos::obtenerInstancia("mpst_full_data.csv");
    vector<Pelicula>& peliculas = bd->obtenerPeliculas();

    cout << "\nTotal de peliculas cargadas: " << peliculas.size() << endl;

    auto indiceMode1 = construirIndice(peliculas);    // Para busquedas por titulo y sinopsis.
    auto indiceEtiqueta = construirIndiceEtiquetas(peliculas);    // Para busquedas por etiqueta.

    vector<Pelicula*> gustadas;
    vector<Pelicula*> verMasTarde;
    vector<Pelicula*> recomendaciones = recomendarPeliculas(peliculas, gustadas);

    cout << "\n=== Inicio ===" << endl;
    if (!verMasTarde.empty()) {
        cout << "\nPeliculas en 'Ver mas tarde':" << endl;
        mostrarListaTitulos(verMasTarde);
    } else {
        cout << "\nNo hay peliculas en 'Ver mas tarde'." << endl;
    }
    if (!recomendaciones.empty()) {
        cout << "\nRecomendaciones:" << endl;
        mostrarListaTitulos(recomendaciones);
    } else {
        cout << "\nNo hay recomendaciones (aun no has dado Like a ninguna pelicula)." << endl;
    }

    while (true) {
        cout << "\n=== Menu Principal ===" << endl;
        cout << "1. Buscar peliculas" << endl;
        cout << "2. Ver recomendaciones" << endl;
        cout << "3. Ver 'Ver mas tarde'" << endl;
        cout << "4. Ver 'Peliculas a las que le di like'" << endl;
        cout << "5. Ver historial de busquedas" << endl;
        cout << "6. Salir" << endl;

        int op;
        while (true) {
            cout << "Seleccione una opcion (1-6): " << flush;
            cin >> op;
            if (cin.fail() || op < 1 || op > 6) {
                cin.clear();
                cin.ignore(numeric_limits<streamsize>::max(), '\n');
                cout << "Ingrese un numero entre 1 y 6." << endl;
            } else {
                cin.ignore(numeric_limits<streamsize>::max(), '\n');
                break;
            }
        }

        if (op == 6) {
            cout << "\nSaliendo del programa. Gracias por utilizar la Plataforma de Streaming." << endl;
            break;
        }
        else if (op == 1) {
            manejarBusqueda(peliculas, indiceMode1, indiceEtiqueta, gustadas, verMasTarde);
        } 
        else if (op == 2) {
            manejarLista(recomendaciones, "Recomendaciones", gustadas, verMasTarde);
        } 
        else if (op == 3) {
            manejarLista(verMasTarde, "Ver mas tarde", gustadas, verMasTarde);
        } 
        else if (op == 4) {
            manejarLista(gustadas, "Peliculas a las que le di like", gustadas, verMasTarde);
        } 
        else if (op == 5) {
            manejarHistorialBusquedas();
        }
        recomendaciones = recomendarPeliculas(peliculas, gustadas);
    }

    cout << "\nPrograma finalizado." << endl;
    return 0;
}

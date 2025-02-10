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
using namespace std;

// Constantes para formateo
const int ANCHO = 80;         // Ancho para el recuadro de la sinopsis
const int ANCHO_TITULO = 60;    // Ancho para justificar los titulos

// Variable global para persistir el modo de busqueda:
// 1: Titulo y sinopsis, 2: Tag.
int globalModoBusqueda = 1;

// ===================== PROTOTIPOS =====================
struct Pelicula;

string aMinusculas(const string &s);
string recortar(const string &s);
int contarComillas(const string &s);
vector<string> parsearLineaCSV(const string &linea);
vector<string> tokenizar(const string &s);
string normalizarEspacios(const string &s);
vector<Pelicula> cargarPeliculas(const string &nombreArchivo);
unordered_map<string, set<Pelicula*>> construirIndice(const vector<Pelicula>& peliculas);
unordered_map<string, set<Pelicula*>> construirIndiceTags(const vector<Pelicula>& peliculas);
vector<string> justificarTexto(const string &texto, int ancho);
void imprimirCuadro(const string &texto, int ancho);
void imprimirTituloJustificado(const string &titulo);
string extraerSnippet(const string &sinopsis, const string &consulta);
int calcularRelevanciaMode1(const Pelicula* pelicula, const vector<string>& tokens);
int calcularRelevanciaMode2(const Pelicula* pelicula);
vector<Pelicula*> recomendarPeliculas(const vector<Pelicula>& peliculas, const vector<Pelicula*>& liked);
void mostrarListaTitulos(const vector<Pelicula*>& lista);
// Ahora, el submenú de pelicula retorna un bool: true si el usuario desea volver directamente al menu principal.
bool submenuPelicula(Pelicula* seleccionada, vector<Pelicula*>& liked, vector<Pelicula*>& verMasTarde);
void manejarLista(const vector<Pelicula*>& lista, const string &nombreLista, vector<Pelicula*>& liked, vector<Pelicula*>& verMasTarde);
void manejarBusqueda(vector<Pelicula>& peliculas,
                     unordered_map<string, set<Pelicula*>> &indiceMode1,
                     unordered_map<string, set<Pelicula*>> &indiceTag,
                     vector<Pelicula*>& liked,
                     vector<Pelicula*>& verMasTarde);

// ===================== DEFINICIONES =====================

// Estructura para almacenar la informacion de cada pelicula.
struct Pelicula {
    string titulo;
    string sinopsis;
    vector<string> tags;
    string source;   // Fuente de la sinopsis (columna 6 del CSV)
    int likes = 0;   // 0 o 1; se permite un unico like.
};

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

// Carga las peliculas desde un CSV (respetando registros multilinea).
// Se separan los tags usando coma como delimitador para conservar tags compuestos.
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
            // Separar la columna de tags usando coma como delimitador.
            istringstream iss(columnas[3]);
            string tag;
            while (getline(iss, tag, ',')) {
                tag = recortar(tag);
                if (!tag.empty())
                    pelicula.tags.push_back(tag);
            }
        }
        pelicula.source = columnas[5]; // Fuente de la sinopsis.
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

// Construye un indice para busquedas por tag.
unordered_map<string, set<Pelicula*>> construirIndiceTags(const vector<Pelicula>& peliculas) {
    unordered_map<string, set<Pelicula*>> indiceTags;
    for (const auto &pelicula : peliculas) {
        for (const auto &tag : pelicula.tags) {
            string tagLower = aMinusculas(tag);
            indiceTags[tagLower].insert(const_cast<Pelicula*>(&pelicula));
        }
    }
    return indiceTags;
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
string extraerSnippet(const string &sinopsis, const string &consulta) {
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

// Calcula la relevancia para busquedas en modo 1 (titulo y sinopsis) usando busqueda por subcadena.
// Se otorga un bonus si, al normalizar espacios, el titulo es exactamente igual a la consulta.
int calcularRelevanciaMode1(const Pelicula* pelicula, const vector<string>& tokens) {
    int puntaje = 0;
    for (const auto &token : tokens) {
         if (aMinusculas(pelicula->titulo).find(token) != string::npos)
             puntaje += 3;
         if (aMinusculas(pelicula->sinopsis).find(token) != string::npos)
             puntaje += 2;
    }
    return puntaje;
}

// Para busquedas por tag (modo 2), se asigna un puntaje fijo.
int calcularRelevanciaMode2(const Pelicula* pelicula) {
    return 5;
}

// Genera recomendaciones basadas en los likes.
// Se omiten las peliculas ya liked y se ordenan por puntaje segun coincidencia de tags.
vector<Pelicula*> recomendarPeliculas(const vector<Pelicula>& peliculas, const vector<Pelicula*>& liked) {
    set<string> tagsLiked;
    for (auto movie : liked)
        for (const auto &tag : movie->tags)
            tagsLiked.insert(tag);
    vector<pair<Pelicula*, int>> puntajes;
    for (const auto &pelicula : peliculas) {
        bool yaLike = false;
        for (auto likedMovie : liked) {
            if (likedMovie == const_cast<Pelicula*>(&pelicula)) {
                yaLike = true;
                break;
            }
        }
        if (yaLike)
            continue;
        int puntaje = 0;
        for (const auto &tag : pelicula.tags)
            if (tagsLiked.count(tag))
                puntaje++;
        if (puntaje > 0)
            puntajes.push_back(make_pair(const_cast<Pelicula*>(&pelicula), puntaje));
    }
    sort(puntajes.begin(), puntajes.end(), [](auto &a, auto &b) { return a.second > b.second; });
    vector<Pelicula*> recomendadas;
    for (size_t i = 0; i < puntajes.size() && i < 5; i++)
         recomendadas.push_back(puntajes[i].first);
    return recomendadas;
}

// Muestra solo los titulos de una lista, justificados.
void mostrarListaTitulos(const vector<Pelicula*>& lista) {
    for (size_t i = 0; i < lista.size(); i++) {
        cout << i + 1 << ". " << endl;
        imprimirTituloJustificado(lista[i]->titulo);
    }
}

// Submenu para la pelicula seleccionada.
// Se imprime el titulo, los tags (separados por coma), la sinopsis en recuadro y la fuente.
// Se agrego la opcion 4 para volver directamente al menu principal.
// La funcion retorna true si el usuario eligio volver al menu principal, false en caso contrario.
bool submenuPelicula(Pelicula* seleccionada, vector<Pelicula*>& liked, vector<Pelicula*>& verMasTarde) {
    while (true) {
        cout << "\n========================================" << endl;
        cout << "       DETALLES DE LA PELICULA" << endl;
        cout << "Titulo: " << seleccionada->titulo << endl;
        cout << "Tags: ";
        for (size_t i = 0; i < seleccionada->tags.size(); i++) {
            cout << seleccionada->tags[i];
            if (i < seleccionada->tags.size() - 1)
                cout << ", ";
        }
        cout << endl;
        cout << "Sinopsis:" << endl;
        imprimirCuadro(seleccionada->sinopsis, ANCHO);
        cout << "Fuente de la sinopsis: " << seleccionada->source << endl;
        cout << "========================================" << endl;
        cout << "\nOpciones para esta pelicula:" << endl;
        cout << "1. " << (find(liked.begin(), liked.end(), seleccionada) != liked.end() ? "Quitar Like" : "Dar Like") << endl;
        cout << "2. " << (find(verMasTarde.begin(), verMasTarde.end(), seleccionada) != verMasTarde.end() ? "Quitar de Ver mas tarde" : "Agregar a Ver mas tarde") << endl;
        cout << "3. Regresar al submenu" << endl;
        cout << "4. Volver al Menu Principal" << endl;
        int subOpcion;
        while (true) {
            cout << "Seleccione una opcion (1-4): " << flush;
            cin >> subOpcion;
            if (cin.fail() || subOpcion < 1 || subOpcion > 4) {
                cin.clear();
                cin.ignore(numeric_limits<streamsize>::max(), '\n');
                cout << "Por favor, ingrese un numero entre 1 y 4." << endl;
            } else {
                cin.ignore(numeric_limits<streamsize>::max(), '\n');
                break;
            }
        }
        if (subOpcion == 4)
            return true;  // Indica que se desea volver al menu principal.
        else if (subOpcion == 3)
            return false;
        else if (subOpcion == 1) {
            if (find(liked.begin(), liked.end(), seleccionada) == liked.end()) {
                seleccionada->likes = 1;
                liked.push_back(seleccionada);
                cout << "Like anadido a " << seleccionada->titulo << "!" << endl;
            } else {
                liked.erase(remove(liked.begin(), liked.end(), seleccionada), liked.end());
                seleccionada->likes = 0;
                cout << "Like removido de " << seleccionada->titulo << "." << endl;
            }
        } else if (subOpcion == 2) {
            if (find(verMasTarde.begin(), verMasTarde.end(), seleccionada) == verMasTarde.end()) {
                verMasTarde.push_back(seleccionada);
                cout << seleccionada->titulo << " agregada a Ver mas tarde." << endl;
            } else {
                verMasTarde.erase(remove(verMasTarde.begin(), verMasTarde.end(), seleccionada), verMasTarde.end());
                cout << seleccionada->titulo << " removida de Ver mas tarde." << endl;
            }
        }
    }
}

// Funcion para manejar listas (como "Ver mas tarde", "Liked", "Recomendaciones").
// Se muestran solo los titulos justificados y, al seleccionar una pelicula, se abre el submenu.
void manejarLista(const vector<Pelicula*>& lista, const string &nombreLista, vector<Pelicula*>& liked, vector<Pelicula*>& verMasTarde) {
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
    bool volverAlMenuPrincipal = submenuPelicula(seleccionada, liked, verMasTarde);
    if (volverAlMenuPrincipal)
        return;
}

// Funcion para manejar la busqueda de peliculas.
void manejarBusqueda(vector<Pelicula>& peliculas,
                     unordered_map<string, set<Pelicula*>> &indiceMode1,
                     unordered_map<string, set<Pelicula*>> &indiceTag,
                     vector<Pelicula*>& liked,
                     vector<Pelicula*>& verMasTarde) {
    // Se utiliza el modo de busqueda persistente en la variable global.
    int modoBusqueda = globalModoBusqueda;
    cout << "\n--- Busqueda de Peliculas ---" << endl;
    cout << "Modo actual de busqueda: "
         << (modoBusqueda == 1 ? "Titulo y sinopsis" : "Tag") << endl;
    cout << "Ingrese 'modo' para cambiar el modo de busqueda, o ingrese su consulta: " << flush;
    string consulta;
    getline(cin, consulta);
    if (aMinusculas(consulta) == "modo") {
        cout << "Seleccione modo de busqueda (1: Titulo y sinopsis, 2: Tag): " << flush;
        cin >> modoBusqueda;
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
        globalModoBusqueda = modoBusqueda; // Se persiste el modo seleccionado.
        cout << "Modo de busqueda actualizado a: "
             << (modoBusqueda == 1 ? "Titulo y sinopsis" : "Tag") << endl;
        cout << "Ingrese su consulta: " << flush;
        getline(cin, consulta);
    }
    vector<pair<Pelicula*, int>> resultados;
    if (modoBusqueda == 1) {
        // Busqueda en modo 1: se normalizan los espacios y se compara
        string consultaLower = aMinusculas(consulta);
        string consultaNorm = normalizarEspacios(consultaLower);
        for (auto &pelicula : peliculas) {
            string tituloLower = aMinusculas(pelicula.titulo);
            string tituloNorm = normalizarEspacios(tituloLower);
            string sinopsisLower = aMinusculas(pelicula.sinopsis);
            int puntaje = 0;
            if (tituloLower.find(consultaLower) != string::npos) {
                puntaje += 3;
                if (tituloNorm == consultaNorm)
                    puntaje += 50; // Bonus para coincidencia exacta.
            }
            if (sinopsisLower.find(consultaLower) != string::npos)
                puntaje += 2;
            if (puntaje > 0)
                resultados.push_back(make_pair(&pelicula, puntaje));
        }
    } else if (modoBusqueda == 2) {
        // Busqueda por tag: si hay comas se separa por comas; sino, se separa por espacios.
        vector<string> queryTags;
        if (consulta.find(',') != string::npos) {
            istringstream iss(consulta);
            string token;
            while(getline(iss, token, ',')) {
                token = recortar(token);
                if (!token.empty())
                    queryTags.push_back(aMinusculas(token));
            }
        } else {
            queryTags = tokenizar(consulta);
        }
        if(queryTags.empty()){
            cout << "\nNo se proporcionaron tags." << endl;
            return;
        }
        // Calcular la interseccion: conservar solo las peliculas que contengan TODOS los tokens en alguno de sus tags.
        set<Pelicula*> interseccion;
        for (auto &entrada : indiceTag) {
            if (entrada.first.find(queryTags[0]) != string::npos)
                interseccion.insert(entrada.second.begin(), entrada.second.end());
        }
        for (size_t i = 1; i < queryTags.size(); i++) {
            string qt = queryTags[i];
            set<Pelicula*> temp;
            for (auto p : interseccion) {
                bool match = false;
                for (const auto &tag : p->tags) {
                    if (aMinusculas(tag).find(qt) != string::npos) {
                        match = true;
                        break;
                    }
                }
                if(match)
                    temp.insert(p);
            }
            interseccion = temp;
        }
        for (auto pelicula : interseccion)
            resultados.push_back(make_pair(pelicula, 5)); // Puntaje fijo.
    }
    sort(resultados.begin(), resultados.end(), [](auto &a, auto &b) { return a.second > b.second; });
    if (resultados.empty()) {
        cout << "\nNo se encontraron peliculas para la consulta." << endl;
        return;
    }
    int totalPaginas = (resultados.size() + 4) / 5;
    int paginaActual = 0;
    string consultaParaSnippet = aMinusculas(consulta);
    while (true) {
        cout << "\n--- Resultados (pagina " << (paginaActual + 1) << " de " << totalPaginas << ") ---" << endl;
        for (int i = paginaActual * 5; i < min((int)resultados.size(), (paginaActual + 1) * 5); i++) {
            Pelicula* p = resultados[i].first;
            string salida = p->titulo;
            if (modoBusqueda == 1) {
                bool encontroTitulo = (aMinusculas(p->titulo).find(consultaParaSnippet) != string::npos);
                bool encontroSinopsis = (aMinusculas(p->sinopsis).find(consultaParaSnippet) != string::npos);
                if (encontroTitulo && encontroSinopsis)
                    salida += " [Encontrado en titulo y sinopsis]";
                else if (encontroTitulo)
                    salida += " [Encontrado solo en titulo]";
                else if (encontroSinopsis)
                    salida += " [Encontrado solo en sinopsis]";
                if (!encontroTitulo && encontroSinopsis) {
                    string snippet = extraerSnippet(p->sinopsis, consultaParaSnippet);
                    salida += " - " + snippet;
                }
            } else if (modoBusqueda == 2) {
                salida += " [Encontrado en tag]";
            }
            cout << to_string(i + 1) << ". " << endl;
            imprimirTituloJustificado(salida);
        }
        cout << "\nOpciones:" << endl;
        cout << "1. Seleccionar pelicula" << endl;
        cout << "2. Pagina anterior" << endl;
        cout << "3. Siguiente pagina" << endl;
        cout << "4. Regresar al menu principal" << endl;
        int opcion;
        while (true) {
            cout << "Seleccione una opcion (1-4): " << flush;
            cin >> opcion;
            if (cin.fail() || opcion < 1 || opcion > 4) {
                cin.clear();
                cin.ignore(numeric_limits<streamsize>::max(), '\n');
                cout << "Por favor, ingrese un numero entre 1 y 4." << endl;
            } else {
                cin.ignore(numeric_limits<streamsize>::max(), '\n');
                break;
            }
        }
        if (opcion == 4)
            break;
        else if (opcion == 2) {
            if (paginaActual > 0)
                paginaActual--;
            else
                cout << "No hay pagina anterior." << endl;
        }
        else if (opcion == 3) {
            if (paginaActual < totalPaginas - 1)
                paginaActual++;
            else
                cout << "No hay mas paginas." << endl;
        }
        else if (opcion == 1) {
            cout << "Seleccione el numero de la pelicula (segun el listado mostrado): " << flush;
            int indiceSel;
            cin >> indiceSel;
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            int indiceGlobal = paginaActual * 5 + indiceSel;
            if (indiceSel < 1 || indiceSel > 5 || indiceGlobal > (int)resultados.size()) {
                cout << "Seleccion invalida." << endl;
            } else {
                // Si el submenú retorna true, se desea volver directamente al menú principal.
                bool volverMenu = submenuPelicula(resultados[indiceGlobal - 1].first, liked, verMasTarde);
                if (volverMenu)
                    return;
            }
        }
    }
}

// Funcion principal: Menu principal.
int main() {
    cout << "========================================" << endl;
    cout << " BIENVENIDO A LA PLATAFORMA DE STREAMING" << endl;
    cout << "========================================" << endl;

    string archivoCSV = "mpst_full_data.csv";
    vector<Pelicula> peliculas = cargarPeliculas(archivoCSV);

    cout << "\nTotal de peliculas cargadas: " << peliculas.size() << endl;

    auto indiceMode1 = construirIndice(peliculas);    // Para busquedas por titulo y sinopsis.
    auto indiceTag = construirIndiceTags(peliculas);    // Para busquedas por tag.

    vector<Pelicula*> liked;
    vector<Pelicula*> verMasTarde;
    vector<Pelicula*> recomendaciones = recomendarPeliculas(peliculas, liked);

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
        cout << "5. Salir" << endl;

        int opcionMain;
        while (true) {
            cout << "Seleccione una opcion (1-5): " << flush;
            cin >> opcionMain;
            if (cin.fail() || opcionMain < 1 || opcionMain > 5) {
                cin.clear();
                cin.ignore(numeric_limits<streamsize>::max(), '\n');
                cout << "Por favor, ingrese un numero entre 1 y 5." << endl;
            } else {
                cin.ignore(numeric_limits<streamsize>::max(), '\n');
                break;
            }
        }

        if (opcionMain == 5) {
            cout << "\nSaliendo del programa. Gracias por utilizar la Plataforma de Streaming." << endl;
            break;
        } else if (opcionMain == 1) {
            manejarBusqueda(peliculas, indiceMode1, indiceTag, liked, verMasTarde);
        } else if (opcionMain == 2) {
            manejarLista(recomendaciones, "Recomendaciones", liked, verMasTarde);
        } else if (opcionMain == 3) {
            manejarLista(verMasTarde, "Ver mas tarde", liked, verMasTarde);
        } else if (opcionMain == 4) {
            manejarLista(liked, "Peliculas a las que le di like", liked, verMasTarde);
        }
        recomendaciones = recomendarPeliculas(peliculas, liked);
    }

    cout << "\nPrograma finalizado." << endl;
    return 0;
}

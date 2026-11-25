#include "Sistema.h"
#include "GestorPersistencia.h"
#include "Utilidades.h"
#include <stdexcept>
#include <filesystem>
#include <algorithm>
#include <sstream>
#include <cmath>

namespace {
    std::string raiz(const std::string& ruta) {
        return minusculas(recortar(ruta.substr(0, ruta.find('/'))));
    }
    Contenido crearContenido(int id, const char* titulo, const char* categoria,
                            double popularidad, const char* etiquetas) {
        Contenido c;
        c.id = id;
        c.titulo = titulo;
        c.categoria = categoria;
        c.popularidad = popularidad;
        std::string datos = etiquetas;
        size_t inicio = 0;
        while (inicio <= datos.size()) {
            size_t fin = datos.find(',', inicio);
            c.etiquetas.agregar(datos.substr(inicio, fin == std::string::npos ? fin : fin - inicio));
            if (fin == std::string::npos) break;
            inicio = fin + 1;
        }
        return c;
    }

}

Sistema::Sistema(const std::string& carpetaDatos)
    : usuarios_(17), contenidos_(31), carpetaDatos_(carpetaDatos) {
    GestorPersistencia::recuperar(carpetaDatos_);
    std::filesystem::create_directories(carpetaDatos_);
    if (GestorPersistencia::hayAlgunArchivo(carpetaDatos_)) {
        if (!GestorPersistencia::cargar(*this, carpetaDatos_)) {
            usuarios_.recorrer([](int, Usuario* u) { delete u; });
            contenidos_.recorrer([](int, Contenido* c) { delete c; });
            throw std::runtime_error("Datos incompletos o invalidos. Revise la carpeta de datos (no se sobrescribio).");
        }
        GestorPersistencia::confirmarRecuperacion(carpetaDatos_);
    } else {
        datosIniciales();
        if (!guardarCambios()) {
            usuarios_.recorrer([](int, Usuario* u) { delete u; });
            contenidos_.recorrer([](int, Contenido* c) { delete c; });
            throw std::runtime_error("No se pudo guardar el catalogo inicial.");
        }
    }
}

Sistema::~Sistema() {
    usuarios_.recorrer([](int, Usuario* u) { delete u; });
    contenidos_.recorrer([](int, Contenido* c) { delete c; });
}

bool Sistema::guardarCambios() const {
    return GestorPersistencia::guardar(*this, carpetaDatos_);
}

const Usuario* Sistema::autenticar(int id, const std::string& clave) const {
    const Usuario* u = usuario(id);
    return u && u->clave == clave ? u : nullptr;
}

bool Sistema::registrarUsuario(int id, const std::string& nombre, const std::string& clave,
                              RolUsuario rol) {
    if (id <= 0 || !textoPersistible(nombre) || !textoPersistible(clave) || usuario(id) ||
        (rol != RolUsuario::NORMAL && rol != RolUsuario::ADMINISTRADOR)) return false;
    Usuario* u = new Usuario;
    u->id = id;
    u->nombre = recortar(nombre);
    u->clave = clave;
    u->rol = rol;
    if (!usuarios_.insertar(id, u)) { delete u; return false; }
    if (guardarCambios()) return true;
    usuarios_.eliminar(id); delete u; return false;
}

bool Sistema::agregarPreferencia(int id, const std::string& texto) {
    Usuario* u = usuarios_.buscar(id);
    if (!u) return false;
    ListaTextos anterior = u->preferencias;
    if (!u->preferencias.agregar(texto)) return false;
    if (guardarCambios()) return true;
    u->preferencias.intercambiar(anterior); return false;
}

bool Sistema::quitarPreferencia(int id, const std::string& texto) {
    Usuario* u = usuarios_.buscar(id);
    if (!u) return false;
    ListaTextos anterior = u->preferencias;
    if (!u->preferencias.quitar(texto)) return false;
    if (guardarCambios()) return true;
    u->preferencias.intercambiar(anterior); return false;
}

double Sistema::similitudCalculada(const Contenido& a, const Contenido& b) {
    double peso = 0.0;
    if (textoIgual(a.categoria, b.categoria)) peso += 0.55;
    else if (raiz(a.categoria) == raiz(b.categoria)) peso += 0.20;
    int comunes = 0;
    for (int i = 0; i < a.etiquetas.cantidad(); ++i)
        if (b.etiquetas.contiene(a.etiquetas.obtener(i))) ++comunes;
    int unionEtiquetas = a.etiquetas.cantidad() + b.etiquetas.cantidad() - comunes;
    if (unionEtiquetas) peso += 0.45 * comunes / unionEtiquetas;
    return std::min(1.0, peso);
}

void Sistema::reconstruirIndices() {
    arbol_.limpiar();
    grafo_.limpiar();
    contenidos_.recorrer([&](int id, const Contenido* c) {
        arbol_.agregarRuta(c->categoria);
        grafo_.agregarVertice(id);
    });
    contenidos_.recorrer([&](int idA, const Contenido* a) {
        contenidos_.recorrer([&](int idB, const Contenido* b) {
            if (idA >= idB) return;
            double peso = similitudCalculada(*a, *b);
            if (peso >= 0.20) grafo_.agregarRelacion(idA, idB, peso);
        });
    });
}

// Conserva los indices originales hasta confirmar el guardado.
bool Sistema::actualizarIndicesYGuardar() {
    Grafo anterior;
    ArbolCategorias categoriasAnteriores;
    grafo_.intercambiar(anterior);
    arbol_.intercambiar(categoriasAnteriores);
    reconstruirIndices();
    if (guardarCambios()) return true;
    grafo_.intercambiar(anterior);
    arbol_.intercambiar(categoriasAnteriores);
    return false;
}

bool Sistema::registrarContenido(const Contenido& nuevo) {
    if (nuevo.id <= 0 || contenido(nuevo.id) || !textoPersistible(nuevo.titulo) ||
        !rutaValida(nuevo.categoria) || !std::isfinite(nuevo.popularidad) || nuevo.popularidad < 0 || nuevo.popularidad > 100)
        return false;
    Contenido* c = new Contenido(nuevo);
    if (!contenidos_.insertar(c->id, c)) { delete c; return false; }
    if (actualizarIndicesYGuardar()) return true;
    contenidos_.eliminar(c->id); delete c; return false;
}

bool Sistema::modificarContenido(const Contenido& nuevo) {
    Contenido* anterior = contenidos_.buscar(nuevo.id);
    if (!anterior || !textoPersistible(nuevo.titulo) || !rutaValida(nuevo.categoria) ||
        !std::isfinite(nuevo.popularidad) || nuevo.popularidad < 0 || nuevo.popularidad > 100) return false;
    Contenido respaldo = *anterior;
    *anterior = nuevo;
    if (actualizarIndicesYGuardar()) return true;
    *anterior = respaldo; return false;
}

bool Sistema::eliminarContenido(int id) {
    Contenido* c = contenidos_.buscar(id);
    if (!c) return false;
    PilaAcciones<Interaccion> respaldo;
    interacciones_.porContenido(id, [&](const Interaccion& i) { respaldo.apilar(i); });
    interacciones_.eliminarContenido(id);
    contenidos_.eliminar(id);
    if (!actualizarIndicesYGuardar()) {
        contenidos_.insertar(id, c);
        while (!respaldo.vacia()) { interacciones_.guardar(respaldo.cima()); respaldo.desapilar(); }
        return false;
    }
    delete c;
    // Evita restaurar acciones de un contenido eliminado (incluso si se reutiliza su ID).
    usuarios_.recorrer([&](int, Usuario* u) {
        u->acciones.eliminarSi([&](const CambioInteraccion& a) { return a.anterior.idContenido == id; });
    });
    return true;
}

void Sistema::restaurarInteraccion(const CambioInteraccion& cambio) {
    if (cambio.existia) interacciones_.guardar(cambio.anterior);
    else interacciones_.eliminarInteraccion(cambio.anterior.idUsuario, cambio.anterior.idContenido);
}

bool Sistema::registrarInteraccion(int idUsuario, int idContenido, int tipo, int valor) {
    Usuario* u = usuarios_.buscar(idUsuario);
    if (!u || !contenido(idContenido)) return false;
    CambioInteraccion cambio;
    cambio.anterior.idUsuario = idUsuario;
    cambio.anterior.idContenido = idContenido;
    cambio.existia = interacciones_.consultar(idUsuario, idContenido, cambio.anterior);
    // No registrar una accion sin cambio real.
    if (tipo == 1 && cambio.existia && cambio.anterior.calificacion == valor) return true;
    if (tipo == 2 && cambio.existia && cambio.anterior.favorito == (valor != 0)) return true;
    bool exito = tipo == 0 ? interacciones_.registrarVisualizacion(idUsuario, idContenido) :
                 tipo == 1 ? interacciones_.calificar(idUsuario, idContenido, valor) :
                             interacciones_.marcarFavorito(idUsuario, idContenido, valor != 0);
    if (!exito) return false;
    u->acciones.apilar(cambio);
    if (guardarCambios()) return true;
    restaurarInteraccion(cambio);
    u->acciones.desapilar();
    return false;
}

bool Sistema::visualizar(int idUsuario, int idContenido) {
    return registrarInteraccion(idUsuario, idContenido, 0, 0);
}

bool Sistema::calificar(int idUsuario, int idContenido, int nota) {
    if (nota < 1 || nota > 5) return false;
    return registrarInteraccion(idUsuario, idContenido, 1, nota);
}

bool Sistema::marcarFavorito(int idUsuario, int idContenido, bool favorito) {
    return registrarInteraccion(idUsuario, idContenido, 2, favorito ? 1 : 0);
}

bool Sistema::deshacerInteraccion(int idUsuario) {
    Usuario* u = usuarios_.buscar(idUsuario);
    if (!u || u->acciones.vacia()) return false;
    const CambioInteraccion& cambio = u->acciones.cima();
    CambioInteraccion actual;
    actual.anterior.idUsuario = idUsuario;
    actual.anterior.idContenido = cambio.anterior.idContenido;
    actual.existia = interacciones_.consultar(idUsuario, actual.anterior.idContenido, actual.anterior);
    restaurarInteraccion(cambio);
    if (!guardarCambios()) { restaurarInteraccion(actual); return false; }
    u->acciones.desapilar();
    return true;
}

int Sistema::recomendar(int idUsuario, Candidato* salida, int limite) const {
    const Usuario* u = usuario(idUsuario);
    if (!u) return 0;
    return motor_.generar(*u, contenidos_, interacciones_, grafo_, salida, limite);
}

void Sistema::datosIniciales() {
    auto crearUsuario = [&](int id, const char* nombre, const char* clave, RolUsuario rol) {
        Usuario* u = new Usuario;
        u->id = id; u->nombre = nombre; u->clave = clave; u->rol = rol;
        usuarios_.insertar(id, u);
        return u;
    };
    crearUsuario(1, "Administrador", "admin123", RolUsuario::ADMINISTRADOR);
    Usuario* fredd = crearUsuario(2, "Usuario Demo", "demo123", RolUsuario::NORMAL);
    fredd->preferencias.agregar("Programacion");
    fredd->preferencias.agregar("Ciencia ficcion");
    fredd->preferencias.agregar("Videojuegos");
    Usuario* alex = crearUsuario(3, "Alex", "demo123", RolUsuario::NORMAL);
    alex->preferencias.agregar("Arte");
    alex->preferencias.agregar("Marvel");

    const struct Datos { int id; const char* titulo; const char* categoria; double pop; const char* tags; } base[] = {
        {101,"C++ desde cero","Tecnologia/Programacion",80,"cpp,algoritmos,desarrollo"},
        {102,"Estructuras de datos","Tecnologia/Programacion",84,"algoritmos,cpp,listas"},
        {103,"Algoritmos de busqueda","Tecnologia/Programacion",78,"algoritmos,busqueda"},
        {104,"Desarrollo de videojuegos","Tecnologia/Videojuegos",89,"videojuegos,motores,programacion"},
        {105,"Introduccion a inteligencia artificial","Tecnologia/Inteligencia artificial",92,"algoritmos,tecnologia,IA"},
        {106,"Bases de datos relacionales","Tecnologia/Bases de datos",75,"SQL,datos,programacion"},
        {107,"Aventuras de ciencia ficcion","Entretenimiento/Ciencia ficcion",90,"espacio,aventura,ciencia ficcion"},
        {108,"Viajes en el tiempo","Entretenimiento/Ciencia ficcion",86,"tiempo,ficcion,misterio"},
        {109,"Cuentos fantasticos","Literatura/Fantasia",72,"magia,fantasia,aventura"},
        {110,"Guia de dibujo","Arte/Ilustracion",68,"dibujo,creatividad,arte"},
        {111,"Pintura al oleo","Arte/Pintura",73,"oleo,pintura,creatividad"},
        {112,"Musica y emociones","Arte/Musica",85,"musica,emociones,creatividad"},
        {113,"Fundamentos de fisica","Ciencia/Fisica",79,"ciencia,fisica,matematicas"},
        {114,"Astronomia para curiosos","Ciencia/Astronomia",88,"espacio,ciencia,astronomia"},
        {115,"Marvel y narrativa","Entretenimiento/Comics",94,"marvel,superheroes,historias"},
        {116,"Estrategias de videojuegos","Entretenimiento/Videojuegos",86,"videojuegos,estrategia,competitivo"},
        {117,"Redes de computadoras","Tecnologia/Redes",77,"redes,informatica,protocolos"},
        {118,"Matematicas discretas","Ciencia/Matematicas",71,"algoritmos,matematicas,logica"},
        {119,"Explorando mundos abiertos","Entretenimiento/Videojuegos",82,"videojuegos,aventura,exploracion"}
    };
    for (const Datos& d : base) {
        Contenido* c = new Contenido(crearContenido(d.id, d.titulo, d.categoria, d.pop, d.tags));
        contenidos_.insertar(c->id, c);
    }
    reconstruirIndices();
    interacciones_.calificar(2, 101, 5);
    interacciones_.registrarVisualizacion(2, 101);
    interacciones_.calificar(2, 107, 5);
    interacciones_.marcarFavorito(2, 107, true);
    interacciones_.calificar(2, 111, 2);
    interacciones_.registrarVisualizacion(2, 116);
    interacciones_.calificar(3, 110, 5);
    interacciones_.calificar(3, 115, 5);
}

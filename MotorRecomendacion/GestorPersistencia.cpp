#include "GestorPersistencia.h"
#include "Sistema.h"
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <cmath>
#include <stdexcept>

namespace fs = std::filesystem;

namespace {
    constexpr const char* archivos[] = {
        "usuarios.txt", "contenidos.txt", "interacciones.txt", "similitudes.txt"
    };
    fs::path rutaDatos(const std::string& carpeta) {
        fs::path dir = fs::absolute(carpeta).lexically_normal();
        if (dir.filename().empty()) dir = dir.parent_path();
        return dir;
    }
    fs::path auxiliar(const fs::path& dir, const char* sufijo) {
        fs::path resultado = dir;
        resultado += sufijo;
        return resultado;
    }
    bool carpetaSegura(const fs::path& dir) {
        if (!fs::is_directory(fs::symlink_status(dir))) return false;
        for (const auto& entrada : fs::directory_iterator(dir)) {
            bool conocida = false;
            for (const char* nombre : archivos)
                if (entrada.path().filename() == nombre) conocida = true;
            if (!conocida || !fs::is_regular_file(entrada.symlink_status())) return false;
        }
        return true;
    }
    bool completa(const fs::path& dir) {
        if (!carpetaSegura(dir)) return false;
        for (const char* nombre : archivos)
            if (!fs::is_regular_file(fs::symlink_status(dir / nombre))) return false;
        return true;
    }
    template<typename Funcion>
    bool escribirArchivo(const fs::path& archivo, Funcion escribir) {
        std::ofstream salida(archivo, std::ios::trunc);
        if (!salida) return false;
        escribir(salida);
        salida.close();
        return !salida.fail();
    }
    bool finRegistro(std::istream& in) { in >> std::ws; return in.eof(); }
    bool lineaVacia(const std::string& linea) {
        return linea.find_first_not_of(" \t\r") == std::string::npos;
    }
}

// Si se interrumpio el intercambio antes de publicar el conjunto nuevo,
// recuperar el anterior. Si ambos existen, el nuevo ya fue publicado.
void GestorPersistencia::recuperar(const std::string& carpeta) {
    fs::path dir = rutaDatos(carpeta), respaldo = auxiliar(dir, ".fase3.bak");
    if (!fs::exists(respaldo)) return;
    if (!carpetaSegura(respaldo)) throw std::runtime_error("Respaldo de datos invalido; revise manualmente.");
    if (!fs::exists(dir)) fs::rename(respaldo, dir);
    else if (!completa(dir)) throw std::runtime_error("Datos y respaldo coexistentes; revise manualmente.");
}

void GestorPersistencia::confirmarRecuperacion(const std::string& carpeta) {
    fs::path respaldo = auxiliar(rutaDatos(carpeta), ".fase3.bak");
    // Solo se descarta un respaldo coexistente despues de cargar y validar el nuevo conjunto.
    std::error_code error;
    fs::remove_all(respaldo, error);
}

bool GestorPersistencia::hayAlgunArchivo(const std::string& carpeta) {
    for (const char* nombre : archivos)
        if (fs::exists(fs::path(carpeta) / nombre)) return true;
    return false;
}

bool GestorPersistencia::guardar(const Sistema& s, const std::string& carpeta) {
    fs::path dir = rutaDatos(carpeta);
    fs::path temporal = auxiliar(dir, ".fase3.tmp"), respaldo = auxiliar(dir, ".fase3.bak");
    bool temporalCreado = false, movido = false;
    try {
        // No borrar archivos ajenos ni sobreescribir respaldos pendientes.
        if (!carpetaSegura(dir) || fs::exists(temporal) || fs::exists(respaldo)) return false;
        if (!fs::create_directory(temporal)) return false;
        temporalCreado = true;
        bool a = escribirArchivo(temporal / archivos[0], [&](std::ostream& out) {
            s.usuarios_.recorrer([&](int, const Usuario* u) {
                out << u->id << ' ' << std::quoted(u->nombre) << ' '
                    << std::quoted(u->clave) << ' ' << static_cast<int>(u->rol)
                    << ' ' << u->preferencias.cantidad();
                for (int i = 0; i < u->preferencias.cantidad(); ++i)
                    out << ' ' << std::quoted(u->preferencias.obtener(i));
                out << '\n';
            });
        });
        bool b = escribirArchivo(temporal / archivos[1], [&](std::ostream& out) {
            s.contenidos_.recorrer([&](int, const Contenido* c) {
                out << c->id << ' ' << std::setprecision(17) << c->popularidad << ' '
                    << std::quoted(c->titulo) << ' ' << std::quoted(c->categoria)
                    << ' ' << c->etiquetas.cantidad();
                for (int i = 0; i < c->etiquetas.cantidad(); ++i)
                    out << ' ' << std::quoted(c->etiquetas.obtener(i));
                out << '\n';
            });
        });
        bool c = escribirArchivo(temporal / archivos[2], [&](std::ostream& out) {
            s.interacciones_.recorrer([&](const Interaccion& i) {
                out << i.idUsuario << ' ' << i.idContenido << ' ' << i.visualizaciones
                    << ' ' << i.calificacion << ' ' << static_cast<int>(i.favorito) << '\n';
            });
        });
        bool d = escribirArchivo(temporal / archivos[3], [&](std::ostream& out) {
            s.grafo_.recorrerRelaciones([&](int idA, int idB, double peso) {
                out << idA << ' ' << idB << ' ' << std::setprecision(17) << peso << '\n';
            });
        });
        if (!(a && b && c && d)) {
            std::error_code error; fs::remove_all(temporal, error);
            return false;
        }
        fs::rename(dir, respaldo);
        movido = true;
        fs::rename(temporal, dir);
        temporalCreado = false;
        movido = false;
        // El nuevo conjunto ya es visible; un fallo de limpieza no revierte el commit.
        std::error_code error;
        fs::remove_all(respaldo, error);
        return true;
    } catch (const fs::filesystem_error&) {
        std::error_code error;
        if (movido) {
            fs::rename(respaldo, dir, error);
            if (error) throw std::runtime_error("No se pudo restaurar el respaldo de datos. Reinicie y revise los archivos.");
        }
        if (temporalCreado) fs::remove_all(temporal, error);
        return false;
    }
}

bool GestorPersistencia::cargar(Sistema& s, const std::string& carpeta) {
    fs::path dir(carpeta);
    for (const char* archivo : archivos)
        if (!fs::exists(dir / archivo)) return false;

    std::string linea;
    std::ifstream usuarios(dir / archivos[0]);
    if (!usuarios) return false;
    while (std::getline(usuarios, linea)) {
        if (lineaVacia(linea)) continue;
        std::istringstream in(linea);
        int id, rol, cuenta;
        std::string nombre, clave;
        if (!(in >> id >> std::quoted(nombre) >> std::quoted(clave) >> rol >> cuenta) ||
            id <= 0 || !textoPersistible(nombre) || !textoPersistible(clave) || (rol != 0 && rol != 1) ||
            cuenta < 0 || cuenta > 1000 || s.usuarios_.buscar(id)) return false;
        Usuario* u = new Usuario;
        u->id = id; u->nombre = nombre; u->clave = clave;
        u->rol = static_cast<RolUsuario>(rol);
        for (int i = 0; i < cuenta; ++i) {
            std::string pref;
            if (!(in >> std::quoted(pref)) || !u->preferencias.agregar(pref)) {
                delete u; return false;
            }
        }
        if (!finRegistro(in) || !s.usuarios_.insertar(id, u)) { delete u; return false; }
    }
    if (!usuarios.eof() && usuarios.fail()) return false;

    std::ifstream contenidos(dir / archivos[1]);
    if (!contenidos) return false;
    while (std::getline(contenidos, linea)) {
        if (lineaVacia(linea)) continue;
        std::istringstream in(linea);
        int id, cuenta;
        double pop;
        std::string titulo, categoria;
        if (!(in >> id >> pop >> std::quoted(titulo) >> std::quoted(categoria) >> cuenta) ||
            id <= 0 || !textoPersistible(titulo) || !rutaValida(categoria) || !std::isfinite(pop) ||
            pop < 0 || pop > 100 || cuenta < 0 || cuenta > 1000 || s.contenidos_.buscar(id))
            return false;
        Contenido* c = new Contenido;
        c->id = id; c->titulo = titulo; c->categoria = categoria; c->popularidad = pop;
        for (int i = 0; i < cuenta; ++i) {
            std::string tag;
            if (!(in >> std::quoted(tag)) || !c->etiquetas.agregar(tag)) {
                delete c; return false;
            }
        }
        if (!finRegistro(in) || !s.contenidos_.insertar(id, c)) { delete c; return false; }
    }
    if (!contenidos.eof() && contenidos.fail()) return false;

    // Cargar los indices sin recalcular relaciones que ya existen en disco.
    s.contenidos_.recorrer([&](int id, const Contenido* c) {
        s.arbol_.agregarRuta(c->categoria);
        s.grafo_.agregarVertice(id);
    });
    std::ifstream similitudes(dir / archivos[3]);
    if (!similitudes) return false;
    while (std::getline(similitudes, linea)) {
        if (lineaVacia(linea)) continue;
        std::istringstream in(linea);
        int a, b; double peso;
        if (!(in >> a >> b >> peso) || !std::isfinite(peso) ||
            !finRegistro(in) || s.grafo_.similitud(a, b) > 0 || !s.grafo_.agregarRelacion(a, b, peso)) return false;
    }
    if (!similitudes.eof() && similitudes.fail()) return false;

    std::ifstream interacciones(dir / archivos[2]);
    if (!interacciones) return false;
    while (std::getline(interacciones, linea)) {
        if (lineaVacia(linea)) continue;
        std::istringstream in(linea);
        Interaccion i;
        int favorito;
        if (!(in >> i.idUsuario >> i.idContenido >> i.visualizaciones >> i.calificacion >> favorito) ||
            !finRegistro(in) || (favorito != 0 && favorito != 1) || !s.usuarios_.buscar(i.idUsuario) ||
            !s.contenidos_.buscar(i.idContenido)) return false;
        i.favorito = favorito == 1;
        Interaccion anterior;
        if (s.interacciones_.consultar(i.idUsuario, i.idContenido, anterior)) return false;
        if (!s.interacciones_.guardar(i)) return false;
    }
    if (!interacciones.eof() && interacciones.fail()) return false;
    return true;
}

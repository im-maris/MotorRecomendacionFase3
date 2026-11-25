#include "../MotorRecomendacion/Sistema.h"
#include "../MotorRecomendacion/HeapMaximo.h"
#include "../MotorRecomendacion/TablaHash.h"
#include "../MotorRecomendacion/Cola.h"
#include "../MotorRecomendacion/PilaAcciones.h"
#include <fstream>
#include <limits>
#include <chrono>
#include <memory>
#include <iomanip>
#include <stdexcept>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <sstream>

namespace fs = std::filesystem;
static int verificaciones = 0;
#define COMPROBAR(expresion) do { ++verificaciones; if (!(expresion)) \
    throw std::runtime_error(std::string("Fallo en linea ") + std::to_string(__LINE__) + ": " #expresion); } while (false)

// Carpeta exclusiva de cada prueba: no tocar los datos reales del usuario.
struct CarpetaPrueba {
    fs::path ruta;
    CarpetaPrueba() {
        auto marca = std::chrono::steady_clock::now().time_since_epoch().count();
        ruta = fs::temp_directory_path() / ("motor_fase3_" + std::to_string(marca));
        if (!fs::create_directory(ruta)) throw std::runtime_error("No se pudo crear carpeta de pruebas");
    }
    ~CarpetaPrueba() { std::error_code e; fs::remove_all(ruta, e); }
};

void probarTablaHash() {
    TablaHash<int> tabla(3);
    int datos[75];
    for (int i = 0; i < 75; ++i) {
        datos[i] = i * 7;
        COMPROBAR(tabla.insertar(i * 3 + 1, &datos[i]));
    }
    COMPROBAR(tabla.cantidad() == 75);
    COMPROBAR(!tabla.insertar(1, &datos[0]));
    for (int i = 0; i < 75; ++i) COMPROBAR(*tabla.buscar(i * 3 + 1) == i * 7);
    COMPROBAR(tabla.eliminar(1));
    COMPROBAR(tabla.eliminar(73));
    COMPROBAR(!tabla.eliminar(111111));
    COMPROBAR(tabla.buscar(1) == nullptr);
    COMPROBAR(tabla.cantidad() == 73);
}

void probarGrafo() {
    Grafo grafo;
    COMPROBAR(grafo.agregarVertice(1));
    COMPROBAR(grafo.agregarVertice(2));
    COMPROBAR(grafo.agregarVertice(3));
    COMPROBAR(!grafo.agregarVertice(1));
    COMPROBAR(grafo.agregarRelacion(1, 2, 0.75));
    COMPROBAR(grafo.agregarRelacion(2, 3, 0.5));
    COMPROBAR(std::abs(grafo.similitud(2, 1) - 0.75) < 1e-9);
    COMPROBAR(!grafo.agregarRelacion(1, 9, 0.3));
    COMPROBAR(!grafo.agregarRelacion(1, 2, 1.5));
    COMPROBAR(grafo.eliminarVertice(2));
    COMPROBAR(grafo.similitud(1, 2) == 0);
    COMPROBAR(grafo.similitud(3, 2) == 0);
}

void probarArbol() {
    ArbolCategorias arbol;
    COMPROBAR(arbol.agregarRuta("Tecnologia/Programacion/C++"));
    COMPROBAR(arbol.agregarRuta("Tecnologia/Programacion/Python"));
    COMPROBAR(arbol.agregarRuta("Arte/Pintura"));
    COMPROBAR(arbol.existeRuta("Tecnologia/Programacion"));
    COMPROBAR(arbol.existeRuta("Arte/Pintura"));
    COMPROBAR(!arbol.existeRuta("Tecnologia/Astronomia"));
    std::ostringstream salida;
    arbol.mostrar(salida);
    COMPROBAR(salida.str().find("C++") != std::string::npos);
    arbol.limpiar();
    COMPROBAR(!arbol.existeRuta("Arte/Pintura"));
}

void probarMatriz() {
    MatrizDispersa m;
    COMPROBAR(m.registrarVisualizacion(10, 100));
    COMPROBAR(m.registrarVisualizacion(10, 100));
    COMPROBAR(m.calificar(10, 100, 4));
    COMPROBAR(m.calificar(20, 100, 5));
    COMPROBAR(m.marcarFavorito(10, 200, true));
    COMPROBAR(m.calificar(30, 300, 1));
    COMPROBAR(m.cantidad() == 4);
    COMPROBAR(!m.calificar(10, 100, 6));
    Interaccion i;
    COMPROBAR(m.consultar(10, 100, i));
    COMPROBAR(i.visualizaciones == 2 && i.calificacion == 4);
    int contUsuario = 0, contContenido = 0;
    m.porUsuario(10, [&](const Interaccion&) { ++contUsuario; });
    m.porContenido(100, [&](const Interaccion&) { ++contContenido; });
    COMPROBAR(contUsuario == 2 && contContenido == 2);
    m.eliminarUsuario(10);
    COMPROBAR(m.cantidad() == 2);
    COMPROBAR(!m.consultar(10, 100, i));
    COMPROBAR(m.consultar(20, 100, i));
    m.eliminarContenido(100);
    COMPROBAR(m.cantidad() == 1);
    COMPROBAR(m.consultar(30, 300, i));
    m.limpiar();
    COMPROBAR(m.cantidad() == 0);
}

void probarHeap() {
    HeapMaximo heap(2);
    heap.insertar({8, 0.6});
    heap.insertar({6, 0.9});
    heap.insertar({7, 0.9});
    heap.insertar({9, 0.1});
    COMPROBAR(heap.extraerMaximo().idContenido == 6); // Empate, menor ID.
    COMPROBAR(heap.extraerMaximo().idContenido == 7);
    COMPROBAR(heap.extraerMaximo().idContenido == 8);
    COMPROBAR(heap.extraerMaximo().idContenido == 9);
    COMPROBAR(heap.vacio());
}

void probarSistema() {
    CarpetaPrueba temporal;
    const std::string ruta = (temporal.ruta / "datos").string();
    {
        Sistema s(ruta);
        COMPROBAR(s.cantidadContenidos() == 19);
        COMPROBAR(s.cantidadUsuarios() == 3);
        COMPROBAR(s.autenticar(1, "admin123") != nullptr);
        COMPROBAR(s.autenticar(2, "error") == nullptr);
        COMPROBAR(s.grafo().similitud(101, 102) > 0);
        COMPROBAR(s.categorias().existeRuta("Tecnologia/Programacion"));
        COMPROBAR(s.cantidadInteracciones() == 6);
        Candidato resultados[20];
        int n = s.recomendar(2, resultados, 20);
        COMPROBAR(n == 15);
        for (int j = 0; j < n; ++j) {
            COMPROBAR(resultados[j].puntuacion >= 0 && resultados[j].puntuacion <= 1);
            if (j > 0) COMPROBAR(resultados[j - 1].puntuacion >= resultados[j].puntuacion);
            Interaccion i;
            COMPROBAR(!s.matriz().consultar(2, resultados[j].idContenido, i));
        }
        COMPROBAR(s.registrarUsuario(9, "Eva", "claveEva"));
        COMPROBAR(!s.registrarUsuario(9, "Duplicado", "otra"));
        COMPROBAR(s.agregarPreferencia(9, "Tecnologia"));
        COMPROBAR(!s.agregarPreferencia(9, "tecnologia"));
        COMPROBAR(s.quitarPreferencia(9, "Tecnologia"));
        Contenido c;
        c.id = 200;
        c.titulo = "Contenido de prueba";
        c.categoria = "Tecnologia/Programacion";
        c.popularidad = 75;
        c.etiquetas.agregar("cpp");
        COMPROBAR(s.registrarContenido(c));
        COMPROBAR(s.contenido(200));
        COMPROBAR(s.grafo().similitud(101, 200) > 0);
        COMPROBAR(s.visualizar(9, 200));
        COMPROBAR(s.calificar(9, 200, 5));
        COMPROBAR(s.marcarFavorito(9, 200, true));
        COMPROBAR(s.cantidadInteracciones() == 7);
        c.titulo = "Contenido modificado";
        c.categoria = "Arte/Musica";
        COMPROBAR(s.modificarContenido(c));
        COMPROBAR(s.contenido(200)->titulo == "Contenido modificado");
        COMPROBAR(s.categorias().existeRuta("Arte/Musica"));
        COMPROBAR(s.guardar());
    }
    {
        Sistema s(ruta);
        COMPROBAR(s.cantidadContenidos() == 20);
        COMPROBAR(s.cantidadUsuarios() == 4);
        COMPROBAR(s.autenticar(9, "claveEva") != nullptr);
        COMPROBAR(s.contenido(200)->titulo == "Contenido modificado");
        COMPROBAR(s.cantidadInteracciones() == 7);
        Interaccion i;
        COMPROBAR(s.matriz().consultar(9, 200, i));
        COMPROBAR(i.visualizaciones == 1 && i.calificacion == 5 && i.favorito);
        COMPROBAR(s.eliminarContenido(200));
        COMPROBAR(!s.contenido(200));
        COMPROBAR(!s.matriz().consultar(9, 200, i));
        COMPROBAR(s.cantidadInteracciones() == 6);
    }
    {
        Sistema s(ruta);
        COMPROBAR(s.cantidadContenidos() == 19);
        COMPROBAR(s.cantidadInteracciones() == 6);
    }
    fs::remove_all(ruta);
}

void probarColaYPila() {
    Cola<int> cola;
    PilaAcciones<int> pila;
    COMPROBAR(cola.vacia() && pila.vacia());
    bool error = false;
    try { cola.desencolar(); } catch (const std::out_of_range&) { error = true; }
    COMPROBAR(error);
    error = false;
    try { pila.desapilar(); } catch (const std::out_of_range&) { error = true; }
    COMPROBAR(error);
    cola.encolar(1); cola.encolar(2); cola.encolar(3);
    pila.apilar(1); pila.apilar(2); pila.apilar(3);
    COMPROBAR(cola.desencolar() == 1 && cola.desencolar() == 2 && cola.desencolar() == 3);
    COMPROBAR(cola.vacia()); cola.encolar(9); COMPROBAR(cola.desencolar() == 9);
    COMPROBAR(pila.cima() == 3); pila.desapilar();
    pila.eliminarSi([](int v) { return v == 1; });
    COMPROBAR(pila.cima() == 2); pila.desapilar(); COMPROBAR(pila.vacia());
}

void probarBFS() {
    Grafo g;
    for (int id = 1; id <= 6; ++id) COMPROBAR(g.agregarVertice(id));
    COMPROBAR(g.agregarRelacion(1, 2, .4));
    COMPROBAR(g.agregarRelacion(1, 3, .9));
    COMPROBAR(g.agregarRelacion(2, 4, .5));
    COMPROBAR(g.agregarRelacion(3, 4, .8));
    COMPROBAR(g.agregarRelacion(4, 5, .6));
    COMPROBAR(!g.agregarRelacion(1, 6, std::numeric_limits<double>::quiet_NaN()));
    COMPROBAR(!g.agregarRelacion(1, 6, std::numeric_limits<double>::infinity()));
    AlcanceBFS salida[6];
    COMPROBAR(g.buscarAnchura(1, salida, 6, 0) == 1);
    COMPROBAR(g.buscarAnchura(1, salida, 6, 1) == 3);
    int n = g.buscarAnchura(1, salida, 6, 2);
    COMPROBAR(n == 4 && salida[0].id == 1 && salida[0].saltos == 0);
    COMPROBAR(salida[3].id == 4 && salida[3].saltos == 2 && std::abs(salida[3].peso - .72) < 1e-9);
    COMPROBAR(g.buscarAnchura(1, salida, 6, 9) == 5); // ciclo y nodo desconectado
    COMPROBAR(g.buscarAnchura(6, salida, 6) == 1);
    COMPROBAR(g.buscarAnchura(99, salida, 6) == 0);
    COMPROBAR(g.buscarAnchura(1, salida, 2) == 0);
    COMPROBAR(g.eliminarVertice(3));
    COMPROBAR(g.buscarAnchura(1, salida, 6, 2) == 3);
    COMPROBAR(salida[2].id == 4 && std::abs(salida[2].peso - .2) < 1e-9);
    g.limpiar(); COMPROBAR(g.buscarAnchura(1, salida, 6) == 0);
}

void probarAfinidad() {
    TablaHash<Contenido> catalogo;
    Contenido a, b, c, d;
    a.id = 1; b.id = 2; c.id = 3; d.id = 4;
    a.categoria = "Arte/Musica"; b.categoria = "Tecnologia/Programacion";
    c.categoria = "Ciencia/Fisica"; d.categoria = "Otros/Prueba";
    c.popularidad = 80;
    catalogo.insertar(1, &a); catalogo.insertar(2, &b); catalogo.insertar(3, &c); catalogo.insertar(4, &d);
    Grafo g;
    for (int j = 1; j <= 4; ++j) g.agregarVertice(j);
    g.agregarRelacion(1, 2, .8); g.agregarRelacion(2, 3, .9);
    Usuario u; u.id = 1; u.preferencias.agregar("Fisica");
    MatrizDispersa m; m.calificar(1, 1, 5);
    MotorRecomendacion motor;
    DetalleAfinidad detalle = motor.explicar(u, c, catalogo, m, g);
    COMPROBAR(detalle.relacionesIndirectas == 1 && std::abs(detalle.grafo - .5) < 1e-9);
    COMPROBAR(std::abs(detalle.total() - .58) < 1e-9); // .35 + .15 + .08
    COMPROBAR(motor.explicar(u, b, catalogo, m, g).grafo == 1.0);
    COMPROBAR(motor.explicar(u, d, catalogo, m, g).grafo == 0.0);
    Candidato resultados[5];
    COMPROBAR(motor.generar(u, catalogo, m, g, resultados, 0) == 0);
    COMPROBAR(motor.generar(u, catalogo, m, g, nullptr, 5) == 0);
    COMPROBAR(motor.generar(u, catalogo, m, g, resultados, 5) == 3);
    for (int j = 2; j <= 4; ++j) m.calificar(1, j, 3);
    COMPROBAR(motor.generar(u, catalogo, m, g, resultados, 5) == 0);
    TablaHash<Contenido> vacio;
    COMPROBAR(motor.generar(u, vacio, m, g, resultados, 5) == 0);
    Usuario nuevo; nuevo.id = 2;
    COMPROBAR(motor.generar(nuevo, catalogo, m, g, resultados, 5) == 4);
    COMPROBAR(resultados[0].idContenido == 3 && std::abs(resultados[0].puntuacion - .08) < 1e-9);
}

void probarCasosLimiteYDeshacer() {
    ArbolCategorias arbol;
    COMPROBAR(!arbol.agregarRuta("Nueva/ /Invalida") && !arbol.existeRuta("Nueva"));
    COMPROBAR(!arbol.agregarRuta("A/") && !arbol.agregarRuta("/A"));
    ListaTextos lista;
    COMPROBAR(!lista.agregar("a\nb"));
    for (int j = 0; j < 1000; ++j) COMPROBAR(lista.agregar(std::to_string(j)));
    COMPROBAR(!lista.agregar("exceso"));
    MatrizDispersa m;
    COMPROBAR(!m.marcarFavorito(1, 1, false) && m.cantidad() == 0);
    COMPROBAR(m.marcarFavorito(1, 1, true) && m.marcarFavorito(1, 1, false) && m.cantidad() == 0);
    COMPROBAR(m.guardar({1,1,std::numeric_limits<int>::max(),0,false}));
    COMPROBAR(!m.registrarVisualizacion(1,1));
    CarpetaPrueba temporal;
    std::string ruta = (temporal.ruta / "datos").string();
    {
        Sistema s(ruta);
        COMPROBAR(!s.deshacerInteraccion(2));
        COMPROBAR(!s.registrarUsuario(8, "a\nb", "clave"));
        Contenido invalido; invalido.id = 200; invalido.titulo = "Prueba"; invalido.categoria = "Nueva/ /Invalida";
        COMPROBAR(!s.registrarContenido(invalido) && !s.categorias().existeRuta("Nueva"));
        COMPROBAR(!s.marcarFavorito(2, 102, false));
        COMPROBAR(s.calificar(2, 102, 3) && s.calificar(2, 102, 5));
        COMPROBAR(s.marcarFavorito(3, 102, true));
        Interaccion i;
        COMPROBAR(s.deshacerInteraccion(2) && s.matriz().consultar(2, 102, i) && i.calificacion == 3);
        COMPROBAR(s.deshacerInteraccion(2) && !s.matriz().consultar(2, 102, i));
        COMPROBAR(s.matriz().consultar(3, 102, i) && i.favorito);
        COMPROBAR(s.deshacerInteraccion(3) && !s.matriz().consultar(3, 102, i));
        COMPROBAR(s.visualizar(2, 101) && s.deshacerInteraccion(2));
        COMPROBAR(s.matriz().consultar(2, 101, i) && i.visualizaciones == 1);
        COMPROBAR(s.marcarFavorito(2, 102, true) && s.marcarFavorito(2, 102, false));
        COMPROBAR(s.deshacerInteraccion(2) && s.matriz().consultar(2, 102, i) && i.favorito);
        COMPROBAR(s.eliminarContenido(102) && !s.deshacerInteraccion(2));
    }
    Sistema reinicio(ruta);
    COMPROBAR(!reinicio.contenido(102) && !reinicio.deshacerInteraccion(2));
}

void probarPersistenciaRobusta() {
    CarpetaPrueba temporal;
    std::string ruta = (temporal.ruta / "datos").string();
    {
        Sistema s(ruta);
        COMPROBAR(s.calificar(2, 102, 4));
        fs::create_directory(ruta + ".fase3.tmp"); // simula bloqueo del guardado sin tocar originales
        COMPROBAR(!s.registrarUsuario(20, "Prueba", "clave") && !s.usuario(20));
        COMPROBAR(!s.agregarPreferencia(2, "Nueva") && !s.usuario(2)->preferencias.contiene("Nueva"));
        COMPROBAR(!s.quitarPreferencia(2, "Programacion") && s.usuario(2)->preferencias.contiene("Programacion"));
        Contenido c = *s.contenido(102); c.id = 200;
        COMPROBAR(!s.registrarContenido(c) && !s.contenido(200) && s.grafo().totalVertices() == 19);
        c.id = 102; c.titulo = "Cambio";
        COMPROBAR(!s.modificarContenido(c) && s.contenido(102)->titulo != "Cambio");
        COMPROBAR(!s.eliminarContenido(102) && s.contenido(102));
        COMPROBAR(!s.calificar(2, 102, 5));
        COMPROBAR(!s.deshacerInteraccion(2));
        Interaccion i;
        COMPROBAR(s.matriz().consultar(2, 102, i) && i.calificacion == 4);
        COMPROBAR(s.grafo().similitud(101, 102) > 0);
        fs::remove(ruta + ".fase3.tmp");
        COMPROBAR(s.deshacerInteraccion(2) && !s.matriz().consultar(2, 102, i));
    }
    fs::rename(ruta, ruta + ".fase3.bak"); // interrupcion antes de publicar
    {
        Sistema s(ruta);
        COMPROBAR(s.cantidadContenidos() == 19 && !fs::exists(ruta + ".fase3.bak"));
    }
    fs::copy(ruta, ruta + ".fase3.bak", fs::copy_options::recursive); // despues de publicar
    { Sistema s(ruta); COMPROBAR(!fs::exists(ruta + ".fase3.bak")); }
    { std::ofstream f(fs::path(ruta) / "interacciones.txt", std::ios::app); f << "registro roto\n"; }
    bool fallo = false;
    try { Sistema s(ruta); } catch (const std::runtime_error&) { fallo = true; }
    COMPROBAR(fallo && fs::exists(fs::path(ruta) / "interacciones.txt"));
    fs::remove(fs::path(ruta) / "usuarios.txt");
    fallo = false;
    try { Sistema s(ruta); } catch (const std::runtime_error&) { fallo = true; }
    COMPROBAR(fallo && !fs::exists(fs::path(ruta) / "usuarios.txt"));
}

// Medicion reproducible del motor aislado, con un grafo disperso sintetico.
void medirRendimiento() {
    using Reloj = std::chrono::steady_clock;
    std::cout << "contenidos,aristas,bfs_ms,recomendacion_ms\n";
    for (int n : {19, 100, 500, 1000, 5000}) {
        std::unique_ptr<Contenido[]> datos(new Contenido[n]);
        TablaHash<Contenido> catalogo; Grafo g; MatrizDispersa m; Usuario u; u.id = 1;
        u.preferencias.agregar("Tema");
        for (int j = 0; j < n; ++j) {
            datos[j].id = j+1; datos[j].titulo = "Tema " + std::to_string(j+1);
            datos[j].categoria = "Prueba/Tema"; datos[j].popularidad = 80;
            catalogo.insertar(j+1, &datos[j]); g.agregarVertice(j+1);
        }
        for (int j = 1; j < n; ++j) g.agregarRelacion(j, j+1, .7);
        for (int j = 1; j <= 5; ++j) m.calificar(1, j, 5);
        std::unique_ptr<AlcanceBFS[]> alcance(new AlcanceBFS[n]);
        MotorRecomendacion motor; Candidato salida[10];
        auto inicio = Reloj::now();
        for (int j = 0; j < 100; ++j) g.buscarAnchura(1, alcance.get(), n, n);
        auto medio = Reloj::now();
        for (int j = 0; j < 3; ++j) motor.generar(u, catalogo, m, g, salida, 10);
        auto fin = Reloj::now();
        std::cout << n << ',' << n-1 << ',' << std::fixed << std::setprecision(4)
                  << std::chrono::duration<double,std::milli>(medio-inicio).count()/100 << ','
                  << std::chrono::duration<double,std::milli>(fin-medio).count()/3 << '\n';
    }
}

int main(int argc, char** argv) {
    try {
        if (argc > 1 && std::string(argv[1]) == "--rendimiento") { medirRendimiento(); return 0; }
        probarTablaHash(); probarGrafo(); probarArbol(); probarMatriz(); probarHeap(); probarSistema();
        probarColaYPila(); probarBFS(); probarAfinidad(); probarCasosLimiteYDeshacer(); probarPersistenciaRobusta();
        std::cout << "TODAS LAS PRUEBAS PASARON: 11 grupos, " << verificaciones << " verificaciones.\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n'; return 1;
    }
}

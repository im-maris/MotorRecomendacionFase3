#include "Grafo.h"
#include "Cola.h"
#include <cmath>
#include <algorithm>

Grafo::~Grafo() { limpiar(); }

Grafo::Vertice* Grafo::buscarVertice(int id) const {
    return indice_.buscar(id);
}

void Grafo::limpiar() {
    while (vertices_) {
        Vertice* v = vertices_;
        vertices_ = v->siguiente;
        Arista* a = v->aristas;
        while (a) {
            Arista* borrar = a;
            a = a->siguiente;
            delete borrar;
        }
        indice_.eliminar(v->id);
        delete v;
    }
    totalVertices_ = 0;
}

bool Grafo::agregarVertice(int id) {
    if (id <= 0 || buscarVertice(id)) return false;
    Vertice* nuevo = new Vertice{id, nullptr, vertices_};
    indice_.insertar(id, nuevo);
    vertices_ = nuevo;
    ++totalVertices_;
    return true;
}

void Grafo::ponerArista(Vertice* origen, int destino, double peso) {
    for (Arista* a = origen->aristas; a; a = a->siguiente) {
        if (a->destino == destino) { a->peso = peso; return; }
    }
    origen->aristas = new Arista{destino, peso, origen->aristas};
}

void Grafo::quitarArista(Vertice* origen, int destino) {
    Arista** enlace = &origen->aristas;
    while (*enlace) {
        if ((*enlace)->destino == destino) {
            Arista* borrar = *enlace;
            *enlace = borrar->siguiente;
            delete borrar;
            return;
        }
        enlace = &(*enlace)->siguiente;
    }
}

bool Grafo::agregarRelacion(int a, int b, double peso) {
    if (a == b || !std::isfinite(peso) || peso <= 0.0 || peso > 1.0) return false;
    Vertice* va = buscarVertice(a);
    Vertice* vb = buscarVertice(b);
    if (!va || !vb) return false;
    ponerArista(va, b, peso);
    ponerArista(vb, a, peso);
    return true;
}

bool Grafo::eliminarVertice(int id) {
    Vertice** enlace = &vertices_;
    while (*enlace && (*enlace)->id != id) enlace = &(*enlace)->siguiente;
    if (!*enlace) return false;
    Vertice* borrar = *enlace;
    for (Vertice* v = vertices_; v; v = v->siguiente)
        if (v != borrar) quitarArista(v, id);
    Arista* a = borrar->aristas;
    while (a) { Arista* siguiente = a->siguiente; delete a; a = siguiente; }
    *enlace = borrar->siguiente;
    indice_.eliminar(id);
    delete borrar;
    --totalVertices_;
    return true;
}

double Grafo::similitud(int a, int b) const {
    Vertice* v = buscarVertice(a);
    if (!v) return 0.0;
    for (Arista* arista = v->aristas; arista; arista = arista->siguiente)
        if (arista->destino == b) return arista->peso;
    return 0.0;
}

// BFS descubre cada vertice una vez. Entre rutas con los mismos saltos,
// conserva el mayor producto de pesos; la cola procesa niveles completos.
// salida debe tener capacidad >= totalVertices() para evitar resultados parciales.
int Grafo::buscarAnchura(int origen, AlcanceBFS* salida, int capacidad, int maxSaltos) const {
    if (!salida || capacidad < totalVertices_ || maxSaltos < 0 || !buscarVertice(origen)) return 0;
    TablaHash<AlcanceBFS> visitados;
    Cola<AlcanceBFS*> pendientes;
    int n = 1;
    salida[0] = {origen, 0, 1.0};
    visitados.insertar(origen, &salida[0]);
    pendientes.encolar(&salida[0]);
    while (!pendientes.vacia()) {
        AlcanceBFS* actual = pendientes.desencolar();
        if (actual->saltos >= maxSaltos) continue;
        Vertice* v = buscarVertice(actual->id);
        for (Arista* a = v->aristas; a; a = a->siguiente) {
            double peso = actual->peso * a->peso;
            AlcanceBFS* previo = visitados.buscar(a->destino);
            if (previo) {
                if (previo->saltos == actual->saltos + 1) previo->peso = std::max(previo->peso, peso);
                continue;
            }
            salida[n] = {a->destino, actual->saltos + 1, peso};
            visitados.insertar(a->destino, &salida[n]);
            pendientes.encolar(&salida[n++]);
        }
    }
    return n;
}

#pragma once
#include "TablaHash.h"

struct AlcanceBFS {
    int id = 0;
    int saltos = 0;
    double peso = 0.0; // producto de aristas en la mejor ruta de distancia minima
};

class Grafo {
    struct Arista {
        int destino;
        double peso;
        Arista* siguiente;
    };
    struct Vertice {
        int id;
        Arista* aristas;
        Vertice* siguiente;
    };
    TablaHash<Vertice> indice_;
    Vertice* vertices_ = nullptr;
    int totalVertices_ = 0;
    Vertice* buscarVertice(int id) const;
    static void ponerArista(Vertice* origen, int destino, double peso);
    static void quitarArista(Vertice* origen, int destino);
public:
    Grafo() = default;
    Grafo(const Grafo&) = delete;
    Grafo& operator=(const Grafo&) = delete;
    ~Grafo();
    void limpiar();
    void intercambiar(Grafo& otro) noexcept {
        indice_.intercambiar(otro.indice_);
        std::swap(vertices_, otro.vertices_);
        std::swap(totalVertices_, otro.totalVertices_);
    }
    bool agregarVertice(int id);
    bool agregarRelacion(int a, int b, double peso); // No dirigido, peso (0,1].
    bool eliminarVertice(int id);
    double similitud(int a, int b) const;
    int buscarAnchura(int origen, AlcanceBFS* salida, int capacidad, int maxSaltos = 2) const;
    int totalVertices() const { return totalVertices_; }
    template<typename Funcion>
    void recorrerRelaciones(Funcion funcion) const {
        for (Vertice* v = vertices_; v; v = v->siguiente)
            for (Arista* a = v->aristas; a; a = a->siguiente)
                if (v->id < a->destino) funcion(v->id, a->destino, a->peso);
    }
};

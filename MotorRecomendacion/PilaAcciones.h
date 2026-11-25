#pragma once
#include <stdexcept>

// Pila LIFO enlazada propia. Se usa para restaurar interacciones anteriores.
template<typename T>
class PilaAcciones {
    struct Nodo { T valor; Nodo* siguiente; };
    Nodo* cima_ = nullptr;
public:
    PilaAcciones() = default;
    PilaAcciones(const PilaAcciones&) = delete;
    PilaAcciones& operator=(const PilaAcciones&) = delete;
    ~PilaAcciones() { limpiar(); }
    bool vacia() const { return cima_ == nullptr; }
    void apilar(const T& valor) { cima_ = new Nodo{valor, cima_}; }
    const T& cima() const {
        if (vacia()) throw std::out_of_range("Pila vacia");
        return cima_->valor;
    }
    void desapilar() {
        if (vacia()) throw std::out_of_range("Pila vacia");
        Nodo* n = cima_; cima_ = n->siguiente; delete n;
    }
    void limpiar() { while (!vacia()) desapilar(); }
    template<typename Predicado>
    void eliminarSi(Predicado predicado) {
        Nodo** enlace = &cima_;
        while (*enlace) {
            Nodo* n = *enlace;
            if (predicado(n->valor)) { *enlace = n->siguiente; delete n; }
            else enlace = &n->siguiente;
        }
    }
};

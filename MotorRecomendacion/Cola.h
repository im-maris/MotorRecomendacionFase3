#pragma once
#include <stdexcept>

// Cola FIFO enlazada propia: insertar al final y extraer al frente O(1).
template<typename T>
class Cola {
    struct Nodo { T valor; Nodo* siguiente; };
    Nodo* frente_ = nullptr;
    Nodo* fin_ = nullptr;
public:
    Cola() = default;
    Cola(const Cola&) = delete;
    Cola& operator=(const Cola&) = delete;
    ~Cola() { while (frente_) { Nodo* n = frente_; frente_ = n->siguiente; delete n; } }
    bool vacia() const { return frente_ == nullptr; }
    void encolar(const T& valor) {
        Nodo* n = new Nodo{valor, nullptr};
        if (fin_) fin_->siguiente = n; else frente_ = n;
        fin_ = n;
    }
    T desencolar() {
        if (vacia()) throw std::out_of_range("Cola vacia");
        T valor = frente_->valor;
        Nodo* n = frente_; frente_ = n->siguiente;
        if (!frente_) fin_ = nullptr;
        delete n;
        return valor;
    }
};

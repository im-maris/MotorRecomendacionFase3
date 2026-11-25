#pragma once
#include "ListaTextos.h"
#include "PilaAcciones.h"
#include <string>

enum class RolUsuario { NORMAL = 0, ADMINISTRADOR = 1 };

struct Contenido {
    int id = 0;
    std::string titulo;
    std::string categoria; // Ruta, ejemplo: Tecnologia/Programacion.
    ListaTextos etiquetas;
    double popularidad = 0.0; // De 0 a 100.
};

struct Interaccion {
    int idUsuario = 0;
    int idContenido = 0;
    int visualizaciones = 0;
    int calificacion = 0; // 0 significa sin calificar; valores validos: 1 a 5.
    bool favorito = false;
};

struct CambioInteraccion {
    Interaccion anterior;
    bool existia = false;
};

struct Usuario {
    int id = 0;
    std::string nombre;
    std::string clave; // Prototipo academico: se almacena en texto plano.
    RolUsuario rol = RolUsuario::NORMAL;
    ListaTextos preferencias;
    PilaAcciones<CambioInteraccion> acciones; // historial temporal por usuario
};

struct DetalleAfinidad {
    double preferencias = 0.0, historial = 0.0, grafo = 0.0, popularidad = 0.0;
    int relacionesIndirectas = 0;
    double total() const { return 0.35 * preferencias + 0.25 * historial + 0.30 * grafo + 0.10 * popularidad; }
};

struct Candidato {
    int idContenido = 0;
    double puntuacion = 0.0; // 0 a 1.
    DetalleAfinidad detalle{};
};

#pragma once
#include <string>

class Sistema;

class GestorPersistencia {
public:
    static void recuperar(const std::string& carpeta);
    static void confirmarRecuperacion(const std::string& carpeta);
    static bool hayAlgunArchivo(const std::string& carpeta);
    static bool cargar(Sistema& sistema, const std::string& carpeta);
    static bool guardar(const Sistema& sistema, const std::string& carpeta);
};

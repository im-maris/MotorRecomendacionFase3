#include "MotorRecomendacion.h"
#include "HeapMaximo.h"
#include "Utilidades.h"
#include <algorithm>
#include <memory>

namespace {
    std::string raizCategoria(const std::string& ruta) {
        size_t pos = ruta.find('/');
        return minusculas(recortar(ruta.substr(0, pos)));
    }
}

double MotorRecomendacion::valoracion(const Interaccion& i) {
    double v = i.calificacion > 0 ? (i.calificacion - 1) / 4.0 : 0.5;
    if (i.favorito && v < 0.9) v = 0.9;
    return v;
}

double MotorRecomendacion::preferencia(const Usuario& u, const Contenido& c) {
    if (u.preferencias.cantidad() == 0) return 0.0;
    int aciertos = 0;
    for (int j = 0; j < u.preferencias.cantidad(); ++j) {
        const std::string& pref = u.preferencias.obtener(j);
        bool coincide = contieneTexto(c.categoria, pref) || contieneTexto(c.titulo, pref);
        for (int k = 0; !coincide && k < c.etiquetas.cantidad(); ++k)
            coincide = contieneTexto(c.etiquetas.obtener(k), pref);
        if (coincide) ++aciertos;
    }
    return static_cast<double>(aciertos) / u.preferencias.cantidad();
}

double MotorRecomendacion::categoriaComun(const Contenido& a, const Contenido& b) {
    if (textoIgual(a.categoria, b.categoria)) return 1.0;
    return raizCategoria(a.categoria) == raizCategoria(b.categoria) ? 0.4 : 0.0;
}

DetalleAfinidad MotorRecomendacion::explicar(const Usuario& usuario, const Contenido& candidato,
    const TablaHash<Contenido>& catalogo, const MatrizDispersa& matriz,
    const Grafo& grafo) const {
    double sumaHist = 0.0, pesosHist = 0.0;
    double sumaGrafo = 0.0, pesosGrafo = 0.0;

    DetalleAfinidad detalle;
    std::unique_ptr<AlcanceBFS[]> alcance(new AlcanceBFS[grafo.totalVertices()]);
    int n = grafo.buscarAnchura(candidato.id, alcance.get(), grafo.totalVertices(), 2);
    TablaHash<AlcanceBFS> indice;
    for (int j = 0; j < n; ++j) indice.insertar(alcance[j].id, &alcance[j]);
    matriz.porUsuario(usuario.id, [&](const Interaccion& i) {
        const Contenido* visto = catalogo.buscar(i.idContenido);
        if (!visto || visto->id == candidato.id) return;
        double satisfaccion = valoracion(i);
        double pesoCat = categoriaComun(candidato, *visto);
        if (pesoCat > 0) {
            sumaHist += pesoCat * satisfaccion;
            pesosHist += pesoCat;
        }
        const AlcanceBFS* relacion = indice.buscar(visto->id);
        if (relacion && relacion->peso > 0) {
            // Dos saltos aportan menos evidencia que una relacion directa.
            double factor = relacion->saltos == 2 ? 0.5 : 1.0;
            if (relacion->saltos == 2) ++detalle.relacionesIndirectas;
            sumaGrafo += relacion->peso * satisfaccion * factor;
            pesosGrafo += relacion->peso;
        }
    });

    const double p = preferencia(usuario, candidato);
    const double h = pesosHist > 0 ? sumaHist / pesosHist : 0.0;
    const double g = pesosGrafo > 0 ? sumaGrafo / pesosGrafo : 0.0;
    const double popularidad = std::clamp(candidato.popularidad / 100.0, 0.0, 1.0);
    detalle.preferencias = p;
    detalle.historial = h;
    detalle.grafo = g;
    detalle.popularidad = popularidad;
    return detalle;
}

double MotorRecomendacion::afinidad(const Usuario& usuario, const Contenido& candidato,
    const TablaHash<Contenido>& catalogo, const MatrizDispersa& matriz, const Grafo& grafo) const {
    return std::clamp(explicar(usuario, candidato, catalogo, matriz, grafo).total(), 0.0, 1.0);
}

int MotorRecomendacion::generar(const Usuario& usuario,
    const TablaHash<Contenido>& catalogo, const MatrizDispersa& matriz,
    const Grafo& grafo, Candidato* salida, int limite) const {
    if (limite <= 0 || !salida) return 0;
    HeapMaximo heap;
    catalogo.recorrer([&](int id, const Contenido* c) {
        Interaccion existente;
        // Se excluye cualquier interaccion registrada (vista, calificada o favorita).
        if (matriz.consultar(usuario.id, id, existente)) return;
        DetalleAfinidad detalle = explicar(usuario, *c, catalogo, matriz, grafo);
        heap.insertar(Candidato{id, std::clamp(detalle.total(), 0.0, 1.0), detalle});
    });
    int n = 0;
    while (n < limite && !heap.vacio()) salida[n++] = heap.extraerMaximo();
    return n;
}

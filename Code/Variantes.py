from Code import GestorVariantes


def editaVariante(procesador, gestorBase, fen, lineaPGN, titulo=None, siEngineActivo=False, siCompetitivo=False):
    wpantalla = procesador.pantalla
    liKibitzersActivas = procesador.liKibitzersActivas
    xtutor = procesador.XTutor()
    procesadorVariantes = procesador.clonVariantes(wpantalla, liKibitzersActivas, xtutor)

    gestorVariantes = GestorVariantes.GestorVariantes(procesadorVariantes)
    if hasattr(gestorBase, "tablero"):
        siBlancasAbajo = gestorBase.tablero.siBlancasAbajo
    else:
        siBlancasAbajo = True
    # siBlancasAbajo = gestorBase.siBlancasAbajo if hasattr( gestorBase, "siBlancasAbajo" ) else True
    gestorVariantes.inicio(fen, lineaPGN, gestorVariantes.siMiraKibitzers(), siBlancasAbajo, siEngineActivo, siCompetitivo)
    procesadorVariantes.gestor = gestorVariantes

    if titulo is None:
        titulo = lineaPGN

    procesadorVariantes.pantalla.muestraVariantes(titulo)

    return gestorVariantes.valor()  # pgn y a1h8, el a1h8 nos servira para editar las aperturas


def editaVarianteMoves(procesador, wpantalla, siBlancasAbajo, fen, lineaPGN, titulo=None):
    procesadorVariantes = procesador.clonVariantes(wpantalla)

    gestorVariantes = GestorVariantes.GestorVariantes(procesadorVariantes)
    gestorVariantes.inicio(fen, lineaPGN, False, siBlancasAbajo)
    procesadorVariantes.gestor = gestorVariantes

    if titulo is None:
        titulo = lineaPGN
    procesadorVariantes.pantalla.muestraVariantes(titulo)

    return gestorVariantes.valor()  # pgn y a1h8, el a1h8 nos servira para editar las aperturas

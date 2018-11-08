kInicio, kJugando, kFinJuego, kEntrenando, kVisorPGNini, kVisorPGNfic, kVisorPGNpas = range(7)

kSigueApertura, kSigueUsuario, kMalApertura = range(3)

(
    kJugNueva, kJugEntPos, kJugPGN, kJugEntMaq, kJugGM, kJugRemoto, kJugSolo, kJug60, kJugElo, kJugMicElo,
    kJugBooks, kJugAperturas, kJugOpeningLines, kJugBoxing, kJugEntTac, kJugMvM, kJugAlbum, kJugFics, kJugFide,
    kJugLichess, kJugWorldMap, kJugRoute, kJugEntLight, kJugWashingCreate, kJugWashingTactics, kJugWashingReplay,
    kJugSingularMoves, kJugAnotar
) = range(28)

kFinNormal, kFinReinicio = range(2)

(
    kGanamos, kGanaRival, kTablas, kTablasRepeticion, kTablas50, kTablasFaltaMaterial, kGanamosTiempo,
    kGanaRivalTiempo, kTablasAcuerdo, kDesconocido
) = range(10)

kMoverAdelante, kMoverAtras, kMoverInicio, kMoverFinal, kMoverLibre, kMoverReloj = range(6)

(
    k_terminar, k_play, k_mainmenu, k_competicion, k_competir, k_entrenamiento, k_opciones,
    k_informacion, k_grabar, k_grabarComo, k_file, k_recuperar, k_abandonar, k_reiniciar, k_atras,
    k_aplazar, k_finpartida, k_ent_empezar, k_ent_otro, k_pgnFin, k_pgnPaste,
    k_pgnFichero, k_pgnInformacion, k_pgnFicheroRepite, k_pgnNuestroFichero, k_jugadadia, k_pgnComandoExterno,
    k_rendirse, k_tablas, k_libros,
    k_peliculaTerminar, k_peliculaLento, k_peliculaPausa, k_peliculaSeguir, k_peliculaRapido, k_peliculaRepetir,
    k_peliculaPGN, k_jugar, k_anterior, k_siguiente, k_trasteros,
    k_ayuda, k_mateNivel, k_ayudaMover,
    k_aceptar, k_cancelar,
    k_configurar, k_utilidades, k_variantes, k_tools, k_elo, k_cambiar, k_libre, k_showtext, k_enviar, k_atajos,
    k_forceEnd
) = range(57)

kMP_1, kMP_2, kMP_3, kMP_4, kMP_5, kMP_6, kMP_7 = range(7)

kZvalue_pieza, kZvalue_piezaMovimiento = 10, 20

kTutorH, kTutorH2_1, kTutorH1_2, kTutorV = range(4)

(
    kAjustarMejor, kAjustarSuperior, kAjustarSimilar, kAjustarInferior, kAjustarPeor, kAjustarPlayer,
    kAjustarNivelAlto, kAjustarNivelMedio, kAjustarNivelBajo, kAjustarSuperiorM, kAjustarSuperiorMM,
    kAjustarInferiorM, kAjustarInferiorMM
) = range(13)

kControlTableroNo, kControlTableroGeneral, kControlTableroParticular = range(3)

kNoBlindfold, kBlindfoldConfig, kBlindfoldWhite, kBlindfoldBlack, kBlindfoldAll = range(5)


class KRegistro:
    pass


def prlk(*x):
    import sys
    for l in x:
        sys.stdout.write(str(l))
        sys.stdout.write(" ")


def prlkn(*x):
    import sys
    for l in x:
        sys.stdout.write(l)
        sys.stdout.write(" ")
    sys.stdout.write("\n")


def stack(siPrevio=False):
    import traceback

    if siPrevio:
        prlk("-" * 80 + "\n")
        prlk(traceback.format_stack())
        prlk("\n" + "-" * 80 + "\n")
    for line in traceback.format_stack()[:-1]:
        prlk(line.strip() + "\n")

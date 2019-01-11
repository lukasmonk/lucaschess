from PyQt4 import QtCore

from Code import DGT

from Code.QT import FormLayout
from Code.QT import Iconos
from Code import Util
from Code.Constantes import *


def opciones(parent, configuracion):
    separador = (None, None)

    # Datos generales
    liGen = [separador]

    # # Nombre del jugador
    liGen.append((_("Player's name") + ":", configuracion.jugador))

    # # Estilo
    liGen.append((_("Window style") + ":", configuracion.estilos()))

    liTraducciones = configuracion.listaTraducciones()
    trActual = configuracion.traductor if configuracion.traductor else "en"
    li = [trActual]
    for k, trad, porc, author in liTraducciones:
        rotulo = "%s" % trad
        if int(porc) < 90:
           rotulo += " (%s%%)"%porc
        li.append((k, rotulo))
    liGen.append((_("Language") + ":", li))
    liGen.append(separador)
    liGen.append((_("Check for updates at startup") + ":", configuracion.checkforupdate))

    # Engines
    liEng = [separador]
    liMedidas = [configuracion.centipawns,
                 (True, "%s = 100 %s  ->   64, 19, -19, +23, -120, +130" % (_("One pawn"), _("points"))),
                 (False, "%s = 1.00 %s ->   0.64, 0.19, -0.19, +0.23, -1.20, +1.30" % (_("One pawn"), _("points")))]
    liEng.append((_("By showing scores from the engines") + ":", liMedidas))
    liEng.append(separador)
    liEng.append((_("OS 64bits BMI2 compatible") + ":", configuracion.bmi2))
    liEng.append(separador)
    liEng.append((_("Work in the background, when possible") + ":", not configuracion.notbackground))
    liEng.append(separador)
    liEng.append(("%s -> %s:" %(_("Save engines log"), "UsrData/EngineLogs"), configuracion.siLogEngines))

    # Aspect
    liAsp = []

    liAsp.append((_("By default") + ":", False))
    liAsp.append(separador)

    ## font general
    liAsp.append((FormLayout.FontCombobox(_("Font")), configuracion.familia))

    ## Menus
    liAsp.append((None, _("Menus") + ":"))
    liAsp.append((FormLayout.Spinbox(_("Font size"), 5, 64, 60), configuracion.puntosMenu))
    liAsp.append((_("Bold") + ":", configuracion.boldMenu))

    ## Toolbars
    liAsp.append(separador)
    liAsp.append((None, _("Toolbars") + ":"))
    liAsp.append((FormLayout.Spinbox(_("Font size"), 5, 64, 60), configuracion.puntosTB))
    liAsp.append((_("Bold") + ":", configuracion.boldTB))
    li = (
            (_("Only display the icon"), QtCore.Qt.ToolButtonIconOnly),
            (_("Only display the text"), QtCore.Qt.ToolButtonTextOnly),
            (_("The text appears beside the icon"), QtCore.Qt.ToolButtonTextBesideIcon),
            (_("The text appears under the icon"), QtCore.Qt.ToolButtonTextUnderIcon)
    )
    config = FormLayout.Combobox(_("Icons"), li)
    liAsp.append((config, configuracion.iconsTB))

    ## PGN table
    liAsp.append(separador)
    liAsp.append((None, _("PGN table") + ":"))
    liAsp.append((FormLayout.Spinbox(_("Width"), 283, 1000, 70), configuracion.anchoPGN))
    liAsp.append((FormLayout.Spinbox(_("Height of each row"), 18, 99, 40), configuracion.altoFilaPGN))
    liAsp.append((FormLayout.Spinbox(_("Font size"), 10, 99, 40), configuracion.puntosPGN))
    liAsp.append((_("PGN always in English") + ":", configuracion.siNomPiezasEN))
    liAsp.append((_("PGN with figurines") + ":", configuracion.figurinesPGN))

    liAsp.append(separador)
    liAsp.append((FormLayout.Spinbox(_("Font size of information labels"), 8, 30, 40), configuracion.tamFontRotulos))

    # Sonidos
    liSon = [separador]
    liSon.append(separador)
    # Si a_adimos sonido tras cada jugada
    liSon.append((_("Beep after opponent's move") + ":", configuracion.siSuenaBeep))
    liSon.append(separador)
    liSon.append((None, _("Sound on in") + ":"))
    liSon.append((_("Results") + ":", configuracion.siSuenaResultados))
    liSon.append((_("Rival moves") + ":", configuracion.siSuenaJugada))
    liSon.append(separador)
    liSon.append((_("Activate sounds with our moves") + ":", configuracion.siSuenaNuestro))
    liSon.append(separador)

    # Tutor
    liTT = [separador]
    liTT.append((_("Engine") + ":", configuracion.ayudaCambioTutor()))
    liTT.append((_("Duration of tutor analysis (secs)") + ":", float(configuracion.tiempoTutor / 1000.0)))
    liDepths = [("--", 0)]
    for x in range(1, 51):
        liDepths.append((str(x), x))
    config = FormLayout.Combobox(_("Depth"), liDepths)
    liTT.append((config, configuracion.depthTutor))

    li = [(_("Maximum"), 0)]
    for x in (1, 3, 5, 10, 15, 20, 30, 40, 50, 75, 100, 150, 200):
        li.append((str(x), x))
    config = FormLayout.Combobox(_("Number of moves evaluated by engine(MultiPV)"), li)
    liTT.append((config, configuracion.tutorMultiPV))
    liTT.append(separador)
    liTT.append((_("Tutor enabled"), configuracion.tutorActivoPorDefecto))
    liTT.append(separador)

    # Mostrando el tutor
    # kTutorH, kTutorH2_1, kTutorH1_2, kTutorV
    liPosTutor = [configuracion.vistaTutor, (kTutorH, _("Horizontal")),
                  (kTutorH2_1, _("Horizontal") + " 2+1"),
                  (kTutorH1_2, _("Horizontal") + " 1+2"),
                  (kTutorV, _("Vertical"))]
    liTT.append((_("Tutor boards position") + ":", liPosTutor))
    liTT.append(separador)
    liTT.append(separador)
    liTT.append((None, _("Sensitivity")))
    liTT.append((FormLayout.Spinbox(_("Minimum difference in centipawns"), 0, 1000, 70), configuracion.tutorDifPts))
    liTT.append((FormLayout.Spinbox(_("Minimum difference in %"), 0, 1000, 70), configuracion.tutorDifPorc))

    # Perfomance
    perf = configuracion.perfomance
    liPR = [separador]
    def d(num):
        return " (%s %d)" % (_("default"), num)
    liPR.append((None, _("Bad moves: lost points to consider a move as bad")))
    liPR.append((FormLayout.Spinbox(_("Bad move") + d(90), 20, 1000, 60), perf.bad_lostp))
    liPR.append((FormLayout.Spinbox(_("Very bad move") + d(200), 50, 1000, 60), perf.very_bad_lostp))
    liPR.append(separador)
    liPR.append((FormLayout.Spinbox(_("Degree of effect of bad moves on the game elo") + d(2), 0, 5, 40), perf.bad_factor))
    liPR.append(separador)
    liPR.append((None, _("Good moves: minimum depth required by the engine to discover the move")))
    liPR.append((FormLayout.Spinbox(_("Good move") + d(3), 2, 20, 40), perf.good_depth))
    liPR.append((FormLayout.Spinbox(_("Very good move") + d(6), 3, 20, 40), perf.very_good_depth))

    # Modo no competitivo
    liNC = [separador]
    liNC.append((FormLayout.Spinbox(_("Lucas-Elo"), 0, 3200, 70), configuracion.eloNC))
    liNC.append((FormLayout.Spinbox(_("Tourney-Elo"), 0, 3200, 70), configuracion.micheloNC))
    liNC.append((FormLayout.Spinbox(_("Fics-Elo"), 0, 3200, 70), configuracion.ficsNC))
    liNC.append((FormLayout.Spinbox(_("Fide-Elo"), 0, 3200, 70), configuracion.fideNC))
    liNC.append((FormLayout.Spinbox(_("Lichess-Elo"), 0, 3200, 70), configuracion.lichessNC))

    # Salvado automatico
    liSA = [separador]

    config = FormLayout.Fichero(_("Autosave to a PGN file"), "pgn", True)
    liSA.append((config, configuracion.salvarFichero))
    liSA.append((_("Won games") + ":", configuracion.salvarGanados))
    liSA.append((_("Lost/Drawn games") + ":", configuracion.salvarPerdidos))
    liSA.append((_("Unfinished games") + ":", configuracion.salvarAbandonados))
    liSA.append(separador)
    liSA.append((_("Save as variant tutor's suggestion") + ":", configuracion.guardarVariantesTutor))
    liSA.append(separador)
    config = FormLayout.Fichero(_("Autosave to a CSV file moves played"), "csv", True)
    liSA.append((config, configuracion.salvarCSV))

    # Boards
    liB = [separador]

    liB.append((_("Visual effects") + ":", configuracion.efectosVisuales))

    drap = {1: 100, 2: 150, 3: 200, 4: 250, 5: 300, 6: 350, 7: 400, 8: 450, 9: 500}
    drapV = {}
    for x in drap:
        drapV[drap[x]] = x
    liB.append((FormLayout.Dial("%s (%s=1)" % (_("Speed"), _("Default")), 1, len(drap), siporc=False),
                drapV.get(configuracion.rapidezMovPiezas, 100)))
    liB.append(separador)

    liMouseSH = [configuracion.siAtajosRaton,
                 (False, _("Type fixed: you must always indicate origin and destination")),
                 (True, _("Type predictive: program tries to guess your intention"))]
    liB.append((_("Mouse shortcuts") + ":", liMouseSH))
    liB.append((_("Show candidates") + ":", configuracion.showCandidates))
    liB.append((_("Show arrows of variants") + ":", configuracion.showVariantes))
    liB.append((_("Always promote to queen\nALT key allows to change") + ":", configuracion.autocoronacion))
    liB.append((_("Show cursor when engine is thinking") + ":", configuracion.cursorThinking))
    liB.append(separador)
    liB.append((_("Enable captured material window by default") + ":", configuracion.siActivarCapturas))
    liMat = [configuracion.tipoMaterial, ("D", _("Difference material")), ("C", _("Captured material at beginning")), ("M", _("Material advantage"))]
    liB.append((_("Show material") + ":", liMat))
    liB.append(separador)
    liB.append((_("Enable information panel by default") + ":", configuracion.siActivarInformacion))
    liB.append(separador)
    liB.append((_X(_("Enable %1"), _("DGT board")) + ":", configuracion.siDGT))
    liB.append(separador)
    # liT.append((FormLayout.Dial(_("Opacity of tool icon"), 1, 9, siporc=False), configuracion.opacityToolBoard))
    liB.append((_("Show configuration icon") + ":", configuracion.opacityToolBoard > 6))
    liPos = [configuracion.positionToolBoard, ("B", _("Bottom")), ("T", _("Top"))]
    liB.append((_("Configuration icon position") + ":", liPos))
    liB.append(separador)
    liB.append((_("Show icon when position has graphic information") + ":", configuracion.directorIcon))
    liB.append(separador)
    liB.append((_("Live graphics with the right mouse button") + ":", configuracion.directGraphics))

    liB.append(separador)

    lista = []
    lista.append((liGen, _("General"), ""))
    lista.append((liSon, _("Sounds"), ""))
    lista.append((liTT, _("Tutor"), ""))
    lista.append((liB, _("Boards"), ""))
    lista.append((liEng, _("Engines"), ""))
    lista.append((liAsp, _("Appearance"), ""))
    lista.append((liPR, _("Performance"), ""))
    lista.append((liSA, _("Autosave"), ""))
    lista.append((liNC, _("Non competitive mode"), ""))

    # Editamos
    resultado = FormLayout.fedit(lista, title=_("Configuration"), parent=parent, anchoMinimo=640, icon=Iconos.Opciones())

    if resultado:
        accion, resp = resultado

        liGen, liSon, liTT, liB, liEng, liAsp, liPR, liSA, liNC = resp

        (configuracion.jugador, configuracion.estilo, configuracion.traductor, configuracion.checkforupdate) = liGen

        porDefecto = liAsp[0]
        if porDefecto:
            liAsp = "", 11, False, 11, False,  QtCore.Qt.ToolButtonTextUnderIcon, 283, 22, 10, False, True, 10
        else:
            del liAsp[0]
        (configuracion.familia, configuracion.puntosMenu, configuracion.boldMenu,
            configuracion.puntosTB, configuracion.boldTB, configuracion.iconsTB,
            configuracion.anchoPGN, configuracion.altoFilaPGN, configuracion.puntosPGN,
            configuracion.siNomPiezasEN, configuracion.figurinesPGN,
            configuracion.tamFontRotulos) = liAsp
        if configuracion.familia == "System":
            configuracion.familia = ""

        (configuracion.siSuenaBeep, configuracion.siSuenaResultados,
         configuracion.siSuenaJugada, configuracion.siSuenaNuestro) = liSon

        (configuracion.tutor.clave, tiempoTutor, configuracion.depthTutor, configuracion.tutorMultiPV,
            configuracion.tutorActivoPorDefecto, configuracion.vistaTutor,
            configuracion.tutorDifPts, configuracion.tutorDifPorc ) = liTT
        configuracion.tiempoTutor = int(tiempoTutor * 1000)

        (configuracion.eloNC, configuracion.micheloNC, configuracion.ficsNC,
         configuracion.fideNC, configuracion.lichessNC) = liNC

        (configuracion.centipawns, configuracion.bmi2, workinbackground, configuracion.siLogEngines) = liEng
        configuracion.notbackground = not workinbackground

        (configuracion.efectosVisuales, rapidezMovPiezas,
            configuracion.siAtajosRaton, configuracion.showCandidates, configuracion.showVariantes,
            configuracion.autocoronacion,
            configuracion.cursorThinking, configuracion.siActivarCapturas, configuracion.tipoMaterial,
            configuracion.siActivarInformacion, siDGT, toolIcon, configuracion.positionToolBoard,
            configuracion.directorIcon, configuracion.directGraphics) = liB
        configuracion.opacityToolBoard = 10 if toolIcon else 1
        configuracion.rapidezMovPiezas = drap[rapidezMovPiezas]
        if configuracion.siDGT != siDGT:
            if siDGT:
                DGT.ponON()
            configuracion.siDGT = siDGT

        perf.bad_lostp, perf.very_bad_lostp, perf.bad_factor, perf.good_depth, perf.very_good_depth = liPR
        perf.very_bad_factor = perf.bad_factor * 4

        (configuracion.salvarFichero, configuracion.salvarGanados, configuracion.salvarPerdidos,
            configuracion.salvarAbandonados, configuracion.guardarVariantesTutor,
            configuracion.salvarCSV) = liSA
        configuracion.salvarCSV = Util.dirRelativo(configuracion.salvarCSV)

        return True
    else:
        return False


def opcionesPrimeraVez(parent, configuracion):
    separador = (None, None)

    # Datos generales
    liGen = [separador]

    # # Nombre del jugador
    liGen.append((_("Player's name") + ":", configuracion.jugador))

    # Editamos
    resultado = FormLayout.fedit(liGen, title=_("Configuration"), parent=parent, anchoMinimo=560,
                                 icon=Iconos.Opciones())

    if resultado:
        accion, resp = resultado

        liGen = resp

        configuracion.jugador = liGen[0]

        return True
    else:
        return False

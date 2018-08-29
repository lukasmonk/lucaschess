from Code import Books
from Code.QT import FormLayout
from Code.QT import Iconos
from Code.QT import QTUtil
from Code.QT import QTUtil2
from Code import Util
from Code import EngineThread

SEPARADOR = FormLayout.separador


def leeDicParametros(configuracion):
    fichero = configuracion.ficheroAnalisis
    dic = Util.recuperaVar(fichero)
    if not dic:
        dic = {}
    alm = Util.Almacen()
    alm.motor = dic.get("MOTOR", configuracion.tutor.clave)
    alm.tiempo = dic.get("TIEMPO", configuracion.tiempoTutor)
    alm.depth = dic.get("DEPTH", configuracion.depthTutor)
    alm.timedepth = dic.get("TIMEDEPTH", False)
    alm.kblunders = dic.get("KBLUNDERS", 50)
    alm.ptbrilliancies = dic.get("PTBRILLIANCIES", 100)
    alm.dpbrilliancies = dic.get("DPBRILLIANCIES", 7)
    alm.desdeelfinal = dic.get("DESDEELFINAL", False)
    alm.multiPV = dic.get("MULTIPV", "PD")
    alm.priority = dic.get("PRIORITY", EngineThread.priorities.normal)

    alm.libro = dic.get("LIBRO", None)

    alm.masvariantes = dic.get("MASVARIANTES", True)
    alm.limitemasvariantes = dic.get("LIMITEMASVARIANTES", 0)
    alm.mejorvariante = dic.get("MEJORVARIANTE", False)
    alm.infovariante = dic.get("INFOVARIANTE", True)
    alm.unmovevariante = dic.get("UNMOVEVARIANTE", False)
    alm.siBorrarPrevio = dic.get("SIBORRARPREVIO", True)
    alm.siPDT = dic.get("SIPDT", False)

    alm.showGraphs = dic.get("SHOWGRAPHS", True)

    alm.stability = dic.get("STABILITY", False)
    alm.st_centipawns = dic.get("ST_CENTIPAWNS", 5)
    alm.st_depths = dic.get("ST_DEPTHS", 3)
    alm.st_timelimit = dic.get("ST_TIMELIMIT", 5)

    alm.blancas = dic.get("BLANCAS", True)
    alm.negras = dic.get("NEGRAS", True)

    return alm


def formBlundersBrilliancies(alm, configuracion):
    liBlunders = [SEPARADOR]

    liBlunders.append((FormLayout.Editbox(_("Is considered wrong move when the loss of points is greater than"),
                                          tipo=int, ancho=50), alm.kblunders))
    liBlunders.append(SEPARADOR)

    def fileNext(base, ext):
        return Util.fileNext(configuracion.dirPersonalTraining, base, ext)

    path_pgn = fileNext("Blunders", "pgn")

    liBlunders.append((None, _("Generate a training file with these moves")))

    config = FormLayout.Editbox(_("Tactics name"), rx="[^\\\:/|?*^%><()]*")
    liBlunders.append((config, ""))

    config = FormLayout.Fichero(_("PGN Format"), "%s (*.pgn)" % _("PGN Format"), True, anchoMinimo=280,
                                ficheroDefecto=path_pgn)
    liBlunders.append((config, ""))

    liBlunders.append((_("Also add complete game to PGN") + ":", False))

    liBlunders.append(SEPARADOR)

    eti = '"%s"' % _("Find best move")
    liBlunders.append((_X(_("Add to the training %1 with the name"), eti) + ":", ""))

    liBrilliancies = [SEPARADOR]

    liBrilliancies.append((FormLayout.Spinbox(_("Minimum depth"), 3, 30, 50), alm.dpbrilliancies))

    liBrilliancies.append((FormLayout.Spinbox(_("Minimum gain points"), 30, 30000, 50), alm.ptbrilliancies))
    liBrilliancies.append(SEPARADOR)

    path_fns = fileNext("Brilliancies", "fns")
    path_pgn = fileNext("Brilliancies", "pgn")

    liBrilliancies.append((None, _("Generate a training file with these moves")))

    config = FormLayout.Fichero(_("List of FENs"), "%s (*.fns)" % _("List of FENs"), True, anchoMinimo=280,
                                ficheroDefecto=path_fns)
    liBrilliancies.append((config, ""))

    config = FormLayout.Fichero(_("PGN Format"), "%s (*.pgn)" % _("PGN Format"), True, anchoMinimo=280,
                                ficheroDefecto=path_pgn)
    liBrilliancies.append((config, ""))

    liBrilliancies.append((_("Also add complete game to PGN"), False))

    liBrilliancies.append(SEPARADOR)

    eti = '"%s"' % _("Find best move")
    liBrilliancies.append((_X(_("Add to the training %1 with the name"), eti) + ":", ""))

    return liBlunders, liBrilliancies


def paramAnalisis(parent, configuracion, siModoAmpliado, siTodosMotores=False):
    alm = leeDicParametros(configuracion)

    # Datos
    liGen = [SEPARADOR]

    # # Tutor
    if siTodosMotores:
        li = configuracion.ayudaCambioCompleto(alm.motor)
    else:
        li = configuracion.ayudaCambioTutor()
        li[0] = alm.motor
    liGen.append((_("Engine") + ":", li))

    # # Time
    liGen.append(SEPARADOR)
    config = FormLayout.Editbox(_("Duration of engine analysis (secs)"), 40, tipo=float)
    liGen.append((config, alm.tiempo / 1000.0))

    # Depth
    liDepths = [("--", 0)]
    for x in range(1, 51):
        liDepths.append((str(x), x))
    config = FormLayout.Combobox(_("Depth"), liDepths)
    liGen.append((config, alm.depth))

    # Time+Depth
    liGen.append(("%s+%s:" % (_("Time"), _("Depth")), alm.timedepth))

    # MultiPV
    liGen.append(SEPARADOR)
    li = [(_("Default"), "PD"),
          (_("Maximum"), "MX")]
    for x in (1, 3, 5, 10, 15, 20, 30, 40, 50, 75, 100, 150, 200):
        li.append((str(x), str(x)))
    config = FormLayout.Combobox(_("Number of moves evaluated by engine(MultiPV)"), li)
    liGen.append((config, alm.multiPV))

    # Priority
    liGen.append(SEPARADOR)
    config = FormLayout.Combobox(_("Process priority"), EngineThread.priorities.combo())
    liGen.append((config, alm.priority))

    # Completo
    if siModoAmpliado:
        liGen.append(SEPARADOR)

        liJ = [(_("White"), "BLANCAS"), (_("Black"), "NEGRAS"), (_("White & Black"), "AMBOS")]
        config = FormLayout.Combobox(_("Analyze only color"), liJ)
        if alm.blancas and alm.negras:
            color = "AMBOS"
        elif alm.negras:
            color = "NEGRAS"
        elif alm.blancas:
            color = "BLANCAS"
        else:
            color = "AMBOS"
        liGen.append((config, color))

        config = FormLayout.Editbox("<div align=\"right\">" + _("Moves") + "<br>" +
                                    _("By example:") + " -5,8-12,14,19-",
                                    rx="[0-9,\-,\,]*")
        liGen.append((config, ""))

        fvar = configuracion.ficheroBooks
        listaLibros = Books.ListaLibros()
        listaLibros.recuperaVar(fvar)
        # Comprobamos que todos esten accesibles
        listaLibros.comprueba()
        li = [("--", None)]
        defecto = listaLibros.lista[0] if alm.libro else None
        for libro in listaLibros.lista:
            if alm.libro == libro.nombre:
                defecto = libro
            li.append((libro.nombre, libro))
        config = FormLayout.Combobox(_("Do not scan the opening moves based on book"), li)
        liGen.append((config, defecto))
        liGen.append(SEPARADOR)

        liGen.append((_("Redo any existing prior analyses (if they exist)") + ":", alm.siBorrarPrevio))

        liGen.append((_("Start from the end of the game") + ":", alm.desdeelfinal))

        liGen.append(SEPARADOR)

        liGen.append((_("Show graphics") + ":", alm.showGraphs))

        liVar = [SEPARADOR]
        liVar.append((_("Add analysis to variants") + ":", alm.masvariantes))
        liVar.append(SEPARADOR)

        liVar.append((FormLayout.Spinbox(_("Minimum points lost"), 0, 1000, 60), alm.limitemasvariantes))
        liVar.append(SEPARADOR)

        liVar.append((_("Only add better variant") + ":", alm.mejorvariante))
        liVar.append(SEPARADOR)

        liVar.append((_("Include info about engine") + ":", alm.infovariante))
        liVar.append(SEPARADOR)

        liVar.append(("%s %s/%s/%s:" % (_("Format"), _("Points"), _("Depth"), _("Time")), alm.siPDT))
        liVar.append(SEPARADOR)

        liVar.append((_("Only one move of each variant") + ":", alm.unmovevariante))

        liBlunders, liBrilliancies = formBlundersBrilliancies(alm, configuracion)

        liST = [SEPARADOR]
        liST.append((_("Activate") + ":", alm.stability))
        liST.append(SEPARADOR)
        liST.append((FormLayout.Spinbox(_("Last depths to control same best move"), 2, 10, 40), alm.st_depths))
        liST.append(SEPARADOR)
        liST.append((FormLayout.Spinbox(_("Maximum difference among last evaluations"), 0, 99999, 60), alm.st_centipawns))
        liST.append(SEPARADOR)
        liST.append((FormLayout.Spinbox(_("Additional time limit"), 0, 99999, 60), alm.st_timelimit))

        lista = []
        lista.append((liGen, _("General options"), ""))
        lista.append((liVar, _("Variants"), ""))
        lista.append((liBlunders, _("Wrong moves"), ""))
        lista.append((liBrilliancies, _("Brilliancies"), ""))
        lista.append((liST, _("Stability control"), ""))

    else:
        lista = liGen

    reg = Util.Almacen()
    reg.form = None

    def dispatchR(valor):
        if reg.form is None:
            if isinstance(valor, FormLayout.FormTabWidget):
                reg.form = valor
                reg.wtime = valor.getWidget(0, 1)
                reg.wdepth = valor.getWidget(0, 2)
                reg.wdt = valor.getWidget(0, 3)
            elif isinstance(valor, FormLayout.FormWidget):
                reg.form = valor
                reg.wtime = valor.getWidget(1)
                reg.wdepth = valor.getWidget(2)
                reg.wdt = valor.getWidget(3)
        else:
            sender = reg.form.sender()
            if not reg.wdt.isChecked():
                if sender == reg.wtime:
                    if reg.wtime.textoFloat() > 0:
                        reg.wdepth.setCurrentIndex(0)
                elif sender == reg.wdepth:
                    if reg.wdepth.currentIndex() > 0:
                        reg.wtime.ponFloat(0.0)
                elif sender == reg.wdt:
                    if reg.wtime.textoFloat() > 0:
                        reg.wdepth.setCurrentIndex(0)
                    elif reg.wdepth.currentIndex() > 0:
                        reg.wtime.ponFloat(0.0)

                QTUtil.refreshGUI()

    resultado = FormLayout.fedit(lista, title=_("Analysis Configuration"), parent=parent, anchoMinimo=460,
                                 icon=Iconos.Opciones(), dispatch=dispatchR)

    if resultado:
        accion, liResp = resultado

        if siModoAmpliado:
            liGen, liVar, liBlunders, liBrilliancies, liST = liResp
        else:
            liGen = liResp

        alm.motor = liGen[0]
        alm.tiempo = int(liGen[1] * 1000)
        alm.depth = liGen[2]
        alm.timedepth = liGen[3]
        alm.multiPV = liGen[4]
        alm.priority = liGen[5]

        if siModoAmpliado:
            color = liGen[6]
            alm.blancas = color != "NEGRAS"
            alm.negras = color != "BLANCAS"
            alm.jugadas = liGen[7]
            alm.libroAperturas = liGen[8]
            alm.libro = alm.libroAperturas.nombre if alm.libroAperturas else None
            alm.siBorrarPrevio = liGen[9]
            alm.desdeelfinal = liGen[10]
            alm.showGraphs = liGen[11]

            (alm.masvariantes, alm.limitemasvariantes, alm.mejorvariante, alm.infovariante,
                alm.siPDT, alm.unmovevariante) = liVar

            (alm.kblunders, alm.tacticblunders, alm.pgnblunders, alm.oriblunders, alm.bmtblunders) = liBlunders

            (alm.dpbrilliancies, alm.ptbrilliancies, alm.fnsbrilliancies, alm.pgnbrilliancies,
                alm.oribrilliancies, alm.bmtbrilliancies) = liBrilliancies

            (alm.stability, alm.st_depths, alm.st_centipawns, alm.st_timelimit) = liST

        dic = {}
        for x in dir(alm):
            if not x.startswith("__"):
                dic[x.upper()] = getattr(alm, x)
        Util.guardaVar(configuracion.ficheroAnalisis, dic)

        return alm
    else:
        return None


def paramAnalisisMasivo(parent, configuracion, siVariosSeleccionados, siDatabase=False):
    alm = leeDicParametros(configuracion)

    # Datos
    liGen = [SEPARADOR]

    # # Tutor
    li = configuracion.ayudaCambioTutor()
    li[0] = alm.motor
    liGen.append((_("Engine") + ":", li))

    liGen.append(SEPARADOR)

    # # Time
    config = FormLayout.Editbox(_("Duration of engine analysis (secs)"), 40, tipo=float)
    liGen.append((config, alm.tiempo / 1000.0))

    # Depth
    liDepths = [("--", 0)]
    for x in range(1, 31):
        liDepths.append((str(x), x))
    config = FormLayout.Combobox(_("Depth"), liDepths)
    liGen.append((config, alm.depth))

    # Time+Depth
    liGen.append(("%s+%s:" % (_("Time"), _("Depth")), alm.timedepth))

    # MultiPV
    liGen.append(SEPARADOR)
    li = [(_("Default"), "PD"),
          (_("Maximum"), "MX")]
    for x in (1, 3, 5, 10, 15, 20, 30, 40, 50, 75, 100, 150, 200):
        li.append((str(x), str(x)))
    config = FormLayout.Combobox(_("Number of moves evaluated by engine(MultiPV)"), li)
    liGen.append((config, alm.multiPV))
    liGen.append(SEPARADOR)

    liJ = [(_("White"), "BLANCAS"), (_("Black"), "NEGRAS"), (_("White & Black"), "AMBOS")]
    config = FormLayout.Combobox(_("Analyze only color"), liJ)
    if alm.blancas and alm.negras:
        color = "AMBOS"
    elif alm.negras:
        color = "NEGRAS"
    elif alm.blancas:
        color = "BLANCAS"
    else:
        color = "AMBOS"
    liGen.append((config, color))

    liGen.append(("<div align=\"right\">" + _("Only player moves") + ":<br>%s</div>" % _(
            "(You can add multiple aliases separated by ; and wildcard * )"), ""))

    fvar = configuracion.ficheroBooks
    listaLibros = Books.ListaLibros()
    listaLibros.recuperaVar(fvar)
    # Comprobamos que todos esten accesibles
    listaLibros.comprueba()
    defecto = listaLibros.lista[0]
    li = [("--", None)]
    for libro in listaLibros.lista:
        if libro.nombre == alm.libro:
            defecto = libro
        li.append((libro.nombre, libro))
    config = FormLayout.Combobox(_("Do not scan the opening moves based on book"), li)
    liGen.append((config, defecto))

    liGen.append((_("Start from the end of the game") + ":", alm.desdeelfinal))

    liGen.append(SEPARADOR)
    liGen.append((_("Redo any existing prior analyses (if they exist)") + ":", alm.siBorrarPrevio))

    liGen.append(SEPARADOR)
    liGen.append((_("Only selected games") + ":", siVariosSeleccionados))

    liBlunders, liBrilliancies = formBlundersBrilliancies(alm, configuracion)

    lista = []
    lista.append((liGen, _("General options"), ""))
    lista.append((liBlunders, _("Wrong moves"), ""))
    lista.append((liBrilliancies, _("Brilliancies"), ""))

    reg = Util.Almacen()
    reg.form = None

    def dispatchR(valor):
        if reg.form is None:
            if isinstance(valor, FormLayout.FormTabWidget):
                reg.form = valor
                reg.wtime = valor.getWidget(0, 1)
                reg.wdepth = valor.getWidget(0, 2)
                reg.wdt = valor.getWidget(0, 3)
            elif isinstance(valor, FormLayout.FormWidget):
                reg.form = valor
                reg.wtime = valor.getWidget(1)
                reg.wdepth = valor.getWidget(2)
                reg.wdt = valor.getWidget(3)
        else:
            sender = reg.form.sender()
            if not reg.wdt.isChecked():
                if sender == reg.wtime:
                    if reg.wtime.textoFloat() > 0:
                        reg.wdepth.setCurrentIndex(0)
                elif sender == reg.wdepth:
                    if reg.wdepth.currentIndex() > 0:
                        reg.wtime.ponFloat(0.0)
                elif sender == reg.wdt:
                    if reg.wtime.textoFloat() > 0:
                        reg.wdepth.setCurrentIndex(0)
                    elif reg.wdepth.currentIndex() > 0:
                        reg.wtime.ponFloat(0.0)

                QTUtil.refreshGUI()

    resultado = FormLayout.fedit(lista, title=_("Mass analysis"), parent=parent, anchoMinimo=460,
                                 icon=Iconos.Opciones(), dispatch=dispatchR)

    if resultado:
        accion, liResp = resultado

        liGen, liBlunders, liBrilliancies = liResp

        alm.motor, tiempo, alm.depth, alm.timedepth, alm.multiPV, color, cjug, alm.libroAperturas, \
            alm.desdeelfinal, alm.siBorrarPrevio, alm.siVariosSeleccionados = liGen

        alm.tiempo = int(tiempo * 1000)
        alm.blancas = color != "NEGRAS"
        alm.negras = color != "BLANCAS"
        cjug = cjug.strip()
        alm.liJugadores = cjug.upper().split(";") if cjug else None
        alm.libro = alm.libroAperturas.nombre if alm.libroAperturas is not None else None

        alm.kblunders, alm.tacticblunders, alm.pgnblunders, alm.oriblunders, alm.bmtblunders = liBlunders

        alm.dpbrilliancies, alm.ptbrilliancies, alm.fnsbrilliancies, alm.pgnbrilliancies, \
            alm.oribrilliancies, alm.bmtbrilliancies = liBrilliancies

        dic = {}
        for x in dir(alm):
            if not x.startswith("__"):
                dic[x.upper()] = getattr(alm, x)
        Util.guardaVar(configuracion.ficheroAnalisis, dic)

        if not (alm.tacticblunders or alm.pgnblunders or alm.bmtblunders or alm.fnsbrilliancies or
                    alm.pgnbrilliancies or alm.bmtbrilliancies or siDatabase):
            QTUtil2.mensError(parent, _("No file was specified where to save results"))
            return

        return alm
    else:
        return None

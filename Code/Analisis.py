import codecs
import copy
import os

from Code import AnalisisIndexes
from Code import BMT
from Code import Jugada
from Code import Partida
from Code.QT import PantallaAnalisis
from Code.QT import PantallaPGN
from Code.QT import PantallaAnalisisParam
from Code.QT import QTUtil
from Code.QT import QTUtil2
from Code import Util


class AnalizaPartida:
    def __init__(self, procesador, alm, is_massiv, li_moves=None):
        self.procesador = procesador
        self.alm = alm
        self.siMasivo = is_massiv

        self.configuracion = procesador.configuracion
        conf_engine = copy.deepcopy(self.configuracion.buscaMotor(alm.motor))
        if alm.multiPV:
            conf_engine.actMultiPV(alm.multiPV)
        self.xgestor = procesador.creaGestorMotor(conf_engine, alm.tiempo, alm.depth, True, priority=alm.priority)
        self.tiempo = alm.tiempo
        self.depth = alm.depth
        self.siVariantes = (not is_massiv) and alm.masvariantes

        self.stability = alm.stability
        self.st_centipawns = alm.st_centipawns
        self.st_depths = alm.st_depths
        self.st_timelimit = alm.st_timelimit

        # Asignacion de variables para blunders:
        # kblunders: puntos de perdida para considerar un blunder
        # tacticblunders: folder donde guardar tactic
        # pgnblunders: fichero pgn donde guardar la partidas
        # oriblunders: si se guarda la partida original
        # bmtblunders: nombre del entrenamiento BMT a crear
        self.kblunders = alm.kblunders
        self.tacticblunders = os.path.join(self.configuracion.dirPersonalTraining, "Tactics",
                                           alm.tacticblunders) if alm.tacticblunders else None
        self.pgnblunders = alm.pgnblunders
        self.oriblunders = alm.oriblunders
        self.bmtblunders = alm.bmtblunders
        self.bmt_listaBlunders = None

        self.siTacticBlunders = False

        self.rutDispatchBP = None

        self.siBorrarPrevio = True

        # dpbrilliancies: depth de control para saber si es brilliancie
        # ptbrilliancies: puntos de ganancia
        # fnsbrilliancies: fichero fns donde guardar posiciones fen
        # pgnbrilliancies: fichero pgn donde guardar la partidas
        # oribrilliancies: si se guarda la partida original
        # bmtbrilliancies: nombre del entrenamiento BMT a crear
        self.dpbrilliancies = alm.dpbrilliancies
        self.ptbrilliancies = alm.ptbrilliancies
        self.fnsbrilliancies = alm.fnsbrilliancies
        self.pgnbrilliancies = alm.pgnbrilliancies
        self.oribrilliancies = alm.oribrilliancies
        self.bmtbrilliancies = alm.bmtbrilliancies
        self.bmt_listaBrilliancies = None

        # Asignacion de variables comunes
        # blancas: si se analizan las blancas
        # negras: si se analizan las negras
        # liJugadores: si solo se miran los movimiento de determinados jugadores
        # libroAperturas: si se usa un libro de aperturas para no analizar los iniciales
        # listaElegidas: si se indica un alista de movimientos concreta que analizar
        # desdeelfinal: se determina si se empieza de atras adelante o al reves
        # siBorrarPrevio: si la partida tiene un analisis previo, se determina si se hace o no
        self.blancas = alm.blancas
        self.negras = alm.negras
        self.liJugadores = alm.liJugadores if is_massiv else None
        self.libroAperturas = alm.libroAperturas
        if self.libroAperturas:
            self.libroAperturas.polyglot()
        self.listaElegidas = li_moves
        self.desdeelfinal = alm.desdeelfinal
        self.siBorrarPrevio = alm.siBorrarPrevio

    def terminarBMT(self, bmt_lista, nombre):
        """
        Si se estan creando registros para el entrenamiento BMT (Best move Training), al final hay que grabarlos
        @param bmt_lista: lista a grabar
        @param nombre: nombre del entrenamiento
        """
        if bmt_lista and len(bmt_lista) > 0:
            bmt = BMT.BMT(self.configuracion.ficheroBMT)
            dbf = bmt.leerDBF(False)

            reg = dbf.baseRegistro()
            reg.ESTADO = "0"
            reg.NOMBRE = nombre
            reg.EXTRA = ""
            reg.TOTAL = len(bmt_lista)
            reg.HECHOS = 0
            reg.PUNTOS = 0
            reg.MAXPUNTOS = bmt_lista.maxPuntos()
            reg.FINICIAL = Util.dtos(Util.hoy())
            reg.FFINAL = ""
            reg.SEGUNDOS = 0
            reg.BMT_LISTA = Util.var2blob(bmt_lista)
            reg.HISTORIAL = Util.var2blob([])
            reg.REPE = 0
            reg.ORDEN = 0

            dbf.insertarReg(reg, siReleer=False)

            bmt.cerrar()

    def terminar(self, siBMT):
        """
        Proceso final, para cerrar el motor que hemos usado
        @param siBMT: si hay que grabar el registro de BMT
        """
        self.xgestor.terminar()
        if siBMT:
            self.terminarBMT(self.bmt_listaBlunders, self.bmtblunders)
            self.terminarBMT(self.bmt_listaBrilliancies, self.bmtbrilliancies)

    def dispatchBP(self, rutDispatchBP):
        """
        Se determina la rutina que se llama cada analisis
        """
        self.rutDispatchBP = rutDispatchBP

    def grabaFNS(self, fichero, fen):
        """
        Graba cada fen encontrado en el fichero "fichero"
        """
        if not fichero:
            return

        f = open(fichero, "ab")
        f.write("%s\r\n" % fen)
        f.close()
        self.procesador.entrenamientos.menu = None

    def grabaTactic(self, dicCab, partida, njg, mrm, posAct):
        if not self.tacticblunders:
            return

        # Esta creado el folder
        before = "AvoidBlunders.fns"
        after = "ExploitBlunders.fns"
        if not os.path.isdir(self.tacticblunders):
            dtactics = os.path.join(self.configuracion.dirPersonalTraining, "Tactics")
            if not os.path.isdir(dtactics):
                os.mkdir(dtactics)
            os.mkdir(self.tacticblunders)
            f = open(os.path.join(self.tacticblunders, "Config.ini"), "wb")
            f.write("""[COMMON]
PUZZLES=20
REPEAT=0
SHOWTEXT=1
[TACTIC1]
MENU=%s
FILESW=%s:100
[TACTIC2]
MENU=%s
FILESW=%s:100
""" % (_("Avoid the blunder"), before, _("Take advantage of blunder"), after))
            f.close()

        cab = ""
        for k, v in dicCab.iteritems():
            ku = k.upper()
            if ku not in ("RESULT", "FEN"):
                cab += '[%s "%s"]' % (k, v)
        jg = partida.jugada(njg)

        fen = jg.posicionBase.fen()
        p = Partida.Partida(fen=fen)
        rm = mrm.liMultiPV[0]
        p.leerPV(rm.pv)
        f = open(os.path.join(self.tacticblunders, before), "ab")
        f.write("%s||%s|%s%s\n" % (fen, p.pgnBaseRAW(), cab, partida.pgnBaseRAWcopy(None, njg - 1)))
        f.close()

        fen = jg.posicion.fen()
        p = Partida.Partida(fen=fen)
        rm = mrm.liMultiPV[posAct]
        li = rm.pv.split(" ")
        p.leerPV(" ".join(li[1:]))
        f = open(os.path.join(self.tacticblunders, after), "ab")
        f.write("%s||%s|%s%s\n" % (fen, p.pgnBaseRAW(), cab, partida.pgnBaseRAWcopy(None, njg)))
        f.close()

        self.siTacticBlunders = True
        self.procesador.entrenamientos.menu = None

    def grabaPGN(self, fichero, nombre, dicCab, fen, jg, rm, mj):
        """
        Graba una partida en un pgn

        @param fichero: pgn donde grabar
        @param nombre: nombre del motor que hace el analisis
        @param dicCab: etiquetas de cabecera del PGN
        @param fen: fen de la posicion
        @param jg: jugada analizada
        @param rm: respuesta motor
        @param mj: respuesta motor con la mejor jugada, usado en caso de blunders, para incluirla
        """
        if not fichero:
            return False

        p = Partida.Partida()

        if mj:  # blunder
            p.reset(jg.posicionBase)
            p.leerPV(rm.pv)
            jg0 = p.jugada(0)
            jg0.comentario = rm.texto()
            variante = p.pgnBaseRAW()

        p.reset(jg.posicionBase)
        if mj:  # blunder
            rm = mj
        p.leerPV(rm.pv)
        if p.siTerminada():
            result = p.resultado()
            mas = ""  # ya lo anade en la ultima jugada
        else:
            mas = " *"
            result = "*"

        jg0 = p.jugada(0)
        t = "%0.2f" % (float(self.tiempo) / 1000.0,)
        t = t.rstrip("0")
        if t[-1] == ".":
            t = t[:-1]
        etiT = "%s %s" % (t, _("Second(s)"))

        jg0.comentario = "%s %s: %s\n" % (nombre, etiT, rm.texto())
        if mj:
            jg0.variantes = variante

        cab = ""
        for k, v in dicCab.iteritems():
            ku = k.upper()
            if ku not in ("RESULT", "FEN"):
                cab += '[%s "%s"]\n' % (k, v)
        # Nos protegemos de que se hayan escrito en el pgn original de otra forma
        cab += '[FEN "%s"]\n' % fen
        cab += '[Result "%s"]\n' % result

        f = codecs.open(fichero, "a", 'utf-8', 'ignore')
        texto = (cab + "\n" + p.pgnBase() + mas + "\n\n").replace("\n", "\r\n")
        f.write(texto)
        f.close()

        return True

    def grabaBMT(self, siBlunder, fen, mrm, posAct, clpartida, txtPartida):
        """
        Se graba una posicion en un entrenamiento BMT
        @param siBlunder: si es blunder o brilliancie
        @param fen: posicion
        @param mrm: multirespuesta del motor
        @param posAct: posicion de la posicion elegida en mrm
        @param clpartida: clave de la partida
        @param txtPartida: la partida completa en texto
        """

        previa = 999999999
        nprevia = -1
        tniv = 0
        partidaBMT = Partida.Partida()
        cp = partidaBMT.iniPosicion
        cp.leeFen(fen)

        if len(mrm.liMultiPV) > 16:
            mrmBMT = copy.deepcopy(mrm)
            if posAct > 15:
                mrmBMT.liMultiPV[15] = mrmBMT.liMultiPV[posAct]
                posAct = 15
            mrmBMT.liMultiPV = mrmBMT.liMultiPV[:16]
        else:
            mrmBMT = mrm

        for n, rm in enumerate(mrmBMT.liMultiPV):
            pts = rm.puntosABS()
            if pts != previa:
                previa = pts
                nprevia += 1
            tniv += nprevia
            rm.nivelBMT = nprevia
            rm.siElegida = False
            rm.siPrimero = n == posAct
            partidaBMT.reset(cp)
            partidaBMT.leerPV(rm.pv)
            rm.txtPartida = partidaBMT.guardaEnTexto()

        bmt_uno = BMT.BMT_Uno(fen, mrmBMT, tniv, clpartida)

        bmt_lista = self.bmt_listaBlunders if siBlunder else self.bmt_listaBrilliancies
        bmt_lista.nuevo(bmt_uno)
        bmt_lista.compruebaPartida(clpartida, txtPartida)

    def xprocesa(self, dicPGN, partida, tmpBP, textoOriginal):
        """
        Realiza el analisis
        @param dicPGN: etiquetas de cabecera de la partida
        @param partida: partida a analizar
        @param textoOriginal: texto de la partida, por si se graba la partida original
        """

        self.siBMTblunders = False
        self.siBMTbrilliancies = False

        siBP2 = hasattr(tmpBP, "bp2")   # Para diferenciar el analisis de una partida que usa una progressbar unica del
                                        # analisis de muchas, que usa doble

        def guiDispatch(rm):
            return not tmpBP.siCancelado()

        self.xgestor.ponGuiDispatch(guiDispatch)    # Dispatch del motor, si esta puesto a 4 minutos por ejemplo que
                                                    # compruebe si se ha indicado que se cancele.

        siBlunders = self.kblunders > 0
        siBrilliancies = self.fnsbrilliancies or self.pgnbrilliancies or self.bmtbrilliancies

        if self.bmtblunders and self.bmt_listaBlunders is None:
            self.bmt_listaBlunders = BMT.BMT_Lista()

        if self.bmtbrilliancies and self.bmt_listaBrilliancies is None:
            self.bmt_listaBrilliancies = BMT.BMT_Lista()

        xlibroAperturas = self.libroAperturas

        xblancas = self.blancas
        xnegras = self.negras

        if self.liJugadores:
            for x in ["BLACK", "WHITE"]:
                if x in dicPGN:
                    jugador = dicPGN[x].upper()
                    si = False
                    for uno in self.liJugadores:
                        siZ = uno.endswith("*")
                        siA = uno.startswith("*")
                        uno = uno.replace("*", "").strip().upper()
                        if siA:
                            if jugador.endswith(uno):
                                si = True
                            if siZ:  # form para poner siA y siZ
                                si = uno in jugador
                        elif siZ:
                            if jugador.startswith(uno):
                                si = True
                        elif uno == jugador:
                            si = True
                        if si:
                            break
                    if not si:
                        if x == "BLACK":
                            xnegras = False
                        else:
                            xblancas = False

        if not (xblancas or xnegras):
            return

        clpartida = Util.microsegundosRnd()
        txtPartida = partida.guardaEnTexto()
        siPonerPGNOriginalBlunders = False
        siPonerPGNOriginalBrilliancies = False

        nJugadas = len(partida)
        if self.listaElegidas:
            liJugadas = self.listaElegidas
        else:
            liJugadas = range(nJugadas)

        if xlibroAperturas:
            liBorrar = []
            for pos, njg in enumerate(liJugadas):

                if tmpBP.siCancelado():
                    self.xgestor.quitaGuiDispatch()
                    return

                # # Si esta en el libro
                jg = partida.jugada(njg)
                if xlibroAperturas.miraListaJugadas(jg.posicionBase.fen()):
                    liBorrar.append(pos)
                    continue
                else:
                    break
            if liBorrar:
                liBorrar.reverse()
                for x in liBorrar:
                    del liJugadas[x]

        if self.desdeelfinal:
            liJugadas.reverse()

        nJugadas = len(liJugadas)
        if siBP2:
            tmpBP.ponTotal(2, nJugadas)
        else:
            tmpBP.ponTotal(nJugadas)

        for npos, njg in enumerate(liJugadas):

            if tmpBP.siCancelado():
                self.xgestor.quitaGuiDispatch()
                return

            jg = partida.jugada(njg)
            if siBP2:
                tmpBP.pon(2, npos + 1)
            else:
                tmpBP.pon(npos)

            if self.rutDispatchBP:
                self.rutDispatchBP(npos, nJugadas, njg)

            if tmpBP.siCancelado():
                self.xgestor.quitaGuiDispatch()
                return

                # # Fin de partida
                # if jg.siJaqueMate or jg.siTablas():
                # continue

            # # blancas y negras
            siBlancas = jg.posicionBase.siBlancas
            if siBlancas:
                if not xblancas:
                    continue
            else:
                if not xnegras:
                    continue

            # -# previos
            if self.siBorrarPrevio:
                jg.analisis = None

            # -# Procesamos
            if jg.analisis is None:
                resp = self.xgestor.analizaJugadaPartida(partida, njg, self.tiempo, depth=self.depth,
                                                         brDepth=self.dpbrilliancies, brPuntos=self.ptbrilliancies,
                                                         stability=self.stability,
                                                         st_centipawns=self.st_centipawns,
                                                         st_depths=self.st_depths,
                                                         st_timelimit=self.st_timelimit)
                if not resp:
                    self.xgestor.quitaGuiDispatch()
                    return

                jg.analisis = resp
            cp = jg.posicionBase
            mrm, posAct = jg.analisis
            jg.complexity = AnalisisIndexes.calc_complexity(cp, mrm)
            jg.winprobability = AnalisisIndexes.calc_winprobability(cp, mrm)
            jg.narrowness = AnalisisIndexes.calc_narrowness(cp, mrm)
            jg.efficientmobility = AnalisisIndexes.calc_efficientmobility(cp, mrm)
            jg.piecesactivity = AnalisisIndexes.calc_piecesactivity(cp, mrm)
            jg.exchangetendency = AnalisisIndexes.calc_exchangetendency(cp, mrm)

            if siBlunders or siBrilliancies or self.siVariantes:
                rm = mrm.liMultiPV[posAct]
                rm.ponBlunder(0)
                mj = mrm.liMultiPV[0]
                rm_pts = rm.puntosABS()

                dif = mj.puntosABS() - rm_pts
                fen = jg.posicionBase.fen()

                if self.siVariantes:
                    limite = self.alm.limitemasvariantes
                    if (limite == 0) or (dif >= limite):
                        if not (self.alm.mejorvariante and dif == 0):
                            jg.analisis2variantes(Partida.Partida(), self.alm, self.siBorrarPrevio)

                if dif >= self.kblunders:
                    rm.ponBlunder(dif)

                    self.grabaTactic(dicPGN, partida, njg, mrm, posAct)

                    if self.grabaPGN(self.pgnblunders, mrm.nombre, dicPGN, fen, jg, rm, mj):
                        siPonerPGNOriginalBlunders = True

                    if self.bmtblunders:
                        self.grabaBMT(True, fen, mrm, posAct, clpartida, txtPartida)
                        self.siBMTblunders = True

                if rm.nivelBrillante():
                    jg.masCritica1_4("3")
                    self.grabaFNS(self.fnsbrilliancies, fen)

                    if self.grabaPGN(self.pgnbrilliancies, mrm.nombre, dicPGN, fen, jg, rm, None):
                        siPonerPGNOriginalBrilliancies = True

                    if self.bmtbrilliancies:
                        self.grabaBMT(False, fen, mrm, posAct, clpartida, txtPartida)
                        self.siBMTbrilliancies = True
                else:
                    nag, color = mrm.setNAG_Color(self.configuracion, rm)
                    if nag:
                        jg.masCritica1_4(str(nag))

        # Ponemos el texto original en la ultima
        if siPonerPGNOriginalBlunders and self.oriblunders:
            f = codecs.open(self.pgnblunders, "a", 'utf-8', 'ignore')
            f.write("\n%s\n\n" % textoOriginal)
            f.close()

        if siPonerPGNOriginalBrilliancies and self.oribrilliancies:
            f = codecs.open(self.pgnbrilliancies, "a", 'utf-8', 'ignore')
            f.write("\n%s\n\n" % textoOriginal)
            f.close()

        self.xgestor.quitaGuiDispatch()


class UnaMuestra:
    def __init__(self, mAnalisis, mrm, posElegida, numero, xmotor):

        self.mAnalisis = mAnalisis
        self.numero = numero
        self.xmotor = xmotor
        self.siFigurines = mAnalisis.configuracion.figurinesPGN

        self.mrm = mrm
        self.posElegida = posElegida

        self.posRMactual = posElegida
        self.posMovActual = 0

        self.jg = mAnalisis.jg

        self.siActiva = False

        self.listaRM = self.hazListaRM()  # rm,nombre

    def etiquetaMotor(self):
        return self.mrm.nombre

    def etiquetaTiempo(self):
        if self.mrm.maxTiempo:
            t = "%0.2f" % (float(self.mrm.maxTiempo) / 1000.0,)
            t = t.rstrip("0")
            if t[-1] == ".":
                t = t[:-1]
            etiT = "%s: %s" % (_("Second(s)"), t)
        elif self.mrm.maxProfundidad:
            etiT = "%s: %d" % (_("Depth"), self.mrm.maxProfundidad)
        else:
            etiT = ""
        return etiT

    def hazListaRM(self):
        li = []
        pb = self.jg.posicionBase
        for rm in self.mrm.liMultiPV:
            pv1 = rm.pv.split(" ")[0]
            desde = pv1[:2]
            hasta = pv1[2:4]
            coronacion = pv1[4].lower() if len(pv1) == 5 else None

            txt = rm.abrTextoBase()
            if txt:
                txt = "(%s)" % txt
            if self.siFigurines:
                nombre = pb.pgn(desde, hasta, coronacion) + txt

            else:
                nombre = pb.pgnSP(desde, hasta, coronacion) + txt
            li.append((rm, nombre, rm.puntosABS()))

        return li

    def ponWMU(self, wmu):
        self.wmu = wmu
        self.siActiva = True

    def desactiva(self):
        self.wmu.hide()
        self.siActiva = False

    def siElegido(self, posRM):
        return posRM == self.posElegida

    def ponPosRMactual(self, posRM):
        self.posRMactual = posRM
        self.rm = self.listaRM[self.posRMactual][0]
        self.partida = Partida.Partida(self.jg.posicionBase)
        self.partida.leerPV(self.rm.pv)
        self.partida.siTerminada()
        self.posMovActual = 0

    def pgnActual(self):
        return self.partida.pgnHTML(self.mAnalisis.posJugada / 2 + 1, siFigurines=self.siFigurines)

    def puntuacionActual(self):
        rm = self.listaRM[self.posRMactual][0]
        return rm.texto()

    def complexity(self):
        return AnalisisIndexes.get_complexity(self.jg.posicionBase, self.mrm)

    def winprobability(self):
        return AnalisisIndexes.get_winprobability(self.jg.posicionBase, self.mrm)

    def narrowness(self):
        return AnalisisIndexes.get_narrowness(self.jg.posicionBase, self.mrm)

    def efficientmobility(self):
        return AnalisisIndexes.get_efficientmobility(self.jg.posicionBase, self.mrm)

    def piecesactivity(self):
        return AnalisisIndexes.get_piecesactivity(self.jg.posicionBase, self.mrm)

    def posicionActual(self):
        nMovs = self.partida.numJugadas()
        if self.posMovActual >= nMovs:
            self.posMovActual = nMovs - 1
        if self.posMovActual < 0:
            self.posMovActual = -1
            return self.partida.jugada(0).posicionBase, None, None
        else:
            jgActual = self.partida.jugada(self.posMovActual)
            return jgActual.posicion, jgActual.desde, jgActual.hasta

    def posicionBaseActual(self):
        nMovs = self.partida.numJugadas()
        if self.posMovActual >= nMovs:
            self.posMovActual = nMovs - 1
        if self.posMovActual < 0:
            self.posMovActual = -1
            return self.partida.jugada(0).posicionBase, None, None
        else:
            jgActual = self.partida.jugada(self.posMovActual)
            return jgActual.posicionBase, jgActual.desde, jgActual.hasta

    def cambiaMovActual(self, accion):
        if accion == "Adelante":
            self.posMovActual += 1
        elif accion == "Atras":
            self.posMovActual -= 1
        elif accion == "Inicio":
            self.posMovActual = -1
        elif accion == "Final":
            self.posMovActual = self.partida.numJugadas() - 1

    def siPosFinal(self):
        return self.posMovActual >= self.partida.numJugadas() - 1

    def fenActual(self):
        jg = self.partida.jugada(self.posMovActual if self.posMovActual > 0 else 0)
        return jg.posicion.fen()

    def analizaExterior(self, wowner, siBlancas):
        jg = self.partida.jugada(self.posMovActual if self.posMovActual >= 0 else 0)
        pts = self.puntuacionActual()
        AnalisisVariantes(wowner, self.xmotor, jg, siBlancas, pts, maxRecursion=self.mAnalisis.maxRecursion)

    def grabarBase(self, partida, rm, siTodos=True):
        siW = self.jg.posicionBase.siBlancas
        nombre = self.etiquetaMotor()
        tiempo = self.etiquetaTiempo()
        comentario = " {%s %s %s} " % (rm.abrTexto(), nombre, tiempo)
        jugada = partida.pgnBaseRAW(self.jg.posicionBase.jugadas)
        li = jugada.split(" ")
        n = 1 if siW else 2
        if siTodos:
            variante = (" ".join(li[:n]) + comentario + " ".join(li[n:])).strip()
        else:
            variante = (" ".join(li[:n]) + comentario).strip()
        variantes = self.jg.variantes
        if variantes:
            if variante in variantes:  # Sin repetidos
                return
            variantes = variantes.strip() + "\n\n" + variante
        else:
            variantes = variante
        self.jg.variantes = variantes

    def ponVistaGestor(self):
        self.mAnalisis.procesador.gestor.ponVista()


class MuestraAnalisis:
    def __init__(self, procesador, jg, maxRecursion, posJugada):

        self.procesador = procesador
        self.configuracion = procesador.configuracion
        self.jg = jg
        self.posJugada = posJugada  # Para mostrar el pgn con los numeros correctos
        self.maxRecursion = maxRecursion
        self.liMuestras = []

    def creaMuestraInicial(self, pantalla, xmotor):
        jg = self.jg
        if hasattr(jg, "analisis") and jg.analisis:
            mrm, pos = jg.analisis
        else:
            me = QTUtil2.mensEspera.inicio(pantalla, _("Analyzing the move...."), posicion="ad") #, siCancelar=True)
            mrm, pos = xmotor.analizaJugada(jg, xmotor.motorTiempoJugada, xmotor.motorProfundidad)
            jg.analisis = mrm, pos
            me.final()

        um = UnaMuestra(self, mrm, pos, 0, xmotor)
        self.liMuestras.append(um)
        return um

    def creaMuestra(self, pantalla, alm):
        xmotor = None
        busca = alm.motor[1:] if alm.motor.startswith("*") else alm.motor
        for um in self.liMuestras:
            if um.xmotor.clave == busca:
                xmotor = um.xmotor
                xmotor.actMultiPV(alm.multiPV)
                break
        if xmotor is None:
            confMotor = self.configuracion.buscaMotor(alm.motor)
            confMotor.actMultiPV(alm.multiPV)
            xmotor = self.procesador.creaGestorMotor(confMotor, alm.tiempo, alm.depth, siMultiPV=True)

        me = QTUtil2.mensEspera.inicio(pantalla, _("Analyzing the move...."), posicion="ad")
        mrm, pos = xmotor.analizaJugada(self.jg, alm.tiempo, alm.depth)
        me.final()

        um = UnaMuestra(self, mrm, pos, self.liMuestras[-1].numero + 1, xmotor)
        self.liMuestras.append(um)
        return um


def muestraAnalisis(procesador, xtutor, jg, siBlancas, maxRecursion, posJugada, pantalla=None, siGrabar=True):
    pantalla = procesador.pantalla if pantalla is None else pantalla

    ma = MuestraAnalisis(procesador, jg, maxRecursion, posJugada)
    if xtutor is None:
        xtutor = procesador.XTutor()
    um0 = ma.creaMuestraInicial(pantalla, xtutor)
    if not um0:
        return
    siLibre = maxRecursion > 0
    wa = PantallaAnalisis.WAnalisis(ma, pantalla, siBlancas, siLibre, siGrabar, um0)
    wa.exec_()

    busca = True
    for uno in ma.liMuestras:
        if busca:
            if uno.siActiva:
                jg.analisis = uno.mrm, uno.posElegida

                busca = False
        xmotor = uno.xmotor
        if not xtutor or xmotor.clave != xtutor.clave:
            xmotor.terminar()


class AnalisisVariantes:
    def __init__(self, owner, xtutor, jg, siBlancas, cPuntosBase, maxRecursion=100000):

        self.owner = owner
        self.xtutor = xtutor
        self.jg = jg
        self.siBlancas = siBlancas
        self.posicionBase = jg.posicionBase
        self.siMoviendoTiempo = False

        self.rm = None

        if self.xtutor.motorTiempoJugada:
            segundosPensando = self.xtutor.motorTiempoJugada / 1000  # esta en milesimas
            if self.xtutor.motorTiempoJugada % 1000 > 0:
                segundosPensando += 1
        else:
            segundosPensando = 3

        self.w = PantallaAnalisis.WAnalisisVariantes(self, self.owner, segundosPensando, self.siBlancas, cPuntosBase,
                                                     maxRecursion)
        self.reset()
        self.w.exec_()

    def reset(self):
        self.w.tablero.ponPosicion(self.posicionBase)
        self.w.tablero.ponFlechaSC(self.jg.desde, self.jg.hasta)
        self.w.tablero.ponMensajero(self.mueveHumano)
        self.w.tablero.activaColor(not self.jg.posicion.siBlancas)

    def mueveHumano(self, desde, hasta, coronacion=None):

        # Peon coronando
        if not coronacion and self.posicionBase.siPeonCoronando(desde, hasta):
            coronacion = self.w.tablero.peonCoronando(not self.jg.posicion.siBlancas)
            if coronacion is None:
                return False

        siBien, mens, jgNueva = Jugada.dameJugada(self.posicionBase, desde, hasta, coronacion)

        if siBien:

            self.movimientosPiezas(jgNueva.liMovs)
            self.w.tablero.ponFlechaSC(jgNueva.desde, jgNueva.hasta)
            self.analizaJugada(jgNueva)
            return True
        else:
            return False

    def analizaJugada(self, jgNueva):
        me = QTUtil2.mensEspera.inicio(self.w, _("Analyzing the move...."))

        secs = self.w.dameSegundos()
        self.rm = self.xtutor.analizaVariante(jgNueva, secs * 1000, self.siBlancas)
        me.final()

        self.partidaTutor = Partida.Partida(jgNueva.posicion)
        self.partidaTutor.leerPV(self.rm.pv)

        if len(self.partidaTutor):
            self.w.tableroT.ponPosicion(self.partidaTutor.jugada(0).posicion)

        self.w.ponPuntuacion(self.rm.texto())

        self.posTutor = 0
        self.maxTutor = len(self.partidaTutor)

        self.mueveTutor(siInicio=True)

    def movimientosPiezas(self, liMovs):
        """
        Hace los movimientos de piezas en el tablero
        """
        for movim in liMovs:
            if movim[0] == "b":
                self.w.tablero.borraPieza(movim[1])
            elif movim[0] == "m":
                self.w.tablero.muevePieza(movim[1], movim[2])
            elif movim[0] == "c":
                self.w.tablero.cambiaPieza(movim[1], movim[2])

        self.w.tablero.desactivaTodas()

        self.w.tablero.escena.update()
        self.w.update()
        QTUtil.xrefreshGUI()

    def procesarTB(self, accion, maxRecursion):
        if self.rm:
            if accion == "MoverAdelante":
                self.mueveTutor(nSaltar=1)
            elif accion == "MoverAtras":
                self.mueveTutor(nSaltar=-1)
            elif accion == "MoverInicio":
                self.mueveTutor(siInicio=True)
            elif accion == "MoverFinal":
                self.mueveTutor(siFinal=True)
            elif accion == "MoverTiempo":
                self.mueveTiempo()
            elif accion == "MoverLibre":
                self.analizaExterior(maxRecursion)
            elif accion == "MoverFEN":
                jg = self.partidaTutor.jugada(self.posTutor)
                QTUtil.ponPortapapeles(jg.posicion.fen())
                QTUtil2.mensaje(self.w, _("FEN is in clipboard"))

    def mueveTutor(self, siInicio=False, nSaltar=0, siFinal=False, siBase=False):
        if nSaltar:
            pos = self.posTutor + nSaltar
            if 0 <= pos < self.maxTutor:
                self.posTutor = pos
            else:
                return
        elif siInicio or siBase:
            self.posTutor = 0
        elif siFinal:
            self.posTutor = self.maxTutor - 1
        if self.partidaTutor.numJugadas():
            jg = self.partidaTutor.jugada(self.posTutor)
            if siBase:
                self.w.tableroT.ponPosicion(jg.posicionBase)
            else:
                self.w.tableroT.ponPosicion(jg.posicion)
                self.w.tableroT.ponFlechaSC(jg.desde, jg.hasta)
        self.w.tableroT.escena.update()
        self.w.update()
        QTUtil.xrefreshGUI()

    def mueveTiempo(self):
        if self.siMoviendoTiempo:
            self.siMoviendoTiempo = False
            self.tiempoOtrosTB(True)
            self.w.paraReloj()
            return

        def otrosTB(siHabilitar):
            for accion in self.w.tb.liAcciones:
                if not accion.clave.endswith("MoverTiempo"):
                    accion.setEnabled(siHabilitar)

        self.tiempoFuncion = self.mueveTutor
        self.tiempoPosMax = self.maxTutor
        self.tiempoPos = -1
        self.tiempoOtrosTB = otrosTB
        self.siMoviendoTiempo = True
        otrosTB(False)
        self.mueveTutor(siBase=True)
        self.w.iniciaReloj(self.mueveTiempo1)

    def mueveTiempo1(self):
        self.tiempoPos += 1
        if self.tiempoPos == self.tiempoPosMax:
            self.siMoviendoTiempo = False
            self.tiempoOtrosTB(True)
            self.w.paraReloj()
            return
        if self.tiempoPos == 0:
            self.tiempoFuncion(siInicio=True)
        else:
            self.tiempoFuncion(nSaltar=1)

    def analizaExterior(self, maxRecursion):
        jg = self.partidaTutor.jugada(self.posTutor)
        pts = self.rm.texto()
        AnalisisVariantes(self.w, self.xtutor, jg, self.siBlancas, pts, maxRecursion)


def analizaPartida(gestor):
    partida = gestor.partida
    procesador = gestor.procesador
    pantalla = gestor.pantalla
    pgn = gestor.pgn

    alm = PantallaAnalisisParam.paramAnalisis(pantalla, procesador.configuracion, True)

    if alm is None:
        return

    liJugadas = []
    lni = Util.ListaNumerosImpresion(alm.jugadas)
    numJugada = int(partida.primeraJugada())
    siBlancas = not partida.siEmpiezaConNegras
    for nRaw in range(partida.numJugadas()):
        siGrabar = lni.siEsta(numJugada)
        if siGrabar:
            if siBlancas:
                if not alm.blancas:
                    siGrabar = False
            elif not alm.negras:
                siGrabar = False
        if siGrabar:
            liJugadas.append(nRaw)
        siBlancas = not siBlancas
        if siBlancas:
            numJugada += 1

    mensaje = _("Analyzing the move....")
    numJugadas = len(liJugadas)
    tmpBP = QTUtil2.BarraProgreso(pantalla, _("Analysis"), mensaje, numJugadas).mostrarAD()

    ap = AnalizaPartida(procesador, alm, False, liJugadas)

    def dispatchBP(pos, ntotal, njg):
        tmpBP.mensaje(mensaje + " %d/%d" % (pos + 1, ntotal))
        jg = partida.jugada(njg)
        gestor.ponPosicion(jg.posicion)
        gestor.pantalla.pgnColocate(njg / 2, (njg + 1) % 2)
        gestor.tablero.ponFlechaSC(jg.desde, jg.hasta)
        gestor.ponVista()

    ap.dispatchBP(dispatchBP)

    ap.xprocesa(pgn.dicCabeceraActual(), partida, tmpBP, pgn.actual())

    notCanceled = not tmpBP.siCancelado()
    ap.terminar(notCanceled)

    if notCanceled:
        liCreados = []
        liNoCreados = []

        if alm.tacticblunders:
            if ap.siTacticBlunders:
                liCreados.append(alm.tacticblunders)
            else:
                liNoCreados.append(alm.tacticblunders)

        for x in (alm.pgnblunders, alm.fnsbrilliancies, alm.pgnbrilliancies):
            if x:
                if Util.existeFichero(x):
                    liCreados.append(x)
                else:
                    liNoCreados.append(x)

        if alm.bmtblunders:
            if ap.siBMTblunders:
                liCreados.append(alm.bmtblunders)
            else:
                liNoCreados.append(alm.bmtblunders)
        if alm.bmtbrilliancies:
            if ap.siBMTbrilliancies:
                liCreados.append(alm.bmtbrilliancies)
            else:
                liNoCreados.append(alm.bmtbrilliancies)

        if liCreados or liNoCreados:
            PantallaPGN.mensajeEntrenamientos(pantalla, liCreados, liNoCreados)

    tmpBP.cerrar()

    gestor.ponteAlFinal()

    if notCanceled:
        if alm.showGraphs:
            gestor.showAnalisis()

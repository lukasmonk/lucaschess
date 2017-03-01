import os

from PyQt4 import QtGui

from Code import CPU
from Code import ControlPosicion
from Code.QT import Colocacion
from Code.QT import Columnas
from Code.QT import Controles
from Code.QT import Delegados
from Code.QT import FormLayout
from Code.QT import Grid
from Code.QT import Iconos
from Code.QT import PantallaPGN
from Code.QT import PantallaTabVFlechas
from Code.QT import PantallaTabVMarcos
from Code.QT import PantallaTabVMarkers
from Code.QT import PantallaTabVPartidas
from Code.QT import PantallaTabVSVGs
from Code.QT import QTUtil
from Code.QT import QTUtil2
from Code.QT import QTVarios
from Code.QT import Tablero
from Code import TabVisual
from Code import Util
from Code import VarGen
from Code.QT import Voyager


class WTabDirector(QTVarios.WDialogo):
    def __init__(self, tableroOwner):

        self.leeRecursos()

        self.tableroOwner = tableroOwner
        titulo = _("Director")
        icono = Iconos.Director()
        extparam = "tabdirector"
        QTVarios.WDialogo.__init__(self, tableroOwner, titulo, icono, extparam)

        liAcciones = [(_("Close"), Iconos.MainMenu(), self.terminar), None,
                      (_("Arrows"), Iconos.Flechas(), self.flechas), None,
                      (_("Boxes"), Iconos.Marcos(), self.marcos), None,
                      (_("Images"), Iconos.SVGs(), self.svgs), None,
                      (_("Markers"), Iconos.Markers(), self.markers), None,
                      None,
                      (_("Clipboard"), Iconos.Clip(), self.portapapeles), None,
                      (_("Save") + " png", Iconos.GrabarFichero(), self.grabarFichero), None,
                      ]
        tb = Controles.TBrutina(self, liAcciones, siTexto=False, tamIcon=32)

        pbLimpia = Controles.PB(self, _("Clean main board"), self.limpiaTableroOwner, plano=False)
        self.siTabCoord = self.dbConfig["COORDINADOS"]
        if self.siTabCoord is None:
            self.siTabCoord = self.dbConfig["COORDINADOS"] = True
        self.chTabCoord = Controles.CHB(self, _("Boards coordinated"), self.siTabCoord)
        self.chTabCoord.capturaCambiado(self, self.cambiadoTabCoord)
        pbCoordina = Controles.PB(self, "", self.coordinaTableros).ponIcono(Iconos.Coordina()).anchoFijo(30)

        # Tablero
        confTablero = VarGen.configuracion.confTablero("Director", 24, padre=tableroOwner.confTablero.id())
        self.tablero = Tablero.TableroDirector(self, confTablero)
        self.tablero.crea()
        self.tablero.ponDispatchEventos(self.dispatch)
        self.tablero.dispatchSize(self.tableroCambiadoTam)
        self.tablero.baseCasillasSC.setAcceptDrops(True)
        self.tablero.ponMensajero(self.muevePieza)

        self.tablero.activaTodas()

        self.guion = TabVisual.Guion()
        self.nomGuion = ""

        # Tools
        listaPiezas = QTVarios.ListaPiezas(self, "P,N,B,R,Q,K,p,n,b,r,q,k", self.tablero, 18, margen=0)

        # Guion
        liAcciones = [(_("New"), Iconos.Nuevo(), self.gnuevo),
                      (_("Insert"), Iconos.Insertar(), self.ginsertar),
                      (_("Copy"), Iconos.Copiar(), self.gcopiar), None,
                      (_("Remove"), Iconos.Borrar(), self.gborrar), None,
                      (_("Up"), Iconos.Arriba(), self.garriba),
                      (_("Down"), Iconos.Abajo(), self.gabajo), None,
                      (_("Mark"), Iconos.Marcar(), self.gmarcar), None,
                      (_("Save"), Iconos.Grabar(), self.ggrabarGuion),
                      (_("Open"), Iconos.Recuperar(), self.grecuperarGuion), None,
                      (_("Remove script"), Iconos.Delete(), self.geliminarGuion)
                      ]
        tbGuion = Controles.TBrutina(self, liAcciones, siTexto=False, tamIcon=20)
        oColumnas = Columnas.ListaColumnas()
        oColumnas.nueva("DIRECTOR", "0", 20, siCentrado=True, siChecked=True)
        oColumnas.nueva("MARCADO", "1", 20, siCentrado=True, siChecked=True)
        oColumnas.nueva("TIPO", _("Type"), 50, siCentrado=True)
        oColumnas.nueva("NOMBRE", _("Name"), 100, siCentrado=True, edicion=Delegados.LineaTextoUTF8())
        oColumnas.nueva("INFO", _("Information"), 100, siCentrado=True)
        self.g_guion = Grid.Grid(self, oColumnas, siCabeceraMovible=False, siEditable=True, siSeleccionMultiple=True)
        self.g_guion.gotop()
        self.g_guion.setSizePolicy(QtGui.QSizePolicy.Expanding, QtGui.QSizePolicy.Expanding)

        self.registrarGrid(self.g_guion)

        # Visuales
        self.dragBanda = QTVarios.DragBanda(self, (5, 5), 32, margen=0)

        # Botones
        lyLC = Colocacion.H().control(pbLimpia).control(pbCoordina).control(self.chTabCoord)

        lyTB = Colocacion.V()
        lyTB.controlc(self.dragBanda)
        lyTB.control(self.tablero)
        lyTB.control(listaPiezas)
        lyTB.otro(lyLC)
        lyTB.control(tbGuion)
        lyTB.control(self.g_guion)
        lyTB.margen(0)

        # Layout
        layout = Colocacion.V().control(tb).otro(lyTB).margen(3)
        self.setLayout(layout)

        self.recuperarVideo()

        self.actualizaBandas()
        li = self.dbConfig["DRAGBANDA"]
        if li:
            self.dragBanda.recuperar(li)

        self.ultDesde = "d4"
        self.ultHasta = "e5"

        self.compruebaTabCoord()

        self.tablero.setFocus()

        self.importaOtroTablero()

    def importaOtroTablero(self):
        if self.tableroOwner.dicMovibles:
            for k, v in self.tableroOwner.dicMovibles.iteritems():
                xobj = str(v)
                if "Marco" in xobj:
                    sc = self.tablero.creaMarco(v.bloqueDatos)
                    tarea = TabVisual.GT_Marco()
                elif "Flecha" in xobj:
                    sc = self.tablero.creaFlecha(v.bloqueDatos)
                    tarea = TabVisual.GT_Flecha()
                elif "SVG" in xobj:
                    sc = self.tablero.creaSVG(v.bloqueDatos)
                    tarea = TabVisual.GT_SVG()
                elif "Marker" in xobj:
                    sc = self.tablero.creaMarker(v.bloqueDatos)
                    tarea = TabVisual.GT_Marker()
                else:
                    continue
                sc.ponRutinaPulsada(self.pulsadoItemSC, tarea.id())
                tarea.itemSC(sc)
                tarea.itemSCOwner(v)
                self.guion.nuevaTarea(tarea)

        self.tablero.copiaPosicionDe(self.tableroOwner)
        self.fenInicial = self.tablero.fenActual()
        self.tablero.activaTodas()
        self.g_guion.gotop()

    def cambiadoTabCoord(self):
        self.siTabCoord = self.chTabCoord.valor()
        self.dbConfig["COORDINADOS"] = self.siTabCoord

    def compruebaTabCoord(self):
        if self.siTabCoord:
            for n in range(len(self.guion)):
                mc = self.guion.marcado(n)
                mcOwner = self.guion.marcadoOwner(n)
                if mc != mcOwner:
                    self.ponMarcadoOwner(n, mc)

    def limpiaTableroOwner(self):
        self.tableroOwner.borraMovibles()
        for n in range(len(self.guion)):
            self.ponMarcadoOwner(n, False)
            self.guion.borraItemTareaOwner(n)
        self.g_guion.refresh()
        cp = ControlPosicion.ControlPosicion()
        cp.leeFen(self.fenInicial)
        self.tableroOwner.ponPosicionBase(cp)

    def tableroCambiadoTam(self):
        self.layout().setSizeConstraint(QtGui.QLayout.SetFixedSize)
        self.show()
        QTUtil.refreshGUI()
        self.layout().setSizeConstraint(QtGui.QLayout.SetMinAndMaxSize)

    def gridDato(self, grid, fila, oColumna):

        clave = oColumna.clave
        if clave == "MARCADO":
            return self.guion.marcado(fila)
        elif clave == "TIPO":
            return self.guion.txt_tipo(fila)
        elif clave == "NOMBRE":
            return self.guion.nombre(fila)
        elif clave == "INFO":
            return self.guion.info(fila)
        elif clave == "DIRECTOR":
            return self.guion.marcadoOwner(fila)

    def pulsadoItemSC(self, xid):
        for n in range(len(self.guion)):
            tarea = self.guion.tarea(n)
            if tarea.id() == xid:
                if tarea.tp() in "FMSX":
                    tarea.coordina()
                    break
        self.g_guion.refresh()

    def miraCambios(self):
        self.g_guion.refresh()

    def dispatch(self, evento, envio):
        t = self.tablero
        if evento == t.EVENTO_DERECHO:
            item = envio
            tarea, nfila = self.guion.tareaItem(item)
            if tarea:
                self.ponMarcado(nfila, False)

        elif evento == t.EVENTO_DROP:
            desde, dato = envio
            self.dispatchDrop(desde, dato)
        elif evento == t.EVENTO_DERECHO_PIEZA:
            ttpieza, desde = envio
            self.creaTarea("B", ttpieza.pieza, desde, -1)

        elif evento == t.EVENTO_FUNCION:
            funcion, desde, hasta = envio
            xid = self.dragBanda.idLB(funcion)
            if xid:
                self.dispatchDrop(desde, xid, hasta)

        self.miraCambios()

    def dispatchDrop(self, desde, dato, hasta=None):

        if dato.isalpha():
            pieza = dato
            self.tablero.cambiaPieza(desde, pieza)
            self.tablero.activaTodas()
            tp = "C"
            xid = pieza
            a1h8 = desde

        elif dato.startswith("_"):
            li = dato.split("_")
            tp = li[1]
            xid = int(li[2])
            if hasta:
                a1h8 = desde + hasta
            else:

                if tp == "F":
                    fd = desde[1]
                    cd = desde[0]
                    ch = "f" if cd == "h" else chr(ord(cd) + 1)
                    fh = "2" if fd == "1" else str(int(fd) - 1)
                    hasta = ch + fh
                    a1h8 = desde + hasta
                elif tp == "M":
                    a1h8 = desde + desde
                elif tp == "S":
                    a1h8 = desde + desde
                elif tp == "X":
                    a1h8 = desde + desde

        self.creaTarea(tp, xid, a1h8, -1)

    def creaTareaBase(self, tp, xid, a1h8, fila):

        tpid = tp, xid
        if tp == "P":
            tarea = TabVisual.GT_PiezaMueve()
            tarea.desdeHasta(a1h8[:2], a1h8[2:])
        elif tp == "C":
            tarea = TabVisual.GT_PiezaCrea()
            tarea.desde(a1h8)
            tarea.pieza(xid)
        elif tp == "B":
            tarea = TabVisual.GT_PiezaBorra()
            tarea.desde(a1h8)
            tarea.pieza(xid)
        else:
            if tp == "F":
                regFlecha = self.dbFlechas[xid]
                if regFlecha is None:
                    return None, None
                regFlecha.tpid = tpid
                regFlecha.a1h8 = a1h8
                sc = self.tablero.creaFlecha(regFlecha)
                tarea = TabVisual.GT_Flecha()
            elif tp == "M":
                regMarco = self.dbMarcos[xid]
                if regMarco is None:
                    return None, None
                regMarco.tpid = tpid
                regMarco.a1h8 = a1h8
                sc = self.tablero.creaMarco(regMarco)
                tarea = TabVisual.GT_Marco()
            elif tp == "S":
                regSVG = self.dbSVGs[xid]
                if regSVG is None:
                    return None, None
                regSVG.tpid = tpid
                regSVG.a1h8 = a1h8
                sc = self.tablero.creaSVG(regSVG, siEditando=True)
                tarea = TabVisual.GT_SVG()
            elif tp == "X":
                regMarker = self.dbMarkers[xid]
                if regMarker is None:
                    return None, None
                regMarker.tpid = tpid
                regMarker.a1h8 = a1h8
                sc = self.tablero.creaMarker(regMarker, siEditando=True)
                tarea = TabVisual.GT_Marker()
            sc.ponRutinaPulsada(self.pulsadoItemSC, tarea.id())
            tarea.itemSC(sc)

        tarea.marcado(True)
        tarea.registro((tp, xid, a1h8))
        fila = self.guion.nuevaTarea(tarea, fila)

        return tarea, fila

    def creaTarea(self, tp, xid, a1h8, fila):
        tarea, fila = self.creaTareaBase(tp, xid, a1h8, fila)
        if tarea is None:
            return
        tarea.registro((tp, xid, a1h8))
        self.g_guion.goto(fila, 0)

        self.ponMarcado(fila, True)

        self.g_guion.refresh()

    def editaNombre(self, nombre):
        liGen = [(None, None)]
        config = FormLayout.Editbox(_("Name"), ancho=160)
        liGen.append((config, nombre))
        ico = Iconos.Grabar()

        resultado = FormLayout.fedit(liGen, title=_("Name"), parent=self, icon=ico)
        if resultado:
            accion, liResp = resultado
            nombre = liResp[0]
            return nombre
        return None

    def execMenuSP(self, tipo, siInsertar):
        if siInsertar:
            fila = self.g_guion.recno()
            filaIni = fila
        else:
            fila = -1
            filaIni = len(self.guion)

        if tipo == "PI":
            fen = self.tablero.fenActual()
            fen = Voyager.voyagerFEN(self, fen)
            if fen is None:
                return None

            nombre = self.editaNombre(_("Start position"))
            if nombre is None:
                return

            tarea = TabVisual.GT_Posicion()
            if not fen:
                fen = ControlPosicion.FEN_INICIAL
            tarea.fen(fen)
            tarea.nombre(nombre)
            self.guion.nuevaTarea(tarea, fila)
            self.gridPonValor(None, filaIni, None, True)

        elif tipo == "PP":
            texto = QTUtil.traePortapapeles()
            if texto:
                cp = ControlPosicion.ControlPosicion()
                try:
                    nombre = self.editaNombre(_("Start position"))
                    if nombre is None:
                        return
                    cp.leeFen(str(texto))
                    tarea = TabVisual.GT_Posicion()
                    tarea.fen(cp.fen())
                    tarea.nombre(nombre)
                    self.guion.nuevaTarea(tarea, fila)
                    self.gridPonValor(None, filaIni, None, True)
                except:
                    return None
        elif tipo == "PA":
            nombre = self.editaNombre(_("Start position"))
            if nombre is None:
                return
            tarea = TabVisual.GT_Posicion()
            tarea.fen(self.tablero.fenActual())
            tarea.nombre(nombre)
            self.guion.nuevaTarea(tarea, fila)
        else:
            if tipo == "PGNF":
                unpgn = PantallaPGN.eligePartida(self)
                partida = unpgn.partida if unpgn else None
            else:
                pgn = QTUtil.traePortapapeles()
                partida = PantallaTabVPartidas.texto2partida(self, pgn) if pgn else None
            if partida and partida.numJugadas():
                w = PantallaTabVPartidas.W_EligeMovimientos(self, partida)
                if w.exec_():
                    for jg in w.resultado:
                        tarea = TabVisual.GT_Jugada()
                        tarea.jugada(jg)
                        self.guion.nuevaTarea(tarea, fila)
                        if fila != -1:
                            fila += 1
                else:
                    return None
            else:
                return None

        self.g_guion.goto(filaIni, 0)
        self.g_guion.refresh()

    def menuTarea(self, siExtendido, siInsertar):

        if siExtendido:
            mp = _("Move piece")
            masMenu = [
                (("_P_0", mp), mp, Iconos.PuntoAzul()),
            ]
            masMenu.extend([
                (("SP", "PI"), _("Start position"), Iconos.Datos()),
                (("SP", "PA"), _("Current position"), Iconos.Copiar()),
                (("SP", "PP"), _("Paste FEN position"), Iconos.Pegar16()),
                (("SP", "PGNF"), _("Read PGN"), Iconos.PGN_Importar()),
                (("SP", "PGNP"), _("Paste PGN"), Iconos.Pegar16()),
            ])
        else:
            masMenu = []

        resp = self.dragBanda.menuParaExterior(masMenu)

        if resp and resp[0] == "SP":
            self.execMenuSP(resp[1], siInsertar)
            return None

        return resp

    def gmarcar(self):
        if len(self.guion):
            menu = QTVarios.LCMenu(self)
            f = Controles.TipoLetra(puntos=8, peso=75)
            menu.ponFuente(f)
            menu.opcion(1, _("All"), Iconos.PuntoVerde())
            menu.opcion(2, _("None"), Iconos.PuntoNaranja())
            resp = menu.lanza()
            if resp:
                siTodos = resp == 1
                for n in range(len(self.guion)):
                    siMarcado = self.guion.tarea(n).marcado()
                    if siTodos:
                        if not siMarcado:
                            self.gridPonValor(None, n, None, True)
                    else:
                        if siMarcado:
                            self.gridPonValor(None, n, None, False)
                self.g_guion.refresh()

    def desdeHasta(self, titulo, desde, hasta):
        liGen = [(None, None)]

        config = FormLayout.Casillabox(_("From square"))
        liGen.append((config, desde))

        config = FormLayout.Casillabox(_("To square"))
        liGen.append((config, hasta))

        resultado = FormLayout.fedit(liGen, title=titulo, parent=self)
        if resultado:
            resp = resultado[1]
            self.ultDesde = desde = resp[0]
            self.ultHasta = hasta = resp[1]
            return desde, hasta
        else:
            return None, None

    def gmas(self, siInsertar):
        resp = self.menuTarea(True, siInsertar)
        if resp:
            xid, tit = resp
            li = xid.split("_")
            tp = li[1]
            xid = int(li[2])
            desde, hasta = self.desdeHasta(tit, self.ultDesde, self.ultHasta)
            if desde:
                fila = self.g_guion.recno() if siInsertar else -1
                self.creaTarea(tp, xid, desde + hasta, fila)

    def gnuevo(self):
        self.gmas(False)

    def ginsertar(self):
        self.gmas(True)

    def gcopiar(self):
        fila = self.g_guion.recno()
        if fila >= 0:
            sc = self.guion.itemTarea(fila)
            if sc:
                bd = sc.bloqueDatos
                tp, xid = bd.tpid
                a1h8 = bd.a1h8
                self.creaTarea(tp, xid, a1h8, fila + 1)

    def gborrar(self):
        li = self.g_guion.recnosSeleccionados()
        if li:
            li.sort(reverse=True)
            for fila in li:
                sc = self.guion.itemTarea(fila)
                if sc:
                    self.tablero.borraMovible(sc)
                self.guion.borra(fila)
            self.g_guion.goto(fila, 0)
        self.g_guion.refresh()

    def garriba(self):
        fila = self.g_guion.recno()
        if self.guion.arriba(fila):
            self.g_guion.goto(fila - 1, 0)
            self.g_guion.refresh()

    def gabajo(self):
        fila = self.g_guion.recno()
        if self.guion.abajo(fila):
            self.g_guion.goto(fila + 1, 0)
            self.g_guion.refresh()

    def gridDobleClick(self, grid, fila, col):
        clave = col.clave
        if clave == "INFO":
            # fila = self.g_guion.recno()
            tarea = self.guion.tarea(fila)
            sc = self.guion.itemTarea(fila)
            if sc:
                a1h8 = tarea.a1h8()
                desde, hasta = self.desdeHasta(tarea.txt_tipo() + " " + tarea.nombre(), a1h8[:2], a1h8[2:])
                if desde:
                    sc = tarea.itemSC()
                    sc.ponA1H8(desde + hasta)
                    tarea.coordina()

            elif isinstance(tarea, TabVisual.GT_Posicion):
                fen = Voyager.voyagerFEN(self, tarea.fen())
                if fen is not None:
                    tarea.fen(fen)

            elif isinstance(tarea, TabVisual.GT_PiezaMueve):
                desde, hasta = tarea.desdeHasta()
                desde, hasta = self.desdeHasta(tarea.txt_tipo() + " " + tarea.nombre(), desde, hasta)
                if desde:
                    tarea.desdeHasta(desde, hasta)
                    tarea.marcado(False)

            mo = tarea.marcadoOwner()
            if mo:
                self.ponMarcadoOwner(fila, mo)

    def ggrabarGuion(self):
        liGen = [(None, None)]

        config = FormLayout.Editbox(_("Name"), ancho=160)
        liGen.append((config, self.nomGuion))

        resultado = FormLayout.fedit(liGen, title=_("Name"), parent=self)
        if resultado:
            resp = resultado[1][0].strip()
            if resp:
                self.dbGuiones[resp] = self.guion.guarda()

    def eligeGuion(self):
        ico = Iconos.PuntoNaranja()
        menu = QTVarios.LCMenu(self)
        for k in self.dbGuiones.keys():
            if k:  # para que no coja el de defecto
                menu.opcion(k, k, ico)
        return menu.lanza()

    def keyPressEvent(self, event):
        self.tablero.keyPressEvent(event)

    def grecuperarGuion(self):
        g = self.eligeGuion()
        if g:
            self.leeGuion(g, True)
            self.g_guion.gotop()
            self.g_guion.setFocus()
            self.g_guion.refresh()

    def geliminarGuion(self):
        g = self.eligeGuion()
        if g:
            if QTUtil2.pregunta(self, _X(_("Delete %1?"), g)):
                del self.dbGuiones[g]

    def leeGuion(self, nomGuion, siFenInicial):
        li = self.dbGuiones.get(nomGuion, TabVisual.Guion())
        self.guion = TabVisual.Guion()
        if li:
            self.tablero.dicMovibles = {}
            # self.tableroOwner.borraMovibles()
            self.tablero.crea()
            for reg in li:
                if reg._registro:
                    tp, xid, a1h8 = reg._registro
                    tarea, fila = self.creaTareaBase(tp, xid, a1h8, -1)
                    if tarea is None:
                        continue
                    tarea.recupera(reg)
                    if hasattr(reg, "_bloqueDatos"):
                        sc = tarea.itemSC()
                        sc.bloqueDatos = reg._bloqueDatos
                        sc.update()
                        sc.setVisible(False)
                    tarea.marcado(False)

                else:
                    tarea = self.guion.recuperaReg(reg)
                    tarea._marcado = False
        if siFenInicial:
            cp = ControlPosicion.ControlPosicion()
            cp.leeFen(self.fenInicial)
            self.tablero.ponPosicionBase(cp)
        self.tablero.activaTodas()
        self.nomGuion = nomGuion

    def gridNumDatos(self, grid):
        return len(self.guion)

    def clonaItemTarea(self, fila):
        tableroOwner = self.tableroOwner
        tarea = self.guion.tarea(fila)
        bloqueDatos = tarea.bloqueDatos()
        tp = tarea.tp()
        if tp == "F":
            sc = tableroOwner.creaFlecha(bloqueDatos)
        elif tp == "M":
            sc = tableroOwner.creaMarco(bloqueDatos)
        elif tp == "S":
            sc = tableroOwner.creaSVG(bloqueDatos)
        elif tp == "X":
            sc = tableroOwner.creaMarker(bloqueDatos)
        else:
            return None
        return sc

    def cambiadaPosicion(self, posicion):
        self.coordinaTableros()

    def coordinaTableros(self):
        self.tablero.copiaPosicionDe(self.tableroOwner)

    def ponMarcadoOwner(self, fila, siMarcado):
        self.guion.cambiaMarcaTareaOwner(fila, siMarcado)
        itemSC = self.guion.itemTareaOwner(fila)
        if not itemSC:
            sc = self.guion.itemTarea(fila)
            if sc:
                itemSC = self.clonaItemTarea(fila)
                if not itemSC:
                    return
                tarea = self.guion.tarea(fila)
                tarea.itemSCOwner(itemSC)
                tarea.coordina()
        self.ponMarcadoItem(fila, self.tableroOwner, itemSC, siMarcado)

        if self.siTabCoord and self.guion:
            mc = self.guion.marcado(fila)
            if mc != siMarcado:
                self.ponMarcado(fila, siMarcado)

        self.g_guion.refresh()

    def ponMarcado(self, fila, siMarcado):
        self.guion.cambiaMarcaTarea(fila, siMarcado)
        itemSC = self.guion.itemTarea(fila)
        self.ponMarcadoItem(fila, self.tablero, itemSC, siMarcado)

        if self.siTabCoord:
            mc = self.guion.marcadoOwner(fila)
            if mc != siMarcado:
                self.ponMarcadoOwner(fila, siMarcado)

        self.g_guion.refresh()

    def ponMarcadoItem(self, fila, tablero, itemSC, siMarcado):

        if itemSC:
            itemSC.setVisible(siMarcado)

        else:
            tarea = self.guion.tarea(fila)
            if isinstance(tarea, TabVisual.GT_PiezaMueve):
                desde, hasta = tarea.desdeHasta()
                if not siMarcado:
                    desde, hasta = hasta, desde
                tablero.muevePieza(desde, hasta)

            elif isinstance(tarea, TabVisual.GT_Posicion):
                if siMarcado:
                    fen = tarea.fen()
                    tarea.fenAnterior(tablero.fenActual())
                else:
                    fen = tarea.fenAnterior()
                    if not fen:
                        return
                cp = ControlPosicion.ControlPosicion()
                cp.leeFen(fen)
                tablero.ponPosicionBase(cp)

            elif isinstance(tarea, TabVisual.GT_Jugada):
                jg = tarea.jugada()
                tablero.ponPosicionBase(jg.posicionBase)
                if siMarcado:
                    if VarGen.configuracion.siSuenaJugada:
                        VarGen.runSound.playLista(jg.listaSonidos())
                    self.movimientosPiezasTV(jg.liMovs, tablero)
                    tablero.ponFlechaSC(jg.desde, jg.hasta)

            elif isinstance(tarea, TabVisual.GT_PiezaCrea):
                if siMarcado:
                    tablero.cambiaPieza(tarea.desde(), tarea.pieza())
                else:
                    tablero.borraPieza(tarea.desde())

            elif isinstance(tarea, TabVisual.GT_PiezaBorra):
                if siMarcado:
                    tablero.borraPieza(tarea.desde())
                else:
                    tablero.cambiaPieza(tarea.desde(), tarea.pieza())

    def gridPonValor(self, grid, fila, oColumna, valor):
        clave = oColumna.clave if oColumna else "MARCADO"
        if clave == "MARCADO":
            self.ponMarcado(fila, valor > 0)
        elif clave == "NOMBRE":
            tarea = self.guion.tarea(fila)
            tarea.nombre(valor.strip())
        elif clave == "DIRECTOR":
            self.ponMarcadoOwner(fila, valor > 0)

    def editarBanda(self, cid):
        li = cid.split("_")
        tp = li[1]
        xid = int(li[2])
        ok = False
        if tp == "F":
            regFlecha = self.dbFlechas[xid]
            w = PantallaTabVFlechas.WTV_Flecha(self, regFlecha, True)
            if w.exec_():
                self.dbFlechas[xid] = w.regFlecha
                ok = True
        elif tp == "M":
            regMarco = self.dbMarcos[xid]
            w = PantallaTabVMarcos.WTV_Marco(self, regMarco)
            if w.exec_():
                self.dbMarcos[xid] = w.regMarco
                ok = True
        elif tp == "S":
            regSVG = self.dbSVGs[xid]
            w = PantallaTabVSVGs.WTV_SVG(self, regSVG)
            if w.exec_():
                self.dbSVGs[xid] = w.regSVG
                ok = True
        elif tp == "X":
            regMarker = self.dbMarkers[xid]
            w = PantallaTabVMarkers.WTV_Marker(self, regMarker)
            if w.exec_():
                self.dbMarkers[xid] = w.regMarker
                ok = True

        if ok:
            self.actualizaBandas()

    def guardarTablero(self):
        self.dbConfig["CONFTABLERO"] = self.tablero.confTablero
        self.dbConfig["DRAGBANDA"] = self.dragBanda.guardar()

    def cierraRecursos(self):
        self.dbGuiones[""] = self.guion.guarda()

        self.dbConfig.close()
        self.dbFlechas.close()
        self.dbMarcos.close()
        self.dbSVGs.close()
        self.dbMarkers.close()

        self.dbGuiones.close()

        self.guardarVideo()

        self.tableroOwner.cierraDirector()
        self.guion = None

    def closeEvent(self, event):
        if self.guion is not None:
            self.cierraRecursos()

    def terminar(self):
        self.guardarTablero()  # Solo guarda con Quit, con la x no, asi hay mas control
        self.close()

    def portapapeles(self):
        self.tableroOwner.salvaEnImagen()
        txt = _("Clipboard")
        QTUtil2.mensajeTemporal(self, _X(_("Saved to %1"), txt), 0.8)

    def grabarFichero(self):
        dirSalvados = VarGen.configuracion.dirSalvados
        resp = QTUtil2.salvaFichero(self, _("File to save"), dirSalvados, _("File") + " PNG (*.png)", False)
        if resp:
            self.tableroOwner.salvaEnImagen(resp, "png")
            txt = resp
            QTUtil2.mensajeTemporal(self, _X(_("Saved to %1"), txt), 0.8)
            direc = os.path.dirname(resp)
            if direc != dirSalvados:
                VarGen.configuracion.dirSalvados = direc
                VarGen.configuracion.graba()

    def flechas(self):
        w = PantallaTabVFlechas.WTV_Flechas(self, self.listaFlechas(), self.dbFlechas)
        w.exec_()
        self.actualizaBandas()
        QTUtil.refreshGUI()

    def listaFlechas(self):
        dic = self.dbFlechas.asDictionary()
        li = [regFlecha for k, regFlecha in dic.iteritems()]
        li.sort(key=lambda x: x.ordenVista)
        return li

    def marcos(self):
        w = PantallaTabVMarcos.WTV_Marcos(self, self.listaMarcos(), self.dbMarcos)
        w.exec_()
        self.actualizaBandas()
        QTUtil.refreshGUI()

    def listaMarcos(self):
        dic = self.dbMarcos.asDictionary()
        li = [regMarco for k, regMarco in dic.iteritems()]
        li.sort(key=lambda x: x.ordenVista)
        return li

    def svgs(self):
        w = PantallaTabVSVGs.WTV_SVGs(self, self.listaSVGs(), self.dbSVGs)
        w.exec_()
        self.actualizaBandas()
        QTUtil.refreshGUI()

    def listaSVGs(self):
        dic = self.dbSVGs.asDictionary()
        li = [regSVG for k, regSVG in dic.iteritems()]
        li.sort(key=lambda x: x.ordenVista)
        return li

    def markers(self):
        w = PantallaTabVMarkers.WTV_Markers(self, self.listaMarkers(), self.dbMarkers)
        w.exec_()
        self.actualizaBandas()
        QTUtil.refreshGUI()

    def listaMarkers(self):
        dic = self.dbMarkers.asDictionary()
        li = [regMarker for k, regMarker in dic.iteritems()]
        li.sort(key=lambda x: x.ordenVista)
        return li

    def leeRecursos(self):
        fdb = VarGen.configuracion.ficheroRecursos
        self.dbConfig = Util.DicSQL(fdb, tabla="Config")
        self.dbFlechas = Util.DicSQL(fdb, tabla="Flechas")
        self.dbMarcos = Util.DicSQL(fdb, tabla="Marcos")
        self.dbSVGs = Util.DicSQL(fdb, tabla="SVGs")
        self.dbMarkers = Util.DicSQL(fdb, tabla="Markers")
        self.dbGuiones = Util.DicSQL(fdb, tabla="Guiones")

    def actualizaBandas(self):
        self.dragBanda.iniActualizacion()

        tipo = _("Arrows")
        for flecha in self.listaFlechas():
            pm = QtGui.QPixmap()
            pm.loadFromData(flecha.png, "PNG")
            xid = "_F_%d" % flecha.id
            nombre = flecha.nombre
            self.dragBanda.actualiza(xid, nombre, pm, tipo)

        tipo = _("Boxes")
        for marco in self.listaMarcos():
            pm = QtGui.QPixmap()
            pm.loadFromData(marco.png, "PNG")
            xid = "_M_%d" % marco.id
            nombre = marco.nombre
            self.dragBanda.actualiza(xid, nombre, pm, tipo)

        tipo = _("Images")
        for svg in self.listaSVGs():
            pm = QtGui.QPixmap()
            pm.loadFromData(svg.png, "PNG")
            xid = "_S_%d" % svg.id
            nombre = svg.nombre
            self.dragBanda.actualiza(xid, nombre, pm, tipo)

        tipo = _("Markers")
        for marker in self.listaMarkers():
            pm = QtGui.QPixmap()
            pm.loadFromData(marker.png, "PNG")
            xid = "_X_%d" % marker.id
            nombre = marker.nombre
            self.dragBanda.actualiza(xid, nombre, pm, tipo)

        self.dragBanda.finActualizacion()

        dicCampos = {
            "F": ("nombre", "altocabeza", "tipo", "destino", "color", "colorinterior", "colorinterior2", "opacidad",
                  "redondeos", "forma", "ancho", "vuelo", "descuelgue"),
            "M": ("nombre", "color", "colorinterior", "colorinterior2", "grosor", "redEsquina", "tipo", "opacidad"),
            "S": ("nombre", "opacidad",),
            "X": ("nombre", "opacidad")
        }
        dicDB = {"F": self.dbFlechas, "M": self.dbMarcos, "S": self.dbSVGs, "X": self.dbMarkers}
        for k, sc in self.tablero.dicMovibles.iteritems():
            bd = sc.bloqueDatos
            try:
                tp, xid = bd.tpid
                bdn = dicDB[tp][xid]
                for campo in dicCampos[tp]:
                    setattr(bd, campo, getattr(bdn, campo))
                sc.update()
            except:
                pass
        self.g_guion.refresh()

    def muevePieza(self, desde, hasta):
        self.tablero.muevePieza(desde, hasta)
        self.creaTarea("P", None, desde + hasta, -1)

    def movimientosPiezasTV(self, liMovs, tablero):
        """
        Hace los movimientos de piezas en el tablero
        """
        if VarGen.configuracion.efectosVisuales:

            cpu = CPU.CPU(tablero.pantalla)
            segundos = None

            # primero los movimientos
            for movim in liMovs:
                if movim[0] == "m":
                    if segundos is None:
                        desde, hasta = movim[1], movim[2]
                        dc = ord(desde[0]) - ord(hasta[0])
                        df = int(desde[1]) - int(hasta[1])
                        # Maxima distancia = 9.9 ( 9,89... sqrt(7**2+7**2)) = 4 segundos
                        dist = (dc ** 2 + df ** 2) ** 0.5
                        segundos = 4.0 * dist / 9.9

                    cpu.muevePieza(movim[1], movim[2], siExclusiva=False, segundos=segundos)

            if segundos is None:
                segundos = 1.0

            # segundo los borrados
            for movim in liMovs:
                if movim[0] == "b":
                    n = cpu.duerme(segundos * 0.90)
                    cpu.borraPieza(movim[1], padre=n)

            # tercero los cambios
            for movim in liMovs:
                if movim[0] == "c":
                    cpu.cambiaPieza(movim[1], movim[2], siExclusiva=True)

            cpu.runLineal()

        else:
            for movim in liMovs:
                if movim[0] == "b":
                    tablero.borraPieza(movim[1])
                elif movim[0] == "m":
                    tablero.muevePieza(movim[1], movim[2])
                elif movim[0] == "c":
                    tablero.cambiaPieza(movim[1], movim[2])

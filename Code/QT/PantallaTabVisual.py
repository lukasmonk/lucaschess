import os

from PyQt4 import QtGui

import Code.VarGen as VarGen
import Code.Util as Util
import Code.CPU as CPU
import Code.ControlPosicion as ControlPosicion
import Code.TabVisual as TabVisual
import Code.QT.QTUtil as QTUtil
import Code.QT.QTUtil2 as QTUtil2
import Code.QT.Colocacion as Colocacion
import Code.QT.Iconos as Iconos
import Code.QT.Controles as Controles
import Code.QT.Tablero as Tablero
import Code.QT.QTVarios as QTVarios
import Code.QT.PantallaTabVFlechas as PantallaTabVFlechas
import Code.QT.PantallaTabVMarcos as PantallaTabVMarcos
import Code.QT.PantallaTabVSVGs as PantallaTabVSVGs
import Code.QT.PantallaTabVPartidas as PantallaTabVPartidas
import Code.QT.PantallaSonido as PantallaSonido
import Code.QT.PantallaPGN as PantallaPGN
import Code.QT.Columnas as Columnas
import Code.QT.Grid as Grid
import Code.QT.Delegados as Delegados
import Code.QT.FormLayout as FormLayout
import Code.QT.WinPosition as WinPosition

class WTabVisual(QTVarios.WDialogo):
    def __init__(self, tableroOwner):

        self.tableroOwner = tableroOwner
        titulo = _("Board -> Image")
        icono = Iconos.Camara()
        extparam = "tabvisual"
        QTVarios.WDialogo.__init__(self, tableroOwner, titulo, icono, extparam)

        liAcciones = [( _("Quit"), Iconos.MainMenu(), "terminar" ), None,
                      ( _("Clipboard"), Iconos.Clip(), "portapapeles" ), None,
                      ( _("Save") + " png", Iconos.GrabarFichero(), "grabarFichero" ), None,
                      ( _("Arrows"), Iconos.Flechas(), "flechas" ), None,
                      ( _("Boxes"), Iconos.Marcos(), "marcos" ), None,
                      ( _("Images"), Iconos.SVGs(), "svgs" ), None,
                      # ( _( "Sounds" ), Iconos.S_LeerWav(), "sonidos" ), None,
        ]
        tb = Controles.TB(self, liAcciones)

        # Tablero
        idTab = "TABVISUAL"
        confTablero = VarGen.configuracion.confTablero(idTab, 24, padre=tableroOwner.confTablero.id())
        self.tablero = Tablero.TableroVisual(self, confTablero)
        self.tablero.crea()
        self.tablero.ponDispatchEventos(self.dispatch)
        self.tablero.dispatchSize(self.tableroCambiadoTam)
        self.tablero.baseCasillasSC.setAcceptDrops(True)
        self.tablero.ponMensajero(self.muevePieza)

        self.tablero.copiaPosicionDe(tableroOwner)
        self.fenInicial = self.tablero.fenActual()
        self.tablero.activaTodas()

        self.leeRecursos()
        self.leeGuion("", False)

        # Tools
        self.listaPiezasW = QTVarios.ListaPiezas(self, "P;N;B;R;Q;K", self.tablero, 32)
        self.listaPiezasB = QTVarios.ListaPiezas(self, "p;n;b;r;q;k", self.tablero, 32)

        # Guion
        liAcciones = [( _("New"), Iconos.Nuevo(), "gnuevo" ),
                      ( _("Insert"), Iconos.Insertar(), "ginsertar" ),
                      ( _("Copy"), Iconos.Copiar(), "gcopiar" ), None,
                      ( _("Remove"), Iconos.Borrar(), "gborrar" ), None,
                      ( _("Up"), Iconos.Arriba(), "garriba" ),
                      ( _("Down"), Iconos.Abajo(), "gabajo" ), None,
                      ( _("Mark"), Iconos.Marcar(), "gmarcar" ), None,
                      ( _("Save"), Iconos.Grabar(), "ggrabarGuion" ),
                      ( _("Open"), Iconos.Recuperar(), "grecuperarGuion" ), None,
                      ( _("Remove script"), Iconos.Delete(), "geliminarGuion" )
        ]
        tbGuion = Controles.TB(self, liAcciones, siTexto=False, tamIcon=20)
        oColumnas = Columnas.ListaColumnas()
        oColumnas.nueva("MARCADO", "", 20, siCentrado=True, siChecked=True)
        oColumnas.nueva("TIPO", _("Type"), 50, siCentrado=True)
        oColumnas.nueva("NOMBRE", _("Name"), 100, siCentrado=True, edicion=Delegados.LineaTextoUTF8())
        oColumnas.nueva("INFO", _("Information"), 100, siCentrado=True)
        self.g_guion = Grid.Grid(self, oColumnas, siCabeceraMovible=False, siEditable=True, siSeleccionMultiple=True)
        self.g_guion.gotop()

        self.registrarGrid(self.g_guion)

        # Visuales
        self.dragBandaArriba = QTVarios.DragBanda(self, (10,), 40)

        # Guion
        lyGuion = Colocacion.V().control(tbGuion).control(self.g_guion)

        # Zona tablero
        lyT = Colocacion.H()
        lyT.controli(self.listaPiezasW).controli(self.tablero).controli(self.listaPiezasB).otro(lyGuion)

        lyTB = Colocacion.V()
        lyTB.controli(self.dragBandaArriba)
        lyTB.otro(lyT)

        # Layout
        layout = Colocacion.V().control(tb).otro(lyTB).margen(3)
        self.setLayout(layout)

        self.recuperarVideo()

        self.actualizaBandas()
        li = self.dbConfig["DRAGBANDAARRIBA"]
        if li:
            self.dragBandaArriba.recuperar(li)

        self.ultDesde = "d4"
        self.ultHasta = "e5"

        self.g_guion.gotop()
        self.g_guion.setFocus()

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

    def pulsadoItemSC(self, dato):
        self.g_guion.refresh()

    def dispatch(self, evento, envio):
        t = self.tablero
        if evento == t.EVENTO_DERECHO:
            item = envio
            item.hide()
            self.guion.desmarcaItem(item)
            self.g_guion.refresh()
        elif evento == t.EVENTO_DROP:
            desde, dato = envio
            self.dispatchDrop(desde, dato)
        elif evento == t.EVENTO_DERECHO_PIEZA:
            ttpieza, desde = envio
            self.creaTarea("B", ttpieza.pieza, desde, -1)

    def dispatchDrop(self, desde, dato):

        if dato.isalpha():
            pieza = dato
            self.tablero.cambiaPieza(desde, pieza)
            self.tableroOwner.cambiaPieza(desde, pieza)
            self.tablero.activaTodas()
            tp = "C"
            id = pieza
            a1h8 = desde

        elif dato.startswith("_"):
            li = dato.split("_")
            tp = li[1]
            id = int(li[2])

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

        self.creaTarea(tp, id, a1h8, -1)

    def creaTareaBase(self, tp, id, a1h8, fila):

        tpid = tp, id
        if tp == "P":
            tarea = TabVisual.GT_PiezaMueve()
            tarea.desdeHasta(a1h8[:2], a1h8[2:])
        elif tp == "C":
            tarea = TabVisual.GT_PiezaCrea()
            tarea.desde(a1h8)
            tarea.pieza(id)
        elif tp == "B":
            tarea = TabVisual.GT_PiezaBorra()
            tarea.desde(a1h8)
            tarea.pieza(id)
        else:
            if tp == "F":
                regFlecha = self.dbFlechas[id]
                if not regFlecha:
                    return None, None
                regFlecha.tpid = tpid
                regFlecha.a1h8 = a1h8
                sc = self.tablero.creaFlecha(regFlecha)

                tarea = TabVisual.GT_Flecha()
            elif tp == "M":
                regMarco = self.dbMarcos[id]
                if regMarco is None:
                    return None, None
                regMarco.tpid = tpid
                regMarco.a1h8 = a1h8
                sc = self.tablero.creaMarco(regMarco)
                tarea = TabVisual.GT_Marco()
            elif tp == "S":
                regSVG = self.dbSVGs[id]
                if regSVG is None:
                    return None, None
                regSVG.tpid = tpid
                regSVG.a1h8 = a1h8
                sc = self.tablero.creaSVG(regSVG, siEditando=True)
                tarea = TabVisual.GT_SVG()
            else:
                return None, 0

            sc.ponRutinaPulsada(self.pulsadoItemSC, tpid)

            tarea.itemSC(sc)
            # self.tablero.registraMovible( sc )
        tarea.marcado(True)
        tarea.registro((tp, id, a1h8))
        fila = self.guion.nuevaTarea(tarea, fila)
        return tarea, fila

    def creaTarea(self, tp, id, a1h8, fila):
        tarea, fila = self.creaTareaBase(tp, id, a1h8, fila)
        if tarea is None:
            return
        tarea.registro((tp, id, a1h8))
        self.g_guion.goto(fila, 0)

        if tp in "PB":
            self.gridPonValor(self.g_guion, fila, None, True)

        self.g_guion.refresh()

    def editaNombre(self, nombre):
        liGen = [(None, None)]
        config = FormLayout.Editbox(_("Name"), ancho=160)
        liGen.append((config, nombre ))
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
            fen = WinPosition.editarPosicion(self, VarGen.configuracion, fen)
            if fen is None:
                return None

            nombre = self.editaNombre(_("Start position"))
            if nombre is None:
                return

            tarea = TabVisual.GT_Posicion()
            if not fen:
                cp = ControlPosicion.FEN_INICIAL
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
                ( ("_P_0", mp), mp, Iconos.PuntoAzul() ),
            ]
            masMenu.extend([
                ( ("SP", "PI"), _("Start position"), Iconos.Datos() ),
                ( ("SP", "PA"), _("Current position"), Iconos.Copiar() ),
                ( ("SP", "PP"), _("Paste FEN position"), Iconos.Pegar16() ),
                ( ("SP", "PGNF"), _("Read PGN"), Iconos.PGN_Importar() ),
                ( ("SP", "PGNP"), _("Paste PGN"), Iconos.Pegar16() ),
            ])
        else:
            masMenu = []

        resp = self.dragBandaArriba.menuParaExterior(masMenu)

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
        liGen.append(( config, desde ))

        config = FormLayout.Casillabox(_("To square"))
        liGen.append(( config, hasta ))

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
            id = int(li[2])
            desde, hasta = self.desdeHasta(tit, self.ultDesde, self.ultHasta)
            if desde:
                fila = self.g_guion.recno() if siInsertar else -1
                self.creaTarea(tp, id, desde + hasta, fila)

    # #--------------------------------------------------------------------------------------------------------------------------------
    def gnuevo(self):
        self.gmas(False)

    def ginsertar(self):
        self.gmas(True)

    def gcopiar(self):
        fila = self.g_guion.recno()
        sc = self.guion.itemTarea(fila)
        if sc:
            bd = sc.bloqueDatos
            tp, id = bd.tpid
            a1h8 = bd.a1h8
            self.creaTarea(tp, id, a1h8, fila + 1)

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

    def gridDobleClick(self, grid, fil, col):
        clave = col.clave
        if clave == "INFO":
            fila = self.g_guion.recno()
            tarea = self.guion.tarea(fila)
            sc = self.guion.itemTarea(fila)
            if sc:
                a1h8 = tarea.a1h8()
                desde, hasta = self.desdeHasta(tarea.txt_tipo() + " " + tarea.nombre(), a1h8[:2], a1h8[2:])
                if desde:
                    sc = tarea.itemSC()
                    sc.ponA1H8(desde + hasta)

            elif isinstance(tarea, TabVisual.GT_Posicion):
                fen = WinPosition.editarPosicion(self, VarGen.configuracion, tarea.fen())
                if fen is not None:
                    tarea.fen(fen)

            elif isinstance(tarea, TabVisual.GT_PiezaMueve):
                desde, hasta = tarea.desdeHasta()
                desde, hasta = self.desdeHasta(tarea.txt_tipo() + " " + tarea.nombre(), desde, hasta)
                if desde:
                    tarea.desdeHasta(desde, hasta)
                    tarea.marcado(False)

    def ggrabarGuion(self):
        liGen = [(None, None)]

        config = FormLayout.Editbox(_("Name"), ancho=160)
        liGen.append((config, self.nomGuion ))

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
            self.tablero.crea()
            for reg in li:
                if reg._registro:
                    tp, id, a1h8 = reg._registro
                    tarea, fila = self.creaTareaBase(tp, id, a1h8, -1)
                    if tarea is None:
                        continue

                    tarea.recupera(reg)
                    if hasattr(reg, "_bloqueDatos"):
                        sc = tarea.itemSC()
                        sc.bloqueDatos = reg._bloqueDatos
                        sc.update()
                else:
                    tarea = self.guion.recuperaReg(reg)
                    tarea._marcado = False
        if siFenInicial:
            cp = ControlPosicion.ControlPosicion()
            cp.leeFen(self.fenInicial)
            self.tablero.ponPosicion(cp)
        self.tablero.activaTodas()
        self.nomGuion = nomGuion

    def gridNumDatos(self, grid):
        return len(self.guion)

    def ponMarcado(self, fila, siMarcado):
        self.guion.cambiaMarcaTarea(fila, siMarcado)
        itemSC = self.guion.itemTarea(fila)
        if itemSC:
            itemSC.setVisible(siMarcado)

        else:
            tarea = self.guion.tarea(fila)
            if isinstance(tarea, TabVisual.GT_PiezaMueve):
                desde, hasta = tarea.desdeHasta()
                if not siMarcado:
                    desde, hasta = hasta, desde
                self.tablero.muevePieza(desde, hasta)

            elif isinstance(tarea, TabVisual.GT_Posicion):
                if siMarcado:
                    fen = tarea.fen()
                    tarea.fenAnterior(self.tablero.fenActual())
                else:
                    fen = tarea.fenAnterior()
                    if not fen:
                        return
                cp = ControlPosicion.ControlPosicion()
                cp.leeFen(fen)
                self.tablero.ponPosicion(cp)

            elif isinstance(tarea, TabVisual.GT_Jugada):
                jg = tarea.jugada()
                self.tablero.ponPosicion(jg.posicionBase)
                if siMarcado:
                    if VarGen.configuracion.siSuenaJugada:
                        VarGen.runSound.playLista(jg.listaSonidos())
                    self.movimientosPiezasTV(jg.liMovs)
                    self.tablero.ponFlechaSC(jg.desde, jg.hasta)

            elif isinstance(tarea, TabVisual.GT_PiezaCrea):
                if siMarcado:
                    self.tablero.cambiaPieza(tarea.desde(), tarea.pieza())
                else:
                    self.tablero.borraPieza(tarea.desde())

            elif isinstance(tarea, TabVisual.GT_PiezaBorra):
                if siMarcado:
                    self.tablero.borraPieza(tarea.desde())
                else:
                    self.tablero.cambiaPieza(tarea.desde(), tarea.pieza())

    def gridPonValor(self, grid, fila, oColumna, valor):
        clave = oColumna.clave if oColumna else "MARCADO"
        if clave == "MARCADO":
            self.ponMarcado(fila, valor > 0)
        elif clave == "NOMBRE":
            tarea = self.guion.tarea(fila)
            tarea.nombre(valor.strip())

    def editarBanda(self, cid):
        li = cid.split("_")
        tp = li[1]
        id = int(li[2])
        ok = False
        if tp == "F":
            regFlecha = self.dbFlechas[id]
            w = PantallaTabVFlechas.WTV_Flecha(self, regFlecha, True)
            if w.exec_():
                self.dbFlechas[id] = w.regFlecha
                ok = True
        elif tp == "M":
            regMarco = self.dbMarcos[id]
            w = PantallaTabVMarcos.WTV_Marco(self, regMarco)
            if w.exec_():
                self.dbMarcos[id] = w.regMarco
                ok = True
        elif tp == "S":
            regSVG = self.dbSVGs[id]
            w = PantallaTabVSVGs.WTV_SVG(self, regSVG)
            if w.exec_():
                self.dbSVGs[id] = w.regSVG
                ok = True

        if ok:
            self.actualizaBandas()

    def guardarTablero(self):
        self.dbConfig["CONFTABLERO"] = self.tablero.confTablero
        self.dbConfig["DRAGBANDAARRIBA"] = self.dragBandaArriba.guardar()

    def cierraRecursos(self):
        self.dbGuiones[""] = self.guion.guarda()

        self.dbConfig.close()
        self.dbFlechas.close()
        self.dbMarcos.close()
        self.dbSVGs.close()
        self.dbSonidos.close()

        self.dbGuiones.close()

        self.guardarVideo()

    def closeEvent(self, event):
        self.cierraRecursos()

    def terminar(self):
        self.guardarTablero()  # Solo guarda con Quit, con la x no, asi hay mas control
        # self.cierraRecursos()
        self.close()

    def portapapeles(self):
        self.tablero.salvaEnImagen()
        txt = _("Clipboard")
        QTUtil2.mensajeTemporal(self, _X(_("Saved to %1"), txt), 0.8)

    def grabarFichero(self):
        dirSalvados = VarGen.configuracion.dirSalvados
        resp = QTUtil2.salvaFichero(self, _("File to save"), dirSalvados, _("File") + " PNG (*.png)", False)
        if resp:
            self.tablero.salvaEnImagen(resp, "png")
            txt = resp
            QTUtil2.mensajeTemporal(self, _X(_("Saved to %1"), txt), 0.8)
            direc = os.path.dirname(resp)
            if direc != dirSalvados:
                VarGen.configuracion.dirSalvados = direc
                VarGen.configuracion.graba()

    def sonidos(self):
        w = PantallaSonido.WSonidosGuion(self, self.dbSonidos)
        w.exec_()

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

    def listaSonidos(self):
        dic = self.dbSonidos.asDictionary()
        li = [regSonido for k, regSonido in dic.iteritems()]
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

    def leeRecursos(self):
        fdb = VarGen.configuracion.ficheroRecursos
        self.dbConfig = Util.DicSQL(fdb, tabla="Config")
        self.dbFlechas = Util.DicSQL(fdb, tabla="Flechas")
        self.dbMarcos = Util.DicSQL(fdb, tabla="Marcos")
        self.dbSVGs = Util.DicSQL(fdb, tabla="SVGs")
        self.dbSonidos = Util.DicSQL(fdb, tabla="Sonidos")
        self.dbGuiones = Util.DicSQL(fdb, tabla="Guiones")

    def procesarTB(self):
        accion = self.sender().clave
        eval("self.%s()" % accion)

    def actualizaBandas(self):
        self.dragBandaArriba.iniActualizacion()

        tipo = _("Arrows")
        for flecha in self.listaFlechas():
            pm = QtGui.QPixmap()
            pm.loadFromData(flecha.png, "PNG")
            id = "_F_%d" % flecha.id
            nombre = flecha.nombre
            self.dragBandaArriba.actualiza(id, nombre, pm, tipo)

        tipo = _("Boxes")
        for marco in self.listaMarcos():
            pm = QtGui.QPixmap()
            pm.loadFromData(marco.png, "PNG")
            id = "_M_%d" % marco.id
            nombre = marco.nombre
            self.dragBandaArriba.actualiza(id, nombre, pm, tipo)

        tipo = _("Images")
        for svg in self.listaSVGs():
            pm = QtGui.QPixmap()
            pm.loadFromData(svg.png, "PNG")
            id = "_S_%d" % svg.id
            nombre = svg.nombre
            self.dragBandaArriba.actualiza(id, nombre, pm, tipo)

        self.dragBandaArriba.finActualizacion()

        dicCampos = {
            "F": ("nombre", "altocabeza", "tipo", "destino", "color", "colorinterior", "colorinterior2", "opacidad",
                  "redondeos", "forma", "ancho", "vuelo", "descuelgue"),
            "M": ( "nombre", "color", "colorinterior", "colorinterior2", "grosor", "redEsquina", "tipo", "opacidad" ),
            "S": ( "nombre", "opacidad", )
        }
        dicDB = {"F": self.dbFlechas, "M": self.dbMarcos, "S": self.dbSVGs}
        for k, sc in self.tablero.dicMovibles.iteritems():
            bd = sc.bloqueDatos
            try:
                tp, id = bd.tpid
                bdn = dicDB[tp][id]
                for campo in dicCampos[tp]:
                    setattr(bd, campo, getattr(bdn, campo))
                sc.update()
            except:
                pass
        self.g_guion.refresh()

    def muevePieza(self, desde, hasta):
        self.tablero.muevePieza(desde, hasta)
        self.creaTarea("P", None, desde + hasta, -1)

    def movimientosPiezasTV(self, liMovs):
        """
        Hace los movimientos de piezas en el tablero
        """
        if VarGen.configuracion.efectosVisuales:

            cpu = CPU.CPU(self)
            segundos = None

            # primero los movimientos
            for movim in liMovs:
                if movim[0] == "m":
                    if segundos is None:
                        desde, hasta = movim[1], movim[2]
                        dc = ord(desde[0]) - ord(hasta[0])
                        df = int(desde[1]) - int(hasta[1])
                        # Maxima distancia = 9.9 ( 9,89... sqrt(7**2+7**2)) = 4 segundos
                        dist = ( dc ** 2 + df ** 2 ) ** 0.5
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
                    self.tablero.borraPieza(movim[1])
                elif movim[0] == "m":
                    self.tablero.muevePieza(movim[1], movim[2])
                elif movim[0] == "c":
                    self.tablero.cambiaPieza(movim[1], movim[2])


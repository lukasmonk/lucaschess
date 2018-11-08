import os

from PyQt4 import QtGui, QtCore

from Code import Util
from Code import TrListas
from Code import TabVisual
from Code.QT import Colocacion
from Code.QT import Columnas
from Code.QT import Controles
from Code.QT import Delegados
from Code.QT import FormLayout
from Code.QT import Grid
from Code.QT import Iconos
from Code.QT import PantallaTab
from Code.QT import PantallaTabVFlechas
from Code.QT import PantallaTabVMarcos
from Code.QT import PantallaTabVMarkers
from Code.QT import PantallaTabVSVGs
from Code.QT import QTUtil
from Code.QT import QTUtil2
from Code.QT import QTVarios


class WPanelDirector(QTVarios.WDialogo):
    def __init__(self, owner, tablero):
        self.owner = owner
        self.posicion = tablero.ultPosicion
        self.tablero = tablero
        self.configuracion = tablero.configuracion
        self.fenM2 = self.posicion.fenM2()
        self.origin_new = None

        self.dbGestor = tablero.dbVisual
        self.leeRecursos()

        titulo = _("Director")
        icono = Iconos.Script()
        extparam = "tabvisualscript"
        QTVarios.WDialogo.__init__(self, tablero, titulo, icono, extparam)

        self.siGrabar = False
        self.ant_foto = None

        self.guion = TabVisual.Guion(tablero, self)

        # Guion
        liAcciones = [
            (_("Close"), Iconos.MainMenu(), self.terminar),
            (_("Cancel"), Iconos.Cancelar(), self.cancelar),
            (_("Save"), Iconos.Grabar(), self.grabar),
            (_("New"), Iconos.Nuevo(), self.gnuevo),
            (_("Insert"), Iconos.Insertar(), self.ginsertar),
            (_("Remove"), Iconos.Borrar(), self.gborrar), None,
            (_("Up"), Iconos.Arriba(), self.garriba),
            (_("Down"), Iconos.Abajo(), self.gabajo), None,
            (_("Mark"), Iconos.Marcar(), self.gmarcar), None,
            (_("File"), Iconos.Recuperar(), self.gfile), None
        ]
        self.tb = Controles.TBrutina(self, liAcciones, siTexto=False, tamIcon=24)
        self.tb.setAccionVisible(self.grabar, False)

        oColumnas = Columnas.ListaColumnas()
        oColumnas.nueva("NUMERO", _("N."), 20, siCentrado=True)
        oColumnas.nueva("MARCADO", "", 20, siCentrado=True, siChecked=True)
        oColumnas.nueva("TIPO", _("Type"), 50, siCentrado=True)
        oColumnas.nueva("NOMBRE", _("Name"), 100, siCentrado=True, edicion=Delegados.LineaTextoUTF8())
        oColumnas.nueva("INFO", _("Information"), 100, siCentrado=True)
        self.g_guion = Grid.Grid(self, oColumnas, siCabeceraMovible=False, siEditable=True, siSeleccionMultiple=True)
        self.g_guion.fixMinWidth()

        self.registrarGrid(self.g_guion)

        self.chbSaveWhenFinished = Controles.CHB(self, _("Save when finished"), self.dbConfig.get("SAVEWHENFINISHED", False))

        # Visuales
        self.selectBanda = PantallaTab.SelectBanda(self)

        lyG = Colocacion.V().control(self.g_guion).control(self.chbSaveWhenFinished)
        lySG = Colocacion.H().control(self.selectBanda).otro(lyG).relleno(1)
        layout = Colocacion.V().control(self.tb).otro(lySG).margen(3)

        self.setLayout(layout)

        self.recuperarVideo()

        self.recuperar()
        self.ant_foto = self.foto()

        self.actualizaBandas()
        li = self.dbConfig["SELECTBANDA"]
        if li:
            self.selectBanda.recuperar(li)
        num_lb = self.dbConfig["SELECTBANDANUM"]
        if num_lb is not None:
            self.selectBanda.seleccionarNum(num_lb)

        self.ultDesde = "d4"
        self.ultHasta = "e5"

        self.g_guion.gotop()

    def addText(self):
        self.guion.cierraPizarra()
        tarea = TabVisual.GT_Texto(self.guion)
        fila = self.guion.nuevaTarea(tarea, -1)
        self.ponMarcado(fila, True)
        self.ponSiGrabar()
        self.guion.pizarra.show()
        self.guion.pizarra.mensaje.setFocus()

    def cambiadaPosicion(self):
        self.posicion = self.tablero.ultPosicion
        self.fenM2 = self.posicion.fenM2()
        self.origin_new = None
        self.recuperar()

    def seleccionar(self, lb):
        if lb is None:
            self.owner.setChange(True)
            self.tablero.activaTodas()
        else:
            self.owner.setChange(False)
            self.tablero.desactivaTodas()

    def funcion(self, numero, siCtrl=False):
        if numero == 9:
            if siCtrl:
                self.selectBanda.seleccionar(None)
            else:
                self.addText()
        elif numero == 0 and siCtrl:  # Ctrl+F1
            self.borraUltimo()
        elif numero == 1 and siCtrl:  # Ctrl+F2
            self.borraTodos()
        else:
            self.selectBanda.seleccionarNum(numero)

    def grabar(self):
        li = self.guion.guarda()
        self.tablero.dbVisual_save(self.fenM2, li)

        self.siGrabar = False
        self.tb.setAccionVisible(self.grabar, False)
        self.tb.setAccionVisible(self.cancelar, False)
        self.tb.setAccionVisible(self.terminar, True)

    def ponSiGrabar(self):
        if not self.siGrabar:
            self.tb.setAccionVisible(self.grabar, True)
            self.tb.setAccionVisible(self.cancelar, True)
            self.tb.setAccionVisible(self.terminar, False)
            self.siGrabar = True

    def ponNoGrabar(self):
        self.tb.setAccionVisible(self.grabar, False)
        self.tb.setAccionVisible(self.cancelar, False)
        self.tb.setAccionVisible(self.terminar, True)
        self.siGrabar = False

    def recuperar(self):
        self.guion.recupera()
        self.ponNoGrabar()
        self.ant_foto = self.foto()
        self.refresh_guion()

    def tableroCambiadoTam(self):
        self.layout().setSizeConstraint(QtGui.QLayout.SetFixedSize)
        self.show()
        QTUtil.refreshGUI()
        self.layout().setSizeConstraint(QtGui.QLayout.SetMinAndMaxSize)

    def gridDato(self, grid, fila, oColumna):
        clave = oColumna.clave
        if clave == "NUMERO":
            return "%d" % (fila + 1,)
        if clave == "MARCADO":
            return self.guion.marcado(fila)
        elif clave == "TIPO":
            return self.guion.txt_tipo(fila)
        elif clave == "NOMBRE":
            return self.guion.nombre(fila)
        elif clave == "INFO":
            return self.guion.info(fila)

    def creaTareaBase(self, tp, xid, a1h8, fila):
        # if tp != TabVisual.TP_FLECHA: # Se indica al terminar en porque puede que no se grabe
            # self.ponSiGrabar()
        tpid = tp, xid
        if tp == "P":
            tarea = TabVisual.GT_PiezaMueve(self.guion)
            desde, hasta = a1h8[:2], a1h8[2:]
            borra = self.tablero.dameNomPiezaEn(hasta)
            tarea.desdeHastaBorra(desde, hasta, borra)
            self.tablero.activaTodas()
        elif tp == "C":
            tarea = TabVisual.GT_PiezaCrea(self.guion)
            borra = self.tablero.dameNomPiezaEn(a1h8)
            tarea.desde(a1h8, borra)
            tarea.pieza(xid)
            self.tablero.activaTodas()
        elif tp == "B":
            tarea = TabVisual.GT_PiezaBorra(self.guion)
            tarea.desde(a1h8)
            tarea.pieza(xid)
        else:
            if tp == TabVisual.TP_FLECHA:
                regFlecha = self.dbFlechas[xid]
                if regFlecha is None:
                    return None, None
                regFlecha.tpid = tpid
                regFlecha.a1h8 = a1h8
                sc = self.tablero.creaFlecha(regFlecha)
                tarea = TabVisual.GT_Flecha(self.guion)
            elif tp == TabVisual.TP_MARCO:
                regMarco = self.dbMarcos[xid]
                if regMarco is None:
                    return None, None
                regMarco.tpid = tpid
                regMarco.a1h8 = a1h8
                sc = self.tablero.creaMarco(regMarco)
                tarea = TabVisual.GT_Marco(self.guion)
            elif tp == TabVisual.TP_SVG:
                regSVG = self.dbSVGs[xid]
                if regSVG is None:
                    return None, None
                regSVG.tpid = tpid
                regSVG.a1h8 = a1h8
                sc = self.tablero.creaSVG(regSVG, siEditando=True)
                tarea = TabVisual.GT_SVG(self.guion)
            elif tp == TabVisual.TP_MARKER:
                regMarker = self.dbMarkers[xid]
                if regMarker is None:
                    return None, None
                regMarker.tpid = tpid
                regMarker.a1h8 = a1h8
                sc = self.tablero.creaMarker(regMarker, siEditando=True)
                tarea = TabVisual.GT_Marker(self.guion)
            sc.ponRutinaPulsada(None, tarea.id())
            tarea.itemSC(sc)

        tarea.marcado(True)
        tarea.registro((tp, xid, a1h8))
        fila = self.guion.nuevaTarea(tarea, fila)

        return tarea, fila

    def creaTarea(self, tp, xid, a1h8, fila):

        tarea, fila = self.creaTareaBase(tp, xid, a1h8, fila)
        if tarea is None:
            return None, None
        tarea.registro((tp, xid, a1h8))

        self.g_guion.goto(fila, 0)

        self.ponMarcado(fila, True)

        return tarea, fila

    def editaNombre(self, nombre):
        liGen = [(None, None)]
        config = FormLayout.Editbox(_("Name"), ancho=160)
        liGen.append((config, nombre))
        ico = Iconos.Grabar()

        resultado = FormLayout.fedit(liGen, title=_("Name"), parent=self, icon=ico)
        if resultado:
            self.ponSiGrabar()
            accion, liResp = resultado
            nombre = liResp[0]
            return nombre
        return None

    def borrarPizarraActiva(self):
        for n in range(len(self.guion)):
            tarea = self.guion.tarea(n)
            if tarea.tp() == TabVisual.TP_TEXTO:
                if tarea.marcado():
                    self.gborrar([n,])

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
                    tarea = self.guion.tarea(n)
                    if tarea.tp() in (TabVisual.TP_TEXTO, TabVisual.TP_ACTION, TabVisual.TP_CONFIGURATION):
                        continue
                    siMarcado = tarea.marcado()
                    if siTodos:
                        if not siMarcado:
                            self.gridPonValor(None, n, None, True)
                    else:
                        if siMarcado:
                            self.gridPonValor(None, n, None, False)
                self.refresh_guion()

    def desdeHasta(self, titulo, desde, hasta):
        liGen = [(None, None)]

        config = FormLayout.Casillabox(_("From square"))
        liGen.append((config, desde))

        config = FormLayout.Casillabox(_("To square"))
        liGen.append((config, hasta))

        resultado = FormLayout.fedit(liGen, title=titulo, parent=self)
        if resultado:
            self.ponSiGrabar()
            resp = resultado[1]
            self.ultDesde = desde = resp[0]
            self.ultHasta = hasta = resp[1]
            return desde, hasta
        else:
            return None, None

    def datosSVG(self, tarea):
        col, fil, ancho, alto = tarea.get_datos()
        liGen = [(None, None)]

        def xconfig(label, value):
            config = FormLayout.Editbox(label, 80, tipo=float, decimales=3)
            liGen.append((config, value))

        xconfig(_("Column"), col)
        xconfig(_("Row"), fil)
        xconfig(_("Width"), ancho)
        xconfig(_("Height"), alto)

        resultado = FormLayout.fedit(liGen, title=tarea.txt_tipo(), parent=self)
        if resultado:
            col, fil, ancho, alto = resultado[1]
            tarea.set_datos(col, fil, ancho, alto)
            self.ponSiGrabar()
            return True
        else:
            return False

    def gfile(self):
        self.test_siGrabar()
        path = self.configuracion.ficheroFEN
        fich = QTUtil2.leeCreaFichero(self, path, "dbl")
        if fich:
            self.configuracion.ficheroFEN = Util.dirRelativo(fich)
            self.configuracion.graba()

            self.tablero.dbVisual_close()

            # self.tablero.borraMovibles()
            self.guion.cierraPizarra()
            self.recuperar()

    def gmas(self, siInsertar):
        ta = TabVisual.GT_Action(None)
        liActions = [(_F(txt), Iconos.PuntoRojo(), "GTA_%s" % action) for action, txt in ta.dicTxt.iteritems()]

        # tc = TabVisual.GT_Configuration(None)
        # liConfigurations = [(txt, Iconos.PuntoVerde(), "GTC_%s" % configuration) for configuration, txt in tc.dicTxt.iteritems()]

        liMore = [
            (_("Text"), Iconos.Texto(), TabVisual.TP_TEXTO),
            (_("Actions"), Iconos.Run(), liActions),
            # (_("Configuration"), Iconos.Configurar(), liConfigurations),
        ]
        resp = self.selectBanda.menuParaExterior(liMore)
        if resp:
            xid = resp
            fila = self.g_guion.recno() if siInsertar else -1
            if xid == TabVisual.TP_TEXTO:
                tarea = TabVisual.GT_Texto(self.guion)
                fila = self.guion.nuevaTarea(tarea, fila)
                self.ponMarcado(fila, True)
                self.ponSiGrabar()
            elif resp.startswith("GTA_"):
                self.creaAction(resp[4:], fila)
            # elif resp.startswith("GTC_"):
            #     key = resp[4:]
            #     txt = tc.dicTxt[key]
            #     if not self.creaConfiguration(txt, key, fila):
            #         return
            else:
                li = xid.split("_")
                tp = li[1]
                xid = int(li[2])
                desde, hasta = self.desdeHasta(_("Director"), self.ultDesde, self.ultHasta)
                if desde:
                    self.creaTarea(tp, xid, desde + hasta, fila)
            if siInsertar:
                self.g_guion.goto(fila, 0)
            else:
                self.g_guion.gobottom()

    def creaAction(self, action, fila):
        tarea = TabVisual.GT_Action(self.guion)
        tarea.action(action)
        fila = self.guion.nuevaTarea(tarea, fila)
        self.ponSiGrabar()
        self.refresh_guion()

    # def creaConfiguration(self, txt, configuration, fila):
    #     liGen = [(None, None)]
    #     config = FormLayout.Editbox(_("Time in milliseconds"), 80, tipo=int)
    #     liGen.append((config, ""))
    #     ico = Iconos.Configurar()

    #     resultado = FormLayout.fedit(liGen, title=txt, parent=self, icon=ico)
    #     if resultado:
    #         accion, liResp = resultado
    #         value = liResp[0]
    #         tarea = TabVisual.GT_Configuration(self.guion)
    #         tarea.configuration(configuration)
    #         tarea.value(value)
    #         self.guion.nuevaTarea(tarea, fila)
    #         self.ponSiGrabar()
    #         self.refresh_guion()
    #         return True
    #     return False

    def gnuevo(self):
        self.gmas(False)

    def ginsertar(self):
        self.gmas(True)

    def borraUltimo(self):
        fila = len(self.guion) - 1
        if fila >= 0:
            lista = [fila,]
            self.gborrar(lista)

    def borraTodos(self):
        num = len(self.guion)
        if num:
            self.gborrar(range(num))

    def gborrar(self, lista=None):
        li = self.g_guion.recnosSeleccionados() if lista is None else lista
        if li:
            li.sort(reverse=True)
            for fila in li:
                self.ponMarcado(fila, False)
                sc = self.guion.itemTarea(fila)
                if sc:
                    self.tablero.borraMovible(sc)
                else:
                    tarea = self.guion.tarea(fila)
                    if tarea.tp() == TabVisual.TP_TEXTO:
                        self.guion.cierraPizarra()
                self.guion.borra(fila)
            if fila >= len(self.guion):
                fila = len(self.guion) - 1
            self.g_guion.goto(fila, 0)
            self.ponSiGrabar()
            self.refresh_guion()

    def garriba(self):
        fila = self.g_guion.recno()
        if self.guion.arriba(fila):
            self.g_guion.goto(fila - 1, 0)
            self.refresh_guion()
            self.ponSiGrabar()

    def gabajo(self):
        fila = self.g_guion.recno()
        if self.guion.abajo(fila):
            self.g_guion.goto(fila + 1, 0)
            self.refresh_guion()
            self.ponSiGrabar()

    def gridDobleClick(self, grid, fila, col):
        clave = col.clave
        if clave == "INFO":
            tarea = self.guion.tarea(fila)
            sc = self.guion.itemTarea(fila)
            if sc:
                if tarea.tp() == TabVisual.TP_SVG:
                    if self.datosSVG(tarea):
                        self.tablero.refresh()

                else:
                    a1h8 = tarea.a1h8()
                    desde, hasta = self.desdeHasta(tarea.txt_tipo() + " " + tarea.nombre(), a1h8[:2], a1h8[2:])
                    if desde:
                        sc = tarea.itemSC()
                        sc.ponA1H8(desde + hasta)
                        self.tablero.refresh()

            mo = tarea.marcadoOwner()
            if mo:
                self.ponMarcadoOwner(fila, mo)
            self.refresh_guion()

    def keyPressEvent(self, event):
        self.owner.keyPressEvent(event)

    def foto(self):
        gn = self.guion.nombre
        gi = self.guion.info
        gt = self.guion.txt_tipo
        return [(gn(f), gi(f), gt(f)) for f in range(len(self.guion))]

    def refresh_guion(self):
        self.g_guion.refresh()
        if self.siGrabar:
            return
        nueva = self.foto()
        nv = len(nueva)
        if self.ant_foto is None or nv != len(self.ant_foto):
            self.ant_foto = nueva
            self.ponSiGrabar()
        else:
            for n in range(nv):
                if self.ant_foto[n] != nueva[n]:
                    self.ant_foto = nueva
                    self.ponSiGrabar()
                    break

    def gridNumDatos(self, grid):
        return len(self.guion) if self.guion else 0

    def clonaItemTarea(self, fila):
        tarea = self.guion.tarea(fila)
        bloqueDatos = tarea.bloqueDatos()
        tp = tarea.tp()
        if tp == TabVisual.TP_FLECHA:
            sc = self.tablero.creaFlecha(bloqueDatos)
        elif tp == TabVisual.TP_MARCO:
            sc = self.tablero.creaMarco(bloqueDatos)
        elif tp == TabVisual.TP_SVG:
            sc = self.tablero.creaSVG(bloqueDatos)
        elif tp == TabVisual.TP_MARKER:
            sc = self.tablero.creaMarker(bloqueDatos)
        else:
            return None
        return sc

    def ponMarcado(self, fila, siMarcado):
        self.guion.cambiaMarcaTarea(fila, siMarcado)
        itemSC = self.guion.itemTarea(fila)
        self.ponMarcadoItem(fila, self.tablero, itemSC, siMarcado)
        self.refresh_guion()

    def ponMarcadoItem(self, fila, tablero, itemSC, siMarcado):
        if itemSC:
            itemSC.setVisible(siMarcado)

        else:
            tarea = self.guion.tarea(fila)
            if isinstance(tarea, TabVisual.GT_PiezaMueve):
                desde, hasta, borra = tarea.desdeHastaBorra()
                if siMarcado:
                    tablero.muevePieza(desde, hasta)
                    tablero.ponFlechaSC(desde, hasta)
                else:
                    tablero.muevePieza(hasta, desde)
                    if borra:
                        tablero.creaPieza(borra, hasta)
                    if tablero.flechaSC:
                        tablero.flechaSC.hide()
                tablero.activaTodas()

            elif isinstance(tarea, TabVisual.GT_PiezaCrea):
                desde, pz_borrada = tarea.desde()
                if siMarcado:
                    tablero.cambiaPieza(desde, tarea.pieza())
                else:
                    tablero.borraPieza(desde)
                    if pz_borrada:
                         tablero.creaPieza(pz_borrada, desde)
                tablero.activaTodas()

            elif isinstance(tarea, TabVisual.GT_PiezaBorra):
                if siMarcado:
                    tablero.borraPieza(tarea.desde())
                else:
                    tablero.cambiaPieza(tarea.desde(), tarea.pieza())
                tablero.activaTodas()

            elif isinstance(tarea, TabVisual.GT_Texto):
                self.guion.cierraPizarra()
                if siMarcado:
                    self.guion.writePizarra(tarea)
                for recno in range(len(self.guion)):
                    tarea = self.guion.tarea(recno)
                    if tarea.tp() == TabVisual.TP_TEXTO and fila != recno:
                        self.guion.cambiaMarcaTarea(recno, False)

            elif isinstance(tarea, TabVisual.GT_Action):
                if siMarcado:
                    tarea.run()
                    self.guion.cambiaMarcaTarea(fila, False)

    def gridPonValor(self, grid, fila, oColumna, valor):
        clave = oColumna.clave if oColumna else "MARCADO"
        if clave == "MARCADO":
            self.ponMarcado(fila, valor > 0)
        elif clave == "NOMBRE":
            tarea = self.guion.tarea(fila)
            tarea.nombre(valor.strip())
            self.ponSiGrabar()

    def editarBanda(self, cid):
        li = cid.split("_")
        tp = li[1]
        xid = li[2]
        ok = False
        if tp == TabVisual.TP_FLECHA:
            regFlecha = self.dbFlechas[xid]
            w = PantallaTabVFlechas.WTV_Flecha(self, regFlecha, True)
            if w.exec_():
                self.dbFlechas[xid] = w.regFlecha
                ok = True
        elif tp == TabVisual.TP_MARCO:
            regMarco = self.dbMarcos[xid]
            w = PantallaTabVMarcos.WTV_Marco(self, regMarco)
            if w.exec_():
                self.dbMarcos[xid] = w.regMarco
                ok = True
        elif tp == TabVisual.TP_SVG:
            regSVG = self.dbSVGs[xid]
            w = PantallaTabVSVGs.WTV_SVG(self, regSVG)
            if w.exec_():
                self.dbSVGs[xid] = w.regSVG
                ok = True
        elif tp == TabVisual.TP_MARKER:
            regMarker = self.dbMarkers[xid]
            w = PantallaTabVMarkers.WTV_Marker(self, regMarker)
            if w.exec_():
                self.dbMarkers[xid] = w.regMarker
                ok = True

        if ok:
            self.actualizaBandas()
            if len(self.guion):
                self.ponSiGrabar()

    def test_siGrabar(self):
        if self.siGrabar:
            if self.chbSaveWhenFinished.valor():
                self.grabar()
            self.siGrabar = False

    def closeEvent(self, event):
        self.test_siGrabar()
        self.cierraRecursos()

    def terminar(self):
        self.cierraRecursos()
        self.close()

    def cancelar(self):
        self.terminar()

    def portapapeles(self):
        self.tablero.salvaEnImagen()
        txt = _("Clipboard")
        QTUtil2.mensajeTemporal(self, _X(_("Saved to %1"), txt), 0.8)

    def grabarFichero(self):
        dirSalvados = self.configuracion.dirSalvados
        resp = QTUtil2.salvaFichero(self, _("File to save"), dirSalvados, _("File") + " PNG (*.png)", False)
        if resp:
            self.tablero.salvaEnImagen(resp, "png")
            txt = resp
            QTUtil2.mensajeTemporal(self, _X(_("Saved to %1"), txt), 0.8)
            direc = os.path.dirname(resp)
            if direc != dirSalvados:
                self.configuracion.dirSalvados = direc
                self.configuracion.graba()

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
        self.dbConfig = self.dbGestor.dbConfig
        self.dbFlechas = self.dbGestor.dbFlechas
        self.dbMarcos = self.dbGestor.dbMarcos
        self.dbSVGs = self.dbGestor.dbSVGs
        self.dbMarkers = self.dbGestor.dbMarcos

    def cierraRecursos(self):
        if self.guion is not None:
            self.guion.cierraPizarra()
            self.dbConfig["SELECTBANDA"] = self.selectBanda.guardar()
            self.dbConfig["SELECTBANDANUM"] = self.selectBanda.numSeleccionada()
            self.dbConfig["SAVEWHENFINISHED"] = self.chbSaveWhenFinished.valor()
            self.dbGestor.close()

            self.guardarVideo()
            self.guion.restoreTablero()
            self.guion = None

    def actualizaBandas(self):
        self.selectBanda.iniActualizacion()

        tipo = _("Arrows")
        for flecha in self.listaFlechas():
            pm = QtGui.QPixmap()
            pm.loadFromData(flecha.png, "PNG")
            xid = "_F_%d" % flecha.id
            nombre = flecha.nombre
            self.selectBanda.actualiza(xid, nombre, pm, tipo)

        tipo = _("Boxes")
        for marco in self.listaMarcos():
            pm = QtGui.QPixmap()
            pm.loadFromData(marco.png, "PNG")
            xid = "_M_%d" % marco.id
            nombre = marco.nombre
            self.selectBanda.actualiza(xid, nombre, pm, tipo)

        tipo = _("Images")
        for svg in self.listaSVGs():
            pm = QtGui.QPixmap()
            pm.loadFromData(svg.png, "PNG")
            xid = "_S_%d" % svg.id
            nombre = svg.nombre
            self.selectBanda.actualiza(xid, nombre, pm, tipo)

        tipo = _("Markers")
        for marker in self.listaMarkers():
            pm = QtGui.QPixmap()
            pm.loadFromData(marker.png, "PNG")
            xid = "_X_%d" % marker.id
            nombre = marker.nombre
            self.selectBanda.actualiza(xid, nombre, pm, tipo)

        self.selectBanda.finActualizacion()

        dicCampos = {
            TabVisual.TP_FLECHA: ("nombre", "altocabeza", "tipo", "destino", "color", "colorinterior", "colorinterior2", "opacidad",
                  "redondeos", "forma", "ancho", "vuelo", "descuelgue"),
            TabVisual.TP_MARCO: ("nombre", "color", "colorinterior", "colorinterior2", "grosor", "redEsquina", "tipo", "opacidad"),
            TabVisual.TP_SVG: ("nombre", "opacidad",),
            TabVisual.TP_MARKER: ("nombre", "opacidad")
        }
        dicDB = {
                TabVisual.TP_FLECHA: self.dbFlechas,
                TabVisual.TP_MARCO: self.dbMarcos,
                TabVisual.TP_SVG: self.dbSVGs,
                TabVisual.TP_MARKER: self.dbMarkers
        }
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
        self.refresh_guion()

    def muevePieza(self, desde, hasta):
        self.creaTarea("P", None, desde + hasta, -1)
        self.tablero.muevePieza(desde, hasta)

    def tableroPress(self, event, origin, siRight, siShift, siAlt, siCtrl):
        if origin:
            if not siRight:
                lb_sel = self.selectBanda.seleccionada
            else:
                if siCtrl:
                    if siAlt:
                        pos = 4
                    elif siShift:
                        pos = 5
                    else:
                        pos = 3
                else:
                    if siAlt:
                        pos = 1
                    elif siShift:
                        pos = 2
                    else:
                        pos = 0
                lb_sel = self.selectBanda.get_pos(pos)
            if lb_sel:
                nada, tp, nid = lb_sel.id.split("_")
                nid = int(nid)
                if tp == TabVisual.TP_FLECHA:
                    self.siGrabarInicio = self.siGrabar
                self.datos_new = self.creaTarea(tp, nid, origin+origin, -1)
                self.tp_new = tp
                if tp in (TabVisual.TP_FLECHA, TabVisual.TP_MARCO):
                    self.origin_new = origin
                    sc = self.datos_new[0].itemSC()
                    sc.mousePressExt(event)
                else:
                    self.origin_new = None

    def tableroMove(self, event):
        if self.origin_new:
            sc = self.datos_new[0].itemSC()
            sc.mouseMoveExt(event)

    def tableroRelease(self, a1, siRight, siShift, siAlt):
        if self.origin_new:
            tarea, fila = self.datos_new
            sc = tarea.itemSC()
            sc.mouseReleaseExt()
            self.g_guion.goto(fila, 0)
            if siRight:
                if a1 == self.origin_new:
                    if siShift:
                        pos = 8
                    elif siAlt:
                        pos = 7
                    else:
                        pos = 6
                    self.gborrar()
                    lb = self.selectBanda.get_pos(pos)
                    nada, tp, nid = lb.id.split("_")
                    nid = int(nid)
                    self.datos_new = self.creaTarea(tp, nid, a1+a1, -1)
                    self.tp_new = tp
                li = self.guion.borraRepeticionUltima()
                if li:
                    self.gborrar(li)
                    self.origin_new = None
                    return

            else:
                if a1 is None or (a1 == self.origin_new and self.tp_new == TabVisual.TP_FLECHA):
                    self.gborrar()
                    if self.tp_new == TabVisual.TP_FLECHA:
                        if not self.siGrabarInicio:
                            self.ponNoGrabar()

                else:
                    self.ponSiGrabar()
                    self.refresh_guion()

            self.origin_new = None

    def tableroRemove(self, itemSC):
        tarea, n = self.guion.tareaItem(itemSC)
        if tarea:
            self.g_guion.goto(n, 0)
            self.gborrar()


class Director:
    def __init__(self, tablero):
        self.tablero = tablero
        self.ultTareaSelect = None
        self.director = False
        self.directorItemSC = None
        self.w = WPanelDirector(self, tablero)
        self.w.show()
        self.guion = self.w.guion

    def show(self):
        self.w.show()

    def cambiadaPosicionAntes(self):
        self.w.test_siGrabar()

    def cambiadaPosicionDespues(self):
        self.w.cambiadaPosicion()
        self.guion.saveTablero()

    def cambiadoMensajero(self):
        self.w.test_siGrabar()
        self.w.terminar()

    def muevePieza(self, desde, hasta, coronacion=None):
        self.w.creaTarea("P", None, desde+hasta,-1)
        self.tablero.muevePieza(desde, hasta)
        return True

    def setChange(self, ok):
        self.director = ok
        self.ultTareaSelect = None
        self.directorItemSC = None
        # if ok:
        #     self.tablero.activaTodas()
        # else:
        #     self.tablero.desactivaTodas()

    def keyPressEvent(self, event):
        k = event.key()
        if QtCore.Qt.Key_F1 <= k <= QtCore.Qt.Key_F10:
            f = k - QtCore.Qt.Key_F1
            m = int(event.modifiers())
            siCtrl = (m & QtCore.Qt.ControlModifier) > 0
            self.w.funcion(f, siCtrl)
            return True
        else:
            return False

    def mousePressEvent(self, event):
        siRight = event.button() == QtCore.Qt.RightButton
        p = event.pos()
        a1h8 = self.punto2a1h8(p)
        m = int(event.modifiers())
        siCtrl = (m & QtCore.Qt.ControlModifier) > 0
        siShift = (m & QtCore.Qt.ShiftModifier) > 0
        siAlt = (m & QtCore.Qt.AltModifier) > 0

        li_tareas = self.guion.tareasPosicion(p)

        if siRight and siShift and siAlt:
            pz_borrar = self.tablero.dameNomPiezaEn(a1h8)
            menu = Controles.Menu(self.tablero)
            dicPieces = TrListas.dicNomPiezas()
            icoPiece = self.tablero.piezas.icono

            if pz_borrar or len(li_tareas):
                mrem = menu.submenu(_("Remove"), Iconos.Delete())
                if pz_borrar:
                    rotulo = dicPieces[pz_borrar.upper()]
                    mrem.opcion(("rem_pz", None), rotulo, icoPiece(pz_borrar))
                    mrem.separador()
                for pos_guion, tarea in li_tareas:
                    rotulo = "%s - %s - %s" % (tarea.txt_tipo(), tarea.nombre(), tarea.info())
                    mrem.opcion(("rem_gr", pos_guion), rotulo, Iconos.Delete())
                    mrem.separador()
                menu.separador()

            for pz in "KQRBNPkqrbnp":
                if pz != pz_borrar:
                    if pz == "k":
                        menu.separador()
                    menu.opcion(("create", pz), dicPieces[pz.upper()], icoPiece(pz))
            resp = menu.lanza()
            if resp is not None:
                orden, arg = resp
                if orden == "rem_gr":
                    self.w.g_guion.goto(arg, 0)
                    self.w.gborrar()
                elif orden == "rem_pz":
                    self.w.creaTarea("B", pz_borrar, a1h8, -1)

                elif orden == "create":
                    self.w.creaTarea("C", arg, a1h8, -1)
            return True

        if self.director:
            return self.mousePressEvent_Drop(event)

        self.w.tableroPress(event, a1h8, siRight, siShift, siAlt, siCtrl)

        return True

    def mousePressEvent_Drop(self, event):
        p = event.pos()
        li_tareas = self.guion.tareasPosicion(p) # (pos_guion, tarea)...
        nli_tareas = len(li_tareas)
        if nli_tareas > 0:
            if nli_tareas > 1:  # Guerra
                posic = None
                for x in range(nli_tareas):
                    if self.ultTareaSelect == li_tareas[x][1]:
                        posic = x
                        break
                if posic is None:
                    posic = 0
                else:
                    posic += 1
                    if posic >= nli_tareas:
                        posic = 0
            else:
                posic = 0

            tarea_elegida = li_tareas[posic][1]

            if self.ultTareaSelect:
                self.ultTareaSelect.itemSC().activa(False)
            self.ultTareaSelect = tarea_elegida
            itemSC = self.ultTareaSelect.itemSC()
            itemSC.activa(True)
            itemSC.mousePressExt(event)
            self.directorItemSC = itemSC

            return True
        else:
            self.ultTareaSelect = None
            return False

    def punto2a1h8(self, punto):
        xc = 1 + int(float(punto.x() - self.tablero.margenCentro) / self.tablero.anchoCasilla)
        yc = 1 + int(float(punto.y() - self.tablero.margenCentro) / self.tablero.anchoCasilla)

        if self.tablero.siBlancasAbajo:
            yc = 9 - yc
        else:
            xc = 9 - xc

        if not ( (1 <= xc <= 8) and (1 <= yc <= 8)):
            return None

        f = chr(48 + yc)
        c = chr(96 + xc)
        a1h8 = c + f
        return a1h8

    def mouseMoveEvent(self, event):
        if self.director:
            if self.directorItemSC:
                self.directorItemSC.mouseMoveEvent(event)
            return False
        self.w.tableroMove(event)
        return True

    def mouseReleaseEvent(self, event):
        if self.director:
            if self.directorItemSC:
                self.directorItemSC.mouseReleaseExt()
                self.directorItemSC.activa(False)
                self.directorItemSC = None
                self.w.refresh_guion()
                return True
            else:
                return False

        a1h8 = self.punto2a1h8(event.pos())
        if a1h8:
            siRight = event.button() == QtCore.Qt.RightButton
            m = int(event.modifiers())
            siShift = (m & QtCore.Qt.ShiftModifier) > 0
            siAlt = (m & QtCore.Qt.AltModifier) > 0
            self.w.tableroRelease(a1h8, siRight, siShift, siAlt)
        return True

    def terminar(self):
        if self.w:
            self.w.terminar()
            self.w = None

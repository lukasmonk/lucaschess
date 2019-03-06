import collections

import LCEngine4 as LCEngine

from PyQt4 import QtGui, QtCore

from Code import ControlPosicion
from Code import Partida
from Code.QT import Colocacion
from Code.QT import Controles
from Code.QT import FormLayout
from Code.QT import Iconos
from Code.QT import PantallaAnalisisParam
from Code.QT import QTUtil
from Code.QT import QTUtil2
from Code.QT import QTVarios
from Code.QT import Tablero
from Code import TrListas
from Code import Util
from Code import VarGen

SIN_VALORACION, MUY_MALO, MALO, BUENO, MUY_BUENO, INTERESANTE, DUDOSA = (0, 4, 2, 1, 3, 5, 6)


class UnMove:
    def __init__(self, listaMovesPadre, pv, dicCache):

        self.listaMovesPadre = listaMovesPadre
        self.listaMovesHijos = None

        self.pv = pv

        self.partida = listaMovesPadre.partidaBase.copia()
        self.partida.leerPV(self.pv)

        self.titulo = self.partida.last_jg().pgnSP()

        if dicCache:
            dic = dicCache.get(self.pv, {})
        else:
            dic = {}

        self.valoracion = dic.get("VAL", 0)
        self.comentario = dic.get("COM", "")
        self.variantes = dic.get("VAR", [])
        self.siOculto = dic.get("OCU", False)

        self.item = None

        self.posActual = self.partida.numJugadas() - 1

    def row(self):
        return self.listaMovesPadre.liMoves.index(self)

    def analisis(self):
        return self.listaMovesPadre.analisisMov(self)

    def conHijosDesconocidos(self, dbCache):
        if self.listaMovesHijos:
            return False
        fenM2 = self.partida.ultPosicion.fenM2()
        return fenM2 in dbCache

    def etiPuntos(self, siExten):
        pts = self.listaMovesPadre.etiPuntosUnMove(self, siExten)
        if not siExten:
            return pts
        nom = self.listaMovesPadre.nomAnalisis()
        if nom:
            return nom + ": " + pts
        else:
            return ""

    def creaHijos(self):
        self.listaMovesHijos = ListaMoves(self, self.partida.ultPosicion.fen(), self.listaMovesPadre.dbCache)
        return self.listaMovesHijos

    def inicio(self):
        self.posActual = -1

    def atras(self):
        self.posActual -= 1
        if self.posActual < -1:
            self.inicio()

    def adelante(self):
        self.posActual += 1
        if self.posActual >= self.partida.numJugadas():
            self.final()

    def final(self):
        self.posActual = self.partida.numJugadas() - 1

    def numVariantes(self):
        return len(self.variantes)

    def damePosicion(self):
        if self.posActual == -1:
            posicion = self.partida.iniPosicion
            desde, hasta = None, None
        else:
            jg = self.partida.jugada(self.posActual)
            posicion = jg.posicion
            desde = jg.desde
            hasta = jg.hasta
        return posicion, desde, hasta

    def ponValoracion(self, valoracion):
        self.valoracion = valoracion

    def ponComentario(self, comentario):
        self.comentario = comentario

    def guardaCache(self, dicCache):
        dic = {}
        if self.valoracion != "-":
            dic["VAL"] = self.valoracion
        if self.comentario:
            dic["COM"] = self.comentario
        if self.variantes:
            dic["VAR"] = self.variantes
        if self.siOculto:
            dic["OCU"] = True
        if dic:
            dicCache[self.pv] = dic

        if self.listaMovesHijos:
            self.listaMovesHijos.guardaCache()


class ListaMoves:
    def __init__(self, moveOwner, fen, dbCache):
        self.moveOwner = moveOwner
        self.dbCache = dbCache

        if not moveOwner:
            self.nivel = 0
            cp = ControlPosicion.ControlPosicion()
            cp.leeFen(fen)
            self.partidaBase = Partida.Partida(cp)
        else:
            self.nivel = self.moveOwner.listaMovesPadre.nivel + 1
            self.partidaBase = self.moveOwner.partida.copia()

        self.fenM2 = self.partidaBase.ultPosicion.fenM2()

        dicCache = self.dbCache[self.fenM2]

        LCEngine.setFen(self.fenM2 + " 0 1")
        liMov = [xpv[1:] for xpv in LCEngine.getMoves()]

        liMov.sort()
        liMoves = []
        for pv in liMov:
            um = UnMove(self, pv, dicCache)
            liMoves.append(um)

        self.liMoves = liMoves
        self.liMovesInicial = liMoves[:]
        self.liAnalisis = dicCache.get("ANALISIS", []) if dicCache else []

        # self.analisisActivo
        # self.dicAnalisis
        self.ponAnalisisActivo(dicCache.get("ANALISIS_ACTIVO", None) if dicCache else None)

    def guardaCache(self):
        dicCache = {}
        for um in self.liMoves:
            um.guardaCache(dicCache)

        if self.liAnalisis:
            dicCache["ANALISIS"] = self.liAnalisis
            dicCache["ANALISIS_ACTIVO"] = self.analisisActivo

        if dicCache:
            self.dbCache[self.fenM2] = dicCache

    def etiPuntosUnMove(self, mov, siExten):
        if self.analisisActivo is None:
            return ""

        if mov.pv in self.dicAnalisis:
            rm = self.dicAnalisis[mov.pv]
            resp = rm.abrTexto() if siExten else rm.abrTextoBase()
        else:
            resp = "?"
        if self.nivel % 2:
            resp += " "
        return resp

    def numVisiblesOcultos(self):
        n = 0
        for mov in self.liMoves:
            if mov.siOculto:
                n += 1
        return len(self.liMoves) - n, n

    def nomAnalisis(self):
        if self.analisisActivo is None:
            return ""
        mrm = self.liAnalisis[self.analisisActivo]
        return mrm.rotulo

    def quitaAnalisis(self, num):
        if num == self.analisisActivo:
            self.ponAnalisisActivo(None)
        del self.liAnalisis[num]

    def analisisMov(self, mov):
        return self.dicAnalisis.get(mov.pv, None)

    def reordenaSegunValoracion(self):
        li = []
        dnum = {MUY_BUENO: 0, BUENO: 1000, INTERESANTE: 1300, DUDOSA: 1700, MALO: 2000, MUY_MALO: 3000,
                SIN_VALORACION: 4000}
        for mov in self.liMovesInicial:
            v = mov.valoracion
            num = dnum[v]
            dnum[v] += 1
            li.append((mov, num))
        li.sort(key=lambda x: x[1])
        liMov = []
        for mov, num in li:
            liMov.append(mov)
        self.liMoves = liMov

    def ponAnalisisActivo(self, num):
        self.analisisActivo = num

        if num is None:
            self.dicAnalisis = {}
            self.reordenaSegunValoracion()
            return

        dic = collections.OrderedDict()

        dicPos = {}

        mrm = self.liAnalisis[num]

        for n, rm in enumerate(mrm.liMultiPV):
            a1h8 = rm.movimiento()
            dic[rm.movimiento()] = rm
            dicPos[a1h8] = n + 1

        li = []
        for mov in self.liMoves:
            pos = dicPos.get(mov.pv, 999999)
            li.append((mov, pos))

        li.sort(key=lambda x: x[1])

        self.liMoves = []
        for mov, pos in li:
            self.liMoves.append(mov)

        self.dicAnalisis = dic

    def listaMovsSiguientes(self, mov):
        pos = self.liMoves.index(mov)
        li = [self.liMoves[x] for x in range(pos + 1, len(self.liMoves))]
        return li

    def listaMovsValoracionVisibles(self, valoracion):
        li = [mov for mov in self.liMoves if mov.valoracion == valoracion and not mov.siOculto]
        return li

    def buscaMovVisibleDesde(self, mov):
        pos = self.liMoves.index(mov)
        li = range(pos, len(self.liMoves))
        if pos:
            li.extend(range(pos, -1, -1))
        for x in li:
            mv = self.liMoves[x]
            if not mv.siOculto:
                return mv
        mov.siOculto = False  # Por si acaso
        return mov


class TreeMoves(QtGui.QTreeWidget):
    def __init__(self, owner, procesador):
        QtGui.QTreeWidget.__init__(self)
        self.owner = owner
        self.dbCache = owner.dbCache
        self.setAlternatingRowColors(True)
        self.listaMoves = owner.listaMoves
        self.procesador = procesador
        self.setContextMenuPolicy(QtCore.Qt.CustomContextMenu)
        self.customContextMenuRequested.connect(self.menuContexto)

        self.setHeaderLabels((_("Moves"), _("Points"), _("Comments"), _("Variants"), "", ""))
        self.setColumnHidden(4, True)

        dicNAGs = TrListas.dicNAGs()
        self.dicValoracion = collections.OrderedDict()
        self.dicValoracion["1"] = (MUY_BUENO, dicNAGs[3])
        self.dicValoracion["2"] = (BUENO, dicNAGs[1])
        self.dicValoracion["3"] = (MALO, dicNAGs[2])
        self.dicValoracion["4"] = (MUY_MALO, dicNAGs[4])
        self.dicValoracion["5"] = (INTERESANTE, dicNAGs[5])
        self.dicValoracion["6"] = (DUDOSA, dicNAGs[6])
        self.dicValoracion["0"] = (SIN_VALORACION, _("No rating"))

        ftxt = Controles.TipoLetra(puntos=9)

        self.setFont(ftxt)

        self.connect(self, QtCore.SIGNAL("currentItemChanged(QTreeWidgetItem *,QTreeWidgetItem *)"), self.seleccionado)
        self.connect(self, QtCore.SIGNAL("itemDoubleClicked(QTreeWidgetItem *,int)"), self.editado)

        hitem = self.header()
        hitem.setClickable(True)
        self.connect(hitem, QtCore.SIGNAL("sectionDoubleClicked(int)"), self.editadoH)

        self.dicItemMoves = {}
        self.ponMoves(self.listaMoves)

        self.sortItems(4, QtCore.Qt.AscendingOrder)

    def editadoH(self, col):
        item = self.currentItem()
        if not item:
            return
        mov = self.dicItemMoves[str(item)]
        lm = mov.listaMovesPadre

        if col == 0:
            lm.reordenaSegunValoracion()
            self.ordenaMoves(lm)
        elif col == 1:
            lm.ponAnalisisActivo(lm.analisisActivo)
            self.ordenaMoves(lm)

    def ponMoves(self, listaMoves):

        liMoves = listaMoves.liMoves
        if liMoves:
            moveOwner = listaMoves.moveOwner
            padre = self if moveOwner is None else moveOwner.item
            for n, mov in enumerate(liMoves):
                n_var = mov.numVariantes()
                c_var = str(n_var) if n_var else ""
                c_ord = "%02d" % (n + 1)
                titulo = mov.titulo
                if mov.conHijosDesconocidos(self.dbCache):
                    titulo += " ^"
                item = QtGui.QTreeWidgetItem(padre, [titulo, mov.etiPuntos(False), mov.comentario, c_var, c_ord])
                item.setTextAlignment(1, QtCore.Qt.AlignRight)
                item.setTextAlignment(3, QtCore.Qt.AlignCenter)
                if mov.siOculto:
                    qm = self.indexFromItem(item, 0)
                    self.setRowHidden(qm.row(), qm.parent(), True)

                self.ponIconoValoracion(item, mov.valoracion)
                mov.item = item
                self.dicItemMoves[str(item)] = mov

            x = 0
            for t in range(3):
                x += self.columnWidth(t)

            mov = listaMoves.buscaMovVisibleDesde(liMoves[0])
            self.setCurrentItem(mov.item)

            x = self.columnWidth(0)
            self.resizeColumnToContents(0)
            dif = self.columnWidth(0) - x
            if dif > 0:
                sz = self.owner.splitter.sizes()
                sz[1] += dif
                self.owner.resize(self.owner.width() + dif, self.owner.height())
                self.owner.splitter.setSizes(sz)

    def editado(self, item, col):
        mov = self.dicItemMoves.get(str(item), None)
        if mov is None:
            return

        if col == 0:
            self.editaValoracion(item, mov)

        elif col == 1:
            self.editaAnalisis(item, mov)

        elif col == 2:
            self.editaComentario(item, mov)

        elif col == 3:
            self.editaVariantes(item, mov)

    def editaComentario(self, item, mov):

        liGen = [(None, None)]

        config = FormLayout.Editbox(_("Comments"), ancho=230)
        liGen.append((config, mov.comentario))

        resultado = FormLayout.fedit(liGen, title=_("Comments") + " " + mov.titulo, parent=self, anchoMinimo=200,
                                     icon=Iconos.ComentarioEditar())
        if resultado is None:
            return

        accion, liResp = resultado
        mov.comentario = liResp[0]

        item.setText(2, mov.comentario)

    def editaValoracion(self, item, mov):
        menu = QTVarios.LCMenu(self)
        for k in self.dicValoracion:
            cl, titulo = self.dicValoracion[k]
            menu.opcion(cl, "%s) %s" % (k, titulo), self.iconoValoracion(cl))
            menu.separador()

        resp = menu.lanza()
        if resp is None:
            return None

        mov.valoracion = resp
        self.ponIconoValoracion(item, resp)

    def editaAnalisis(self, item, mov):

        # Hay un analisis -> se muestra en variantes
        # Analisis.muestraAnalisis( self.procesador, self.xtutor, jg, siBlancas, maxRecursion, pos )
        fen = mov.partida.ultPosicion.fen()

        rm = mov.analisis()
        if rm is None:
            return

        partida = Partida.Partida(mov.partida.ultPosicion)
        partida.leerPV(rm.pv)
        lineaPGN = partida.pgnBaseRAW()
        wowner = self.owner
        tablero = wowner.infoMove.tablero
        import Code.Variantes as Variantes

        Variantes.editaVarianteMoves(self.procesador, wowner, tablero.siBlancasAbajo, fen, lineaPGN,
                                     titulo=mov.titulo + " - " + mov.etiPuntos(True))

    def ponVariantes(self, mov):
        num = len(mov.variantes)
        txt = str(num) if num else ""
        mov.item.setText(3, txt)

    def editaVariantes(self, item, mov):
        import Code.Variantes as Variantes

        if mov.variantes:
            menu = QTVarios.LCMenu(self)
            for num, una in enumerate(mov.variantes):
                menu.opcion(num, una[:40], Iconos.PuntoAzul())
            menu.separador()
            menu.opcion(-1, _("New variant"), Iconos.Mas())
            menu.separador()
            menub = menu.submenu(_("Remove"), Iconos.Delete())
            for num, una in enumerate(mov.variantes):
                menub.opcion(-num - 2, una[:40], Iconos.PuntoNaranja())

            resp = menu.lanza()
            if resp is None:
                return None
            if resp == -1:
                num = None
                lineaPGN = ""
            elif resp >= 0:
                num = resp
                lineaPGN = mov.variantes[num]
            else:
                num = -resp - 2
                una = mov.variantes[num]
                if QTUtil2.pregunta(self, _X(_("Delete %1?"), una[:40])):
                    del mov.variantes[num]
                    self.ponVariantes(mov)
                return
        else:
            lineaPGN = ""
            num = None
        fen = mov.partida.ultPosicion.fen()

        wowner = self.owner
        tablero = wowner.infoMove.tablero
        resp = Variantes.editaVarianteMoves(self.procesador, wowner, tablero.siBlancasAbajo, fen, lineaPGN)
        if resp:
            una = resp[0]
            if num is None:
                mov.variantes.append(una)
                self.ponVariantes(mov)
            else:
                mov.variantes[num] = una

    def mostrarOcultar(self, item, mov):

        lm = mov.listaMovesPadre
        nVisibles, nOcultos = lm.numVisiblesOcultos()
        if nVisibles <= 1 and nOcultos == 0:
            return

        listaMovsSiguientes = lm.listaMovsSiguientes(mov)

        menu = QTVarios.LCMenu(self)

        if nVisibles > 1:
            smenu = menu.submenu(_("Hide"), Iconos.Ocultar())
            smenu.opcion("actual", _("Selected move"), Iconos.PuntoNaranja())
            smenu.separador()
            if listaMovsSiguientes:
                smenu.opcion("siguientes", _("Next moves"), Iconos.PuntoRojo())
                smenu.separador()

            for k in self.dicValoracion:
                valoracion, titulo = self.dicValoracion[k]
                if lm.listaMovsValoracionVisibles(valoracion):
                    smenu.opcion("val_%d" % valoracion, titulo, self.iconoValoracion(valoracion))
                    smenu.separador()

        if nOcultos:
            menu.opcion("mostrar", _("Show hidden"), Iconos.Mostrar())

        resp = menu.lanza()
        if resp is None:
            return

        if resp == "actual":
            mov.siOculto = True

        elif resp == "siguientes":
            for mv in listaMovsSiguientes:
                mv.siOculto = True

        elif resp.startswith("val_"):
            valoracion = int(resp[4])
            for mv in lm.listaMovsValoracionVisibles(valoracion):
                if nVisibles == 1:
                    break
                mv.siOculto = True
                nVisibles -= 1

        elif resp == "mostrar":
            for mv in lm.liMoves:
                mv.siOculto = False

        qmParent = self.indexFromItem(item, 0).parent()
        for nFila, mv in enumerate(lm.liMoves):
            self.setRowHidden(nFila, qmParent, mv.siOculto)

        self.goto(mov)

    def menuContexto(self, position):
        self.owner.wmoves.menuContexto()

    def iconoValoracion(self, valoracion):
        cnf = VarGen.configuracion

        dic = {
            0: "lightgray",
            1: cnf.color_nag1,
            2: cnf.color_nag2,
            3: cnf.color_nag3,
            4: cnf.color_nag4,
            5: cnf.color_nag5,
            6: cnf.color_nag6,
        }
        return QTUtil.colorIcon(dic[valoracion], 8, 8)

    def ponIconoValoracion(self, item, valoracion):

        item.setIcon(0, self.iconoValoracion(valoracion))

    def ordenaMoves(self, listaMoves):
        for n, mov in enumerate(listaMoves.liMoves):
            c_ord = "%02d" % (n + 1)
            mov.item.setText(4, c_ord)
        self.sortItems(4, QtCore.Qt.AscendingOrder)

    def goto(self, mov):
        mov = mov.listaMovesPadre.buscaMovVisibleDesde(mov)
        self.setCurrentItem(mov.item)
        self.owner.muestra(mov)
        # self.owner.wmoves.setFocus()
        self.setFocus()

    def seleccionado(self, item, itemA):
        self.owner.muestra(self.dicItemMoves[str(item)])
        self.setFocus()

    def keyPressEvent(self, event):
        resp = QtGui.QTreeWidget.keyPressEvent(self, event)
        k = event.key()
        if k == 43:
            self.mas()
        elif k == 16777223:
            self.menos()
        elif 48 <= k <= 52:
            item = self.currentItem()
            if item:
                cl, titulo = self.dicValoracion[chr(k)]
                self.ponIconoValoracion(item, cl)
                mov = self.dicItemMoves[str(item)]
                mov.valoracion = cl

        return resp

    def mas(self, mov=None):
        if mov is None:
            item = self.currentItem()
            mov = self.dicItemMoves[str(item)]
        else:
            item = mov.item
        if mov.listaMovesHijos is None:
            item.setText(0, mov.titulo)
            listaMovesHijos = mov.creaHijos()
            self.ponMoves(listaMovesHijos)

    def menos(self, mov=None):
        if mov is None:
            item = self.currentItem()
            mov = self.dicItemMoves[str(item)]
        else:
            item = mov.item
        lm = mov.listaMovesPadre
        nVisibles, nOcultos = lm.numVisiblesOcultos()
        if nVisibles <= 1:
            return

        qm = self.currentIndex()
        self.setRowHidden(qm.row(), qm.parent(), True)
        mov.siOculto = True

        self.goto(mov)

    def currentMov(self):
        item = self.currentItem()
        if item:
            mov = self.dicItemMoves[str(item)]
        else:
            mov = None
        return mov


class WMoves(QtGui.QWidget):
    def __init__(self, owner, procesador):
        QtGui.QWidget.__init__(self)

        self.owner = owner

        # Tree
        self.tree = TreeMoves(owner, procesador)

        # ToolBar
        self.tb = Controles.TBrutina(self, siTexto=False, tamIcon=16)
        self.tb.new(_("Open new branch"), Iconos.Mas(), self.rama)
        self.tb.new(_("Show") + "/" + _("Hide"), Iconos.Mostrar(), self.mostrar)
        self.tb.new(_("Rating"), self.tree.iconoValoracion(3), self.valorar)
        self.tb.new(_("Analyze"), Iconos.Analizar(), self.analizar)
        self.tb.new(_("Comments"), Iconos.ComentarioEditar(), self.comentario)
        self.tb.new(_("Variants"), Iconos.Variantes(), self.variantes)

        layout = Colocacion.V().control(self.tb).control(self.tree).margen(1)

        self.setLayout(layout)

    def menuContexto(self):
        mov = self.tree.currentMov()
        if not mov:
            return

        menu = QTVarios.LCMenu(self)

        for comando in self.tb.liAcciones:
            if comando:
                txt, icono, accion = comando
                if accion == self.rama and mov.listaMovesHijos:
                    continue
                menu.opcion(accion, txt, icono)
                menu.separador()

        resp = menu.lanza()
        if resp is not None:
            self.procesarAccion(resp)

    def rama(self):
        mov = self.tree.currentMov()
        if not mov:
            return
        self.tree.mas()

    def analizar(self):
        mov = self.tree.currentMov()
        if not mov:
            return
        self.owner.analizar(mov)

    def valorar(self):
        mov = self.tree.currentMov()
        if not mov:
            return
        self.tree.editaValoracion(mov.item, mov)

    def comentario(self):
        mov = self.tree.currentMov()
        if not mov:
            return
        self.tree.editaComentario(mov.item, mov)

    def variantes(self):
        mov = self.tree.currentMov()
        if not mov:
            return
        self.tree.editaVariantes(mov.item, mov)

    def mostrar(self):
        mov = self.tree.currentMov()
        if not mov:
            return
        self.tree.mostrarOcultar(mov.item, mov)


class InfoMove(QtGui.QWidget):
    def __init__(self, siBlancasAbajo):
        QtGui.QWidget.__init__(self)

        confTablero = VarGen.configuracion.confTablero("INFOMOVE", 32)
        self.pantalla = self
        self.tablero = Tablero.Tablero(self, confTablero)
        self.tablero.crea()
        self.tablero.ponerPiezasAbajo(siBlancasAbajo)

        btInicio = Controles.PB(self, "", self.inicio).ponIcono(Iconos.MoverInicio())
        btAtras = Controles.PB(self, "", self.atras).ponIcono(Iconos.MoverAtras())
        btAdelante = Controles.PB(self, "", self.adelante).ponIcono(Iconos.MoverAdelante())
        btFinal = Controles.PB(self, "", self.final).ponIcono(Iconos.MoverFinal())

        self.lbAnalisis = Controles.LB(self, "")

        lybt = Colocacion.H().relleno()
        for x in (btInicio, btAtras, btAdelante, btFinal):
            lybt.control(x)
        lybt.relleno()

        lyt = Colocacion.H().relleno().control(self.tablero).relleno()

        lya = Colocacion.H().relleno().control(self.lbAnalisis).relleno()

        layout = Colocacion.V()
        layout.otro(lyt)
        layout.otro(lybt)
        layout.otro(lya)
        layout.relleno()
        self.setLayout(layout)

        self.movActual = None

    def ponValores(self):
        posicion, desde, hasta = self.movActual.damePosicion()
        self.tablero.ponPosicion(posicion)

        if desde:
            self.tablero.ponFlechaSC(desde, hasta)

        self.lbAnalisis.ponTexto("<b>" + self.movActual.etiPuntos(True) + "</b>")

    def inicio(self):
        self.movActual.inicio()
        self.ponValores()

    def atras(self):
        self.movActual.atras()
        self.ponValores()

    def adelante(self):
        self.movActual.adelante()
        self.ponValores()

    def final(self):
        self.movActual.final()
        self.ponValores()

    def muestra(self, mov):
        self.movActual = mov
        self.ponValores()


class PantallaArbol(QTVarios.WDialogo):
    def __init__(self, wParent, partida, nj, procesador):

        pantalla = wParent

        self.procesador = procesador

        titulo = _("Moves tree")
        icono = Iconos.Arbol()
        extparam = "moves"
        QTVarios.WDialogo.__init__(self, pantalla, titulo, icono, extparam)

        dicVideo = self.recuperarDicVideo()

        self.dbCache = Util.DicSQL(VarGen.configuracion.ficheroMoves)
        if nj >= 0:
            posicion = partida.jugada(nj).posicion
        else:
            posicion = partida.ultPosicion
        self.listaMoves = ListaMoves(None, posicion.fen(), self.dbCache)

        tb = QTVarios.LCTB(self)
        tb.new(_("Save"), Iconos.Grabar(), self.grabar)
        tb.new(_("Cancel"), Iconos.Cancelar(), self.cancelar)

        self.infoMove = InfoMove(posicion.siBlancas)

        self.wmoves = WMoves(self, procesador)

        self.splitter = splitter = QtGui.QSplitter(self)
        splitter.addWidget(self.infoMove)
        splitter.addWidget(self.wmoves)

        ly = Colocacion.H().control(splitter).margen(0)

        layout = Colocacion.V().control(tb).otro(ly).margen(3)

        self.setLayout(layout)

        self.wmoves.tree.setFocus()

        anchoTablero = self.infoMove.tablero.width()

        self.recuperarVideo(anchoDefecto=869 - 242 + anchoTablero)
        if not dicVideo:
            dicVideo = {'TREE_3': 27, 'SPLITTER': [260 - 242 + anchoTablero, 617], 'TREE_1': 49, 'TREE_2': 383,
                        'TREE_4': 25}
        sz = dicVideo.get("SPLITTER", None)
        if sz:
            self.splitter.setSizes(sz)
        for x in range(1, 6):
            w = dicVideo.get("TREE_%d" % x, None)
            if w:
                self.wmoves.tree.setColumnWidth(x, w)

    def muestra(self, mov):
        self.infoMove.muestra(mov)

    def salvarVideo(self):
        dicExten = {
            "SPLITTER": self.splitter.sizes(),
        }
        for x in range(1, 6):
            dicExten["TREE_%d" % x] = self.wmoves.tree.columnWidth(x)

        self.guardarVideo(dicExten)

    def grabar(self):
        self.listaMoves.guardaCache()
        self.dbCache.close()
        self.salvarVideo()

        self.accept()

    def cancelar(self):
        self.dbCache.close()
        self.salvarVideo()
        self.reject()

    def closeEvent(self, event):
        self.dbCache.close()
        self.salvarVideo()

    def analizar(self, mov):
        if mov.listaMovesPadre:
            lm = mov.listaMovesPadre
        else:
            lm = self.listaMoves

        # Si tiene ya analisis, lo pedimos o nuevo
        menu = QTVarios.LCMenu(self)
        if lm.liAnalisis:
            for n, mrm in enumerate(lm.liAnalisis):
                menu.opcion(n, mrm.rotulo, Iconos.PuntoVerde())
            menu.separador()

            menu.opcion(-999999, _("New analysis"), Iconos.Mas())
            menu.separador()

            if lm.analisisActivo is not None:
                menu.opcion(-999998, _("Hide analysis"), Iconos.Ocultar())
                menu.separador()

            menu1 = menu.submenu(_("Remove"), Iconos.Delete())
            for n, mrm in enumerate(lm.liAnalisis):
                menu1.opcion(-n - 1, mrm.rotulo, Iconos.PuntoRojo())
                menu1.separador()

            resp = menu.lanza()
            if resp is None:
                return

            if resp >= 0:
                self.ponAnalisis(lm, resp)
                return

            elif resp == -999999:
                self.nuevoAnalisis(lm)
                return

            elif resp == -999998:
                self.ponAnalisis(lm, None)
                return

            else:
                num = -resp - 1
                mrm = lm.liAnalisis[num]
                if QTUtil2.pregunta(self, _X(_("Delete %1?"), mrm.rotulo)):
                    self.quitaAnalisis(lm, num)
                return

        else:
            self.nuevoAnalisis(lm)

    def nuevoAnalisis(self, lm):
        fen = lm.partidaBase.ultPosicion.fen()
        alm = PantallaAnalisisParam.paramAnalisis(self, VarGen.configuracion, False, siTodosMotores=True)
        if alm is None:
            return
        confMotor = VarGen.configuracion.buscaMotor(alm.motor)
        confMotor.actMultiPV(alm.multiPV)
        # confMotor.debug( "rival" )

        xmotor = self.procesador.creaGestorMotor(confMotor, alm.tiempo, alm.depth, siMultiPV=True)

        me = QTUtil2.analizando(self)
        mrm = xmotor.analiza(fen)
        mrm.tiempo = alm.tiempo / 1000.0
        mrm.depth = alm.depth

        tipo = "%s=%d" % (_("Depth"), mrm.depth) if mrm.depth else "%.0f\"" % mrm.tiempo
        mrm.rotulo = ("%s %s" % (mrm.nombre, tipo))
        xmotor.terminar()
        me.final()

        lm.liAnalisis.append(mrm)
        self.ponAnalisis(lm, len(lm.liAnalisis) - 1)

    def ponAnalisis(self, lm, num):

        lm.ponAnalisisActivo(num)

        for um in lm.liMoves:
            um.item.setText(1, um.etiPuntos(False))

        self.wmoves.tree.ordenaMoves(lm)
        self.wmoves.tree.goto(lm.liMoves[0])

        # self.infoMove.ponValores()

    def quitaAnalisis(self, lm, num):

        lm.quitaAnalisis(num)

        for um in lm.liMoves:
            um.item.setText(1, um.etiPuntos(False))

        self.wmoves.tree.ordenaMoves(lm)
        self.wmoves.tree.goto(lm.liMoves[0])

        self.infoMove.ponValores()

import codecs
import os

from PyQt4 import QtGui, QtCore

from Code import AperturasStd
from Code.QT import Colocacion
from Code.QT import Controles
from Code.QT import FormLayout
from Code.QT import Iconos
from Code.QT import QTUtil2
from Code.QT import QTVarios
from Code.QT import WBG_Training
from Code.QT import WBG_Tree
from Code import Tacticas
from Code import Util
from Code import VarGen


class WN_LB(Controles.ED):
    def __init__(self, wNavigator, xid):
        Controles.ED.__init__(self, wNavigator)
        self.id = xid
        self.wNavigator = wNavigator

        # self.soloLectura(True)
        self.si2 = False
        p = wNavigator.palette()
        self.setStyleSheet("background-color: %s;" % p.color(p.Window).name())
        self.ponTipoLetra(peso=60, puntos=10)
        self.ponTexto("")

    def mousePressEvent(self, event):
        Controles.ED.mousePressEvent(self, event)
        if event.button() == QtCore.Qt.LeftButton:
            self.wNavigator.pulsadoBT(self.id, True, posicion=self.cursorPosition())

    def mouseReleaseEvent(self, event):
        Controles.ED.mouseReleaseEvent(self, event)
        if event.button() == QtCore.Qt.LeftButton:
            self.wNavigator.pulsadoBT(self.id, False, si2=self.si2)
        self.si2 = False

    def mouseDoubleClickEvent(self, event):
        self.si2 = True

    def ponTexto(self, txt, maxim=9999):
        Controles.ED.ponTexto(self, txt)
        fm = self.fontMetrics()
        w = fm.boundingRect(txt).width()
        w += 10
        if w > maxim:
            w = maxim
        self.setFixedWidth(w)


class WNavigator(QtGui.QWidget):
    def __init__(self, wmoves):
        QtGui.QWidget.__init__(self)

        self.wmoves = wmoves
        self.move = None
        self.siWorking = False
        self.historia = None

        # Inicial

        self.bt = WN_LB(self, 1)
        self.bt.setBackgroundRole(QtGui.QPalette.Window)

        layout = Colocacion.H().control(self.bt).relleno().margen(3)
        self.setLayout(layout)

    def pulsadoBT(self, xid, siInicio, posicion=None, si2=False):
        if siInicio:  # se muestra el item y se guarda el item
            if self.historia and len(self.historia) > 1:
                txt = self.bt.texto()[:posicion]
                numMove = txt.count(" ") + 1
                self.siWorking = True
                self.tmpItem = self.historia[numMove].item()
                self.wmoves.tree.setCurrentItem(self.tmpItem)
                self.wmoves.tree.scrollToItem(self.tmpItem)

        else:
            if si2:  # se va a esa posici?n
                self.siWorking = False
                self.wmoves.tree.setCurrentItem(self.tmpItem)
            else:  # vuelve a ver el item ultimo
                if self.historia and len(self.historia) > 1:
                    item = self.historia[-1].item()
                    self.wmoves.tree.setCurrentItem(item)
                    self.wmoves.tree.scrollToItem(item)
                self.siWorking = False

    def ponMove(self, move, texto):
        if self.siWorking:
            return
        self.move = move
        self.historia = move.historia()
        while ">" in texto:
            d = texto.find("<")
            if d == -1:
                break
            h = texto.find(">")
            texto = texto[:d] + texto[h + 1:]
        self.bt.ponTexto(texto, self.wmoves.width() - 36)


class WMoves(QtGui.QWidget):
    def __init__(self, procesador, winBookGuide):
        QtGui.QWidget.__init__(self)

        self.winBookGuide = winBookGuide
        self.bookGuide = None  # <--setBookGuide
        self.wsummary = None  # <--setSummary
        self.infoMove = None  # <--setInfoMove

        self.procesador = procesador

        # Tree
        self.tree = WBG_Tree.TreeMoves(self)

        # ToolBar
        liAccionesWork = (
            (_("Close"), Iconos.MainMenu(), self.tw_terminar), None,
            (_("Bookmarks"), Iconos.Favoritos(), self.tw_bookmarks), None,
            (_("Start position"), Iconos.Inicio(), self.tw_inicio),
        )
        self.tbWork = Controles.TBrutina(self, liAccionesWork, tamIcon=24)

        liAccionesGen = (
            (_("Change"), Iconos.Modificar(), self.tg_cambiar),
            (_("New"), Iconos.NuevoMas(), self.tg_crear),
            (_("Copy"), Iconos.Copiar(), self.tg_copiar),
            (_("Remove"), Iconos.Borrar(), self.tg_borrar),
            (_("Rename"), Iconos.Rename(), self.tg_rename),
            (_("Training"), Iconos.Entrenamiento(), self.tg_training),
            (_("Import"), Iconos.Mezclar(), self.tg_import),
        )
        self.tbGen = Controles.TBrutina(self, liAccionesGen, tamIcon=24)

        # Name
        self.lbName = Controles.LB(self, "").ponWrap().alinCentrado().ponColorFondoN("white", "#5178AA").ponTipoLetra(
                puntos=16)

        # Navigator
        self.navigator = WNavigator(self)
        self.btInicial = Controles.PB(self, "", self.tw_inicio).ponIcono(Iconos.Inicio(), tamIcon=24).anchoFijo(24)
        lyN = Colocacion.H().control(self.btInicial).control(self.navigator).relleno().margen(3)

        lyTB = Colocacion.H().control(self.tbWork).relleno().control(self.tbGen)

        layout = Colocacion.V().control(self.lbName).otro(lyTB).control(self.tree).otro(lyN).margen(1)

        self.setLayout(layout)

    def focusInEvent(self, event):
        self.winBookGuide.ultFocus = self

    def actualiza(self):
        ultFocus = self.winBookGuide.ultFocus
        if ultFocus == self.wsummary:
            self.wsummary.cambiaInfoMove()
        else:
            item, move = self.tree.moveActual()
            if move is None:
                self.tw_inicio()
            else:
                self.seleccionado(move)

    def seleccionado(self, move, siSelecciona=False):
        self.infoMove.ponMovimiento(move)
        self.infoMove.modoNormal()
        self.wsummary.actualiza()
        if siSelecciona:
            # self.setFocus()
            # self.tree.setFocus()
            item = move.item()
            self.tree.scrollToItem(item)
            self.tree.setCurrentItem(item)
            item.setExpanded(True)
        self.navigator.ponMove(move, self.infoMove.lbPGN.texto())

    def showActiveName(self):
        nombre = self.bookGuide.name
        if nombre == "Standard opening guide":
            nombre = _("Standard opening guide")

        self.lbName.ponTexto("%s: %s" % (_("Guide"), nombre))

    def setBookGuide(self, bookGuide):
        self.bookGuide = bookGuide
        self.showActiveName()
        self.setToolBar()
        self.tree.clear()
        self.tree.setBookGuide(bookGuide, self.procesador)
        # self.bookGuide.reset()
        self.bookGuide.root.item(self.tree)
        self.compruebaBookmarks()

    def compruebaBookmarks(self):
        self.tbWork.setAccionVisible("tw_bookmarks", len(self.bookGuide.bookmarks) > 0)

    def ponFenM2inicial(self, fenM2inicial, pvInicial):
        self.tree.showChildren(self.bookGuide.root, True)
        if fenM2inicial:
            li = self.bookGuide.getMovesFenM2(fenM2inicial)
            if li:
                msel = li[0]
                for mv in li:
                    if mv.pv() == pvInicial:
                        msel = mv
                        break
                self.seleccionaMove(msel)
                return

        self.tw_inicio()

    def setSummary(self, wsummary):
        self.wsummary = wsummary

    def setInfoMove(self, infoMove):
        self.infoMove = infoMove

    def setToolBar(self):
        li = self.bookGuide.getOtras()
        wd = len(li) > 0
        for key in ("tg_cambiar", "tg_mezclar", "tg_borrar"):
            self.tbGen.setAccionVisible(key, wd)

    def showChildren(self, unMove):
        self.tree.showChildren(unMove, True)

    def tw_terminar(self):
        self.winBookGuide.terminar()

    def tw_bookmarks(self):
        libm = self.bookGuide.bookmarks
        n = len(libm)
        if n == 0:
            return
        else:
            menu = QTVarios.LCMenu(self)

            for mv in libm:
                menu.opcion(mv, mv.mark(), Iconos.PuntoAmarillo())

            menu.separador()
            menu.opcion("all", _("Remove all"), Iconos.Delete())
            resp = menu.lanza()

            if resp is None:
                return

            if resp == "all":
                li = self.bookGuide.bookmarks[:]
                for mv in li:
                    mv.mark("")
                    self.tree.ponIconoBookmark(mv.item(), "")

                self.compruebaBookmarks()
                return

            mvSel = resp

        self.seleccionaMove(mvSel)

    def seleccionaMove(self, move):
        li = move.historia()
        for move in li:
            if not move.item():
                self.showChildren(move.father())

        self.tree.scrollToItem(li[1].item())
        self.seleccionado(move, True)

    def tw_inicio(self):
        self.seleccionado(self.bookGuide.root, False)
        self.tree.setCurrentItem(None)

    def getNewName(self, title, previous=""):
        name = previous

        while True:
            liGen = [(None, None)]
            liGen.append((_("Name") + ":", name))

            resultado = FormLayout.fedit(liGen, title=title, parent=self, anchoMinimo=460,
                                         icon=Iconos.TutorialesCrear())
            if resultado is None:
                return None

            accion, liResp = resultado
            name = liResp[0].strip()
            if not name:
                return None

            name = Util.validNomFichero(name)

            ok = True
            for k in self.bookGuide.getOtras():
                if k.lower() == name.lower():
                    QTUtil2.mensError(self, _("This name is repeated, please select other"))
                    ok = False
                    break
            if ok:
                return name

    def inicializa(self):
        self.tree.clear()
        self.winBookGuide.ultFocus = self
        self.winBookGuide.inicializa()

    def selectOther(self):
        li = self.bookGuide.getOtras()
        if not li:
            return None
        menu = QTVarios.LCMenu(self)
        nico = QTVarios.rondoColores()

        for k in li:
            menu.opcion(k, k, nico.otro())
            menu.separador()
        return menu.lanza()

    def cambiarGuia(self, otraGuia):
        self.bookGuide.changeTo(self, otraGuia)
        self.tree.clear()
        self.bookGuide.root.item(self.tree)
        self.ponFenM2inicial(self.winBookGuide.fenM2inicial, self.winBookGuide.pvInicial)
        self.winBookGuide.ultFocus = self
        self.inicializa()

    def tg_cambiar(self):
        otraGuia = self.selectOther()
        if otraGuia is None:
            return
        self.cambiarGuia(otraGuia)

    def tg_crear(self):
        name = self.getNewName(_("New guide"))
        if name:
            self.cambiarGuia(name)

    def tg_copiar(self):
        name = self.getNewName(_("Copy"))
        if name:
            self.bookGuide.copyTo(name)

    def tg_borrar(self):
        nameGuide = self.selectOther()
        if nameGuide:
            if QTUtil2.pregunta(self, _X(_("Delete %1?"), nameGuide)):
                self.bookGuide.removeOther(nameGuide)

    def tg_rename(self):
        name = self.getNewName(_("Rename"), self.bookGuide.name)
        if name:
            self.bookGuide.renameTo(self, name)
            self.cambiarGuia(name)

    def tg_import(self):
        menu = QTVarios.LCMenu(self)

        li = self.bookGuide.getOtras()
        if li:
            otra = menu.submenu(_("Other guide"), Iconos.BookGuide())
            for k in li:
                otra.opcion(k, k, Iconos.PuntoVerde())
            menu.separador()
        menu.opcion("pgn", _("PGN with variants"), Iconos.Tablero())
        menu.separador()
        menu.opcion("polyglot", _("Polyglot book"), Iconos.Libros())
        menu.separador()
        menu1 = menu.submenu(_("Standard openings"), Iconos.Aperturas())
        menu1.opcion("std_all", _("All"), Iconos.PuntoVerde())
        menu1.separador()
        menu1.opcion("std_basic", _("Only basic"), Iconos.PuntoAmarillo())
        resp = menu.lanza()
        if resp is None:
            return

        if resp == "pgn":
            siGrabado = self.tg_append_pgn()
        elif resp == "std_all":
            siGrabado = self.tg_append_std(False)
        elif resp == "std_basic":
            siGrabado = self.tg_append_std(True)
        elif resp == "polyglot":
            siGrabado = self.tg_append_polyglot()
        else:
            siGrabado = self.tg_append_otra(resp)

        if siGrabado:
            self.cambiarGuia(self.bookGuide.name)

    def tg_append_otra(self, otraGuide):
        return self.bookGuide.appendFrom(self, otraGuide)

    def tg_append_polyglot(self):

        previo = VarGen.configuracion.leeVariables("WBG_MOVES")
        carpeta = previo.get("CARPETABIN", "")

        ficheroBIN = QTUtil2.leeFichero(self, carpeta, "%s (*.bin)" % _("Polyglot book"), titulo=_("File to import"))
        if not ficheroBIN:
            return
        previo["CARPETABIN"] = os.path.dirname(ficheroBIN)
        VarGen.configuracion.escVariables("WBG_MOVES", previo)

        liGen = [(None, None)]

        liGen.append((None, _("Select a maximum number of moves (plies)<br> to consider from each game")))

        liGen.append((FormLayout.Spinbox(_("Depth"), 3, 99, 50), 30))
        liGen.append((None, None))

        liGen.append((_("Only white best moves"), False))
        liGen.append((None, None))

        liGen.append((_("Only black best moves"), False))
        liGen.append((None, None))

        resultado = FormLayout.fedit(liGen, title=os.path.basename(ficheroBIN), parent=self, anchoMinimo=360,
                                     icon=Iconos.PuntoNaranja())

        if resultado:
            accion, liResp = resultado
            depth, whiteBest, blackBest = liResp
            return self.bookGuide.grabarPolyglot(self, ficheroBIN, depth, whiteBest, blackBest)

        return False

    def tg_append_pgn(self):

        previo = VarGen.configuracion.leeVariables("WBG_MOVES")
        carpeta = previo.get("CARPETAPGN", "")

        ficheroPGN = QTUtil2.leeFichero(self, carpeta, "%s (*.pgn)" % _("PGN Format"), titulo=_("File to import"))
        if not ficheroPGN:
            return
        previo["CARPETAPGN"] = os.path.dirname(ficheroPGN)
        VarGen.configuracion.escVariables("WBG_MOVES", previo)

        liGen = [(None, None)]

        liGen.append((None, _("Select a maximum number of moves (plies)<br> to consider from each game")))

        liGen.append((FormLayout.Spinbox(_("Depth"), 3, 999, 50), 30))
        liGen.append((None, None))

        resultado = FormLayout.fedit(liGen, title=os.path.basename(ficheroPGN), parent=self, anchoMinimo=460,
                                     icon=Iconos.PuntoNaranja())

        if resultado:
            accion, liResp = resultado
            depth = liResp[0]
            return self.bookGuide.grabarPGN(self, ficheroPGN, depth)
        return False

    def tg_append_std(self, siBasic):

        self.bookGuide.generarStandard(self, siBasic)
        return True

    def tg_training(self):
        w = WBG_Training.WTraining(self, self.tree.dicValoracion, self.tree.dicVentaja)
        w.exec_()
        if not w.siAceptado:
            return
        um = QTUtil2.unMomento(self)
        dic = w.resultado()
        siBlancas = dic["SIWHITE"]
        tactica = Tacticas.BaseTactica()
        menuname = dic["NAME"]
        depth = dic["DEPTH"]
        reValW, reVenW = dic["WHITE"]
        reValB, reVenB = dic["BLACK"]
        dicVal = {True: reValW, False: reValB}
        dicVen = {True: reVenW, False: reVenB}
        siAddOpening = dic["ADDSTD"]
        if siAddOpening:
            listaAperturasStd = AperturasStd.ap
        tactica.SHOWTEXT = [0 if siAddOpening else 1, ]

        # Leemos todas las lineas=listas de movimientos
        liT = self.bookGuide.allLines()

        # Se calcula el nombre de la carpeta
        name = self.bookGuide.name
        restDir = name
        nomDir = os.path.join(VarGen.configuracion.dirPersonalTraining, "Tactics", restDir)
        nomIni = os.path.join(nomDir, "Config.ini")
        if os.path.isfile(nomIni):
            dic = Util.ini8dic(nomIni)
            n = 1
            while True:
                if "TACTIC%d" % n in dic:
                    if "MENU" in dic["TACTIC%d" % n]:
                        if dic["TACTIC%d" % n]["MENU"].upper() == menuname.upper():
                            break
                    else:
                        break
                    n += 1
                else:
                    break
            nomTactic = "TACTIC%d" % n
        else:
            nomDirTac = os.path.join(VarGen.configuracion.dirPersonalTraining, "Tactics")
            Util.creaCarpeta(nomDirTac)
            Util.creaCarpeta(nomDir)
            nomTactic = "TACTIC1"
            dic = {}
        nomFNS = os.path.join(nomDir, nomTactic + ".fns")

        # Se crea el fichero con los puzzles
        f = codecs.open(nomFNS, "w", "utf-8", 'ignore')
        setPr = set()
        set_fathers_removed = set()
        for nline, liMVbase in enumerate(liT):
            if siBlancas:
                liMV = liMVbase[1:]
            else:
                liMV = liMVbase[2:]
            if not liMV:
                continue

            fenBase = liMV[0].fenBase()

            numJG = 1
            pgn = ""
            if not siBlancas:
                pgn += "%d..." % numJG
                numJG += 1
            colorAct = siBlancas
            if len(liMV) > depth:
                liMV = liMV[:depth]

            for mv in liMV:
                mas = ""

                if mv.adv() in dicVen[colorAct] or \
                                mv.nag() in dicVal[colorAct] or \
                                mv.father in set_fathers_removed:
                    set_fathers_removed.add(mv)
                    continue

                if colorAct == siBlancas:
                    liBrothers = mv.brothers()
                    for mv1 in liBrothers:
                        if mv1.adv() in dicVen[colorAct] or \
                                        mv1.nag() in dicVal[colorAct]:
                            continue
                        mas += "(%s)" % mv1.pgnEN()

                if colorAct:
                    pgn += " %d." % numJG
                    numJG += 1
                pgn += " " + mv.pgnEN()
                if mas:
                    pgn += " " + mas
                colorAct = not colorAct

            k = fenBase + pgn

            if k not in setPr:
                txt = ""
                if siAddOpening:
                    opening = listaAperturasStd.asignaAperturaListaMoves(liMVbase[1:])
                    if opening:
                        txt += "%s: %s<br>" % (_("Opening"), opening)
                txt += mv.comment()

                f.write("%s|%s|%s\n" % (fenBase, txt, pgn))
                setPr.add(k)
        f.close()

        # Se crea el fichero de control
        dic[nomTactic] = d = {}
        d["MENU"] = menuname
        d["FILESW"] = "%s.fns:100" % nomTactic

        d["PUZZLES"] = str(tactica.PUZZLES)
        for field in ("JUMPS", "REPEAT", "PENALIZATION", "SHOWTEXT"):
            d[field] = tactica.comas2texto(field)

        Util.dic8ini(nomIni, dic)

        self.procesador.entrenamientos.rehaz()
        um.final()
        QTUtil2.mensaje(self, _X(_("Tactic training %1 created."), menuname) + "<br>" +
                        _X(_("You can access this training from menu Trainings-Learn tactics by repetition-%1"),
                           name))

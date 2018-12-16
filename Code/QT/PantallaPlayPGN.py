from Code import Partida
from Code.QT import Colocacion
from Code.QT import Columnas
from Code.QT import Controles
from Code.QT import Grid
from Code.QT import Iconos
from Code.QT import PantallaPGN
from Code.QT import QTUtil2
from Code.QT import QTVarios
from Code import TrListas
from Code import Util


class PlayPGNs(Util.DicSQL):
    def __init__(self, fichero):
        Util.DicSQL.__init__(self, fichero)
        self.regKeys = self.keys(True, True)

    def leeRegistro(self, num):
        return self.__getitem__(self.regKeys[num])

    def append(self, valor):
        k = str(Util.hoy())
        self.__setitem__(k, valor)
        self.regKeys = self.keys(True, True)

    def appendHash(self, xhash, valor):
        """Usado desde databases-partidas, el hash = hash del xpv"""
        k = str(Util.hoy()) + "|" + str(xhash)
        self.__setitem__(k, valor)
        self.regKeys = self.keys(True, True)

    def recnoHash(self, xhash):
        """Usado desde databases-partidas"""
        for recno, key in enumerate(self.regKeys):
            if "|" in key:
                h = int(key.split("|")[1])
                if xhash == h:
                    return recno
        return None

    def cambiaRegistro(self, num, valor):
        self.__setitem__(self.regKeys[num], valor)

    def borraRegistro(self, num):
        self.__delitem__(self.regKeys[num])
        self.regKeys = self.keys(True, True)

    def borraLista(self, li):
        li.sort()
        li.reverse()
        for x in li:
            self.__delitem__(self.regKeys[x])
        self.pack()
        self.regKeys = self.keys(True, True)

    def rotulo(self, num):
        r = self.leeRegistro(num)

        def x(k):
            return r.get(k, "")

        date = x("DATE").replace(".?", "").replace("?", "")

        return "%s-%s : %s %s %s" % (x("WHITE"), x("BLACK"), date, x("EVENT"), x("SITE"))


class WPlayBase(QTVarios.WDialogo):
    def __init__(self, procesador):

        titulo = _("Play against a game")
        QTVarios.WDialogo.__init__(self, procesador.pantalla, titulo, Iconos.Law(), "playgame")

        self.procesador = procesador
        self.configuracion = procesador.configuracion
        self.recno = None

        self.db = PlayPGNs(self.configuracion.ficheroPlayPGN)

        # Historico
        oColumnas = Columnas.ListaColumnas()

        def creaCol(clave, rotulo, siCentrado=True):
            oColumnas.nueva(clave, rotulo, 80, siCentrado=siCentrado)

        # # Claves segun orden estandar
        liBasic = ("EVENT", "SITE", "DATE", "ROUND", "WHITE", "BLACK", "RESULT", "ECO", "FEN", "WHITEELO", "BLACKELO")
        for clave in liBasic:
            rotulo = TrListas.pgnLabel(clave)
            creaCol(clave, rotulo, clave != "EVENT")
        self.grid = Grid.Grid(self, oColumnas, siSelecFilas=True, siSeleccionMultiple=True)
        self.grid.setMinimumWidth(self.grid.anchoColumnas() + 20)

        # Tool bar
        liAcciones = (
            (_("Close"), Iconos.MainMenu(), self.terminar), None,
            (_("Play"), Iconos.Empezar(), self.empezar),
            (_("New"), Iconos.Nuevo(), self.nuevo), None,
            (_("Remove"), Iconos.Borrar(), self.borrar), None,
        )
        self.tb = QTVarios.LCTB(self, liAcciones)

        # Colocamos
        lyTB = Colocacion.H().control(self.tb).margen(0)
        ly = Colocacion.V().otro(lyTB).control(self.grid).margen(3)

        self.setLayout(ly)

        self.registrarGrid(self.grid)
        self.recuperarVideo(siTam=False)

        self.grid.gotop()

    def gridDobleClick(self, grid, fila, columna):
        self.empezar()

    def gridNumDatos(self, grid):
        return len(self.db)

    def gridDato(self, grid, fila, oColumna):
        col = oColumna.clave
        reg = self.db.leeRegistro(fila)
        return reg.get(col, "")

    def terminar(self):
        self.guardarVideo()
        self.db.close()
        self.accept()

    def closeEvent(self, QCloseEvent):
        self.guardarVideo()
        self.db.close()

    def nuevo(self):
        unpgn = PantallaPGN.eligePartida(self)
        if unpgn and unpgn.partida.numJugadas():
            reg = unpgn.dic
            unpgn.partida.siTerminada()
            reg["PARTIDA"] = unpgn.partida.guardaEnTexto()
            self.db.append(reg)
            self.grid.refresh()
            self.grid.gotop()

    def borrar(self):
        li = self.grid.recnosSeleccionados()
        if len(li) > 0:
            if QTUtil2.pregunta(self, _("Do you want to delete all selected records?")):
                self.db.borraLista(li)
        self.grid.gotop()
        self.grid.refresh()

    def empezar(self):
        li = self.grid.recnosSeleccionados()
        if len(li) > 0:
            recno = li[0]
            w = WPlay1(self, self.configuracion, self.db, recno)
            if w.exec_():
                self.recno = recno
                self.siBlancas = w.siBlancas
                self.accept()


class WPlay1(QTVarios.WDialogo):
    def __init__(self, owner, configuracion, db, recno):

        QTVarios.WDialogo.__init__(self, owner, _("Play against a game"), Iconos.PlayGame(), "play1game")

        self.owner = owner
        self.db = db
        self.configuracion = configuracion
        self.recno = recno
        self.registro = self.db.leeRegistro(recno)

        self.partida = Partida.Partida()
        self.partida.recuperaDeTexto(self.registro["PARTIDA"])

        self.lbRotulo = Controles.LB(self, self.db.rotulo(recno)).ponTipoLetra(puntos=12).ponColorFondoN("#076C9F", "#EFEFEF")

        self.liIntentos = self.registro.get("LIINTENTOS", [])

        oColumnas = Columnas.ListaColumnas()
        oColumnas.nueva("DATE", _("Date"), 80, siCentrado=True)
        oColumnas.nueva("COLOR", _("Play with"), 80, siCentrado=True)
        oColumnas.nueva("POINTS", _("Points"), 80, siCentrado=True)
        oColumnas.nueva("TIME", _("Time"), 80, siCentrado=True)
        self.grid = Grid.Grid(self, oColumnas, siSelecFilas=True, siSeleccionMultiple=True)
        self.grid.setMinimumWidth(self.grid.anchoColumnas() + 20)

        # Tool bar
        liAcciones = (
            (_("Close"), Iconos.MainMenu(), self.terminar), None,
            (_("Train"), Iconos.Entrenar(), self.empezar), None,
            (_("Remove"), Iconos.Borrar(), self.borrar), None,
        )
        self.tb = QTVarios.LCTB(self, liAcciones)

        # Colocamos
        lyTB = Colocacion.H().control(self.tb).margen(0)
        ly = Colocacion.V().otro(lyTB).control(self.grid).control(self.lbRotulo).margen(3)

        self.setLayout(ly)

        self.registrarGrid(self.grid)
        self.recuperarVideo(siTam=False)

        self.grid.gotop()

    def gridNumDatos(self, grid):
        return len(self.liIntentos)

    def gridDato(self, grid, fila, oColumna):
        col = oColumna.clave
        reg = self.liIntentos[fila]

        if col == "DATE":
            f = reg["DATE"]
            return "%02d/%02d/%d-%02d:%02d" % (f.day, f.month, f.year, f.hour, f.minute)
        if col == "COLOR":
            c = reg["COLOR"]
            if c == "b":
                return _("Black")
            elif c == "w":
                return _("White")
        if col == "POINTS":
            return "%d (%d)" % (reg["POINTS"], reg["POINTSMAX"])
        if col == "TIME":
            s = int(reg["TIME"])
            m = int(s / 60)
            s -= m * 60
            return "%d\' %d\"" % (m, s)

    def guardar(self, dic):
        self.liIntentos.insert(0, dic)
        self.grid.refresh()
        self.grid.gotop()
        self.registro["LIINTENTOS"] = self.liIntentos
        self.db.cambiaRegistro(self.numRegistro, self.registro)

    def terminar(self, siAccept=False):
        self.guardarVideo()
        if siAccept:
            self.accept()
        else:
            self.reject()

    def borrar(self):
        li = self.grid.recnosSeleccionados()
        if len(li) > 0:
            if QTUtil2.pregunta(self, _("Do you want to delete all selected records?")):
                li.sort()
                li.reverse()
                for x in li:
                    del self.liIntentos[x]
        self.grid.gotop()
        self.grid.refresh()

    def empezar(self):
        self.siBlancas = QTVarios.blancasNegras(self)
        self.terminar(True)

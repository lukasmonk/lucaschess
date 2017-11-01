import copy
import collections
import time

from Code.QT import TabTipos
from Code import TrListas
from Code import Util


class PFlecha(TabTipos.Flecha):
    def __init__(self):
        TabTipos.Flecha.__init__(self)
        self.nombre = ""
        self.id = None


class PMarco(TabTipos.Marco):
    def __init__(self):
        TabTipos.Marco.__init__(self)
        self.nombre = ""
        self.id = None


class PSVG(TabTipos.SVG):
    def __init__(self):
        TabTipos.SVG.__init__(self)
        self.nombre = ""
        self.id = None


class PMarker(TabTipos.Marker):
    def __init__(self):
        TabTipos.Marker.__init__(self)
        self.nombre = ""
        self.id = None

TP_FLECHA, TP_MARCO, TP_TEXTO, TP_SVG, TP_MARKER, TP_PIEZACREA, TP_PIEZAMUEVE, TP_PIEZABORRA, TP_ACTION, TP_CONFIGURATION = "F", "M", "T", "S", "X", "PC", "PM", "PB", "A", "C"


class GTarea:
    def __init__(self, guion, tp):
        self.guion = guion
        self._id = Util.nuevoID()
        self._tp = tp
        self._marcado = False
        self._orden = 0
        self._nombre = None
        self._registro = None
        self.xmarcadoOwner = False

    def id(self):
        return self._id

    def tp(self):
        return self._tp

    def marcado(self, si=None):
        if si is not None:
            self._marcado = bool(si)
        return self._marcado

    def marcadoOwner(self, si=None):
        if si is not None:
            self.xmarcadoOwner = bool(si)
        return self.xmarcadoOwner

    def nombre(self, nombre=None):
        if nombre is not None:
            self._nombre = nombre
        return self._nombre if self._nombre else ""

    def registro(self, valores=None):
        if valores:
            self._registro = valores
        return self._registro

    def guarda(self):
        reg = {}
        for atr in dir(self):
            if atr.startswith("_") and not atr.startswith("__"):
                if atr == "_itemSC":
                    reg["_bloqueDatos"] = self._itemSC.bloqueDatos
                else:
                    valor = getattr(self, atr)
                    reg[atr] = valor
        return reg

    def recupera(self, reg):
        for atr in reg:
            if atr.startswith("_") and not atr.startswith("__") and atr != "_id":
                valor = reg[atr]
                setattr(self, atr, valor)


class GT_Item(GTarea):
    def __init__(self, guion, tp):
        GTarea.__init__(self, guion, tp)
        self._itemSC = None
        self._bloqueDatos = None
        self.xitemSCOwner = None

    def itemSC(self, sc=None):
        if sc is not None:
            self._itemSC = sc
        return self._itemSC

    def borraItemSCOwner(self):
        self.xitemSCOwner = None
        self.marcadoOwner(False)

    def itemSCOwner(self, sc=None):
        if sc is not None:
            self.xitemSCOwner = sc
        return self.xitemSCOwner

    def a1h8(self):
        bd = self._itemSC.bloqueDatos
        return bd.a1h8

    def bloqueDatos(self):
        return self._itemSC.bloqueDatos

    def nombre(self, nombre=None):
        if nombre is not None:
            self._nombre = nombre
        if self._nombre:
            return self._nombre
        return self._nombre if self._nombre else getattr(self._itemSC.bloqueDatos, "nombre", "")

    def coordina(self):
        if self.xitemSCOwner:
            if self.tp() == "S":
                self.xitemSCOwner.coordinaPosicionOtro(self._itemSC)
                self.xitemSCOwner.update()
            else:
                bf = copy.deepcopy(self._itemSC.bloqueDatos)
                bf.anchoCasilla = self.xitemSCOwner.bloqueDatos.anchoCasilla
                self.xitemSCOwner.bloqueDatos = bf
                self.xitemSCOwner.reset()
            self.xitemSCOwner.escena.update()


class GT_Texto(GTarea):
    def __init__(self, guion):
        GTarea.__init__(self, guion, TP_TEXTO)
        self._texto = None
        self._continuar = None

    def texto(self, txt=None):
        if txt is not None:
            self._texto = txt
        return self._texto

    def continuar(self, ok=None):
        if ok is not None:
            self._continuar = ok
        return self._continuar

    def info(self):
        mas_texto = "? " if self._continuar else ""
        if not self._texto:
            return mas_texto
        if "</head>" in self._texto:
            li = self._texto.split("</head>")[1].split("<")
            for n in range(len(li)):
                li1 = li[n].split(">")
                if len(li1) == 2:
                    li[n] = li1[1]
            return mas_texto + "".join(li)
        else:
            return mas_texto + self._texto

    def txt_tipo(self):
        return _("Text")

    def run(self):
        self.guion.writePizarra(self)


class GT_Flecha(GT_Item):
    def __init__(self, guion):
        GT_Item.__init__(self, guion, TP_FLECHA)

    def txt_tipo(self):
        return _("Arrow")

    def info(self):
        bd = self._itemSC.bloqueDatos
        return bd.a1h8

    def run(self):
        if self._itemSC:
            self._itemSC.show()

        sc = self.guion.tablero.creaFlecha(self._bloqueDatos)
        sc.ponRutinaPulsada(None, self.id())
        self.itemSC(sc)
        self.marcado(True)


class GT_Marco(GT_Item):
    def __init__(self, guion):
        GT_Item.__init__(self, guion, TP_MARCO)

    def txt_tipo(self):
        return _("Box")

    def info(self):
        bd = self._itemSC.bloqueDatos
        return bd.a1h8

    def run(self):
        if self._itemSC:
            self._itemSC.show()

        sc = self.guion.tablero.creaMarco(self._bloqueDatos)
        sc.ponRutinaPulsada(None, self.id())
        self.itemSC(sc)
        self.marcado(True)


class GT_SVG(GT_Item):
    def __init__(self, guion):
        GT_Item.__init__(self, guion, TP_SVG)

    def txt_tipo(self):
        return _("Image")

    def info(self):
        return "(%.02f,%.02f)-(%.02f,%.02f)" % self.get_datos()

    def get_datos(self):
        bd = self._itemSC.bloqueDatos
        p = bd.posicion

        def f(n):
            return float(n * 1.0 / bd.anchoCasilla)

        return (f(p.x), f(p.y), f(p.ancho), f(p.alto))

    def set_datos(self, col, fil, ancho, alto):
        bd = self._itemSC.bloqueDatos
        p = bd.posicion

        def f(n):
            return float(n * bd.anchoCasilla)

        p.x = f(col)
        p.y = f(fil)
        p.ancho = f(ancho)
        p.alto = f(alto)

    def run(self):
        if self._itemSC:
            self._itemSC.show()

        siEditando = self.guion.siEditando()

        sc = self.guion.tablero.creaSVG(self._bloqueDatos, siEditando=siEditando)
        sc.ponRutinaPulsada(None, self.id())
        sc.bloqueDatos = self._bloqueDatos # necesario para svg con posicion no ajustado a casillas
        sc.update()
        self.itemSC(sc)
        self.marcado(True)


class GT_Marker(GT_Item):
    def __init__(self, guion):
        GT_Item.__init__(self, guion, TP_MARKER)

    def txt_tipo(self):
        return _("Marker")

    def info(self):
        bd = self._itemSC.bloqueDatos
        return bd.a1h8

    def run(self):
        if self._itemSC:
            self._itemSC.show()

        siEditando = self.guion.siEditando()
        sc = self.guion.tablero.creaMarker(self._bloqueDatos, siEditando=siEditando)
        self.itemSC(sc)
        self.marcado(True)


class GT_Action(GTarea):
    GTA_INICIO, GTA_MAINARROW_REMOVE, GTA_PIECES_REMOVEALL, GTA_GRAPHICS_REMOVEALL, GTA_PIZARRA_REMOVE = "I", "MAR", "PRA", "GRA", "PR"
    dicTxt = {
                GTA_INICIO:_("Initial position"),
                GTA_MAINARROW_REMOVE: _("Remove main arrow"),
                GTA_PIECES_REMOVEALL:_("Remove all pieces"),
                GTA_GRAPHICS_REMOVEALL:_("Remove all graphics"),
                GTA_PIZARRA_REMOVE: _("Remove text")
    }

    def __init__(self, guion):
        GTarea.__init__(self, guion, TP_ACTION)
        self._action = None

    def action(self, action=None):
        if action:
            self._action = action
        return self._action

    def txt_tipo(self):
        return _("Action")

    def info(self):
        return self.dicTxt[self._action] if self._action else "?"

    def run(self):
        guion = self.guion
        tablero = guion.tablero
        if self._action == self.GTA_INICIO:
            guion.restoreTablero()
        elif self._action == self.GTA_MAINARROW_REMOVE:
            if tablero.flechaSC:
                tablero.flechaSC.hide()
        elif self._action == self.GTA_PIECES_REMOVEALL:
            tablero.removePieces()
        elif self._action == self.GTA_GRAPHICS_REMOVEALL:
            tablero.borraMovibles()
        elif self._action == self.GTA_PIZARRA_REMOVE:
            guion.cierraPizarra()


class GT_Configuration(GTarea):
    GTC_TRANSITION, GTC_NEXT_TRANSITION = "T", "NT"
    dicTxt = collections.OrderedDict({
                GTC_TRANSITION:_("General transition time"),
                GTC_NEXT_TRANSITION:_("Next transition time"),
            })
    def __init__(self, guion):
        GTarea.__init__(self, guion, TP_CONFIGURATION)
        self._configuration = None
        self._value = 0

    def configuration(self, configuration=None):
        if configuration:
            self._configuration = configuration
        return self._configuration

    def value(self, value=None):
        if type(value) == int:
            self._value = value
        return self._value

    def txt_tipo(self):
        return _("Configuration")

    def info(self):
        return "%d=%s"%(self._value, self.dicTxt[self._configuration] if self._configuration else "?")

    def run(self):
        guion = self.guion
        if self._configuration == self.GTC_TRANSITION:
            guion.transition = self._value
        elif self._configuration == self.GTC_NEXT_TRANSITION:
            guion.nextTransition = self._value


class GT_PiezaMueve(GTarea):
    def __init__(self, guion):
        GTarea.__init__(self, guion, TP_PIEZAMUEVE)
        self._desde = None
        self._hasta = None
        self._borra = None

    def setPosicion(self, posicion):
        self._posicion = posicion

    def posicion(self):
        return self._posicion

    def desdeHastaBorra(self, desde=None, hasta=None, pieza_borra=None):
        if desde is not None:
            self._desde = desde
            self._hasta = hasta
            self._borra = pieza_borra
        return self._desde, self._hasta, self._borra

    def txt_tipo(self):
        return _("Move piece")

    def info(self):
        return self._desde + " -> " + self._hasta


class GT_PiezaCrea(GTarea):
    def __init__(self, guion):
        GTarea.__init__(self, guion, TP_PIEZACREA)
        self._pieza = None
        self._desde = None
        self._borra = None

    def desde(self, desde=None, borra=None):
        if desde is not None:
            self._desde = desde
            self._borra = borra
        return self._desde, self._borra

    def pieza(self, pz=None):
        if pz is not None:
            self._pieza = pz
        return self._pieza

    def txt_tipo(self):
        return _("Create piece")

    def info(self):
        pz = TrListas.letterPiece(self._pieza)
        return (pz if pz.isupper() else pz.lower()) + " -> " + self._desde


class GT_PiezaBorra(GTarea):
    def __init__(self, guion):
        GTarea.__init__(self, guion, TP_PIEZABORRA)
        self._pieza = None
        self._desde = None

    def desde(self, desde=None):
        if desde is not None:
            self._desde = desde
        return self._desde

    def pieza(self, pz=None):
        if pz is not None:
            self._pieza = pz
        return self._pieza

    def txt_tipo(self):
        return _("Delete piece")

    def info(self):
        pz = TrListas.letterPiece(self._pieza)
        return (pz if pz.isupper() else pz.lower()) + " -> " + self._desde


class Guion:
    def __init__(self, tablero, winDirector=None):
        self.liGTareas = []
        self.pizarra = None
        self.anchoPizarra = 250
        self.posPizarra = "R"
        self.tablero = tablero
        self.winDirector = winDirector
        self.saveTablero()
        self.cerrado = False

    def siEditando(self):
        return self.winDirector is not None

    def saveTablero(self):
        self.tablero_ultPosicion = self.tablero.ultPosicion
        self.tablero_siBlancasAbajo = self.tablero.siBlancasAbajo
        if self.tablero.flechaSC and self.tablero.flechaSC.isVisible():
            a1h8 = self.tablero.flechaSC.bloqueDatos.a1h8
            self.tablero_flechaSC = a1h8[:2], a1h8[2:]
        else:
            self.tablero_flechaSC = None

        if self.winDirector:
            if not hasattr(self, "tablero_mensajero") or self.tablero_mensajero != self.winDirector.muevePieza:
                self.tablero_mensajero = self.tablero.mensajero
                self.tablero.mensajero = self.winDirector.muevePieza

        self.tablero_activasPiezas = self.tablero.siActivasPiezas, self.tablero.siActivasPiezasColor

    def restoreTablero(self):
        self.tablero.dirvisual = None
        self.tablero.ponPosicion(self.tablero_ultPosicion)
        if self.tablero_flechaSC:
            desde, hasta  = self.tablero_flechaSC
            self.tablero.ponFlechaSC(desde, hasta)
        if self.winDirector:
            self.tablero.mensajero = self.tablero_mensajero
        if self.tablero_activasPiezas[0]:
            self.tablero.activaColor(self.tablero_activasPiezas[1])
        self.tablero.siDirector = True
        self.cierraPizarra()

    def nuevaTarea(self, tarea, fila=-1):
        if fila == -1:
            self.liGTareas.append(tarea)
            fila = len(self.liGTareas) - 1
        else:
            self.liGTareas.insert(fila, tarea)
        return fila

    def savedPizarra(self):
        self.winDirector.refresh_guion()
        self.winDirector.ponSiGrabar()

    def writePizarra(self, tarea):
        if self.pizarra is None:
            self.pizarra = TabTipos.Pizarra(self, self.tablero, self.anchoPizarra,
                                            editMode=self.winDirector is not None,
                                            withContinue=tarea.continuar())
            self.pizarra.mensaje.setFocus()
        self.pizarra.write(tarea)
        self.pizarra.show()

    def cierraPizarra(self):
        if self.pizarra:
            self.pizarra.close()
            self.pizarra = None

    def nuevaCopia(self, ntarea):
        tarea = copy.copy(self.tarea(ntarea))
        tarea._id = Util.nuevoID()
        return self.nuevaTarea(tarea, ntarea + 1)

    def borra(self, nTarea):
        del self.liGTareas[nTarea]

    def cambiaMarcaTarea(self, nTarea, valor):
        tarea = self.liGTareas[nTarea]
        tarea.marcado(valor)
        return tarea

    def cambiaMarcaTareaOwner(self, nTarea, valor):
        tarea = self.liGTareas[nTarea]
        tarea.marcadoOwner(valor)
        return tarea

    def tareaItem(self, item):
        for n, tarea in enumerate(self.liGTareas):
            if isinstance(tarea, GT_Item) and tarea.itemSC() == item:
                return tarea, n
        return None, -1

    def tareasPosicion(self, pos):
        li = []
        for n, tarea in enumerate(self.liGTareas):
            if isinstance(tarea, GT_Item) and tarea.itemSC().contiene(pos):
                li.append((n, tarea))
        return li

    def itemTarea(self, nTarea):
        tarea = self.liGTareas[nTarea]
        return tarea.itemSC() if isinstance(tarea, GT_Item) else None

    def itemTareaOwner(self, nTarea):
        tarea = self.liGTareas[nTarea]
        return tarea.itemSCOwner() if isinstance(tarea, GT_Item) else None

    def borraItemTareaOwner(self, nTarea):
        tarea = self.liGTareas[nTarea]
        if isinstance(tarea, GT_Item):
            tarea.borraItemSCOwner()

    def marcado(self, nTarea):
        return self.liGTareas[nTarea].marcado()

    def marcadoOwner(self, nTarea):
        return self.liGTareas[nTarea].marcadoOwner()

    def desmarcaItem(self, item):
        for tarea in self.liGTareas:
            if isinstance(tarea, GT_Item) and tarea._itemSC == item:
                tarea.marcado(False)
                return

    def id(self, nTarea):
        return self.liGTareas[nTarea].id()

    def tarea(self, nTarea):
        return self.liGTareas[nTarea]

    def arriba(self, nTarea):
        if nTarea > 0:
            self.liGTareas[nTarea], self.liGTareas[nTarea - 1] = self.liGTareas[nTarea - 1], self.liGTareas[nTarea]
            return True
        else:
            return False

    def abajo(self, nTarea):
        if nTarea < (len(self.liGTareas) - 1):
            self.liGTareas[nTarea], self.liGTareas[nTarea + 1] = self.liGTareas[nTarea + 1], self.liGTareas[nTarea]
            return True
        else:
            return False

    def __len__(self):
        return len(self.liGTareas)

    def txt_tipo(self, fila):
        tarea = self.liGTareas[fila]
        return tarea.txt_tipo()

    def nombre(self, fila):
        tarea = self.liGTareas[fila]
        return tarea.nombre()

    def info(self, fila):
        tarea = self.liGTareas[fila]
        return tarea.info()

    def guarda(self):
        lista = []
        for tarea in self.liGTareas:
            lista.append(tarea.guarda())
        return lista

    def recuperaReg(self, reg):
        dic = {
            TP_FLECHA: GT_Flecha, TP_MARCO: GT_Marco, TP_SVG: GT_SVG, TP_MARKER: GT_Marker,
            TP_TEXTO: GT_Texto,
            TP_PIEZACREA: GT_PiezaCrea, TP_PIEZAMUEVE: GT_PiezaMueve, TP_PIEZABORRA: GT_PiezaBorra,
            TP_ACTION: GT_Action, TP_CONFIGURATION: GT_Configuration,
        }
        tarea = dic[reg["_tp"]](self)
        tarea.recupera(reg)
        self.nuevaTarea(tarea, -1)
        return tarea

    def recupera(self):
        fenM2 = self.tablero.ultPosicion.fenM2()
        lista = self.tablero.configuracion.dbFEN(fenM2)
        self.liGTareas = []
        if lista is None:
            return

        for reg in lista:
            self.recuperaReg(reg)

        if self.winDirector:
            for tarea in self.liGTareas:
                if tarea.tp() not in (TP_ACTION, TP_CONFIGURATION, TP_TEXTO):
                    tarea.run()
                    tarea.marcado(True)
                else:
                    tarea.marcado(False)

    def play(self):
        self.cerrado = False
        for tarea in self.liGTareas:
            tarea.run()
            if tarea.tp() == TP_TEXTO and tarea.continuar():
                while self.pizarra is not None and self.pizarra.siBloqueada():
                    time.sleep(0.05)
            if self.cerrado:
                return


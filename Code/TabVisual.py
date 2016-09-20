import copy

from Code import Jugada
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

TP_FLECHA, TP_MARCO, TP_TEXTO, TP_SVG, TP_MARKER, TP_JUGADA, TP_POSICION, TP_PIEZACREA, TP_PIEZAMUEVE, TP_PIEZABORRA, TP_SONIDO = "F", "M", "T", "S", "X", "J", "PO", "PC", "PM", "PB", "SO"

class GTarea:
    def __init__(self, tp):
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
        reg = Util.Almacen()
        for atr in dir(self):
            if atr.startswith("_") and not atr.startswith("__"):
                if atr == "_itemSC":
                    reg._bloqueDatos = self._itemSC.bloqueDatos
                else:
                    valor = getattr(self, atr)
                    setattr(reg, atr, valor)
        return reg

    def recupera(self, reg):

        for atr in dir(reg):
            if atr.startswith("_") and not atr.startswith("__") and atr != "_id":
                valor = getattr(reg, atr)
                setattr(self, atr, valor)

class GT_Item(GTarea):
    def __init__(self, tp):
        GTarea.__init__(self, tp)
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
        return self._nombre if self._nombre else self._itemSC.bloqueDatos.nombre

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

class GT_Texto(GT_Item):
    def __init__(self):
        GT_Item.__init__(self, TP_TEXTO)
        self._texto = None
        self._familia = None
        self._tam = 100

    def texto(self, txt=None):
        if txt is not None:
            self._texto = txt
        return self._texto

    def familia(self, fam=None):
        if fam is not None:
            self._familia = fam
        return self._familia

    def tam(self, tam=None):
        if tam is not None:
            return self._tam
        return self._tam

class GT_Flecha(GT_Item):
    def __init__(self):
        GT_Item.__init__(self, TP_FLECHA)

    def txt_tipo(self):
        return _("Arrow")

    def info(self):
        bd = self._itemSC.bloqueDatos
        return bd.a1h8

class GT_Marco(GT_Item):
    def __init__(self):
        GT_Item.__init__(self, TP_MARCO)

    def txt_tipo(self):
        return _("Box")

    def info(self):
        bd = self._itemSC.bloqueDatos
        return bd.a1h8

class GT_SVG(GT_Item):
    def __init__(self):
        GT_Item.__init__(self, TP_SVG)

    def txt_tipo(self):
        return _("Image")

    def info(self):
        bd = self._itemSC.bloqueDatos
        p = bd.posicion

        def f(n):
            return float(n * 1.0 / bd.anchoCasilla)

        return "(%.02f,%.02f)-(%.02f,%.02f)" % (f(p.x), f(p.y), f(p.ancho), f(p.alto))

class GT_Marker(GT_Item):
    def __init__(self):
        GT_Item.__init__(self, TP_MARKER)

    def txt_tipo(self):
        return _("Marker")

    def info(self):
        bd = self._itemSC.bloqueDatos
        return bd.a1h8

class GT_Posicion(GTarea):
    def __init__(self):
        GTarea.__init__(self, TP_POSICION)
        self._fen = None
        self._fenAnterior = None

    def fen(self, fen=None):
        if fen:
            self._fen = fen
        return self._fen

    def fenAnterior(self, fen=None):
        if fen:
            self._fenAnterior = fen
        return self._fenAnterior

    def txt_tipo(self):
        return _("Start position")

    def info(self):
        return self._fen if self._fen else ""

class GT_PiezaMueve(GTarea):
    def __init__(self):
        GTarea.__init__(self, TP_PIEZAMUEVE)
        self._desde = None
        self._hasta = None

    def desdeHasta(self, desde=None, hasta=None):
        if desde is not None:
            self._desde = desde
            self._hasta = hasta
        return self._desde, self._hasta

    def txt_tipo(self):
        return _("Move piece")

    def info(self):
        return self._desde + " -> " + self._hasta

class GT_PiezaCrea(GTarea):
    def __init__(self):
        GTarea.__init__(self, TP_PIEZACREA)
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
        return _("Create piece")

    def info(self):
        pz = TrListas.letterPiece(self._pieza)
        return (pz if pz.isupper() else pz.lower()) + " -> " + self._desde

class GT_PiezaBorra(GTarea):
    def __init__(self):
        GTarea.__init__(self, TP_PIEZABORRA)
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

class GT_Jugada(GTarea):
    def __init__(self):
        GTarea.__init__(self, TP_JUGADA)
        self._jg = None

    def jugada(self, jg=None):
        if jg is not None:
            self._jg = jg
        return self._jg

    def txt_tipo(self):
        return _("Move")

    def info(self):
        return self._jg.etiquetaSP()

    def guarda(self):
        reg = Util.Almacen()
        for atr in dir(self):
            if atr.startswith("_") and not atr.startswith("__"):
                if atr == "_jg":
                    reg._jg = self._jg.guardaEnTexto()
                else:
                    valor = getattr(self, atr)
                    setattr(reg, atr, valor)
        return reg

    def recupera(self, reg):
        for atr in dir(reg):
            if atr.startswith("_") and not atr.startswith("__"):
                if atr == "_jg":
                    self._jg = Jugada.Jugada()
                    self._jg.recuperaDeTexto(reg._jg)
                else:
                    valor = getattr(reg, atr)
                    setattr(self, atr, valor)

class Guion:
    def __init__(self):
        self.liGTareas = []

    def nuevaTarea(self, tarea, fila=-1):
        if fila == -1:
            self.liGTareas.append(tarea)
            fila = len(self.liGTareas) - 1
        else:
            self.liGTareas.insert(fila, tarea)
        return fila

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
        li = []
        for tarea in self.liGTareas:
            li.append(tarea.guarda())
        return li

    def recuperaReg(self, reg):
        dic = {TP_FLECHA: GT_Flecha, TP_MARCO: GT_Marco, TP_TEXTO: GT_Texto, TP_SVG: GT_SVG, TP_MARKER: GT_Marker,
               TP_JUGADA: GT_Jugada, TP_POSICION: GT_Posicion, TP_PIEZACREA: GT_PiezaCrea,
               TP_PIEZAMUEVE: GT_PiezaMueve, TP_PIEZABORRA: GT_PiezaBorra}
        tarea = dic[reg._tp]()
        tarea.recupera(reg)
        self.nuevaTarea(tarea, -1)
        return tarea

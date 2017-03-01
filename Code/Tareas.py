import copy

from PyQt4 import QtCore

VTEXTO = 0
VENTERO = 1
VDECIMAL = 2


class Variable:
    def __init__(self, nombre, tipo, inicial):
        self.id = None
        self.nombre = nombre
        self.tipo = tipo
        self.inicial = inicial
        self.valor = inicial
        self.id = None


class Tarea:
    def enlaza(self, cpu):
        self.cpu = cpu
        self.id = cpu.nuevaID()
        self.junks = cpu.junks
        self.padre = 0


class TareaDuerme(Tarea):
    def __init__(self, segundos):
        self.segundos = segundos

    def enlaza(self, cpu):
        Tarea.enlaza(self, cpu)
        self.totalPasos = int(self.segundos * self.junks)

        self.pasoActual = 0

    def unPaso(self):
        self.pasoActual += 1
        return self.pasoActual >= self.totalPasos  # si es ultimo

    def __str__(self):
        return "DUERME %0.2f" % self.segundos


class TareaToolTip(Tarea):
    def __init__(self, texto):
        self.texto = texto

    def unPaso(self):
        self.cpu.tablero.setToolTip(self.texto)
        return True

    def __str__(self):
        return "TOOLTIP %s" % self.texto


class TareaPonPosicion(Tarea):
    def __init__(self, posicion):
        self.posicion = posicion

    def unPaso(self):
        self.cpu.tablero.ponPosicion(self.posicion)
        return True

    def __str__(self):
        return self.posicion.fen()


class TareaCambiaPieza(Tarea):
    def __init__(self, a1h8, pieza):
        self.a1h8 = a1h8
        self.pieza = pieza

    def unPaso(self):
        self.cpu.tablero.cambiaPieza(self.a1h8, self.pieza)
        return True

    def __str__(self):
        return _X(_("Change piece in %1 to %2"), self.a1h8, self.pieza)

    def directo(self, tablero):
        return tablero.cambiaPieza(self.a1h8, self.pieza)


class TareaBorraPieza(Tarea):
    def __init__(self, a1h8, tipo=None):
        self.a1h8 = a1h8
        self.tipo = tipo

    def unPaso(self):
        if self.tipo:
            self.cpu.tablero.borraPiezaTipo(self.a1h8, self.tipo)
        else:
            self.cpu.tablero.borraPieza(self.a1h8)
        return True

    def __str__(self):
        return _X(_("Remove piece on %1"), self.a1h8)

    def directo(self, tablero):
        tablero.borraPieza(self.a1h8)


class TareaMuevePieza(Tarea):
    def __init__(self, desdeA1H8, hastaA1H8, segundos=0.0):
        self.pieza = None
        self.desdeA1H8 = desdeA1H8
        self.hastaA1H8 = hastaA1H8
        self.segundos = segundos

    def enlaza(self, cpu):
        Tarea.enlaza(self, cpu)

        self.tablero = self.cpu.tablero

        dx, dy = self.a1h8_xy(self.desdeA1H8)
        hx, hy = self.a1h8_xy(self.hastaA1H8)

        linea = QtCore.QLineF(dx, dy, hx, hy)

        pasos = int(self.segundos * self.junks)
        self.liPuntos = []
        for x in range(1, pasos + 1):
            self.liPuntos.append(linea.pointAt(float(x) / pasos))
        self.nPaso = 0
        self.totalPasos = len(self.liPuntos)

    def a1h8_xy(self, a1h8):
        fila = int(a1h8[1])
        columna = ord(a1h8[0]) - 96
        x = self.tablero.columna2punto(columna)
        y = self.tablero.fila2punto(fila)
        return x, y

    def unPaso(self):
        if self.pieza is None:
            self.pieza = self.tablero.damePiezaEn(self.desdeA1H8)
            if self.pieza is None:
                return True
        p = self.liPuntos[self.nPaso]
        bp = self.pieza.bloquePieza
        bp.posicion.x = p.x()
        bp.posicion.y = p.y()
        self.pieza.rehazPosicion()
        self.nPaso += 1
        siUltimo = self.nPaso >= self.totalPasos
        if siUltimo:
            # Para que este al final en la posicion correcta
            self.tablero.colocaPieza(bp, self.hastaA1H8)
        return siUltimo

    def __str__(self):
        return _X(_("Move piece from %1 to %2 on %3 second (s)"), self.desdeA1H8, self.hastaA1H8,
                  "%0.2f" % self.segundos)

    def directo(self, tablero):
        tablero.muevePieza(self.desdeA1H8, self.hastaA1H8)


class TareaMuevePiezaV(TareaMuevePieza):
    def __init__(self, desdeA1H8, hastaA1H8, vsegundos):
        TareaMuevePieza.__init__(self, desdeA1H8, hastaA1H8, 0.0)
        self.vsegundos = vsegundos

    def enlaza(self, cpu):
        self.segundos = self.vsegundos.valor
        TareaMuevePieza.enlaza(self, cpu)

    def __str__(self):
        return _X(_("Move piece from %1 to %2 on %3 second (s)"), self.desdeA1H8, self.hastaA1H8,
                  "%0.2f (%s)" % (self.vsegundos.valor, self.vsegundos.nombre))

    def directo(self, tablero):
        tablero.muevePieza(self.desdeA1H8, self.hastaA1H8)


class TareaMuevePiezaLI(Tarea):
    def __init__(self, lista, segundos):
        self.lista = lista
        self.segundos = segundos

    def enlaza(self, cpu):
        Tarea.enlaza(self, cpu)
        self.tablero = self.cpu.tablero
        self.pieza = None
        self.desdeA1H8 = self.lista[0][0]
        self.hastaA1H8 = self.lista[-1][1]
        self.liPuntos = []
        pasos1 = int(self.segundos * self.junks / len(self.lista))

        for desdeA1H8, hastaA1H8 in self.lista:
            dx, dy = self.a1h8_xy(desdeA1H8)
            hx, hy = self.a1h8_xy(hastaA1H8)

            linea = QtCore.QLineF(dx, dy, hx, hy)

            for x in range(1, pasos1 + 1):
                self.liPuntos.append(linea.pointAt(float(x) / pasos1))
        self.nPaso = 0
        self.totalPasos = len(self.liPuntos)

    def a1h8_xy(self, a1h8):
        fila = int(a1h8[1])
        columna = ord(a1h8[0]) - 96
        x = self.tablero.columna2punto(columna)
        y = self.tablero.fila2punto(fila)
        return x, y

    def unPaso(self):
        if self.pieza is None:
            self.pieza = self.tablero.damePiezaEn(self.desdeA1H8)
        p = self.liPuntos[self.nPaso]
        bp = self.pieza.bloquePieza
        bp.posicion.x = p.x()
        bp.posicion.y = p.y()
        self.pieza.rehazPosicion()
        self.nPaso += 1
        siUltimo = self.nPaso >= self.totalPasos
        if siUltimo:
            # Para que este al final en la posicion correcta
            self.tablero.colocaPieza(bp, self.hastaA1H8)
        return siUltimo


class TareaCreaFlecha(Tarea):
    def __init__(self, tutorial, desde, hasta, idFlecha):
        self.tutorial = tutorial
        self.idFlecha = idFlecha
        self.desde = desde
        self.hasta = hasta
        self.scFlecha = None

    def unPaso(self):
        regFlecha = copy.deepcopy(self.tutorial.dameFlecha(self.idFlecha))
        regFlecha.siMovible = True
        regFlecha.a1h8 = self.desde + self.hasta
        self.scFlecha = self.cpu.tablero.creaFlecha(regFlecha)
        return True

    def __str__(self):
        vFlecha = self.tutorial.dameFlecha(self.idFlecha)
        return _("Arrow") + " " + vFlecha.nombre + " " + self.desde + self.hasta

    def directo(self, tablero):
        regFlecha = copy.deepcopy(self.tutorial.dameFlecha(self.idFlecha))
        regFlecha.siMovible = True
        regFlecha.a1h8 = self.desde + self.hasta
        self.scFlecha = tablero.creaFlecha(regFlecha)
        return True


class TareaCreaMarco(Tarea):
    def __init__(self, tutorial, desde, hasta, idMarco):
        self.tutorial = tutorial
        self.idMarco = idMarco
        self.desde = desde
        self.hasta = hasta

    def unPaso(self):
        regMarco = copy.deepcopy(self.tutorial.dameMarco(self.idMarco))
        regMarco.siMovible = True
        regMarco.a1h8 = self.desde + self.hasta
        self.scMarco = self.cpu.tablero.creaMarco(regMarco)
        return True

    def __str__(self):
        vMarco = self.tutorial.dameMarco(self.idMarco)
        return _("Box") + " " + vMarco.nombre + " " + self.desde + self.hasta

    def directo(self, tablero):
        regMarco = copy.deepcopy(self.tutorial.dameMarco(self.idMarco))
        regMarco.siMovible = True
        regMarco.a1h8 = self.desde + self.hasta
        self.scMarco = tablero.creaMarco(regMarco)
        return True


class TareaCreaSVG(Tarea):
    def __init__(self, tutorial, desde, hasta, idSVG):
        self.tutorial = tutorial
        self.idSVG = idSVG
        self.desde = desde
        self.hasta = hasta

    def unPaso(self):
        regSVG = copy.deepcopy(self.tutorial.dameSVG(self.idSVG))
        regSVG.siMovible = True
        regSVG.a1h8 = self.desde + self.hasta
        self.scSVG = self.cpu.tablero.creaSVG(regSVG)
        return True

    def __str__(self):
        vSVG = self.tutorial.dameSVG(self.idSVG)
        return _("Image") + " " + vSVG.nombre + " " + self.desde + self.hasta

    def directo(self, tablero):
        regSVG = copy.deepcopy(self.tutorial.dameSVG(self.idSVG))
        regSVG.siMovible = True
        regSVG.a1h8 = self.desde + self.hasta
        self.scSVG = tablero.creaSVG(regSVG)
        return True

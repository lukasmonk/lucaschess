from Code.QT import PantallaMemoria
from Code import Util


class Memoria:
    def __init__(self, procesador):

        self.procesador = procesador
        self.fichero = procesador.configuracion.ficheroMemoria

        self.dicDatos = Util.recuperaDIC(self.fichero)
        if self.dicDatos is None:
            self.dicDatos = {}
            for x in range(6):
                self.dicDatos[x] = [0] * 25

        self.categorias = procesador.configuracion.rival.categorias
        self.pantalla = procesador.pantalla

    def nivel(self, numcategoria):
        li = self.dicDatos[numcategoria]
        for n, t in enumerate(li):
            if t == 0:
                return n - 1
        return 24

    def maxnivel(self, numcategoria):
        nm = self.nivel(numcategoria) + 1
        if nm > 24:
            nm = 24
        if numcategoria:
            nma = self.nivel(numcategoria - 1)
            nm = min(nm, nma)
        return nm

    def record(self, numcategoria, nivel):
        li = self.dicDatos[numcategoria]
        return li[nivel]

    def siActiva(self, numcategoria):
        if numcategoria == 0:
            return True
        return self.nivel(numcategoria - 1) > 0

    def lanza(self, numcategoria):

        # pedimos el nivel
        while True:
            cat = self.categorias.numero(numcategoria)
            maxnivel = self.maxnivel(numcategoria)
            nivelMas1 = PantallaMemoria.paramMemoria(self.procesador.pantalla, cat.nombre(), maxnivel + 1)
            if nivelMas1 is None:
                return
            nivel = nivelMas1 - 1
            if nivel < 0:
                return
            else:
                if self.lanzaNivel(numcategoria, nivel):
                    if nivel == 24 and numcategoria < 5:
                        numcategoria += 1
                else:
                    return

    def lanzaNivel(self, numcategoria, nivel):

        piezas = nivel + 3
        segundos = (6 - numcategoria) * piezas

        liFen = self.dameListaFen(piezas)
        if not liFen:
            return

        cat = self.categorias.numero(numcategoria)

        record = self.record(numcategoria, nivel)
        tiempo = PantallaMemoria.lanzaMemoria(self.procesador, cat.nombre(), nivel, segundos, liFen, record)
        if tiempo:
            if record == 0 or tiempo < record:
                li = self.dicDatos[numcategoria]
                li[nivel] = tiempo
                Util.guardaDIC(self.dicDatos, self.fichero)

            return True
        return False

    def dameListaFen(self, piezas):
        me = self.procesador.unMomento()

        li = []

        fedu = Util.listfiles(".", "Trainings", "Checkmates by Eduardo Sadier", "*.fns")[0]
        with open(fedu) as f:
            for l in f:
                if l:
                    pz = 0
                    l = l.split("|")[0]
                    for c in l:
                        if c == " ":
                            break
                        if not (c.isdigit() or c == "/"):
                            pz += 1
                    if pz == piezas:
                        li.append(l)

        me.final()

        return li

import copy

import Code.SQL.Base as SQLBase


class BMT(SQLBase.DBBase):
    def __init__(self, nomFichero):
        SQLBase.DBBase.__init__(self, nomFichero)

        self.tabla = "DATOS"

        if not self.existeTabla(self.tabla):
            cursor = self.conexion.cursor()
            for sql in (
                    "CREATE TABLE %s( ESTADO VARCHAR(1),ORDEN INTEGER,NOMBRE TEXT,EXTRA TEXT,TOTAL INTEGER,HECHOS INTEGER,"
                    "PUNTOS INTEGER,MAXPUNTOS INTEGER,FINICIAL VARCHAR(8),FFINAL VARCHAR(8),SEGUNDOS INTEGER,REPE INTEGER,BMT_LISTA BLOB,HISTORIAL BLOB);",
                    "CREATE INDEX [NOMBRE] ON '%s'(ORDEN DESC,NOMBRE);"):
                cursor.execute(sql % self.tabla)
                self.conexion.commit()
            cursor.close()
        self.db = None

    def leerDBF(self, siTerminadas):
        select = "ESTADO,ORDEN,NOMBRE,EXTRA,TOTAL,HECHOS,PUNTOS,MAXPUNTOS,FFINAL,SEGUNDOS,REPE"
        condicion = "HECHOS=TOTAL" if siTerminadas else "HECHOS<TOTAL"
        orden = "ORDEN DESC,NOMBRE"
        dbf = self.dbf(self.tabla, select, condicion, orden)
        dbf.leer()
        self.db = dbf
        return dbf

    def cerrar(self):
        if self.db:
            self.db.cerrar()
            self.db = None
        if self.conexion:
            self.conexion.close()
            self.conexion = None


class BMT_Uno:
    def __init__(self, fen, mrm, maxPuntos, clpartida):
        self.fen = fen
        self.mrm = mrm
        self.ponColor()

        self.puntos = maxPuntos
        self.maxPuntos = maxPuntos
        self.segundos = 0
        self.estado = 0
        self.siTerminado = False
        self.clpartida = clpartida

    def ponColor(self):
        siBlancas = "w" in self.fen
        self.mrm.siBlancas = siBlancas
        for rm in self.mrm.liMultiPV:
            rm.siBlancas = siBlancas

    def condiciones(self):
        try:
            return "%s - %d %s" % (self.mrm.nombre, self.mrm.tiempo / 1000, _("Second(s)")) if self.mrm.nombre else ""
        except:
            return ""

    def actualizaEstado(self):
        self.estado = 0
        if self.siTerminado:
            if self.maxPuntos:
                self.estado = int(7.0 * self.puntos / self.maxPuntos) + 1

    def reiniciar(self):
        for rm in self.mrm.liMultiPV:
            rm.siElegirPartida = False
        self.puntos = self.maxPuntos
        self.segundos = 0
        self.estado = 0
        self.siTerminado = False

    def siHayPrimero(self):
        for rm in self.mrm.liMultiPV:
            if rm.siPrimero:
                return rm
        return None


class BMT_Lista:
    def __init__(self):
        self.liBMT_Uno = []
        self.dicPartidas = {}

    def compruebaColor(self):
        for uno in self.liBMT_Uno:
            uno.ponColor()

    def nuevo(self, bmt_uno):
        self.liBMT_Uno.append(bmt_uno)

    def __len__(self):
        return len(self.liBMT_Uno)

    def estado(self, num):
        return self.liBMT_Uno[num].estado

    def siTerminado(self, num):
        return self.liBMT_Uno[num].siTerminado

    def siTerminada(self):
        for bmt in self.liBMT_Uno:
            if not bmt.siTerminado:
                return False
        return True

    def compruebaPartida(self, clpartida, txtPartida):
        if clpartida not in self.dicPartidas:
            self.dicPartidas[clpartida] = txtPartida

    def dameUno(self, num):
        return self.liBMT_Uno[num]

    def maxPuntos(self):
        mx = 0
        for bmt in self.liBMT_Uno:
            mx += bmt.maxPuntos
        return mx

    def reiniciar(self):
        for bmt in self.liBMT_Uno:
            bmt.reiniciar()

    def calculaTHPSE(self):
        hechos = 0
        t_estado = 0
        t_segundos = 0
        total = len(self.liBMT_Uno)
        t_puntos = 0
        for uno in self.liBMT_Uno:
            if uno.siTerminado:
                hechos += 1
                t_estado += uno.estado
                t_puntos += uno.puntos
            t_segundos += uno.segundos
        return total, hechos, t_puntos, t_segundos, t_estado

    def extrae(self, desde, hasta):
        nv = BMT_Lista()
        for x in range(desde, hasta):
            uno = copy.deepcopy(self.liBMT_Uno[x])
            if uno.clpartida:
                nv.dicPartidas[uno.clpartida] = self.dicPartidas[uno.clpartida]
            uno.reiniciar()
            nv.nuevo(uno)
        return nv

    def extraeLista(self, lni):
        nv = BMT_Lista()
        for num, bmt in enumerate(self.liBMT_Uno):
            if lni.siEsta(num + 1):
                uno = copy.deepcopy(bmt)
                if uno.clpartida:
                    nv.dicPartidas[uno.clpartida] = self.dicPartidas[uno.clpartida]
                uno.reiniciar()
                nv.nuevo(uno)
        return nv

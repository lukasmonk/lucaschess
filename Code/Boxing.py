# -*- coding: latin-1 -*-
import collections

import Code.Util as Util

class Boxing():
    def __init__(self, procesador, tipo):
        # Variables
        self.configuracion = procesador.configuracion
        self.tipo = tipo

        self.fichDB = self.configuracion.ficheroBoxing + tipo
        self.db = Util.DicSQL(self.fichDB)
        self.conf = self.db["CONFIG"]
        if self.conf is None:
            self.conf = {"SEGUNDOS": 5, "PUNTOS": 100, "NIVELHECHO": 0}

        self.liMotores = self.configuracion.comboMotoresCompleto(siOrdenar=False)  # nombre, clave
        self.claveActual = self.calcClaveActual()
        self.dicActual = self.dameDicActual()

    def calcClaveActual(self):
        return "S%dP%d" % ( self.conf["SEGUNDOS"], self.conf["PUNTOS"] )

    def cambiaConfiguracion(self, segundos, puntos):
        self.conf["SEGUNDOS"] = segundos
        self.conf["PUNTOS"] = puntos
        self.db["CONFIG"] = self.conf
        self.claveActual = self.calcClaveActual()
        self.dicActual = self.dameDicActual()

    def numEngines(self):
        return len(self.liMotores)

    def dameEtiEngine(self, fila):
        return self.liMotores[fila][0]

    def dameClaveEngine(self, fila):
        return self.liMotores[fila][1]

    def dameResultado(self, campo, numEngine):
        engine = self.liMotores[numEngine][1]
        dicEngine = self.dicActual.get(engine, None)
        if dicEngine is None:
            return None, None
        recordFecha = dicEngine.get("RECORD_FECHA_%s" % campo, None)
        recordMovimientos = dicEngine.get("RECORD_MOVIMIENTOS_%s" % campo, None)
        return recordFecha, recordMovimientos

    def ponResultado(self, numEngine, clave, movimientos):
        engine = self.liMotores[numEngine][1]
        dicEngine = self.dicActual.get(engine, collections.OrderedDict())
        historico = dicEngine.get("HISTORICO_%s" % clave, [])
        hoy = Util.hoy()
        historico.append((hoy, movimientos))
        recordMovimientos = dicEngine.get("RECORD_MOVIMIENTOS_%s" % clave, 0)
        siRecord = movimientos > recordMovimientos
        if siRecord:
            dicEngine["RECORD_FECHA_%s" % clave] = hoy
            dicEngine["RECORD_MOVIMIENTOS_%s" % clave] = movimientos
        self.dicActual[engine] = dicEngine

        self.db[self.claveActual] = self.dicActual

        return siRecord

    def dameEti(self, fecha, moves):
        if not fecha:
            return "-"
        if moves > 2000:
            mv = _("Won") + " %d" % (moves - 2000)
        elif moves > 1000:
            mv = _("Draw") + " %d" % (moves - 1000)
        else:
            mv = "%d %s" % (moves, _("Moves"))

        return "%s -> %s" % ( Util.localDate(fecha), mv )

    def dameEtiRecord(self, campo, fila):
        fecha, moves = self.dameResultado(campo, fila)
        return self.dameEti(fecha, moves)

    def dameDicActual(self):
        dicActual = self.db[self.claveActual]
        if dicActual is None:
            dicActual = {}
        return dicActual

    def actual(self):
        return self.conf["SEGUNDOS"], self.conf["PUNTOS"]

    def segundos(self):
        return self.conf["SEGUNDOS"]

    def borraRegistros(self, numEngine):
        engine = self.liMotores[numEngine][1]
        if engine in self.dicActual:
            del self.dicActual[engine]
            self.db[self.claveActual] = self.dicActual

    def cerrar(self):
        self.db.close()


import atexit
import sqlite3

from Code import Util

import DBF
import DBFcache


class DBBase:
    """
    Hace referencia a una base de datos.
    Establece la conexion y permite cerrarla.
    """

    def __init__(self, nomFichero):
        self.nomFichero = unicode(nomFichero)
        existe = Util.existeFichero(nomFichero)
        self.conexion = sqlite3.connect(self.nomFichero)
        self.conexion.text_factory = lambda x: unicode(x, "utf-8", "ignore")
        if not existe:
            cursor = self.conexion.cursor()
            cursor.execute("PRAGMA page_size = 4096")
            cursor.execute("PRAGMA synchronous = NORMAL")
            cursor.close()
        atexit.register(self.cerrar)

    def cerrar(self):
        """
        Cierra la conexion a esta base de datos
        """
        if self.conexion:
            self.conexion.close()
            self.conexion = None

    def existeTabla(self, tabla):
        cursor = self.conexion.cursor()
        cursor.execute("pragma table_info(%s)" % tabla)
        liCampos = cursor.fetchall()
        cursor.close()
        return liCampos

    def dbf(self, ctabla, select, condicion="", orden=""):
        """
        Acceso a una tabla con un navegador tipo DBF, con lectura inicial de los RowIDs

        @param ctabla: nombre de la tabla
        @param select: lista de campos separados por comas
        @param condicion: sentencia de condicion SQL
        @param orden: sentencia de orden SQL
        """
        return DBF.DBF(self.conexion, ctabla, select, condicion, orden)

    def dbfCache(self, ctabla, select, condicion="", orden=""):
        """

        @param ctabla: nombre de la tabla
        @param select: lista de campos separados por comas
        @param condicion: sentencia de condicion SQL
        @param orden: sentencia de orden SQL
        """
        return DBFcache.DBFcache(self.conexion, ctabla, select, condicion, orden)

    def dbfT(self, ctabla, select, condicion="", orden=""):
        """
        Acceso a una tabla con un navegador tipo DBF, con lectura completa de todos los datos.

        @param ctabla: nombre de la tabla
        @param select: lista de campos separados por comas
        @param condicion: sentencia de condicion SQL
        @param orden: sentencia de orden SQL
        """
        return DBF.DBFT(self.conexion, ctabla, select, condicion, orden)

    def generarTabla(self, tb):
        cursor = self.conexion.cursor()
        cursor.execute("PRAGMA page_size = 4096")
        cursor.execute("PRAGMA synchronous = NORMAL")
        tb.crearBase(cursor)
        cursor.close()


class TablaBase:
    """
    Definicion generica de una tabla.
    """

    def __init__(self, nombre):
        self.liCampos = []
        self.liIndices = []
        self.nombre = nombre

    def crearBase(self, cursor):
        sql = "CREATE TABLE %s (" % self.nombre
        for x in self.liCampos:
            sql += x.create().rstrip() + ","
        sql = sql[:-1] + " );"
        cursor.execute(sql)

        for x in self.liIndices:
            c = "UNIQUE" if x.siUnico else ""
            cursor.execute("CREATE %s INDEX [%s] ON '%s'(%s);" % (c, x.nombre, self.nombre, x.campos))

    def nuevoCampo(self, nombre, tipo, notNull=False, primaryKey=False, autoInc=False):
        campo = Campo(nombre, tipo, notNull, primaryKey, autoInc)
        self.liCampos.append(campo)

    def nuevoIndice(self, nombre, campos, siUnico=False):
        indice = Indice(nombre, campos, siUnico)
        self.liIndices.append(indice)


class Campo:
    """
    Definicion generica de un campo de una tabla.
    """

    def __init__(self, nombre, tipo, notNull=False, primaryKey=False, autoInc=False):
        self.nombre = nombre
        self.tipo = tipo
        self.notNull = notNull
        self.primaryKey = primaryKey
        self.autoInc = autoInc

    def create(self):
        c = ""
        if self.notNull:
            c += "NOT NULL"
        if self.primaryKey:
            c += " PRIMARY KEY"
        if self.autoInc:
            c += " AUTOINCREMENT"
        return "%s %s %s" % (self.nombre, self.tipo, c)


class Indice:
    """
    Definicion generica de un indice de una tabla.
    """

    def __init__(self, nombre, campos, siUnico=False):
        self.nombre = nombre
        self.campos = campos
        self.siUnico = siUnico


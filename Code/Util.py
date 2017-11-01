import atexit
import base64
import cPickle
import codecs
import collections
import datetime
import gc
import glob
import hashlib
import os
import random
import shutil
import sqlite3
import time
import zlib
import threading
from itertools import izip, cycle

import chardet.universaldetector


def xor_crypt(data, key):
    """
    http://bytes.com/topic/python/answers/881561-xor-encryption
    Author = http://bytes.com/profile/247871/darktemp/
    """
    if key:
        return ''.join(chr(ord(x) ^ ord(y)) for (x, y) in izip(data, cycle(key)))
    else:
        return data


def nuevoID():
    d = datetime.datetime.now()
    r = random.randint
    t = (((((r(1, d.year) * 12 + r(1, d.month)) * 31 + d.day) * 24 + d.hour) * 60 + d.minute) * 60 + d.second) * 1000 + r(1, d.microsecond + 737) / 1000
    return t


def guardaDIC(dic, fich):
    with open(fich, "w") as q:
        q.write(base64.encodestring(cPickle.dumps(dic)))


def recuperaDIC(fich):
    try:
        with open(fich) as f:
            s = f.read()
        dic = cPickle.loads(base64.decodestring(s))
    except:
        dic = None
    return dic


def guardaVar(fich, v):
    with open(fich, "w") as q:
        q.write(cPickle.dumps(v))


def recuperaVar(fich):
    try:
        with open(fich) as f:
            s = f.read()
        v = cPickle.loads(s)
    except:
        v = None
    return v


def var2blob(var):
    varp = cPickle.dumps(var)
    varz = zlib.compress(varp, 7)
    return sqlite3.Binary(varz)


def blob2var(blob):
    if blob is None:
        return None
    varp = zlib.decompress(blob)
    return cPickle.loads(varp)


def dic2blob(dic):
    varp = str(dic).replace(', ', ',').replace(': ', ':')
    varz = zlib.compress(varp, 7)
    return sqlite3.Binary(varz)


def blob2dic(blob):
    if blob is None:
        return {}
    varp = zlib.decompress(blob)
    return eval(varp)


def str2blob(varp):
    varz = zlib.compress(varp, 7)
    return sqlite3.Binary(varz)


def blob2str(blob):
    if blob is None:
        return ""
    return str(zlib.decompress(blob))


def dic2txt(dic):
    return base64.encodestring(cPickle.dumps(dic)).replace("\n", "|")


def txt2dic(txt):
    txt = txt.replace("|", "\n")
    dic = cPickle.loads(base64.decodestring(txt))
    return dic


def var2txt(var):
    return cPickle.dumps(var)


def txt2var(txt):
    return cPickle.loads(txt)


def renombraNum(origen):
    num = 1
    while os.path.isfile("%s.%d" % (origen, num)):
        num += 1
    os.rename(origen, "%s.%d" % (origen, num))


class Almacen:
    pass


class Record:
    pass


def hoy():
    return datetime.datetime.now()


def dtos(f):
    return "%04d%02d%02d" % (f.year, f.month, f.day)


def stod(txt):
    if txt and len(txt) == 8 and txt.isdigit():
        return datetime.date(int(txt[:4]), int(txt[4:6]), int(txt[6:]))
    return None


def dtosext(f):
    return "%04d%02d%02d%02d%02d%02d" % (f.year, f.month, f.day, f.hour, f.minute, f.second)


def stodext(txt):
    if txt and len(txt) == 14 and txt.isdigit():
        return datetime.datetime(int(txt[:4]), int(txt[4:6]), int(txt[6:8]),
                                 int(txt[8:10]), int(txt[10:12]), int(txt[12:]))
    return None


def primeraMayuscula(txt):
    return txt[0].upper() + txt[1:]


def huella():
    m = hashlib.md5()
    m.update(str(random.random()) + str(hoy()))
    return m.hexdigest()


def microsegundosRnd():
    d = datetime.datetime.now()
    return random.randint(0, 1000) + 1000 * (
        d.microsecond + 1000000 * (d.second + 60 * (d.minute + 60 * (d.hour + 24 * d.toordinal()))))


def fileNext(folder, base, ext):
    n = 1
    path_ = os.path.join(folder, "%s%s.%s" % (base, "%d", ext))
    while existeFichero(path_ % n):
        n += 1
    return path_ % n


def ficheroTemporal(pathTemp, extension):
    creaCarpeta(pathTemp)
    while True:
        fich = os.path.join(pathTemp, "%d.%s" % (random.randint(1, 999999999), extension))
        if not existeFichero(fich):
            return fich


def tamFichero(fichero):
    return os.path.getsize(fichero) if os.path.isfile(fichero) else -1


def existeFichero(fichero):
    return tamFichero(fichero) >= 0


def copiaFichero(origen, destino):
    if existeFichero(origen):
        if borraFichero(destino):
            shutil.copy2(origen, destino)
            return True
    return False


def renombraFichero(origen, destino):
    if not existeFichero(origen):
        return False
    origen = os.path.abspath(origen)
    destino = os.path.abspath(destino)
    if origen == destino:
        return True
    if origen.lower() == destino.lower():
        os.rename(origen, destino)
        return True
    if borraFichero(destino):
        shutil.move(origen, destino)
        return True
    return False


def borraFichero(fichero):
    try:
        os.remove(fichero)
    except:
        pass
    return not os.path.isfile(fichero)


def ini2lista(fichero, etiClave="CLAVE"):
    li = []

    if os.path.isfile(fichero):

        f = open(fichero, "rb")

        for linea in f:
            linea = linea.strip()
            if linea:
                if linea.startswith("["):
                    clave = linea[1:-1]
                    dic = collections.OrderedDict()
                    li.append(dic)
                    dic[etiClave] = clave
                else:
                    n = linea.find("=")
                    if n:
                        clave1 = linea[:n].strip()
                        valor = linea[n + 1:].strip()
                        dic[clave1] = valor
        f.close()

    return li


def lista2ini(fichero, lista, etiClave="CLAVE"):
    f = open(fichero, "wb")
    for dic in lista:
        f.write("[%s]\n" % dic[etiClave])
        for k in dic:
            if k != etiClave:
                f.write("%s=%s\n" % (k, dic[k]))
    f.close()


def ini2dic(fichero):
    dicBase = collections.OrderedDict()

    if os.path.isfile(fichero):

        f = open(fichero, "rb")

        for linea in f:
            linea = linea.strip()
            if linea and not linea.startswith("#"):
                if linea.startswith("["):
                    clave = linea[1:-1]
                    dic = collections.OrderedDict()
                    dicBase[clave] = dic
                else:
                    n = linea.find("=")
                    if n > 0:
                        clave1 = linea[:n].strip()
                        valor = linea[n + 1:].strip()
                        dic[clave1] = valor
        f.close()

    return dicBase


def ini8dic(fichero):
    dicBase = collections.OrderedDict()

    if os.path.isfile(fichero):

        f = codecs.open(fichero, "r", "utf-8", 'ignore')

        for linea in f:
            linea = linea.strip()
            if linea and not linea.startswith("#"):
                if linea.startswith("["):
                    clave = linea[1:-1]
                    dic = collections.OrderedDict()
                    dicBase[clave] = dic
                else:
                    n = linea.find("=")
                    if n > 0:
                        clave1 = linea[:n].strip()
                        valor = linea[n + 1:].strip()
                        dic[clave1] = valor
        f.close()

    return dicBase


def dic8ini(fichero, dic):
    f = codecs.open(fichero, "w", "utf-8", 'ignore')
    for k in dic:
        f.write("[%s]\n" % k)
        for clave in dic[k]:
            f.write("%s=%s\n" % (clave, dic[k][clave]))
    f.close()


def iniBase8dic(fichero):
    dic = {}

    if os.path.isfile(fichero):

        f = codecs.open(fichero, "r", "utf-8", 'ignore')

        for linea in f:
            linea = linea.strip()
            if linea.startswith("#"):
                continue
            if linea:
                n = linea.find("=")
                if n:
                    clave = linea[:n].strip()
                    valor = linea[n + 1:].strip()
                    dic[clave] = valor
        f.close()

    return dic


def dic8iniBase(fichero, dic):
    f = codecs.open(fichero, "w", "utf-8", 'ignore')
    for k in dic:
        f.write("%s=%s\n" % (k, dic[k]))
    f.close()


def creaCarpeta(carpeta):
    try:
        os.mkdir(carpeta)
    except:
        pass


def secs2str(s):
    m = s/60
    s = s%60
    h = m/60
    m = m%60
    return "%02d:%02d:%02d" % (h, m, s)


class ListaNumerosImpresion:
    def __init__(self, txt):
        # Formas
        # 1. <num>            1, <num>, 0
        #   2. <num>-           2, <num>, 0
        #   3. <num>-<num>      3, <num>,<num>
        #   4. -<num>           4, <num>, 0
        self.lista = []
        if txt:
            txt = txt.replace("--", "-").replace(",,", ",").replace(" ", "")

            for bloque in txt.split(","):

                if bloque.startswith("-"):
                    num = bloque[1:]
                    if num.isdigit():
                        self.lista.append((4, int(num)))

                elif bloque.endswith("-"):
                    num = bloque[:-1]
                    if num.isdigit():
                        self.lista.append((2, int(num)))

                elif "-" in bloque:
                    li = bloque.split("-")
                    if len(li) == 2:
                        num1, num2 = li
                        if num1.isdigit() and num2.isdigit():
                            i1 = int(num1)
                            i2 = int(num2)
                            if i1 <= i2:
                                self.lista.append((3, i1, i2))

                elif bloque.isdigit():
                    self.lista.append((1, int(bloque)))

    def siEsta(self, pos):
        if not self.lista:
            return True

        for patron in self.lista:
            modo = patron[0]
            i1 = patron[1]
            if modo == 1:
                if pos == i1:
                    return True
            elif modo == 2:
                if pos >= i1:
                    return True
            elif modo == 3:
                i2 = patron[2]
                if i1 <= pos <= i2:
                    return True
            elif modo == 4:
                if pos <= i1:
                    return True

        return False

    def selected(self, lista):
        return [x for x in lista if self.siEsta(x)]


def speed():
    t = time.time()
    for x in xrange(100000):
        for i in xrange(10):
            oct(i)
        gc.enable()
    return time.time() - t


class SymbolDict:
    def __init__(self, dic=None):
        self._dic = {}
        self._keys = []
        if dic:
            for k, v in dic.iteritems():
                self.__setitem__(k, v)

    def __contains__(self, clave):
        return clave.upper() in self._dic

    def __len__(self):
        return len(self._keys)

    def __getitem__(self, clave):
        if type(clave) == int:
            return self._keys[clave]
        return self._dic[clave.upper()]

    def __setitem__(self, clave, valor):
        clu = clave.upper()
        if clu not in self._dic:
            self._keys.append(clave)
        self._dic[clu] = valor

    def get(self, clave, default=None):
        clu = clave.upper()
        if clu not in self._dic:
            return default
        return self.__getitem__(clave)

    def iteritems(self):
        for k in self._keys:
            yield k, self.__getitem__(k)

    def keys(self):
        return self._keys[:]

    def __str__( self ):
        x = ""
        for t in self._keys:
           x += "[%s]=[%s]\n"%(t, str(self.__getitem__(t)) )
        return x.strip()


class IPC(object):
    def __init__(self, nomFichero, siPush):
        if siPush and os.path.isfile(nomFichero):
            try:
                os.remove(nomFichero)
            except:
                pass
        self._conexion = sqlite3.connect(nomFichero)
        atexit.register(self.close)

        if siPush:
            sql = "CREATE TABLE DATOS( DATO BLOB );"
            self._conexion.cursor().execute(sql)
            self._conexion.commit()

        self.key = 0

    def pop(self):
        cursor = self._conexion.cursor()
        nk = self.key + 1
        sql = "SELECT dato FROM DATOS WHERE ROWID = %d" % nk
        cursor.execute(sql)
        reg = cursor.fetchone()
        if reg:
            valor = cPickle.loads(str(reg[0]))
            self.key = nk
        else:
            valor = None
        cursor.close()
        return valor

    def push(self, valor):
        cursor = self._conexion.cursor()
        dato = sqlite3.Binary(cPickle.dumps(valor))
        sql = "INSERT INTO DATOS (dato) values(?)"
        cursor.execute(sql, [dato, ])
        cursor.close()
        self._conexion.commit()

    def close(self):
        if self._conexion:
            self._conexion.close()
            self._conexion = None


class Rondo:
    def __init__(self, *lista):
        self.pos = -1
        self.lista = lista
        self.tope = len(self.lista)

    def shuffle(self):
        li = list(self.lista)
        random.shuffle(li)
        self.lista = li

    def otro(self):
        self.pos += 1
        if self.pos == self.tope:
            self.pos = 0
        return self.lista[self.pos]

    def reset(self):
        self.pos = -1


def validNomFichero(nombre):
    nombre = nombre.strip()
    for x in "\\:/|?*^%><()":
        if x in nombre:
            nombre = nombre.replace(x, "_")
    return nombre


class Timer:
    def __init__(self, tiempoPendiente):

        self.tiempoPendiente = tiempoPendiente
        self.marcaTiempo = None
        self.txt = ""
        self.marcaZeitnot = 0

    def texto(self, segs):
        if segs <= 0.0:
            segs = 0.0
        tp = int(segs)
        txt = "%02d:%02d" % (int(tp / 60), tp % 60)
        return txt

    def ponSegExtra(self, segs):
        self.tiempoPendiente += segs

    def dameSegundos(self):
        if self.marcaTiempo:
            tp = self.tiempoPendiente - (time.time() - self.marcaTiempo)
        else:
            tp = self.tiempoPendiente
        if tp <= 0.0:
            tp = 0
        return int(tp)

    def dameSegundos2(self):
        if self.marcaTiempo:
            tp2 = int(time.time() - self.marcaTiempo)
            tp = int(self.tiempoPendiente) - tp2
        else:
            tp = self.tiempoPendiente
            tp2 = 0
        if tp <= 0.0:
            tp = 0
        return int(tp), tp2

    def etiqueta(self):
        return self.texto(self.dameSegundos())

    def etiqueta2(self):
        tp, tp2 = self.dameSegundos2()
        return self.texto(tp), self.texto(tp2)

    def etiquetaDif(self):
        nvEti = self.etiqueta()
        if nvEti != self.txt:
            self.txt = nvEti
            return nvEti

        return None

    def etiquetaDif2(self):
        nvEti, nvEti2 = self.etiqueta2()
        if nvEti != self.txt:
            self.txt = nvEti
            return nvEti, nvEti2

        return None, None

    def etiquetaDGT(self):
        segs = self.dameSegundos()
        mins = segs / 60
        segs -= mins * 60
        hors = mins / 60
        mins -= hors * 60

        return "%d:%02d:%02d" % (hors, mins, segs)

    def siAgotado(self):
        if self.marcaTiempo:
            if (self.tiempoPendiente - (time.time() - self.marcaTiempo)) <= 0.0:
                return True
        else:
            return self.tiempoPendiente <= 0.0
        return False

    def isZeitnot(self):
        if self.marcaZeitnot:
            if self.marcaTiempo:
                t = self.tiempoPendiente - (time.time() - self.marcaTiempo)
            else:
                t = self.tiempoPendiente
            if t > 0:
                resp = t < self.marcaZeitnot
                if resp:
                    self.marcaZeitnot = None
                return resp
        return False

    def setZeitnot(self, segs):
        self.marcaZeitnot = segs

    def iniciaMarcador(self):
        self.marcaTiempo = time.time()

    def paraMarcador(self, tiempoJugada):
        if self.marcaTiempo:
            self.tiempoPendiente -= (time.time() - self.marcaTiempo) - tiempoJugada
            self.marcaTiempo = None

    def tiempoAplazamiento(self):
        self.paraMarcador(0.00)
        return self.tiempoPendiente


def fideELO(eloJugador, eloRival, resultado):
    if resultado == +1:
        resultado = 1.0
    elif resultado == 0:
        resultado = 0.5
    else:
        resultado = 0.0
    if eloJugador <= 1200:
        k = 40.0
    elif eloJugador <= 2100:
        k = 32.0
    elif eloRival < 2400:
        k = 24.0
    else:
        k = 16.0
    probabilidad = 1.0 / (1.0 + (10.0 ** ((eloRival - eloJugador) / 400.0)))
    return int(k * (resultado - probabilidad))

date_format = ["%Y.%m.%d", ]


def localDate(date):
    return date.strftime(date_format[0])


def localDateT(date):
    return "%s %02d:%02d" % (date.strftime(date_format[0]), date.hour, date.minute)


def listfiles(*lista):
    f = lista[0]
    if len(lista) > 1:
        for x in lista[1:]:
            f = os.path.join(f, x)
    return glob.glob(f)


def listdir(txt, siUnicode=False):
    return os.listdir(unicode(txt)) if siUnicode else os.listdir(txt)


def dirRelativo(dr):
    if dr:
        try:
            nr = os.path.relpath(dr)
            if not nr.startswith(".."):
                dr = nr
        except:
            pass
    else:
        dr = ""
    return dr


def cX():
    b3 = "Sibuscasresultadosdistintosnohagassiemprelomismo"
    n7 = 669558
    t1 = len(b3)
    p3 = 0
    c9 = ""
    while n7 > 0:
        nr = n7 % 10
        n7 /= 10
        p3 += nr
        if p3 >= t1:
            p3 -= t1
        c9 += b3[p3] + str(p3)
    c9 = list(c9)
    c9.reverse()
    c9 = "".join(c9)
    return c9


def enc(cad):
    pos = 0
    resp = ""
    cl = cX()
    lcl = len(cl)
    lcad = len(cad)
    for i in range(lcad):
        h = ord(cad[i]) + i + ord(cl[pos])
        resp += "%03d" % h
        pos += 1
        if pos >= lcl:
            pos = 0
    return resp
    # def dec( cad ):
    # pos = 0
    # resp = ""
    # cl = cX()
    # lcl = len(cl)
    # lcad = len(cad)/3
    # for i in range(lcad):
    # nc = i*3
    # c0 = cad[nc:nc+3]
    # h = int(c0)
    # h = h - i - ord(cl[pos])
    # resp += chr( h )
    # pos += 1
    # if pos >= lcl:
    # pos = 0
    # return resp


def creaID():
    r = random.randint(1, 9999)
    d = datetime.datetime.now()
    s = "%d,%s" % (r, d.isoformat()[2:].strip("0").replace("-", "").replace("T", "").replace(":", "").replace(".", ""))
    cr = enc(s)
    return cr

# ~ import subprocess
# ~ class Proceso():

# ~ def __init__( self, exe ):
# ~ self.setWorkingDirectory ( os.path.abspath(os.path.dirname(exe)) )
# ~ if "critter" in exe.lower():
# ~ startupinfo = subprocess.STARTUPINFO()
# ~ startupinfo.dwFlags |= subprocess.STARTF_USESHOWWINDOW
# ~ startupinfo.wShowWindow = subprocess.SW_HIDE
# ~ self.popen = p = subprocess.Popen("", executable=exe, stdin=subprocess.PIPE, stdout=subprocess.PIPE, shell=False, startupinfo=startupinfo)
# ~ else:
# ~ self.popen = p = subprocess.Popen( "", executable=exe, shell=True, \
# ~ stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.STDOUT )
# ~ self.stdin = p.stdin
# ~ self.stdout = p.stdout

# ~ def escribeLinea( self, linea ):
# ~ self.stdin.write( linea + "\n" )

# ~ def esperaRespuesta( self, x=None ):
# ~ return self.stdout.readline()

# ~ def terminar( self ):
# ~ try:
# ~ self.popen.terminate()
# ~ except:
# ~ pass

# ~ from ctypes import *
# ~ from ctypes.wintypes import *
# ~ class MEMORYSTATUS(Structure):
# ~ _fields_ = [
# ~ ('dwLength', DWORD),
# ~ ('dwMemoryLoad', DWORD),
# ~ ('dwTotalPhys', DWORD),
# ~ ('dwAvailPhys', DWORD),
# ~ ('dwTotalPageFile', DWORD),
# ~ ('dwAvailPageFile', DWORD),
# ~ ('dwTotalVirtual', DWORD),
# ~ ('dwAvailVirtual', DWORD),
# ~ ]
# ~ def winmem():
# ~ x = MEMORYSTATUS()
# ~ windll.kernel32.GlobalMemoryStatus(byref(x))
# ~ return x

# def detectCPUs():
# """
# Detects the number of CPUs on a system. Cribbed from pp.
# http://codeliberates.blogspot.com/2008/05/detecting-cpuscores-in-python.html
# Literal : I found this interesting function in a post on Twister and distributed programming by Bruce Eckel. It uses the Python os package to detect the number of CPUs/cores on a machine. Archiving it here for future reference
# """
# # Linux, Unix and MacOS:
# if hasattr(os, "sysconf"):
# if os.sysconf_names.has_key("SC_NPROCESSORS_ONLN"):
# # Linux & Unix:
# ncpus = os.sysconf("SC_NPROCESSORS_ONLN")
# if isinstance(ncpus, int) and ncpus > 0:
# return ncpus
# else: # OSX:
# return int(os.popen2("sysctl -n hw.ncpu")[1].read())
# # Windows:
# if os.environ.has_key("NUMBER_OF_PROCESSORS"):
# ncpus = int(os.environ["NUMBER_OF_PROCESSORS"]);
# if ncpus > 0:
# return ncpus
# return 1 # Default


class DicSQL(object):
    def __init__(self, nomDB, tabla="Data", maxCache=2048):
        self.table = tabla
        self.maxCache = maxCache
        self.cache = collections.OrderedDict()

        self._conexion = sqlite3.connect(nomDB)
        self._conexion.text_factory = lambda x: unicode(x, "utf-8", "ignore")
        atexit.register(self.close)

        cursor = self._conexion.cursor()
        cursor.execute("pragma table_info(%s)" % tabla)
        if not cursor.fetchall():
            sql = "CREATE TABLE %s( KEY TEXT PRIMARY KEY, VALUE TEXT );" % tabla
            cursor.execute(sql)
            self._conexion.commit()
            cursor.close()

        self.stKeys = set()
        cursor = self._conexion.cursor()
        sql = "SELECT KEY FROM %s" % self.table
        cursor.execute(sql)
        li = cursor.fetchall()
        for reg in li:
            self.stKeys.add(reg[0])
        cursor.close()

    def __contains__(self, key):
        return key in self.stKeys

    def addCache(self, key, obj):
        if len(self.cache) > self.maxCache:
            del self.cache[self.cache.keys[0]]
        self.cache[key] = obj

    def __setitem__(self, key, obj):
        cursor = self._conexion.cursor()
        dato = base64.encodestring(cPickle.dumps(obj))
        key = str(key)
        siYaEsta = key in self.stKeys
        if siYaEsta:
            sql = "UPDATE %s SET VALUE=? WHERE KEY = ?" % self.table
        else:
            sql = "INSERT INTO %s (VALUE,KEY) values(?,?)" % self.table
            self.stKeys.add(key)
        cursor.execute(sql, (dato, key))
        cursor.close()
        self._conexion.commit()

        self.addCache(key, obj)

    def __getitem__(self, key):
        key = str(key)
        if key in self.stKeys:
            if key in self.cache:
                return self.cache[key]

            cursor = self._conexion.cursor()
            sql = "SELECT VALUE FROM %s WHERE KEY= ?" % self.table
            cursor.execute(sql, (key,))
            li = cursor.fetchone()
            cursor.close()
            dato = base64.decodestring(li[0])
            obj = cPickle.loads(dato)
            self.addCache(key, obj)
            return obj
        else:
            return None

    def __delitem__(self, key):
        key = str(key)
        if key in self.stKeys:
            self.stKeys.remove(key)
            if key in self.cache:
                del self.cache[key]
            cursor = self._conexion.cursor()
            sql = "DELETE FROM %s WHERE KEY= ?" % self.table
            cursor.execute(sql, (key,))
            cursor.close()
            self._conexion.commit()

    def __len__(self):
        return len(self.stKeys)

    def close(self):
        if self._conexion:
            self._conexion.close()
            self._conexion = None

    def keys(self, siOrdenados=False, siReverse=False):
        li = list(self.stKeys)
        return sorted(li, reverse=siReverse) if siOrdenados else li

    def get(self, key, default):
        key = str(key)
        if key in self.stKeys:
            return self.__getitem__(key)
        else:
            return default

    def asDictionary(self):
        dic = collections.OrderedDict()
        cursor = self._conexion.cursor()
        sql = "SELECT KEY,VALUE FROM %s" % self.table
        cursor.execute(sql)
        li = cursor.fetchall()
        for key, dato in li:
            dato = base64.decodestring(dato)
            dic[key] = cPickle.loads(dato)
        cursor.close()

        return dic

    def pack(self):
        cursor = self._conexion.cursor()
        cursor.execute("VACUUM")
        cursor.close()
        self._conexion.commit()

    def __enter__(self):
        return self

    def __exit__(self, xtype, value, traceback):
        self.close()


class LIdisk:
    def __init__(self, nomFichero):

        self.nomFichero = nomFichero
        self._conexion = sqlite3.connect(nomFichero)
        self._conexion.text_factory = lambda x: unicode(x, "utf-8", "ignore")
        atexit.register(self.close)

        try:
            sql = "CREATE TABLE datos( DATO TEXT );"
            self._conexion.cursor().execute(sql)
        except:
            pass

    def append(self, valor):
        sql = "INSERT INTO datos( DATO ) VALUES( ? )"
        liValores = [cPickle.dumps(valor)]

        cursor = self._conexion.cursor()
        cursor.execute(sql, liValores)
        cursor.close()
        self._conexion.commit()

    def __getitem__(self, xid):
        sql = "select DATO from datos where ROWID=%d" % (xid + 1,)
        cursor = self._conexion.cursor()
        cursor.execute(sql)
        dato = cursor.fetchone()
        cursor.close()
        return cPickle.loads(str(dato[0]))

    def __len__(self):
        sql = "select COUNT(DATO) from datos"
        cursor = self._conexion.cursor()
        cursor.execute(sql)
        resp = cursor.fetchone()
        cursor.close()
        return resp[0]

    def close(self):
        if self._conexion:
            self._conexion.close()
            self._conexion = None


class DicRaw:
    def __init__(self, nomDB, tabla="Data"):
        self.table = tabla

        self._conexion = sqlite3.connect(nomDB)
        atexit.register(self.close)

        cursor = self._conexion.cursor()
        cursor.execute("pragma table_info(%s)" % tabla)
        if not cursor.fetchall():
            sql = "CREATE TABLE %s( KEY TEXT PRIMARY KEY, VALUE TEXT );" % tabla
            cursor.execute(sql)
            self._conexion.commit()
        cursor.close()

    def __setitem__(self, key, obj):
        key = str(key)
        if self.__contains__(key):
            sql = "UPDATE %s SET VALUE=? WHERE KEY = ?" % self.table
        else:
            sql = "INSERT INTO %s (VALUE,KEY) values(?,?)" % self.table
        cursor = self._conexion.cursor()
        dato = base64.encodestring(cPickle.dumps(obj))
        cursor.execute(sql, (dato, key))
        cursor.close()
        self._conexion.commit()

    def __getitem__(self, key):
        cursor = self._conexion.cursor()
        sql = "SELECT VALUE FROM %s WHERE KEY= ?" % self.table
        cursor.execute(sql, (key,))
        li = cursor.fetchone()
        cursor.close()
        if not li:
            return None
        dato = base64.decodestring(li[0])
        return cPickle.loads(dato)

    def __delitem__(self, key):
        cursor = self._conexion.cursor()
        sql = "DELETE FROM %s WHERE KEY= ?" % self.table
        cursor.execute(sql, (key,))
        cursor.close()
        self._conexion.commit()

    def __contains__(self, key):
        cursor = self._conexion.cursor()
        sql = "SELECT KEY FROM %s WHERE KEY= ?" % self.table
        cursor.execute(sql, (key,))
        li = cursor.fetchone()
        cursor.close()
        return True if li else False

    def close(self):
        if self._conexion:
            self._conexion.close()
            self._conexion = None

    def get(self, key, default):
        v = self.__getitem__(key)
        return v if v else default

    def pack(self):
        cursor = self._conexion.cursor()
        cursor.execute("VACUUM")
        cursor.close()
        self._conexion.commit()

    def __len__(self):
        return len(self.keys())

    def keys(self):
        cursor = self._conexion.cursor()
        sql = "SELECT KEY FROM %s" % self.table
        cursor.execute(sql)
        liKeys = [reg[0] for reg in cursor.fetchall()]
        cursor.close()
        return liKeys

    def __enter__(self):
        return self

    def __exit__(self, xtype, value, traceback):
        self.close()


class DicBLOB(object):
    def __init__(self, nomDB, tabla="Datos"):
        self._conexion = sqlite3.connect(nomDB)
        atexit.register(self.close)

        cursor = self._conexion.cursor()
        cursor.execute("pragma table_info(%s)" % tabla)
        liCampos = cursor.fetchall()

        if not liCampos:
            sql = "CREATE TABLE %s( CLAVE TEXT PRIMARY KEY, DATO BLOB );" % tabla
            cursor.execute(sql)
            self._conexion.commit()
            cursor.close()

        self.dic = {}
        self.tabla = tabla
        self.leeClaves()

    def leeClaves(self):
        cursor = self._conexion.cursor()
        sql = "SELECT clave FROM %s" % self.tabla
        cursor.execute(sql)
        li = cursor.fetchall()
        for clave, in li:
            self.dic[clave] = True
        cursor.close()

    def __contains__(self, clave):
        return clave in self.dic

    def __setitem__(self, clave, wav):
        cursor = self._conexion.cursor()
        dato = sqlite3.Binary(wav)
        liValores = [dato, clave]
        if self.__contains__(clave):
            sql = "UPDATE %s SET dato=? WHERE clave = ?" % self.tabla
        else:
            sql = "INSERT INTO %s (dato,clave) values(?,?)" % self.tabla
            self.dic[clave] = True
        cursor.execute(sql, liValores)
        cursor.close()
        self._conexion.commit()

    def __delitem__(self, clave):
        cursor = self._conexion.cursor()
        sql = "DELETE FROM %s WHERE clave= ?" % self.tabla
        liValores = [clave, ]
        cursor.execute(sql, liValores)
        cursor.close()
        self._conexion.commit()

        del self.dic[clave]

    def __getitem__(self, clave):
        if clave in self.dic:
            cursor = self._conexion.cursor()
            sql = "SELECT dato FROM %s WHERE clave= ?" % self.tabla
            liValores = [clave, ]
            cursor.execute(sql, liValores)
            li = cursor.fetchone()
            cursor.close()

            return li[0]
        else:
            return None

    def __len__(self):
        return len(self.dic)

    def close(self):
        if self._conexion:
            self._conexion.close()
            self._conexion = None

    def keys(self):
        return self.dic.keys()

    def get(self, clave, default):
        if clave in self.dic:
            return self.__getitem__(clave)
        else:
            return default


class Timekeeper:
    def __init__(self):
        self._begin = None

    def start(self):
        self._begin = time.time()

    def stop(self):
        if self._begin:
            b = self._begin
            self._begin = None
            return time.time() - b


class OpenCodec:
    def __init__(self, path):
        with open(path) as f:
            u = chardet.universaldetector.UniversalDetector()
            for n, x in enumerate(f):
                u.feed(x)
                if n == 500:
                    break
            u.close()
            encoding = u.result.get("encoding", "latin-1")
        self.f = codecs.open(path, "r", encoding, 'ignore')

    def __enter__(self):
        return self.f

    def __exit__(self, xtype, value, traceback):
        self.f.close()


def txt_encoding(txt):
    u = chardet.universaldetector.UniversalDetector()
    u.feed(txt)
    u.close()

    return u.result.get("encoding", "latin-1")


def file_encoding(fich, chunk=3000):
    with open(fich) as f:
        u = chardet.universaldetector.UniversalDetector()
        u.feed(f.read(chunk))
        u.close()

    return u.result.get("encoding", "ascii")


class RowidReader():
    def __init__(self, nomFichero, tabla):
        self.nomFichero = nomFichero
        self.tabla = tabla
        self.where = None
        self.order = None
        self.running = False
        self.liRowids = []
        self.chunk = 2000

    def setOrder(self, order):
        self.order = order

    def setWhere(self, where):
        self.where = where

    def run(self, liRowids, filter, order ):
        self.stopnow()
        self.where = filter
        self.order = order
        self.running = True
        self.stop = False
        self.liRowids = liRowids
        self.lock = threading.Lock()
        self.thread = threading.Thread(target=self._run_thread)
        self.thread.daemon = True
        self.thread.start()

    def _run_thread(self):
        conexion = sqlite3.connect(self.nomFichero)
        sql = "SELECT ROWID FROM %s"%self.tabla
        if self.where:
            sql += " WHERE %s"%self.where
        if self.order:
            sql += " ORDER BY %s"%self.order
        cursor = conexion.cursor()
        cursor.execute(sql)
        ch = random.randint(100,300)
        while not self.stop:
            li = cursor.fetchmany(ch)
            if li:
                self.lock.acquire()
                self.liRowids.extend([x[0] for x in li])
                self.lock.release()
            if len(li) < ch:
                break
            ch = self.chunk
        cursor.close()
        conexion.close()
        self.running = False

    def terminado(self):
        return not self.running

    def stopnow(self):
        if self.running:
            self.stop = True
            self.thread.join()

    def reccount(self):
        return len(self.liRowids)


def is64Windows():
    return 'PROGRAMFILES(X86)' in os.environ


class Log:
    def __init__(self, logname):
        self.logname = logname

    def write(self, buf):
        ferr = open(self.logname, "at")
        ferr.write(buf)
        ferr.close()

# -*- coding: utf-8 -*-

import collections

from Code import BaseConfig


def leeRivales():
    dicRivales = collections.OrderedDict()

    def mas(cm):
        dicRivales[cm.clave] = cm

    ConfigMotor = BaseConfig.ConfigMotor

    cm = ConfigMotor("cheng", "Martin Sedlák", "4 ver 0.39", "http://www.vlasak.biz/cheng")
    cm.path = "cheng4_linux_x64"
    cm.elo = 2750
    cm.ponMultiPV(20, 256)
    mas(cm)

    cm = ConfigMotor("cinnamon", "Giuseppe Cannella", "1.2b", "http://cinnamonchess.altervista.org/")
    cm.path = "cinnamon_1.2b-generic"
    cm.ordenUCI("Hash", "32")
    cm.elo = 1930
    mas(cm)

    cm = ConfigMotor("critter", "Richard Vida", "1.6a", "http://www.vlasak.biz/critter/")
    cm.path = "critter-16a-64bit"
    cm.elo = 3091
    cm.ordenUCI("Hash", "32")
    cm.ordenUCI("Threads", "1")
    cm.ponMultiPV(20, 100)
    mas(cm)

    cm = ConfigMotor("clarabit", "Salvador Pallares Bejarano", "1.00", "http://sapabe.googlepages.com")
    cm.path = "clarabit_100_x64_linux"
    cm.elo = 2058
    cm.ordenUCI("OwnBook", "false")
    cm.ordenUCI("Ponder", "false")
    mas(cm)

    cm = ConfigMotor("komodo", "Don Dailey, Larry Kaufman", "10", "http://komodochess.com/")
    cm.path = "Linux/komodo-10-linux"
    cm.ordenUCI("Ponder", "false")
    cm.ordenUCI("Hash", "32")
    cm.elo = 3240
    cm.ponMultiPV(20, 218)
    mas(cm)

    cm = ConfigMotor("stockfish", "Tord Romstad, Marco Costalba, Joona Kiiski", "10", "http://stockfishchess.org/")
    cm.path = "Linux/stockfish_10_x64"
    cm.elo = 3300
    cm.ordenUCI("Ponder", "false")
    cm.ordenUCI("Hash", "64")
    cm.ordenUCI("Threads", "1")
    cm.ponMultiPV(20, 500)
    mas(cm)

    cm = ConfigMotor("greko", "Vladimir Medvedev", "10.2", "http://sourceforge.net/projects/greko")
    cm.path = "Linux/GreKo-102-64-ja"
    cm.elo = 2480
    mas(cm)

    cm = ConfigMotor("fruit", "Fabien Letouzey", "2.3.1", "http://www.fruitchess.com/")
    cm.path = "Fruit-2-3-1-Linux"
    cm.elo = 2784
    cm.ordenUCI("Hash", "32")
    cm.ponMultiPV(20, 256)
    mas(cm)

    cm = ConfigMotor("discocheck", "Lucas Braesch", "5.2.1", "https://github.com/lucasart/")
    cm.path = "discocheck_5.2.1_x86-64"
    cm.elo = 2700
    mas(cm)

    cm = ConfigMotor("gaviota", "Miguel A. Ballicora", "1.0", "https://sites.google.com/site/gaviotachessengine")
    cm.path = "gaviota-1.0-linux64"
    cm.elo = 2548
    cm.ordenUCI("Log", "false")
    mas(cm)

    cm = ConfigMotor("godel", "Juan Manuel Vazquez", "4.4.5", "https://sites.google.com/site/godelchessengine")
    cm.path = "Godel64"
    cm.elo = 2814
    cm.ordenUCI("Hash", "32")
    cm.ordenUCI("Ponder", "false")
    cm.nombre = "Gödel 4.4.5".decode("utf-8")
    mas(cm)

    cm = ConfigMotor("daydreamer", "Aaron Becker", "1.75 JA", "http://github.com/AaronBecker/daydreamer/downloads")
    cm.path = "daydreamer-175-32-ja"
    cm.elo = 2670
    cm.ordenUCI("Hash", "32")
    # cm.ponMultiPV( 20, 256 )
    mas(cm)

    cm = ConfigMotor("glaurung", "Tord RomsTad", "2.2 JA", "http://www.glaurungchess.com/")
    cm.path = "glaurung-64"
    cm.ordenUCI("Ponder", "false")
    cm.elo = 2765
    cm.ponMultiPV(20, 500)
    mas(cm)

    cm = ConfigMotor("gnuchess", "Chua Kong Sian,Stuart Cracraft,Lukas Geyer,Simon Waters,Michael Van den Bergh",
                     "5.50", "http://www.gnu.org/software/chess/")
    cm.path = "gnuchess-5.50-32"
    cm.elo = 2700
    cm.ordenUCI("Ponder", "false")
    cm.ordenUCI("Hash", "32")
    mas(cm)

    cm = ConfigMotor("pawny", "Mincho Georgiev", "1.2", "http://pawny.netii.net/")
    cm.path = "linux/pawny_1.2.x64"
    cm.elo = 2550
    mas(cm)

    cm = ConfigMotor("rocinante", "Antonio Torrecillas", "2.0", "http://sites.google.com/site/barajandotrebejos/")
    cm.path = "rocinante-64"
    cm.elo = 1800
    mas(cm)

    cm = ConfigMotor("simplex", "Antonio Torrecillas", "0.98", "http://sites.google.com/site/barajandotrebejos/")
    cm.path = "simplex-098-32-ja"
    cm.elo = 2396
    mas(cm)

    cm = ConfigMotor("texel", "Peter Österlund", "1.06", "http://web.comhem.se/petero2home/javachess/index.html#texel")
    cm.path = "texel64"
    cm.elo = 2900
    mas(cm)

    cm = ConfigMotor("irina", "Lucas Monge", "0.15", "https://github.com/lukasmonk/irina")
    cm.path = "irina"
    cm.elo = 1200
    mas(cm)

    cm = ConfigMotor("rodentII", "Pawel Koziol", "0.9.64", "http://www.pkoziol.cal24.pl/rodent/rodent.htm")
    cm.path = "rodentII"
    cm.elo = 2912
    cm.ordenUCI("Hash", "64")
    mas(cm)

    cm = ConfigMotor("mccain", "Michael Byrne", "X3", "https://github.com/MichaelB7/Stockfish/releases")
    cm.path = "McCain-X3_x64_linux"
    cm.elo = 3300
    cm.ordenUCI("Contempt", "0")
    cm.ordenUCI("Hash", "64")
    cm.ponMultiPV(20, 256)
    mas(cm)

    cm = ConfigMotor("andscacs", "Daniel José Queraltó", "0.8932n", "http://www.andscacs.com/")
    cm.path = "andscacs"
    cm.elo = 3240
    mas(cm)

    cm = ConfigMotor("zappa", "Anthony Cozzie", "1.1", "http://www.acoz.net/zappa/")
    cm.path = "zappa.exe"
    cm.elo = 2614
    mas(cm)

    return dicRivales


def dicMotoresFixedElo():
    d = leeRivales()
    dic = {}
    for nm, desde, hasta in (
            ("cheng", 800, 2500),
            ("greko", 1600, 2400),
            ("mccain", 1300, 2800),
            ("discocheck", 1500, 2700),
    ):
        for elo in range(desde, hasta + 100, 100):
            cm = d[nm].clona()
            if elo not in dic:
                dic[elo] = []
            cm.ordenUCI("UCI_Elo", str(elo))
            cm.ordenUCI("UCI_LimitStrength", "true")
            cm.clave += " (%d)" % elo
            cm.nombre += " (%d)" % elo
            dic[elo].append(cm)
    return dic

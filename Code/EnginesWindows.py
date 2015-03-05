# -*- coding: latin-1 -*-

import collections

import Code.BaseConfig as BaseConfig

def leeRivales():
    dicRivales = collections.OrderedDict()

    def mas(cm):
        dicRivales[cm.clave] = cm

    ConfigMotor = BaseConfig.ConfigMotor
    cm = ConfigMotor("tarrasch", "Bill Forster", "ToyEngine Beta V0.905", "http://www.triplehappy.com/")
    cm.path = "TarraschToyEngine.exe"
    cm.elo = 1481
    mas(cm)

    cm = ConfigMotor("rocinante", "Antonio Torrecillas", "2.0", "http://sites.google.com/site/barajandotrebejos/")
    cm.path = "Windows/Intel/rocinante-20-32-ja.exe"
    cm.elo = 1800
    mas(cm)

    cm = ConfigMotor("roce", "Roman Hartmann", "0.0390", "http://www.rocechess.ch/rocee.html")
    cm.path = "roce39"
    cm.elo = 1854
    mas(cm)

    cm = ConfigMotor("cinnamon", "Giuseppe Cannella", "1.2b", "http://cinnamonchess.altervista.org/")
    cm.path = "cinnamon_1.2b-generic.exe"
    cm.ordenUCI("Hash", "32")
    cm.elo = 1930
    mas(cm)

    cm = ConfigMotor("bikjump", "Aart J.C. Bik", "2.01 (32-bit)", "http://www.aartbik.com/")
    cm.path = "bikjump.exe"
    cm.elo = 2026
    mas(cm)

    cm = ConfigMotor("clarabit", "Salvador Pallares Bejarano", "1.00", "http://sapabe.googlepages.com")
    cm.path = "clarabit_100_x32_win.exe"
    cm.elo = 2058
    cm.ordenUCI("OwnBook", "false")
    cm.ordenUCI("Ponder", "false")
    mas(cm)

    cm = ConfigMotor("lime", "Richard Allbert", "v 66", "http://www.geocities.com/taciturn_lemon")
    cm.path = "Lime_v66.exe"
    cm.elo = 2119
    cm.ordenUCI("Ponder", "false")
    mas(cm)

    cm = ConfigMotor("chispa", "Federico Corigliano", "4.0.3", "http://chispachess.blogspot.com/")
    cm.path = "chispa403-blend.exe"
    cm.elo = 2227
    cm.ordenUCI("Ponder", "false")
    cm.ordenUCI("Hash", "32")
    mas(cm)

    cm = ConfigMotor("gaia", "Jean-Francois Romang, David Rabel", "3.5", "http://gaiachess.free.fr")
    cm.path = "gaia32.exe"
    cm.elo = 2378
    cm.ordenUCI("Ponder", "false")
    mas(cm)

    cm = ConfigMotor("simplex", "Antonio Torrecillas", "0.9.8", "http://sites.google.com/site/barajandotrebejos/" )
    cm.path = "simplex-098-32-ja.exe"
    cm.elo = 2396
    mas(cm)

    cm = ConfigMotor("greko", "Vladimir Medvedev", "12", "http://sourceforge.net/projects/greko")
    cm.path = "12/GreKo.exe"
    cm.elo = 2508
    mas(cm)

    cm = ConfigMotor("pawny", "Mincho Georgiev", "0.3.1", "http://pawny.netii.net/")
    cm.path = "windows/pawny_0.3.1_x86.exe"
    cm.elo = 2484
    cm.ordenUCI("OwnBook", "false")
    mas(cm)

    cm = ConfigMotor("hamsters", "Alessandro Scotti", "0.5", "https://chessprogramming.wikispaces.com/Alessandro+Scotti")
    cm.path = "Hamsters.exe"
    cm.elo = 2487
    cm.ordenUCI("OwnBook", "false")
    mas(cm)

    cm = ConfigMotor("umko", "Borko Boskovic", "0.7", "http://umko.sourceforge.net/")
    cm.path = "w32/umko_x32.exe"
    cm.elo = 2488
    mas(cm)

    cm = ConfigMotor("garbochess", "Gary Linscott", "2.20", "http://forwardcoding.com/projects/chess/chess.html")
    cm.path = "GarboChess2-32.exe"
    cm.elo = 2526
    cm.ordenUCI("Hash", "32")
    mas(cm)

    cm = ConfigMotor("ufim", "Niyas Khasanov", "8.02", "http://wbec-ridderkerk.nl/html/details1/Ufim.html")
    cm.path = "ufim802.exe"
    cm.elo = 2532
    cm.ordenUCI("Ponder", "false")
    cm.ordenUCI("Hash", "32")
    mas(cm)

    cm = ConfigMotor("amyan", "Antonio Dieguez R.", "1.62", "http://www.pincha.cl/amyan/amyane.html")
    cm.path = "amyan.exe"
    cm.elo = 2545
    mas(cm)

    cm = ConfigMotor("alaric", "Peter Fendrich", "707", "http://alaric.fendrich.se/index.html")
    cm.path = "alaric707.exe"
    cm.elo = 2662
    cm.ordenUCI("BookFile", "")
    cm.ordenUCI("Ponder", "false")
    cm.ordenUCI("Hash", "32")
    mas(cm)

    cm = ConfigMotor("cyrano", "Harald Johnsen", "06B17", "http://sites.estvideo.net/tipunch/cyrano/")
    cm.path = "cyrano.exe"
    cm.elo = 2647
    cm.ordenUCI("Ponder", "false")
    cm.ordenUCI("Hash", "32")
    mas(cm)

    cm = ConfigMotor("daydreamer", "Aaron Becker", "1.75 JA", "http://github.com/AaronBecker/daydreamer/downloads")
    cm.path = "windows/32 bit/daydreamer-175-32-ja.exe"
    cm.elo = 2670
    cm.ordenUCI("Hash", "32")
    mas(cm)

    cm = ConfigMotor("godel", "Juan Manuel Vazquez", "3.4.9", "https://sites.google.com/site/godelchessengine")
    cm.path = "godel.exe"
    cm.elo = 2720
    cm.ordenUCI("Hash", "32")
    cm.ordenUCI("Ponder", "false")
    mas(cm)

    cm = ConfigMotor("rhetoric", "Alberto Sanjuan", "1.4.1", "http://www.chessrhetoric.com/")
    cm.path = "Rhetoric_x32.exe"
    cm.elo = 2750
    cm.ordenUCI("Hash", "32")
    cm.ponMultiPV(1, 4)
    mas(cm)

    cm = ConfigMotor("cheng", "Martin Sedlák", "4 0.38", "http://www.vlasak.biz/cheng")
    cm.path = "cheng4.exe"
    cm.elo = 2750
    cm.ponMultiPV(20, 256)
    mas(cm)

    cm = ConfigMotor("glaurung", "Tord RomsTad", "2.2 JA", "http://www.glaurungchess.com/")
    cm.path = "windows/glaurung-w32.exe"
    cm.ordenUCI("Ponder", "false")
    cm.elo = 2765
    cm.ponMultiPV(20, 500)
    mas(cm)

    cm = ConfigMotor("fruit", "Fabien Letouzey", "2.3.1", "http://www.fruitchess.com/")
    cm.path = "Fruit-2-3-1.exe"
    cm.elo = 2786
    cm.ponMultiPV(20, 256)
    mas(cm)

    cm = ConfigMotor("rodent", "Pawel Koziol", "1.6", "http://www.pkoziol.cal24.pl/rodent/rodent.htm")
    cm.path = "rodent32.exe"
    cm.elo = 2790
    cm.ordenUCI("Hash", "32")
    mas(cm)

    cm = ConfigMotor("discocheck", "Lucas Braesch", "5.2", "https://github.com/lucasart")
    cm.path = "DiscoCheck52_x32.exe"
    cm.elo = 2890
    mas(cm)

    cm = ConfigMotor("gaviota", "Miguel A. Ballicora", "1.0", "https://sites.google.com/site/gaviotachessengine")
    cm.path = "gaviota-1.0-win32.exe"
    cm.elo = 2950
    cm.ponMultiPV(20, 32)
    mas(cm)

    cm = ConfigMotor("toga", "WHMoweryJr,Thomas Gaksch,Fabien Letouzey", "deepTogaNPS 1.9.6", "http://www.computerchess.info/tdbb/phpBB3/viewtopic.php?f=9&t=357")
    cm.path = "DeepToga1.9.6nps.exe"
    cm.elo = 2843
    cm.ordenUCI("Hash", "32")
    cm.ponMultiPV(20, 40)
    mas(cm)

    cm = ConfigMotor("komodo", "Don Dailey, Larry Kaufman", "5.1r2 32bit", "http://komodochess.com/")
    cm.path = "komodo51r2-32bit.exe"
    cm.ordenUCI("Ponder", "false")
    cm.ordenUCI("Hash", "32")
    cm.elo = 3053
    cm.ponMultiPV(20, 99)
    mas(cm)

    cm = ConfigMotor("rybka", "Vasik Rajlich", "2.3.2a 32-bit", "http://rybkachess.com/")
    cm.path = "Rybka v2.3.2a.w32.exe"
    cm.elo = 2936
    cm.ordenUCI("Hash", "32")
    cm.ordenUCI("Ponder", "false")
    cm.ordenUCI("Max CPUs", "1")
    cm.ponMultiPV(20, 100)
    mas(cm)

    cm = ConfigMotor("critter", "Richard Vida", "1.6a 32bits", "http://www.vlasak.biz/critter/")
    cm.path = "Critter_1.6a_32bit.exe"
    cm.elo = 3091
    cm.ordenUCI("Hash", "32")
    cm.ordenUCI("Threads", "1")
    cm.ponMultiPV(20, 100)
    mas(cm)

    cm = ConfigMotor("texel", "Peter Österlund", "1.05", "http://web.comhem.se/petero2home/javachess/index.html#texel")
    cm.path = "texel32old.exe"
    cm.elo = 3100
    cm.ordenUCI("Hash", "32")
    cm.ordenUCI("Ponder", "false")
    cm.ponMultiPV(20, 256)
    mas(cm)

    cm = ConfigMotor("stockfish", "Tord Romstad, Marco Costalba, Joona Kiiski", "6", "http://stockfishchess.org/")
    cm.path = "Windows\stockfish-6-32.exe"
    cm.elo = 3300
    cm.ordenUCI("Ponder", "false")
    cm.ordenUCI("Hash", "32")
    cm.ordenUCI("Threads", "1")
    cm.ponMultiPV(20, 500)
    mas(cm)

    return dicRivales

def dicMotoresFixedElo():
    d = leeRivales()
    dic = {}
    for nm, desde, hasta in (
                                ("amyan", 1000, 2400),
                                ("rhetoric", 1300, 2600),
                                ("cheng", 800, 2500),
                                ("greko", 1600, 2400),
                                ("hamsters", 1000, 2000),
                                ("rybka", 1200, 2400),
                                ("ufim", 700, 2000),
                                ("discocheck", 1500, 2700),
                                ("rodent", 600, 2600),
                            ):
        for elo in range(desde, hasta+100, 100):
            cm = d[nm].clona()
            if elo not in dic:
                dic[elo] = []
            cm.ordenUCI("UCI_LimitStrengh", "true")
            cm.ordenUCI("UCI_Elo", str(elo))
            cm.nombre += " (%d)"%elo
            cm.clave += " (%d)"%elo
            dic[elo].append(cm)
    return dic

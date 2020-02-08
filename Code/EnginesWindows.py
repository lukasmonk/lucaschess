# -*- coding: utf-8 -*-

import collections

from Code import BaseConfig


def leeRivales():
    dicRivales = collections.OrderedDict()

    def mas(cm):
        dicRivales[cm.clave] = cm
        # p-rint """["%s", "%s", "%s"],"""%(cm.nombre, cm.autor, cm.url)

    ConfigMotor = BaseConfig.ConfigMotor
    cm = ConfigMotor("acqua", "Giovanni Di Maria", "2.0", "http://www.elektrosoft.it/scacchi/acqua/acqua.asp")
    cm.path = "acqua.exe"
    cm.elo = 844
    mas(cm)

    cm = ConfigMotor("tarrasch", "Bill Forster", "ToyEngine Beta V0.905", "http://www.triplehappy.com/")
    cm.path = "TarraschToyEngine.exe"
    cm.elo = 1481
    mas(cm)

    cm = ConfigMotor("rocinante", "Antonio Torrecillas", "2.0", "http://sites.google.com/site/barajandotrebejos/")
    cm.path = "Windows/Intel/rocinante-20-32-ja.exe"
    cm.elo = 1800
    mas(cm)

    cm = ConfigMotor("roce", "Roman Hartmann", "0.0390", "http://www.rocechess.ch/rocee.html")
    cm.path = "roce39.exe"
    cm.elo = 1854
    mas(cm)

    cm = ConfigMotor("cinnamon", "Giuseppe Cannella", "1.2c", "http://cinnamonchess.altervista.org/")
    cm.path = "cinnamon_1.2c-generic.exe"
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
    cm.nombre = "Gaïa 3.5".decode("utf-8")
    cm.elo = 2378
    cm.ordenUCI("Ponder", "false")
    mas(cm)

    cm = ConfigMotor("simplex", "Antonio Torrecillas", "0.9.8", "http://sites.google.com/site/barajandotrebejos/")
    cm.path = "Windows/simplex-098-32-ja.exe"
    cm.elo = 2396
    mas(cm)

    cm = ConfigMotor("pawny", "Mincho Georgiev", "0.3.1", "http://pawny.netii.net/")
    cm.path = "windows/pawny_0.3.1_x86.exe"
    cm.elo = 2484
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

    cm = ConfigMotor("godel", "Juan Manuel Vazquez", "4.4.5", "https://sites.google.com/site/godelchessengine")
    cm.path = "Godel32.exe"
    cm.elo = 2814
    cm.ordenUCI("Hash", "32")
    cm.ordenUCI("Ponder", "false")
    cm.nombre = "Gödel 4.4.5".decode("utf-8")
    mas(cm)

    cm = ConfigMotor("rhetoric", "Alberto Sanjuan", "1.4.3", "http://www.chessrhetoric.com/")
    cm.path = "Rhetoric_x32.exe"
    cm.elo = 2810
    cm.ordenUCI("Hash", "32")
    cm.ponMultiPV(1, 4)
    mas(cm)

    cm = ConfigMotor("cheng", "Martin Sedlák".decode("utf-8"), "4 0.39", "http://www.vlasak.biz/cheng")
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

    cm = ConfigMotor("discocheck", "Lucas Braesch", "5.2.1", "https://github.com/lucasart")
    cm.path = "DiscoCheck.exe"
    cm.elo = 2890
    mas(cm)

    cm = ConfigMotor("gaviota", "Miguel A. Ballicora", "1.0", "https://sites.google.com/site/gaviotachessengine")
    cm.path = "gaviota-1.0-win32.exe"
    cm.elo = 2950
    cm.ponMultiPV(20, 32)
    cm.ordenUCI("Log", "false")
    mas(cm)

    cm = ConfigMotor("komodo", "Don Dailey, Larry Kaufman, Mark Lefler", "10 32bit", "http://komodochess.com/")
    cm.path = "komodo-10-32bit.exe"
    cm.path_64 = "komodo-10-64bit.exe", "10 64bit"
    cm.ordenUCI("Ponder", "false")
    cm.ordenUCI("Hash", "64")
    cm.elo = 3240
    cm.ponMultiPV(20, 218)
    mas(cm)

    cm = ConfigMotor("rybka", "Vasik Rajlich", "2.3.2a 32-bit", "http://rybkachess.com/")
    cm.path = "Rybka v2.3.2a.w32.exe"
    cm.elo = 2936
    cm.ordenUCI("Hash", "32")
    cm.ordenUCI("Ponder", "false")
    cm.ordenUCI("Max CPUs", "1")
    cm.ponMultiPV(20, 100)
    mas(cm)

    cm = ConfigMotor("critter", "Richard Vida", "1.6a 32bit", "http://www.vlasak.biz/critter/")
    cm.path = "Critter_1.6a_32bit.exe"
    cm.elo = 3091
    cm.ordenUCI("Hash", "32")
    cm.ordenUCI("Threads", "1")
    cm.ponMultiPV(20, 100)
    mas(cm)

    cm = ConfigMotor("texel", "Peter Österlund".decode("utf-8"), "1.07 32bit", "http://hem.bredband.net/petero2b/javachess/index.html#texel")
    cm.path = "texel32old.exe"
    cm.elo = 3100
    cm.ordenUCI("Hash", "32")
    cm.ordenUCI("Ponder", "false")
    cm.ponMultiPV(20, 256)
    mas(cm)

    cm = ConfigMotor("stockfish", "Tord Romstad, Marco Costalba, Joona Kiiski", "11 32bits", "http://stockfishchess.org/")
    cm.path = "Windows/stockfish_20011801_32bit.exe"
    cm.path_64 = "Windows/stockfish_20011801_x64_bmi2.exe", "11 64bits bmi2"
    cm.elo = 3300
    cm.ordenUCI("Ponder", "false")
    cm.ordenUCI("Hash", "64")
    cm.ordenUCI("Threads", "1")
    cm.ponMultiPV(20, 500)
    mas(cm)

    cm = ConfigMotor("honey", "Michael Byrne", "XI 32bit", "https://github.com/MichaelB7/Stockfish/tree/honey")
    cm.path = "Honey-XI_x32_old.exe"
    cm.path_64 = "Honey-XI_x64_bmi2.exe", "XI 64bit bmi2"
    cm.elo = 3300
    cm.ordenUCI("Contempt", "0")
    cm.ordenUCI("Hash", "64")
    cm.ponMultiPV(20, 256)
    mas(cm)

    cm = ConfigMotor("gull", "Vadim Demichev", "3 32bit", "https://sourceforge.net/projects/gullchess/")
    cm.path = "Gull 3 w32 XP.exe"
    cm.elo = 3125
    cm.ordenUCI("Hash", "32")
    cm.ordenUCI("Threads", "1")
    # cm.ponMultiPV(20, 64)
    mas(cm)

    cm = ConfigMotor("irina", "Lucas Monge", "0.15", "https://github.com/lukasmonk/irina")
    cm.path = "irina.exe"
    cm.elo = 1200
    mas(cm)

    cm = ConfigMotor("rodentII", "Pawel Koziol", "0.9.64", "http://www.pkoziol.cal24.pl/rodent/rodent.htm")
    cm.path = "RodentII_x32.exe"
    cm.path_64 = "RodentII_x64.exe", "0.9.64 64bit"
    cm.elo = 2912
    cm.ordenUCI("Hash", "64")
    mas(cm)

    cm = ConfigMotor("amyan", "Antonio Dieguez R.", "1.62", "http://www.pincha.cl/amyan/amyane.html")
    cm.path = "amyan.exe"
    cm.elo = 2545
    mas(cm)

    cm = ConfigMotor("hamsters", "Alessandro Scotti", "0.5", "https://chessprogramming.wikispaces.com/Alessandro+Scotti")
    cm.path = "Hamsters.exe"
    cm.elo = 2487
    cm.ordenUCI("OwnBook", "false")
    cm.removeLog("problem_log.txt")
    mas(cm)

    cm = ConfigMotor("toga", "WHMoweryJr,Thomas Gaksch,Fabien Letouzey", "deepTogaNPS 1.9.6",
                     "http://www.computerchess.info/tdbb/phpBB3/viewtopic.php?f=9&t=357")
    cm.path = "DeepToga1.9.6nps.exe"
    cm.elo = 2843
    cm.ordenUCI("Hash", "32")
    cm.ponMultiPV(20, 40)
    mas(cm)

    cm = ConfigMotor("greko", "Vladimir Medvedev", "12.9", "http://sourceforge.net/projects/greko")
    cm.path = "12/GreKo.exe"
    cm.elo = 2508
    mas(cm)

    cm = ConfigMotor("delfi", "Fabio Cavicchio", "5.4", "http://www.msbsoftware.it/delfi/")
    cm.path = "delfi.exe"
    cm.elo = 2686
    mas(cm)

    # cm = ConfigMotor("smartthink", "Sergei S. Markoff", "1.97", "http://genes1s.net/smarthink.php")
    # cm.path = "SmarThink_v197_x32.exe"
    # cm.nombre = "SmartThink"
    # cm.elo = 2970
    # mas(cm)

    cm = ConfigMotor("monarch", "Steve Maughan", "1.7", "http://www.monarchchess.com/")
    cm.path = "Monarch(v1.7).exe"
    cm.elo = 2100
    mas(cm)

    cm = ConfigMotor("andscacs", "Daniel José Queraltó".decode("utf-8"), "0.9432n", "http://www.andscacs.com/")
    cm.path = "andscacs_32_no_popcnt.exe"
    cm.elo = 3150
    mas(cm)

    cm = ConfigMotor("arminius", "Volker Annus", "2017-01-01", "http://www.nnuss.de/Hermann/Arminius2017-01-01.zip")
    cm.path = "Arminius2017-01-01-32Bit.exe"
    cm.elo = 2662
    mas(cm)

    cm = ConfigMotor("wildcat", "Igor Korshunov", "8", "http://www.igorkorshunov.narod.ru/WildCat")
    cm.path = "WildCat_8.exe"
    cm.elo = 2627
    mas(cm)

    cm = ConfigMotor("demolito", "Lucas Braesch", "32bit", "https://github.com/lucasart/Demolito")
    cm.path = "demolito_32bit_old.exe"
    cm.elo = 2627
    mas(cm)

    cm = ConfigMotor("spike", "Volker Böhm and Ralf Schäfer".decode("utf-8"), "1.4", "http://spike.lazypics.de/index_en.html")
    cm.path = "Spike1.4.exe"
    cm.elo = 2921
    mas(cm)

    cm = ConfigMotor("zappa", "Anthony Cozzie", "1.1", "http://www.acoz.net/zappa/")
    cm.path = "zappa.exe"
    cm.elo = 2581
    cm.removeLog("zappa_log.txt")
    mas(cm)

    cm = ConfigMotor("houdini", "Robert Houdart", "1.5a", "http://www.cruxis.com/chess/houdini.htm")
    cm.path = "Houdini_15a_w32.exe"
    cm.elo = 3093
    mas(cm)

    cm = ConfigMotor("hannibal", "Samuel N. Hamilton and Edsel G. Apostol", "1.4b", "http://sites.google.com/site/edapostol/hannibal")
    cm.path = "Hannibal1.4bx32.exe"
    cm.elo = 3000
    cm.removeLog("logfile.txt")
    mas(cm)

    cm = ConfigMotor("paladin", "Ankan Banerjee", "0.1", "https://github.com/ankan-ban/chess_cpu")
    cm.path = "Paladin_32bits_old.exe"
    cm.elo = 2254
    mas(cm)

    cm = ConfigMotor("cdrill", "Ferdinand Mosca", "1800 Build 4", "https://sites.google.com/view/cdrill")
    cm.path = "CDrill_1800_Build_4.exe"
    cm.elo = 1800
    mas(cm)

    return dicRivales


def dicMotoresFixedElo():
    d = leeRivales()
    dic = {}
    for nm, desde, hasta in (
            ("rodent", 600, 2600),
            ("amyan", 1000, 2400),
            ("honey", 1000, 2900),
            ("rhetoric", 1300, 2600),
            ("cheng", 800, 2500),
            ("greko", 1600, 2400),
            ("hamsters", 1000, 2000),
            ("rybka", 1200, 2400),
            ("ufim", 700, 2000),
            ("delfi", 1000, 1000),
            ("spike", 1100, 2500),
    ):
        for elo in range(desde, hasta + 100, 100):
            cm = d[nm].clona()
            if elo not in dic:
                dic[elo] = []
            cm.ordenUCI("UCI_Elo", str(elo))
            cm.ordenUCI("UCI_LimitStrength", "true")
            cm.nombre += " (%d)" % elo
            cm.clave += " (%d)" % elo
            dic[elo].append(cm)
    return dic

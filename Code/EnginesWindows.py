import collections

import Code.BaseConfig as BaseConfig

def leeRivales():
    dicRivales = collections.OrderedDict()

    def mas(cm):
        dicRivales[cm.clave] = cm

    ConfigMotor = BaseConfig.ConfigMotor
    cm = ConfigMotor("tarrasch", "Bill Forster", "ToyEngine Beta V0.905", "http://www.triplehappy.com/")
    cm.path = "TarraschToyEngine.exe"
    # cm.ordenUCI("Ponder", "false")
    cm.elo = 1481
    # cm.ponMultiPV( 4 )
    mas(cm)

    cm = ConfigMotor("rocinante", "Antonio Torrecillas", "2.0", "http://sites.google.com/site/barajandotrebejos/")
    cm.path = "Windows/Intel/rocinante-20-32-ja.exe"
    # cm.ordenUCI( "Ponder", "false" )
    cm.elo = 1800
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

    cm = ConfigMotor("greko", "Vladimir Medvedev", "10.2", "http://greko.110mb.com/index.html")
    cm.path = "10.2/bin/GreKo.exe"
    cm.elo = 2480
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
    # cm.ponMultiPV( 20, 256 )
    mas(cm)

    cm = ConfigMotor("glaurung", "Tord RomsTad", "2.2 JA", "http://www.glaurungchess.com/")
    cm.path = "windows/glaurung-w32.exe"
    cm.ordenUCI("Ponder", "false")
    cm.elo = 2765
    cm.ponMultiPV(20, 500)
    mas(cm)

    cm = ConfigMotor("toga", "WHMoweryJr,Thomas Gaksch,Fabien Letouzey", "deepTogaNPS 1.9.6",
                     "http://www.computerchess.info/tdbb/phpBB3/viewtopic.php?f=9&t=357")
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

    cm = ConfigMotor("stockfish", "Tord Romstad, Marco Costalba, Joona Kiiski", "5", "http://stockfishchess.org/")
    cm.path = "windows/stockfish_15011122_32bit.exe"
    cm.elo = 3079
    cm.ordenUCI("Ponder", "false")
    cm.ordenUCI("Hash", "32")
    cm.ordenUCI("Threads", "1")
    cm.ponMultiPV(20, 500)
    mas(cm)
    return dicRivales

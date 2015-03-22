import os
import shutil
import random

from Code.Constantes import *
import Code.VarGen as VarGen
import Code.Memoria as Memoria
import Code.Util as Util
import Code.PGNreader as PGNreader
import Code.TrListas as TrListas
import Code.Boxing as Boxing
import Code.GestorEntTac as GestorEntTac
import Code.GestorGM as GestorGM
import Code.Gestor60 as Gestor60
import Code.GestorMate as GestorMate
import Code.GestorBooks as GestorBooks
import Code.GestorAperturas as GestorAperturas
import Code.GestorBoxing as GestorBoxing
import Code.Tacticas as Tacticas
import Code.QT.Iconos as Iconos
import Code.QT.QTUtil2 as QTUtil2
import Code.QT.PantallaGM as PantallaGM
import Code.QT.PantallaBooks as PantallaBooks
import Code.QT.DatosNueva as DatosNueva
import Code.QT.QTVarios as QTVarios
import Code.QT.PantallaAperturas as PantallaAperturas
import Code.QT.PantallaDailyTest as PantallaDailyTest
import Code.QT.PantallaBMT as PantallaBMT
import Code.QT.PantallaPotencia as PantallaPotencia
import Code.QT.PantallaPuente as PantallaPuente
import Code.QT.PantallaBoxing as PantallaBoxing
import Code.QT.PantallaHorses as PantallaHorses
import Code.QT.PantallaTacticas as PantallaTacticas
import Code.QT.PantallaLearnPGN as PantallaLearnPGN
import Code.QT.PantallaVisualiza as PantallaVisualiza

class TrainingFNS:
    def __init__(self, path, name):
        self.name = name
        self.path = path

class TrainingDir:
    def __init__(self, carpeta):
        dicTraining = TrListas.dicTraining()

        def trTraining(txt):
            return dicTraining.get(txt, txt)

        self.tr = trTraining

        self.name = trTraining(os.path.basename(carpeta))
        self.read(carpeta)

    def read(self, carpeta):
        isdir = os.path.isdir
        folders = []
        files = []
        for elem in os.listdir(carpeta):
            path = os.path.join(carpeta, elem)
            if isdir(path):
                folders.append(TrainingDir(path))
            elif elem.lower().endswith(".fns"):
                name = self.tr(os.path.basename(path)[:-4])
                files.append(TrainingFNS(path, name))
        self.folders = sorted(folders, key=lambda td: td.name)
        self.files = sorted(files, key=lambda td: td.name)

    def addOtherFolder(self, folder):
        self.folders.append(TrainingDir(folder))

    def vacio(self):
        return (len(self.folders) + len(self.files)) == 0

    def reduce(self):
        liBorrar = []
        for n, folder in enumerate(self.folders):
            folder.reduce()
            if folder.vacio():
                liBorrar.append(n)
        if liBorrar:
            for n in range(len(liBorrar) - 1, -1, -1):
                del self.folders[liBorrar[n]]

    def menu(self, bmenu, xopcion):
        icoOp = Iconos.PuntoNaranja()
        icoDr = Iconos.Carpeta()
        for folder in self.folders:
            submenu1 = bmenu.submenu(folder.name, icoDr)
            folder.menu(submenu1, xopcion)
        for file in self.files:
            xopcion(bmenu, "ep_%s" % file.path, file.name, icoOp)

class Entrenamientos():
    def __init__(self, procesador):
        self.procesador = procesador
        self.parent = procesador.pantalla
        self.configuracion = procesador.configuracion
        self.menu, self.dicMenu = self.creaMenu()

    def menuFNS(self, menu, rotulo, xopcion):
        td = TrainingDir("Trainings")
        td.addOtherFolder(self.procesador.configuracion.dirPersonalTraining)
        bmenu = menu.submenu(rotulo, Iconos.Carpeta())
        td.reduce()  # Elimina carpetas vacias
        td.menu(bmenu, xopcion)

    def creaMenu(self):
        dicMenu = {}
        menu = QTVarios.LCMenu(self.parent)

        def xopcion(menu, clave, texto, icono, siDeshabilitado=False):
            menu.opcion(clave, texto, icono, siDeshabilitado)
            dicMenu[clave] = (clave, texto, icono, siDeshabilitado )

        # Posiciones de entrenamiento --------------------------------------------------------------------------
        self.menuFNS(menu, _("Training positions"), xopcion)
        menu.separador()

        # GM ---------------------------------------------------------------------------------------------------
        xopcion(menu, "gm", _("Play like a grandmaster"), Iconos.GranMaestro())
        menu.separador()

        # Mate --------------------------------------------------------------------------------------------------
        menu1 = menu.submenu(_("Training mates"), Iconos.Mate())
        for mate in range(1, 5):
            xopcion(menu1, "mate%d" % mate, _X(_("Mate in %1"), str(mate)), Iconos.PuntoAzul())
            menu1.separador()
        menu.separador()

        # BMT -------------------------------------------------------------------------------------------
        xopcion(menu, "bmt", _("Find best move"), Iconos.BMT())
        menu.separador()

        # Openings ------------------------------------------------------------------------------------------------
        menu1 = menu.submenu(_("Openings"), Iconos.Aperturas())
        # # Aperturas --------------------------------------------------------------------------------------------
        xopcion(menu1, "aperturas", _("Learn openings by repetition"), Iconos.Apertura())
        menu1.separador()
        ## Books ------------------------------------------------------------------------------------------------
        xopcion(menu1, "polyglot", _("Training with a book"), Iconos.Libros())

        menu.separador()

        # DailyTest ------------------------------------------------------------------------------------------------
        xopcion(menu, "dailytest", _("Your daily test"), Iconos.DailyTest())
        menu.separador()

        # Resistencia ------------------------------------------------------------------------------------------
        menu1 = menu.submenu(_("Resistance Test"), Iconos.Resistencia())
        nico = Util.Rondo(Iconos.Verde(), Iconos.Azul(), Iconos.Amarillo(), Iconos.Naranja())
        xopcion(menu1, "boxing", _("Normal"), nico.otro())
        xopcion(menu1, "boxingc", _("Blindfold chess"), nico.otro())
        xopcion(menu1, "boxingp1", _("Hide only our pieces"), nico.otro())
        xopcion(menu1, "boxingp2", _("Hide only opponent pieces"), nico.otro())
        menu.separador()

        # Tacticas ---------------------------------------------------------------------------------------------
        menu1 = menu.submenu(_("Learn tactics by repetition"), Iconos.Tacticas())
        nico = Util.Rondo(Iconos.Amarillo(), Iconos.Naranja(), Iconos.Verde(), Iconos.Azul(), Iconos.Magenta())
        dicTraining = TrListas.dicTraining()

        def trTraining(txt):
            return dicTraining.get(txt, txt)

        def menuTacticas(tipo, carpetaBase):
            lista = []
            if os.path.isdir(carpetaBase):
                li = Util.listdir(carpetaBase)
                for nombre in li:
                    carpeta = os.path.join(carpetaBase, nombre)
                    ini = os.path.join(carpeta, "Config.ini")
                    if os.path.isdir(carpeta) and os.path.isfile(ini):
                        xopcion(menu1, "tactica|%s|%s|%s|%s" % (tipo, nombre, carpeta, ini), trTraining(nombre),
                                nico.otro())
                        menu1.separador()
                        lista.append((carpeta, nombre))
            return lista

        menuTacticas("B", "Tactics")
        carpetaTacticasP = os.path.join(self.configuracion.dirPersonalTraining, "Tactics")
        lista = menuTacticas("P", carpetaTacticasP)
        if lista:
            ico = Iconos.Delete()
            menub = menu1.submenu(_("Remove"), ico)
            for carpeta, nombre in lista:
                xopcion(menub, "remtactica|%s|%s" % (carpeta, nombre), trTraining(nombre), ico)

        menu.separador()

        # Maps ----------------------------------------------------------------------------------------
        menu1 = menu.submenu(_("Training on a map"), Iconos.Maps())
        xopcion(menu1, "map_Africa", _("Africa map"), Iconos.Africa())
        menu1.separador()
        xopcion(menu1, "map_WorldMap", _("World map"), Iconos.WorldMap())

        # Cebras ---------------------------------------------------------------------------------------------------
        menu.separador()
        menu1 = menu.submenu(_("Resources for zebras"), Iconos.Cebra())
        menu2 = menu1.submenu(_("Check your memory on a chessboard"), Iconos.Memoria())
        rival = self.configuracion.rival

        mem = Memoria.Memoria(self.procesador)

        for x in range(6):
            cat = rival.categorias.numero(x)
            txt = cat.nombre()

            nm = mem.nivel(x)
            if nm > -1:
                txt += " %s %d" % (_("Level"), nm + 1)

            xopcion(menu2, -100 - x, txt, cat.icono(), siDeshabilitado=not mem.siActiva(x))

        menu1.separador()

        menu2 = menu1.submenu(_("Find all moves"), Iconos.J60())
        xopcion(menu2, "j60_rival", _("Opponent"), Iconos.PuntoNaranja())
        xopcion(menu2, "j60_jugador", _("Player"), Iconos.PuntoAzul())

        menu1.separador()
        self.horsesDef = hd = {
            1: ( "N", "Alpha", _("Basic test") ),
            2: ( "p", "Fantasy", _("Four pawns test") ),
            3: ( "Q", "Pirat", _("Jonathan Levitt test") ),
            4: ( "n", "Spatial", _("Basic test") + ": a1" ),
            5: ( "N", "Cburnett", _("Basic test") + ": e4" )
        }
        menu2 = menu1.submenu(_("Becoming a knight tamer"), self.procesador.tablero.piezas.icono("N"))
        vicon = VarGen.todasPiezas.icono
        icl, icn, tit = hd[1]
        menu3 = menu2.submenu(tit, vicon(icl, icn))
        xopcion(menu3, "horses_1", tit, vicon(icl, icn))
        menu3.separador()
        icl, icn, tit = hd[4]
        xopcion(menu3, "horses_4", tit, vicon(icl, icn))
        menu3.separador()
        icl, icn, tit = hd[5]
        xopcion(menu3, "horses_5", tit, vicon(icl, icn))
        menu2.separador()
        icl, icn, tit = hd[2]
        xopcion(menu2, "horses_2", tit, vicon(icl, icn))
        menu2.separador()
        icl, icn, tit = hd[3]
        xopcion(menu2, "horses_3", tit, vicon(icl, icn))

        menu1.separador()
        menu2 = menu1.submenu(_("Moves between two positions"), Iconos.Puente())
        rp = QTVarios.rondoPuntos()
        for x in range(1, 11):
            xopcion(menu2, "puente_%d" % x, "%s %d" % (_("Level"), x), rp.otro())

        menu1.separador()
        xopcion(menu1, "potencia", _("Determine your calculating power"), Iconos.Potencia())

        menu1.separador()
        xopcion(menu1, "learnPGN", _("Learn a game"), Iconos.PGN())

        menu1.separador()
        xopcion(menu1, "visualiza", _("The board at a glance"), Iconos.Gafas())

        # Ratings
        menu.separador()
        menu1 = menu.submenu(_("Training ratings"), Iconos.Elo())
        xopcion(menu1, "lucaselo", "%s (%d)" % (_("Lucas-Elo"), self.configuracion.eloNC), Iconos.Elo())
        menu1.separador()
        xopcion(menu1, "micelo", "%s (%d)" % (_("Tourney-Elo"), self.configuracion.micheloNC), Iconos.EloTimed())
        menu1.separador()
        fics = self.configuracion.ficsNC
        menuf = menu1.submenu("%s (%d)" % (_("Fics-Elo"), fics), Iconos.Fics())
        rp = QTVarios.rondoPuntos()
        for elo in range(900, 2800, 100):
            if (elo == 900) or (0 <= (elo + 99 - fics) <= 400 or 0 <= (fics - elo) <= 400):
                xopcion(menuf, "fics%d" % (elo / 100,), "%d-%d" % (elo, elo + 99), rp.otro())
        menu1.separador()
        fide = self.configuracion.fideNC
        menuf = menu1.submenu("%s (%d)" % (_("Fide-Elo"), fide), Iconos.Fide())
        rp = QTVarios.rondoPuntos()
        for elo in range(1500, 2700, 100):
            if (elo == 1500) or (0 <= (elo + 99 - fide) <= 400 or 0 <= (fide - elo) <= 400):
                xopcion(menuf, "fide%d" % (elo / 100,), "%d-%d" % (elo, elo + 99), rp.otro())

        return menu, dicMenu

    def menuFavoritos(self, liFavoritos):

        menu = QTVarios.LCMenu(self.parent)

        for clave, texto, icono, siDeshabilitado in liFavoritos:
            menu.opcion(clave, texto, icono, siDeshabilitado)
            menu.separador()

        menu.opcion("menu_global", _("Training"), Iconos.Entrenamiento())

        return menu

    def comprueba(self):
        if self.menu is None:
            self.menu, self.dicMenu = self.creaMenu()

    def rehaz(self):
        self.menu, self.dicMenu = self.creaMenu()

    def lanza(self, siFavoritos=True):

        self.comprueba()

        liFavoritos = None
        if siFavoritos:
            liFav = self.procesador.configuracion.liFavoritos
            if liFav:
                li = []
                for elem in liFav:
                    if elem in self.dicMenu:
                        li.append(self.dicMenu[elem])
                liFavoritos = li

        if liFavoritos:
            menu = self.menuFavoritos(liFavoritos)
        else:
            menu = self.menu

        resp = menu.lanza()

        if resp:
            siStr = type(resp) == type("")
            if siStr:
                if resp == "menu_global":
                    self.lanza(False)

                elif resp == "gm":
                    self.entrenaGM()

                elif resp.startswith("mate"):
                    self.jugarMate(int(resp[-1]))

                elif resp == "bmt":
                    self.bmt()

                elif resp == "polyglot":
                    self.entrenaBooks()

                elif resp.startswith("boxing"):
                    self.boxing(resp[6:])

                elif resp in ["j60_rival", "j60_jugador"]:
                    self.jugar60(resp == "j60_jugador")

                elif resp == "aperturas":
                    self.aperturas()

                elif resp == "dailytest":
                    self.dailyTest()

                elif resp == "potencia":
                    self.potencia()

                elif resp == "visualiza":
                    self.visualiza()

                elif resp.startswith("tactica|"):
                    nada, tipo, nombre, carpeta, ini = resp.split("|")
                    self.tacticas(tipo, nombre, carpeta, ini)

                elif resp.startswith("remtactica|"):
                    nada, carpeta, nombre = resp.split("|")
                    self.tacticaRemove(carpeta, nombre)

                elif resp.startswith("puente_"):
                    self.puente(int(resp[7:]))

                elif resp.startswith("horses_"):
                    test = int(resp[7])
                    icl, icn, tit = self.horsesDef[test]
                    icon = VarGen.todasPiezas.icono(icl, icn)
                    self.horses(test, tit, icon)

                elif resp.startswith("ep_"):
                    um = self.procesador.unMomento()
                    entreno = resp[3:].replace("\\", "/")
                    if "/" in entreno:
                        dicTraining = TrListas.dicTraining()
                        titentreno = ""
                        for x in entreno[:-4].split("/")[1:]:
                            titentreno += dicTraining.get(x, x) + "/"
                        titentreno = titentreno[:-1]
                    f = PGNreader.openCodec(entreno)
                    todo = f.read().strip()
                    f.close()
                    liEntrenamientos = todo.split("\n")
                    nPosiciones = len(liEntrenamientos)
                    um.final()
                    if nPosiciones == 0:
                        return
                    elif nPosiciones == 1:
                        pos = 1
                        jump = False
                    else:
                        db = Util.DicSQL(self.configuracion.ficheroTrainings)
                        posUltimo = db[entreno]
                        if posUltimo is None:
                            posUltimo = 1
                        resp = DatosNueva.numPosicion(self.procesador.pantalla,
                                                     titentreno, nPosiciones, posUltimo)
                        if resp is None:
                            return
                        pos, tipo, jump = resp
                        db.close()
                        if tipo.startswith( "r" ):
                            if tipo == "rk":
                                random.seed(pos)
                            random.shuffle(liEntrenamientos)
                    self.procesador.entrenaPos(pos, nPosiciones, titentreno, liEntrenamientos, entreno, jump)

                elif resp == "learnPGN":
                    self.learnPGN()

                elif resp == "lucaselo":
                    self.procesador.lucaselo(False)

                elif resp == "micelo":
                    self.procesador.micelo(False)

                elif resp.startswith("fics"):
                    self.procesador.ficselo(False, int(resp[4:]))

                elif resp.startswith("fide"):
                    self.procesador.fideelo(False, int(resp[4:]))

                elif resp.startswith("map_"):
                    nada, mapa = resp.split("_")
                    self.procesador.trainingMap(mapa)

            else:
                if resp <= -100:
                    self.menu = None  # ya que puede cambiar y la etiqueta es diferente
                    mem = Memoria.Memoria(self.procesador)
                    mem.lanza(abs(resp) - 100)

    def tacticas(self, tipo, nombre, carpeta, ini):
        um = self.procesador.unMomento()
        tacticas = Tacticas.Tacticas(tipo, nombre, carpeta, ini)
        liMenus = tacticas.listaMenus()
        if len(liMenus) == 0:
            um.final()
            return

        nico = QTVarios.rondoPuntos()
        if len(liMenus) > 1:
            menu = QTVarios.LCMenu(self.parent)
            menu.opcion(None, _SP(nombre), Iconos.Tacticas())
            menu.separador()

            dmenu = {}

            for valor, lista in liMenus:
                actmenu = menu
                if len(lista) > 1:
                    t = ""
                    for x in range(len(lista) - 1):
                        t += "|%s" % lista[x]
                        if t not in dmenu:
                            dmenu[t] = actmenu.submenu(_SP(lista[x]), nico.otro())
                            actmenu.separador()
                        actmenu = dmenu[t]
                actmenu.opcion(valor, _SP(lista[-1]), nico.otro())
                actmenu.separador()
            um.final()
            resp = menu.lanza()

        else:
            resp = liMenus[0][0]

        if not resp:
            um.final()
            return

        tactica = tacticas.eligeTactica(resp)

        um.final()
        if tactica:
            self.entrenaTactica(tactica)

    def tacticaRemove(self, carpeta, nombre):
        if QTUtil2.pregunta(self.procesador.pantalla, _X(_("Delete %1?"), nombre)):
            shutil.rmtree(carpeta)
            self.rehaz()

    def entrenaTactica(self, tactica):
        tactica.leeDatos(self.configuracion.carpeta)
        icono = Iconos.PuntoMagenta()
        resp = PantallaTacticas.consultaHistorico(self.procesador.pantalla, tactica, icono)
        if resp:
            if resp != "seguir":
                if resp != "auto":
                    if resp.startswith("copia"):
                        ncopia = int(resp[5:])
                    else:
                        ncopia = None
                    if not PantallaTacticas.edita1tactica(self.procesador.pantalla, tactica, ncopia):
                        return
                um = self.procesador.unMomento()
                tactica.genera()
                um.final()
            self.procesador.tipoJuego = kJugEntTac
            self.procesador.estado = kJugando
            self.procesador.gestor = GestorEntTac.GestorEntTac(self.procesador)
            self.procesador.gestor.inicio(tactica)

    def entrenaGM(self):
        w = PantallaGM.WGM(self.procesador)
        if w.exec_():
            self.procesador.tipoJuego = kJugGM
            self.procesador.estado = kJugando
            self.procesador.gestor = GestorGM.GestorGM(self.procesador)
            self.procesador.gestor.inicio(w.record)

    def entrenaBooks(self):
        w = PantallaBooks.WBooks(self.procesador)
        if w.exec_() and w.libro:
            self.procesador.tipoJuego = kJugBooks
            self.procesador.estado = kJugando
            self.procesador.gestor = GestorBooks.GestorBooks(self.procesador)
            self.procesador.gestor.inicio(w.libro, w.siBlancas, w.jugContrario, w.jugJugador)

    def jugar60(self, siJugador):
        self.procesador.gestor = Gestor60.Gestor60(self.procesador)
        self.procesador.gestor.inicio(siJugador)

    def jugarMate(self, tipo):
        self.procesador.gestor = GestorMate.GestorMate(self.procesador)
        self.procesador.gestor.inicio(tipo)

    def aperturas(self):
        w = PantallaAperturas.EntrenamientoAperturas(self.procesador)
        if w.exec_():
            listaAperturasStd, ficheroDatos, lista, fila, jugamos, repeticiones = w.resultado
            self.procesador.gestor = GestorAperturas.GestorAperturas(self.procesador)
            self.procesador.gestor.inicio(listaAperturasStd, ficheroDatos, lista, fila, jugamos, repeticiones, 0)

    def dailyTest(self):
        PantallaDailyTest.dailyTest(self.procesador)

    def potencia(self):
        PantallaPotencia.pantallaPotencia(self.procesador)

    def visualiza(self):
        PantallaVisualiza.pantallaVisualiza(self.procesador)

    def puente(self, nivel):
        PantallaPuente.pantallaPuente(self.procesador, nivel)

    def horses(self, test, titulo, icono):
        PantallaHorses.pantallaHorses(self.procesador, test, titulo, icono)

    def bmt(self):
        PantallaBMT.pantallaBMT(self.procesador)

    def boxing(self, tipo):
        boxing = Boxing.Boxing(self.procesador, tipo)
        resp = PantallaBoxing.pantallaBoxing(self.procesador.pantalla, boxing)
        if resp is not None:
            numEngine, clave = resp
            self.procesador.gestor = GestorBoxing.GestorBoxing(self.procesador)
            self.procesador.gestor.inicio(boxing, numEngine, clave)

    def learnPGN(self):
        w = PantallaLearnPGN.WLearnBase(self.procesador)
        w.exec_()


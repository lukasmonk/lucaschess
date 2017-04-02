import os
import random
import shutil

from Code import Resistance
from Code import Gestor60
from Code import GestorAperturas
from Code import GestorBooks
from Code import GestorResistance
from Code import GestorEntTac
from Code import GestorTurnOnLights
from Code import GestorGM
from Code import GestorMate
from Code import GestorUnJuego
from Code import Memoria
from Code import Partida
from Code.QT import DatosNueva
from Code.QT import Iconos
from Code.QT import PantallaAperturas
from Code.QT import PantallaBMT
from Code.QT import PantallaBooks
from Code.QT import PantallaResistance
from Code.QT import PantallaDailyTest
from Code.QT import PantallaEverest
from Code.QT import PantallaGM
from Code.QT import PantallaHorses
from Code.QT import PantallaLearnPGN
from Code.QT import PantallaPlayPGN
from Code.QT import PantallaPotencia
from Code.QT import PantallaPuente
from Code.QT import PantallaTacticas
from Code.QT import PantallaVisualiza
from Code.QT import PantallaTurnOnLights
from Code.QT import QTUtil2
from Code.QT import QTVarios
from Code import Tacticas
from Code import TrListas
from Code import Util
from Code import VarGen
from Code.Constantes import *


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
        for xfile in self.files:
            xopcion(bmenu, "ep_%s" % xfile.path, xfile.name, icoOp)


class Entrenamientos:
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
            dicMenu[clave] = (clave, texto, icono, siDeshabilitado)

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

        # Resistencia ------------------------------------------------------------------------------------------
        menu1 = menu.submenu(_("Resistance Test"), Iconos.Resistencia())
        nico = Util.Rondo(Iconos.Verde(), Iconos.Azul(), Iconos.Amarillo(), Iconos.Naranja())
        xopcion(menu1, "resistance", _("Normal"), nico.otro())
        xopcion(menu1, "resistancec", _("Blindfold chess"), nico.otro())
        xopcion(menu1, "resistancep1", _("Hide only our pieces"), nico.otro())
        xopcion(menu1, "resistancep2", _("Hide only opponent pieces"), nico.otro())
        menu.separador()

        # DailyTest ------------------------------------------------------------------------------------------------
        xopcion(menu, "dailytest", _("Your daily test"), Iconos.DailyTest())
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

        # Openings ------------------------------------------------------------------------------------------------
        menu1 = menu.submenu(_("Openings"), Iconos.Aperturas())
        # # Aperturas --------------------------------------------------------------------------------------------
        xopcion(menu1, "aperturas", _("Learn openings by repetition"), Iconos.Apertura())
        menu1.separador()
        # Books ------------------------------------------------------------------------------------------------
        xopcion(menu1, "polyglot", _("Training with a book"), Iconos.Libros())

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

        menu.separador()

        # Longs ----------------------------------------------------------------------------------------
        menu1 = menu.submenu(_("Long-term trainings"), Iconos.Longhaul())
        # Maps
        menu2 = menu1.submenu(_("Training on a map"), Iconos.Maps())
        xopcion(menu2, "map_Africa", _("Africa map"), Iconos.Africa())
        menu2.separador()
        xopcion(menu2, "map_WorldMap", _("World map"), Iconos.WorldMap())
        # Rail
        menu1.separador()
        xopcion(menu1, "transsiberian", _("Transsiberian Railway"), Iconos.Train())
        # Everest
        menu1.separador()
        xopcion(menu1, "everest", _("Expeditions to the Everest"), Iconos.Trekking())
        # TOL
        menu1.separador()
        menu2 = menu1.submenu(_("Turn on the lights"), Iconos.TOL())
        xopcion(menu2, "tol_uned", _("UNED chess school"), Iconos.Uned())
        menu2.separador()
        xopcion(menu2, "tol_uwe", _("Uwe Auerswald"), Iconos.Uwe())
        # Washing
        menu1.separador()
        xopcion(menu1, "washing_machine", _("The Washing Machine"), Iconos.WashingMachine())

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
            1: ("N", "Alpha", _("Basic test")),
            2: ("p", "Fantasy", _("Four pawns test")),
            3: ("Q", "Pirat", _("Jonathan Levitt test")),
            4: ("n", "Spatial", _("Basic test") + ": a1"),
            5: ("N", "Cburnett", _("Basic test") + ": e4")
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
        menu2 = menu1.submenu(_("Learn a game"), Iconos.School())
        xopcion(menu2, "learnPGN", _("Memorizing their moves"), Iconos.LearnGame())
        menu2.separador()
        xopcion(menu2, "playPGN", _("Playing against"), Iconos.Law())

        menu1.separador()
        xopcion(menu1, "visualiza", _("The board at a glance"), Iconos.Gafas())

        # menu2 = menu1.submenu(_("Endings with 3/4 pieces"), Iconos.Puente())
        # xopcion(menu2, "end_t4-1", "%s %d"%(_("Level"), 1), Iconos.PuntoAzul())
        # xopcion(menu2, "end_t4-2", "%s %d"%(_("Level"), 2), Iconos.PuntoMagenta())

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
            if type(resp) == str:
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

                elif resp.startswith("resistance"):
                    self.resistance(resp[10:])

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
                    with Util.OpenCodec(entreno) as f:
                        todo = f.read().strip()
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
                        if tipo.startswith("r"):
                            if tipo == "rk":
                                random.seed(pos)
                            random.shuffle(liEntrenamientos)
                    self.procesador.entrenaPos(pos, nPosiciones, titentreno, liEntrenamientos, entreno, jump)

                elif resp == "learnPGN":
                    self.learnPGN()

                elif resp == "playPGN":
                    self.playPGN()

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

                elif resp == "transsiberian":
                    self.procesador.showRoute()

                elif resp == "everest":
                    self.everest()

                elif resp.startswith("tol_"):
                    self.turn_on_lights(resp[4:])

                elif resp == "washing_machine":
                    self.washing_machine()

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

    def resistance(self, tipo):
        resistance = Resistance.Resistance(self.procesador, tipo)
        resp = PantallaResistance.pantallaResistance(self.procesador.pantalla, resistance)
        if resp is not None:
            numEngine, clave = resp
            self.procesador.gestor = GestorResistance.GestorResistance(self.procesador)
            self.procesador.gestor.inicio(resistance, numEngine, clave)

    def learnPGN(self):
        w = PantallaLearnPGN.WLearnBase(self.procesador)
        w.exec_()

    def playPGN(self):
        w = PantallaPlayPGN.WPlayBase(self.procesador)
        if w.exec_():
            recno = w.recno
            if recno is not None:
                siBlancas = w.siBlancas
                db = PantallaPlayPGN.PlayPGNs(self.configuracion.ficheroPlayPGN)
                reg = db.leeRegistro(recno)
                partidaObj = Partida.Partida()
                partidaObj.recuperaDeTexto(reg["PARTIDA"])
                nombreObj = reg.get("WHITE" if siBlancas else "BLACK", _("Player"))
                self.procesador.gestor = GestorUnJuego.GestorUnJuego(self.procesador)
                self.procesador.gestor.inicio(recno, partidaObj, nombreObj, siBlancas, db.rotulo(recno))
                db.close()

    def everest(self):
        PantallaEverest.everest(self.procesador)

    def turn_on_lights(self, name):
        if name == "uned":
            title = _("UNED chess school")
            folder = "Trainings/Tactics by UNED chess school"
            icono = Iconos.Uned()
            li_tam_blocks = (6, 12, 20, 30, 60)
        elif name == "uwe":
            title = _("Uwe Auerswald")
            folder = "Trainings/Tactics by Uwe Auerswald"
            icono = Iconos.Uwe()
            li_tam_blocks = (5, 10, 20, 40, 80)

        resp = PantallaTurnOnLights.pantallaTurnOnLigths(self.procesador, name, title, icono, folder, li_tam_blocks)
        if resp:
            num_theme, num_block, tol = resp
            self.procesador.tipoJuego = kJugEntLight
            self.procesador.estado = kJugando
            self.procesador.gestor = GestorTurnOnLights.GestorTurnOnLights(self.procesador)
            self.procesador.gestor.inicio(num_theme, num_block, tol)

    def washing_machine(self):
        self.procesador.showWashing()


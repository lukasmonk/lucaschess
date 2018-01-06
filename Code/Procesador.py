import os
import random
import sys
import webbrowser
import logging

from Code import AperturasStd
from Code import Routes
from Code import Util
from Code import VarGen
from Code import XGestorMotor
from Code.Constantes import *

from Code import Albums
from Code import CPU
from Code import Configuracion
from Code import ControlPosicion
from Code import Entrenamientos
from Code import GestorAlbum
from Code import GestorElo
from Code import GestorEntMaq
from Code import GestorEntPos
from Code import GestorEverest
from Code import GestorFideFics
from Code import GestorMateMap
from Code import GestorMicElo
from Code import GestorCompeticion
from Code import GestorPGN
from Code import GestorPerson
from Code import GestorRoutes
from Code import GestorSingularM
from Code import GestorSolo
from Code import GestorPartida
from Code import GestorTorneo
from Code import Presentacion
from Code import GestorWashing
from Code import GestorPlayPGN
from Code.QT import DatosNueva
from Code.QT import Iconos
from Code.QT import Info
from Code.QT import Pantalla
from Code.QT import PantallaAlbumes
from Code.QT import PantallaAperturas
from Code.QT import PantallaBMT
from Code.QT import PantallaColores
from Code.QT import PantallaConfig
from Code.QT import PantallaEntMaq
from Code.QT import PantallaEverest
from Code.QT import PantallaFavoritos
from Code.QT import PantallaMotores
from Code.QT import PantallaRoutes
from Code.QT import PantallaSTS
from Code.QT import PantallaSonido
from Code.QT import PantallaSingularM
from Code.QT import PantallaTorneos
from Code.QT import PantallaUsuarios
from Code.QT import PantallaWashing
from Code.QT import PantallaWorkMap
from Code.QT import PantallaPlayPGN
from Code.QT import Piezas
from Code.QT import QTUtil
from Code.QT import QTUtil2
from Code.QT import QTVarios
from Code.QT import PantallaDatabase
from Code.QT import PantallaManualSave
from Code.QT import WBDatabaseFEN
from Code.QT import WOpeningGuide
from Code.QT import PantallaKibitzers


class Procesador:
    """
    Vinculo entre pantalla y gestores
    """
    def __init__(self, args):
        if VarGen.listaGestoresMotor is None:
            VarGen.listaGestoresMotor = XGestorMotor.ListaGestoresMotor()
        self.args = args

    def iniciaConUsuario(self, user):

        self.user = user

        self.web = "http://lucaschess.pythonanywhere.com"
        self.blog = "http://lucaschess.blogspot.com"

        self.liOpcionesInicio = [k_terminar, k_play, k_entrenamiento, k_competir,
                                 k_tools, k_opciones, k_informacion]  # Lo incluimos aqui porque sino no lo lee, en caso de aplazada

        self.configuracion = Configuracion.Configuracion(user)
        self.configuracion.start(self.version)
        VarGen.configuracion = self.configuracion
        AperturasStd.reset()

        VarGen.todasPiezas = Piezas.TodasPiezas()

        self.gestor = None
        self.teclaPanico = 32  # necesario

        self.siPrimeraVez = True
        self.siPresentacion = False  # si esta funcionando la presentacion

        self.posicionInicial = ControlPosicion.ControlPosicion()
        self.posicionInicial.posInicial()

        self.xrival = None
        self.xtutor = None  # creaTutor lo usa asi que hay que definirlo antes
        self.xanalyzer = None  # cuando se juega GestorEntMaq y el tutor danzando a toda maquina, se necesita otro diferente

        self.replay = None
        self.replayBeep = None

        self.liKibitzersActivas = []

    def setVersion(self, version):
        self.version = version

    def quitaKibitzers(self):
        if hasattr(self, "liKibitzersActivas"):
            for xkibitzer in self.liKibitzersActivas:
                xkibitzer.terminar()

    def iniciarGUI(self):
        self.pantalla = Pantalla.PantallaWidget(self)
        self.pantalla.ponGestor(self)  # antes que muestra
        self.pantalla.muestra()

        self.tablero = self.pantalla.tablero

        self.entrenamientos = Entrenamientos.Entrenamientos(self)
        comando = self.args.chessFile
        fen = self.args.fen
        logging.info("Chess file to use: %s", comando)
        logging.info("Fen to use: %s", fen)


        if self.configuracion.siAplazada:
            aplazamiento = self.configuracion.aplazamiento
            self.juegaAplazada(aplazamiento)
        elif comando:
            comandoL = comando.lower()
            if comandoL.endswith(".pgn"):
                aplazamiento = {}
                aplazamiento["TIPOJUEGO"] = kJugPGN
                aplazamiento["SIBLANCAS"] = True  # Compatibilidad
                self.juegaAplazada(aplazamiento)
                return
            elif comandoL.endswith(".pks"):
                aplazamiento = {}
                aplazamiento["TIPOJUEGO"] = kJugSolo
                aplazamiento["SIBLANCAS"] = True  # Compatibilidad
                self.juegaAplazada(aplazamiento)
                return
            elif comandoL.endswith(".lcg"):
                self.externDatabase(comando)
                return
                # elif comandoL.endswith(".lcf"):
                #     self.externDatabaseFEN(comando)
                #     return
            elif comandoL.endswith(".bmt"):
                self.inicio()
                self.externBMT(comando)
                return
        elif fen:
            self.juegaExterno(fen)
            return
        else:
            self.inicio()

    def reset(self):
        self.pantalla.activaCapturas(False)
        self.pantalla.activaInformacionPGN(False)
        if self.gestor:
            self.gestor.finGestor()
            self.gestor = None
        self.pantalla.ponGestor(self)  # Necesario, no borrar
        self.tablero.indicadorSC.setVisible(False)
        self.tablero.blindfoldQuitar()
        self.pantalla.ponToolBar(self.liOpcionesInicio)
        self.pantalla.activaJuego(False, False)
        self.tablero.exePulsadoNum = None
        self.tablero.ponPosicion(self.posicionInicial)
        self.pantalla.ajustaTam()
        self.pantalla.ponTitulo()
        self.pararMotores()

    def inicio(self):
        self.configuracion.limpiaTemporal()
        self.reset()
        if self.configuracion.siPrimeraVez:
            self.cambiaConfiguracionPrimeraVez()
            self.configuracion.siPrimeraVez = False
            self.pantalla.ponTitulo()
        if self.siPrimeraVez:
            self.siPrimeraVez = False
            self.cpu = CPU.CPU(self.pantalla)
            self.presentacion()

    def presentacion(self, siEmpezar=True):
        self.siPresentacion = siEmpezar
        if not siEmpezar:
            self.cpu.stop()
            self.tablero.ponerPiezasAbajo(True)
            self.tablero.activaMenuVisual(True)
            self.tablero.ponPosicion(self.posicionInicial)
            self.tablero.setToolTip("")
            self.tablero.bloqueaRotacion(False)

        else:
            self.tablero.bloqueaRotacion(True)
            self.tablero.activaMenuVisual(False)
            self.tablero.setToolTip("")

            logo = ControlPosicion.ControlPosicion()
            logo.logo()

            # self.cpu.ponPosicion(logo)
            # hx = self.cpu.duerme(2.0)

            # Presentacion.basico( self, hx )
            # Presentacion.partidaDia(self, hx)
            self.tablero.activaMenuVisual(True)
            Presentacion.GestorChallenge101(self)

            # self.cpu.start()

    def juegaAplazada(self, aplazamiento):
        self.cpu = CPU.CPU(self.pantalla)

        tipoJuego = aplazamiento["TIPOJUEGO"]
        siBlancas = aplazamiento["SIBLANCAS"]

        if tipoJuego == kJugNueva:
            categoria = self.configuracion.rival.categorias.segunClave(aplazamiento["CATEGORIA"])
            nivel = aplazamiento["NIVEL"]
            puntos = aplazamiento["PUNTOS"]
            self.gestor = GestorCompeticion.GestorCompeticion(self)
            self.gestor.inicio(categoria, nivel, siBlancas, puntos, aplazamiento)
        elif tipoJuego == kJugEntMaq:
            if aplazamiento["MODO"] == "Basic":
                self.entrenaMaquina(None, aplazamiento)
            else:
                self.playPersonAplazada(aplazamiento)
        elif tipoJuego == kJugElo:
            self.gestor = GestorElo.GestorElo(self)
            self.gestor.inicio(None, aplazamiento["SICOMPETITIVO"], aplazamiento)
        elif tipoJuego == kJugMicElo:
            self.gestor = GestorMicElo.GestorMicElo(self)
            self.gestor.inicio(None, 0, 0, aplazamiento["SICOMPETITIVO"], aplazamiento)
        elif tipoJuego == kJugAlbum:
            self.gestor = GestorAlbum.GestorAlbum(self)
            self.gestor.inicio(None, None, aplazamiento)
        elif tipoJuego == kJugPGN:
            self.visorPGN("pgn_comandoExterno")
        elif tipoJuego == kJugSolo:
            self.jugarSolo(fichero=self.args.chessFile)
        elif tipoJuego in (kJugFics, kJugFide):
            self.gestor = GestorFideFics.GestorFideFics(self)
            self.gestor.selecciona("Fics" if tipoJuego == kJugFics else "Fide")
            self.gestor.inicio(aplazamiento["IDGAME"], aplazamiento["SICOMPETITIVO"], aplazamiento=aplazamiento)

    def XTutor(self):
        if self.xtutor is None or not self.xtutor.activo:
            self.creaXTutor()
        return self.xtutor

    def creaXTutor(self):
        xtutor = XGestorMotor.GestorMotor(self, self.configuracion.tutor)
        xtutor.nombre += ("(%s)" % _("tutor"))
        xtutor.opciones(self.configuracion.tiempoTutor, self.configuracion.depthTutor, True)
        if self.configuracion.tutorMultiPV == 0:
            xtutor.maximizaMultiPV()
        else:
            xtutor.setMultiPV(self.configuracion.tutorMultiPV)

        self.xtutor = xtutor
        VarGen.xtutor = xtutor

    def cambiaXTutor(self):
        if self.xtutor:
            self.xtutor.terminar()
        self.creaXTutor()
        self.cambiaXAnalyzer()

    def XAnalyzer(self):
        if self.xanalyzer is None or not self.xanalyzer.activo:
            self.creaXAnalyzer()
        return self.xanalyzer

    def creaXAnalyzer(self):
        xanalyzer = XGestorMotor.GestorMotor(self, self.configuracion.tutor)
        xanalyzer.nombre += ("(%s)" % _("analyzer"))
        xanalyzer.opciones(self.configuracion.tiempoTutor, self.configuracion.depthTutor, True)
        if self.configuracion.tutorMultiPV == 0:
            xanalyzer.maximizaMultiPV()
        else:
            xanalyzer.setMultiPV(self.configuracion.tutorMultiPV)

        self.xanalyzer = xanalyzer
        VarGen.xanalyzer = xanalyzer

    def cambiaXAnalyzer(self):
        if self.xanalyzer:
            self.xanalyzer.terminar()
        self.creaXAnalyzer()

    def creaGestorMotor(self, confMotor, tiempo, nivel, siMultiPV=False, priority=None):
        xgestor = XGestorMotor.GestorMotor(self, confMotor)
        xgestor.opciones(tiempo, nivel, siMultiPV)
        xgestor.setPriority(priority)
        return xgestor

    def pararMotores(self):
        VarGen.listaGestoresMotor.closeAll()

    def cambiaRival(self, nuevo):
        """
        Llamado desde DatosNueva, cuando elegimos otro motor para jugar.
        """
        self.configuracion.rival = self.configuracion.buscaRival(nuevo)
        self.configuracion.graba()

    def menuPlay(self):
        menu = QTVarios.LCMenu(self.pantalla)
        menu.opcion(("free", None), _("Play against an engine of your choice"), Iconos.Libre())
        menu.separador()

        # Principiantes ----------------------------------------------------------------------------------------
        menu1 = menu.submenu(_("Opponents for young players"), Iconos.RivalesMP())

        for name, trans, ico in QTVarios.list_irina():
            menu1.opcion(("person", name), trans, ico)
        menu1.separador()

        menu2 = menu1.submenu(_("Albums of animals"), Iconos.Penguin())
        albumes = Albums.AlbumesAnimales()
        dic = albumes.list_menu()
        anterior = None
        for animal in dic:
            siDeshabilitado = False
            if anterior and not dic[anterior]:
                siDeshabilitado = True
            menu2.opcion(("animales", animal), _F(animal), Iconos.icono(animal), siDeshabilitado=siDeshabilitado)
            anterior = animal
        menu1.separador()

        menu2 = menu1.submenu(_("Albums of vehicles"), Iconos.Wheel())
        albumes = Albums.AlbumesVehicles()
        dic = albumes.list_menu()
        anterior = None
        for character in dic:
            siDeshabilitado = False
            if anterior and not dic[anterior]:
                siDeshabilitado = True
            menu2.opcion(("vehicles", character), _F(character), Iconos.icono(character),
                         siDeshabilitado=siDeshabilitado)
            anterior = character

        resp = menu.lanza()
        if resp:
            tipo, rival = resp
            if tipo == "free":
                self.procesarAccion(k_libre)

            elif tipo == "person":
                self.playPerson(rival)

            elif tipo == "animales":
                self.albumAnimales(rival)

            elif tipo == "vehicles":
                self.albumVehicles(rival)

    def playPersonAplazada(self, aplazamiento):
        self.gestor = GestorPerson.GestorPerson(self)
        self.gestor.inicio(None, aplazamiento=aplazamiento)

    def playPerson(self, rival):
        uno = QTVarios.blancasNegrasTiempo(self.pantalla)
        if not uno:
            return
        siBlancas, siTiempo, minutos, segundos, fastmoves = uno
        if siBlancas is None:
            return

        dic = {}
        dic["SIBLANCAS"] = siBlancas
        dic["RIVAL"] = rival

        dic["SITIEMPO"] = siTiempo and minutos > 0
        dic["MINUTOS"] = minutos
        dic["SEGUNDOS"] = segundos

        dic["FASTMOVES"] = fastmoves

        self.gestor = GestorPerson.GestorPerson(self)
        self.gestor.inicio(dic)

    def reabrirAlbum(self, album):
        tipo, nombre = album.claveDB.split("_")
        if tipo == "animales":
            self.albumAnimales(nombre)
        elif tipo == "vehicles":
            self.albumVehicles(nombre)

    def albumAnimales(self, animal):
        albumes = Albums.AlbumesAnimales()
        album = albumes.get_album(animal)
        album.test_finished()
        cromo, siRebuild = PantallaAlbumes.eligeCromo(self.pantalla, self, album)
        if cromo is None:
            if siRebuild:
                albumes.reset(animal)
                self.albumAnimales(animal)
            return

        self.gestor = GestorAlbum.GestorAlbum(self)
        self.gestor.inicio(album, cromo)

    def albumVehicles(self, character):
        albumes = Albums.AlbumesVehicles()
        album = albumes.get_album(character)
        album.test_finished()
        cromo, siRebuild = PantallaAlbumes.eligeCromo(self.pantalla, self, album)
        if cromo is None:
            if siRebuild:
                albumes.reset(character)
                self.albumVehicles(character)
            return

        self.gestor = GestorAlbum.GestorAlbum(self)
        self.gestor.inicio(album, cromo)

    def menuCompetir(self):
        menu = QTVarios.LCMenu(self.pantalla)
        menu.opcion(("competition", None), _("Competition with tutor"), Iconos.NuevaPartida())
        menu.separador()

        submenu = menu.submenu(_("Elo-Rating"), Iconos.Elo())
        submenu.opcion(("lucaselo",0), "%s (%d)" % (_("Lucas-Elo"), self.configuracion.elo), Iconos.Elo())
        submenu.separador()
        if VarGen.isWindows or VarGen.isWine:
            submenu.opcion(("micelo",0), "%s (%d)" % (_("Tourney-Elo"), self.configuracion.michelo), Iconos.EloTimed())
            submenu.separador()
        fics = self.configuracion.fics
        menuf = submenu.submenu("%s (%d)" % (_("Fics-Elo"), fics), Iconos.Fics())
        rp = QTVarios.rondoPuntos()
        for elo in range(900, 2800, 100):
            if (elo == 900) or (0 <= (elo + 99 - fics) <= 400 or 0 <= (fics - elo) <= 400):
                menuf.opcion(("fics", elo / 100), "%d-%d" % (elo, elo + 99), rp.otro())
        submenu.separador()
        fide = self.configuracion.fide
        menuf = submenu.submenu("%s (%d)" % (_("Fide-Elo"), fide), Iconos.Fide())
        for elo in range(1500, 2700, 100):
            if (elo == 1500) or (0 <= (elo + 99 - fide) <= 400 or 0 <= (fide - elo) <= 400):
                menuf.opcion(("fide", elo / 100), "%d-%d" % (elo, elo + 99), rp.otro())
        menu.separador()
        submenu = menu.submenu(_("Singular moves"), Iconos.Singular())
        submenu.opcion(("strenght101", 0), _("Calculate your strength"), Iconos.Strength())
        submenu.separador()
        submenu.opcion(("challenge101",0), _("Challenge 101"), Iconos.Wheel())

        resp = menu.lanza()
        if resp:
            tipo, rival = resp
            if tipo == "competition":
                self.competicion()

            elif tipo == "lucaselo":
                self.lucaselo(True)

            elif tipo == "micelo":
                self.micelo(True)

            elif tipo == "fics":
                self.ficselo(True, rival)

            elif tipo == "fide":
                self.fideelo(True, rival)

            elif tipo == "challenge101":
                Presentacion.GestorChallenge101(self)

            elif tipo == "strenght101":
                self.strenght101()

    def strenght101(self):
        w = PantallaSingularM.WSingularM(self.pantalla, self.configuracion)
        if w.exec_():
            self.gestor = GestorSingularM.GestorSingularM(self)
            self.gestor.inicio(w.sm)

    def procesarAccion(self, clave):
        if self.siPresentacion:
            self.presentacion(False)

        if clave == k_terminar:
            if hasattr(self, "cpu"):
                self.cpu.stop()
            self.pantalla.procesosFinales()
            self.pantalla.accept()

        elif clave == k_play:
            self.menuPlay()

        elif clave == k_competir:
            self.menuCompetir()

        elif clave == k_libre:
            self.libre()

        elif clave == k_entrenamiento:
            self.entrenamientos.lanza()

        elif clave == k_opciones:
            self.opciones()

        elif clave == k_tools:
            self.tools()

        elif clave == k_informacion:
            self.informacion()

    def opciones(self):
        menu = QTVarios.LCMenu(self.pantalla)

        menu.opcion(self.cambiaConfiguracion, _("Configuration"), Iconos.Opciones())
        menu.separador()

        menu1 = menu.submenu(_("Colors"), Iconos.Colores())
        menu1.opcion(self.editaColoresTablero, _("Main board"), Iconos.EditarColores())
        menu1.separador()
        menu1.opcion(self.cambiaColores, _("General"), Iconos.Vista())
        menu.separador()

        menu1 = menu.submenu(_("Sound"), Iconos.SoundTool())
        menu1.opcion(self.sonidos, _("Custom sounds"), Iconos.S_Play())
        menu.separador()
        menu.opcion(self.favoritos, _("Training favorites"), Iconos.Corazon())

        menu.separador()
        menu.opcion(self.setPassword, _("Set password"), Iconos.Password())

        if self.configuracion.siMain:
            menu.separador()
            menu.opcion(self.usuarios, _("Users"), Iconos.Usuarios())
            menu.separador()

            menu1 = menu.submenu(_("User data folder"), Iconos.Carpeta())
            menu1.opcion(self.folder_change, _("Change the folder"), Iconos.FolderChange())
            if not Configuracion.isDefaultFolder():
                menu1.separador()
                menu1.opcion(self.folder_default, _("Set the default"), Iconos.Defecto())

        resp = menu.lanza()
        if resp:
            if isinstance(resp, tuple):
                resp[0](resp[1])
            else:
                resp()

    def cambiaConfiguracion(self):
        if PantallaConfig.opciones(self.pantalla, self.configuracion):
            self.configuracion.graba()
            self.reiniciar()

    def editaColoresTablero(self):
        w = PantallaColores.WColores(self.tablero)
        w.exec_()

    def cambiaColores(self):
        if PantallaColores.cambiaColores(self.pantalla, self.configuracion):
            self.reiniciar()

    def sonidos(self):
        w = PantallaSonido.WSonidos(self)
        w.exec_()

    def favoritos(self):
        PantallaFavoritos.miraFavoritos(self.entrenamientos)

    def folder_change(self):
        carpeta = QTUtil2.leeCarpeta(self.pantalla, self.configuracion.carpeta,
                                     _("Change the folder where all data is saved") + "\n" + _(
                                         "Be careful please"))
        if carpeta:
            if os.path.isdir(carpeta):
                self.configuracion.changeActiveFolder(carpeta)
                self.reiniciar()

    def folder_default(self):
        self.configuracion.changeActiveFolder(None)
        self.reiniciar()

    def reiniciar(self):
        self.pantalla.accept()
        QTUtil.salirAplicacion(kFinReinicio)

    def cambiaConfiguracionPrimeraVez(self):
        if PantallaConfig.opcionesPrimeraVez(self.pantalla, self.configuracion):
            self.configuracion.graba()

    def motoresExternos(self):
        w = PantallaMotores.WMotores(self.pantalla, self.configuracion.ficheroMExternos)
        w.exec_()

    def aperturaspers(self):
        w = PantallaAperturas.AperturasPersonales(self)
        w.exec_()

    def usuarios(self):
        PantallaUsuarios.editaUsuarios(self)

    def setPassword(self):
        PantallaUsuarios.setPassword(self)

    def elo(self):
        menu = QTVarios.LCMenu(self.pantalla)

        menu.opcion("lucaselo", "%s (%d)" % (_("Lucas-Elo"), self.configuracion.elo), Iconos.Elo())
        menu.separador()
        if VarGen.isWindows or VarGen.isWine:
            menu.opcion("micelo", "%s (%d)" % (_("Tourney-Elo"), self.configuracion.michelo), Iconos.EloTimed())
            menu.separador()
        fics = self.configuracion.fics
        menuf = menu.submenu("%s (%d)" % (_("Fics-Elo"), fics), Iconos.Fics())
        rp = QTVarios.rondoPuntos()
        for elo in range(900, 2800, 100):
            if (elo == 900) or (0 <= (elo + 99 - fics) <= 400 or 0 <= (fics - elo) <= 400):
                menuf.opcion("fics%d" % (elo / 100,), "%d-%d" % (elo, elo + 99), rp.otro())
        fide = self.configuracion.fide
        menu.separador()
        menuf = menu.submenu("%s (%d)" % (_("Fide-Elo"), fide), Iconos.Fide())
        rp = QTVarios.rondoPuntos()
        for elo in range(1500, 2700, 100):
            if (elo == 1500) or (0 <= (elo + 99 - fide) <= 400 or 0 <= (fide - elo) <= 400):
                menuf.opcion("fide%d" % (elo / 100,), "%d-%d" % (elo, elo + 99), rp.otro())
        resp = menu.lanza()

        if resp:
            if resp == "lucaselo":
                self.lucaselo(True)
            elif resp == "micelo":
                self.micelo(True)
            elif resp.startswith("fics"):
                self.ficselo(True, int(resp[4:]))
            elif resp.startswith("fide"):
                self.fideelo(True, int(resp[4:]))

    def lucaselo(self, siCompetitivo):
        self.gestor = GestorElo.GestorElo(self)
        resp = PantallaMotores.eligeMotorElo(self.gestor, self.configuracion.eloActivo(siCompetitivo))
        if resp:
            self.gestor.inicio(resp, siCompetitivo)

    def micelo(self, siCompetitivo):
        self.gestor = GestorMicElo.GestorMicElo(self)
        resp = PantallaMotores.eligeMotorMicElo(self.gestor, self.configuracion.miceloActivo(siCompetitivo))
        if resp:
            respT = QTVarios.tiempo(self.pantalla, minMinutos=10 if siCompetitivo else 3, minSegundos=0, maxMinutos=999,
                                    maxSegundos=999)
            if respT:
                minutos, segundos = respT
                self.gestor.inicio(resp, minutos, segundos, siCompetitivo)

    def ficselo(self, siCompetitivo, nivel):
        self.gestor = GestorFideFics.GestorFideFics(self)
        self.gestor.selecciona("Fics")
        xid = self.gestor.eligeJuego(siCompetitivo, nivel)
        self.gestor.inicio(xid, siCompetitivo)

    def fideelo(self, siCompetitivo, nivel):
        self.gestor = GestorFideFics.GestorFideFics(self)
        self.gestor.selecciona("Fide")
        xid = self.gestor.eligeJuego(siCompetitivo, nivel)
        self.gestor.inicio(xid, siCompetitivo)

    def trainingMap(self, mapa):
        resp = PantallaWorkMap.train_map(self, mapa)
        if resp:
            self.gestor = GestorMateMap.GestorMateMap(self)
            self.gestor.inicio(resp)

    def tools(self):
        menu = QTVarios.LCMenu(self.pantalla)

        menu.opcion("juega_solo", _("Create your own game"), Iconos.JuegaSolo())
        menu.separador()

        menu1 = menu.submenu(_("PGN viewer"), Iconos.PGN())
        menu1.opcion("pgn_paste", _("Paste PGN"), Iconos.Pegar())
        menu1.separador()
        menu1.opcion("pgn_fichero", _("Read PGN"), Iconos.Fichero())
        menu1.separador()
        menu1.opcion("pgn_jugadadia", _("Game of the day"), Iconos.LM())
        menu1.separador()
        if self.configuracion.liTrasteros:
            menu1.opcion("pgn_trasteros", _("Boxrooms PGN"), Iconos.Trasteros())
            menu1.separador()
        if self.configuracion.salvarFichero and Util.existeFichero(self.configuracion.salvarFichero):
            menu1.opcion("pgn_nuestroFichero", _("My games"), Iconos.NuestroFichero())
        menu.separador()

        menu1 = menu.submenu(_("Database"), Iconos.Database())
        menu1.opcion("database", _("Complete games"), Iconos.DatabaseC())
        menu1.separador()
        menu1.opcion("databaseFEN", _("Positions"), Iconos.DatabaseF()) # TODO
        menu.separador()

        menu.opcion("manual_save", _("Save positions to FNS/PGN"), Iconos.ManualSave())
        menu.separador()

        menu1 = menu.submenu(_("Openings"), Iconos.Aperturas())
        menu1.opcion("aperturaspers", _("Custom openings"), Iconos.Apertura())
        menu1.separador()
        menu1.opcion("bookguide", _("Personal Opening Guide"), Iconos.BookGuide())
        menu.separador()
        menu.separador()

        menu1 = menu.submenu(_("Engines"), Iconos.Motores())
        menu1.opcion("torneos", _("Tournaments between engines"), Iconos.Torneos())
        menu1.separador()
        menu1.opcion("sts", _("STS: Strategic Test Suite"), Iconos.STS())
        menu1.separador()
        menu1.opcion("motores", _("External engines"), Iconos.Motores())
        menu1.separador()
        menu1.opcion("kibitzers", _("Kibitzers"), Iconos.Kibitzer())
        menu.separador()

        resp = menu.lanza()
        if resp:
            if resp.startswith("pgn_"):
                self.visorPGN(resp)

            elif resp == "juega_solo":
                self.jugarSolo()

            elif resp == "torneos":
                self.torneos()
            elif resp == "motores":
                self.motoresExternos()
            elif resp == "sts":
                self.sts()
            elif resp == "kibitzers":
                self.kibitzers()

            elif resp == "manual_save":
                self.manual_save()

            elif resp == "database":
                self.database()

            elif resp == "databaseFEN":
                self.databaseFEN()

            elif resp == "aperturaspers":
                self.aperturaspers()
            elif resp == "bookguide":
                w = WOpeningGuide.WOpeningGuide(self.pantalla, self)
                w.exec_()

    def kibitzers(self):
        w = PantallaKibitzers.WKibitzers(self.pantalla, self)
        w.exec_()

    def externBMT(self, fichero):
        self.configuracion.ficheroBMT = fichero
        PantallaBMT.pantallaBMT(self)

    def externDatabase(self, fichero):
        self.configuracion.ficheroDBgames = fichero
        self.database()
        self.procesarAccion(k_terminar)

    def database(self):
        w = PantallaDatabase.WBDatabase(self.pantalla, self)
        w.exec_()

    # def externDatabaseFEN(self, fichero): # TODO
    #     self.configuracion.ficheroDBgamesFEN = fichero
    #     self.databaseFEN()
    #     self.procesarAccion(k_terminar)

    def databaseFEN(self): # TODO
        w = WBDatabaseFEN.WBDatabaseFEN(self.pantalla, self)
        w.exec_()

    def manual_save(self):
        PantallaManualSave.manual_save(self)

    def torneos(self):
        xjugar = PantallaTorneos.torneos(self.pantalla)
        while xjugar:
            torneo, liGames = xjugar
            self.gestor = GestorTorneo.GestorTorneo(self)
            self.gestor.inicio(torneo, liGames)
            self.inicio()
            xjugar = PantallaTorneos.unTorneo(self.pantalla, torneo)

    def sts(self):
        PantallaSTS.sts(self, self.pantalla)

    def libre(self):
        dic = PantallaEntMaq.entrenamientoMaquina(self, _("Game against an engine of your choice"))
        if dic:
            self.entrenaMaquina(dic)

    def entrenaMaquina(self, dic, aplazamiento=None):
        self.tipoJuego = kJugEntMaq
        self.estado = kJugando
        self.gestor = GestorEntMaq.GestorEntMaq(self)
        if not aplazamiento:
            color = dic["COLOR"]
            if color == "R":
                color = "B" if random.randint(1, 2) == 1 else "N"
            dic["SIBLANCAS"] = color == "B"
        self.gestor.inicio(dic, aplazamiento)

    def visorPGN(self, opcion):
        self.tipoJuego = kJugPGN
        self.estado = kJugando
        self.gestor = GestorPGN.GestorPGN(self)

        opcion = opcion[4:]
        dic = {"paste": k_pgnPaste, "fichero": k_pgnFichero, "jugadadia": k_jugadadia, "trasteros": k_trasteros,
               "nuestroFichero": k_pgnNuestroFichero,
               "comandoExterno": k_pgnComandoExterno}

        self.gestor.inicio(dic[opcion])

    def juegaExterno(self, fen):
        self.gestor = GestorSolo.GestorSolo(self)

        dic = {}
        dic["FEN"] = fen
        dic["SICAMBIORIVAL"] = True
        dic["FINEXIT"] = True
        self.gestor.inicio(dic)

    def jugarSolo(self, fichero=None, pgn=None, partida=None):
        self.gestor = GestorSolo.GestorSolo(self)
        if pgn is not None:
            dic = GestorSolo.pgn_pks(kJugando, pgn)
            self.gestor.inicio(dic)
        elif partida is not None:
            dic = GestorSolo.partida_pks(kJugando, partida)
            self.gestor.inicio(dic)
        else:
            self.gestor.inicio(fichero=fichero)

    def entrenaPos(self, posicion, nPosiciones, titentreno, liEntrenamientos, entreno, jump):
        self.tipoJuego = kJugEntPos
        self.estado = kJugando
        self.gestor = GestorEntPos.GestorEntPos(self)
        self.gestor.ponEntreno(entreno)
        self.gestor.inicio(posicion, nPosiciones, titentreno, liEntrenamientos, jump=jump)

    def playRoute(self, route):
        if route.state == Routes.BETWEEN:
            self.gestor = GestorRoutes.GestorRoutesTactics(self)
            self.estado = kJugando
            self.tipoJuego = kJugEntPos
            self.gestor.inicio(route)
        elif route.state == Routes.ENDING:
            self.gestor = GestorRoutes.GestorRoutesEndings(self)
            self.estado = kJugando
            self.tipoJuego = kJugEntPos
            self.gestor.inicio(route)
        elif route.state == Routes.PLAYING:
            self.gestor = GestorRoutes.GestorRoutesPlay(self)
            self.estado = kJugando
            self.tipoJuego = kJugEntMaq
            self.gestor.inicio(route)

    def showRoute(self):
        PantallaRoutes.train_train(self)

    def playEverest(self, recno):
        self.gestor = GestorEverest.GestorEverest(self)
        self.estado = kJugando
        self.tipoJuego = kJugEntMaq
        self.gestor.inicio(recno)

    def showEverest(self, recno):
        if PantallaEverest.show_expedition(self.pantalla, self.configuracion, recno):
            self.playEverest(recno)

    def playPGN(self):
        w = PantallaPlayPGN.WPlayBase(self)
        if w.exec_():
            recno = w.recno
            if recno is not None:
                siBlancas = w.siBlancas
                self.gestor = GestorPlayPGN.GestorUnJuego(self)
                self.gestor.inicio(recno, siBlancas)

    def playPGNshow(self, recno):
        db = PantallaPlayPGN.PlayPGNs(self.configuracion.ficheroPlayPGN)
        w = PantallaPlayPGN.WPlay1(self.pantalla, self.configuracion, db, recno)
        if w.exec_():
            if w.recno is not None:
                siBlancas = w.siBlancas
                self.gestor = GestorPlayPGN.GestorUnJuego(self)
                self.gestor.inicio(w.recno, siBlancas)
        db.close()

    def showTurnOnLigths(self, name):
        self.entrenamientos.turn_on_lights(name)

    def playWashing(self):
        GestorWashing.gestorWashing(self)

    def showWashing(self):
        if PantallaWashing.pantallaWashing(self):
            self.playWashing()

    def informacion(self):
        liBlog = (
            ("Tactical training with your own blunders",
             "http://lucaschess.blogspot.com.es/2011/11/tactical-training-with-your-own.html"),
            ("Announcements sounds", "http://lucaschess.blogspot.com.es/2011/10/announcements-sounds.html"),
            ("Personalities in Game against an engine of your choice",
             "http://lucaschess.blogspot.com.es/2011/09/version-60-beta-1-personalities.html"),
            ("Training favourites and Your daily test",
             "http://lucaschess.blogspot.com.es/2011/09/version-60-dev4-with-favourites-and.html"),
            (
                "Captured material panel",
                "http://lucaschess.blogspot.com.es/2011/06/version-53-captures-and-more.html"),
            ("Learn openings by repetition",
             "http://lucaschess.blogspot.com.es/2011/06/version-52-standard-openings.html"),
            ("Kibitzers", "http://lucaschess.blogspot.com.es/2011/06/version-51-with-kibitzers.html"),
            ("Training mate positions",
             "http://lucaschess.blogspot.com.es/2011/03/new-option-training-mate-positions.html"),
        )

        menu = QTVarios.LCMenu(self.pantalla)

        menu.opcion("docs", _("Documents"), Iconos.Ayuda())
        menu.separador()
        menu.opcion("web", _("Homepage"), Iconos.Web())
        menu.separador()
        menu1 = menu.submenu("Fresh news", Iconos.Blog())
        menu1.opcion("blog", "Fresh news", Iconos.Blog())
        menu1.separador()
        for txt, lnk in liBlog:
            menu1.opcion(lnk, txt, Iconos.PuntoAzul())
        menu.separador()
        menu.opcion("mail", _("Contact") + " (%s)" % "lukasmonk@gmail.com", Iconos.Mail())
        menu.separador()

        menu.opcion("acercade", _("About"), Iconos.Aplicacion())

        resp = menu.lanza()
        if resp is None:
            return
        elif resp == "acercade":
            self.acercade()
        elif resp == "docs":
            webbrowser.open("%s/docs" % self.web)
        elif resp == "blog":
            webbrowser.open(self.blog)
        elif resp.startswith("http"):
            webbrowser.open(resp)
        elif resp == "web":
            webbrowser.open("%s/index?lang=%s" % (self.web, self.configuracion.traductor))
        elif resp == "mail":
            webbrowser.open("mailto:lukasmonk@gmail.com")


    def adTitulo(self):
        return "<b>" + _("Lucas Chess") + "</b><br>" + _X(_("version %1"), self.version)

    def adPie(self):
        return '<hr><br><b>%s</b>' % _("Author") + \
               ': <a href="mailto:lukasmonk@gmail.com">Lucas Monge</a> -' + \
               ' <a href="%s">%s</a></a>' % (self.web, self.web) + \
               '(%s <a href="http://www.gnu.org/copyleft/gpl.html"> GPL</a>).<br>' % _("License")

    def acercade(self):
        w = Info.WAbout(self)
        w.exec_()



    def unMomento(self, mensaje=None):
        return QTUtil2.mensEspera.inicio(self.pantalla, mensaje if mensaje else _("One moment please..."),
                                         posicion="ad")

    def numDatos(self):
        return 0

    def competicion(self):
        opciones = DatosNueva.datos(self.pantalla, self.configuracion, self)
        if opciones:
            self.tipoJuego = kJugNueva
            categoria, nivel, siBlancas, puntos = opciones

            self.gestor = GestorCompeticion.GestorCompeticion(self)
            self.gestor.inicio(categoria, nivel, siBlancas, puntos)

    def finalX(self):
        return True

    def finalX0(self):
        return True

    def clonVariantes(self, wpantalla, liKibitzersActivas=None, xtutor=None):
        if liKibitzersActivas is None:
            liKibitzersActivas = []
        return ProcesadorVariantes(wpantalla, liKibitzersActivas, xtutor)

    def gestorUnPGN(self, wpantalla, pgn, jugadaInicial=None, siGrabar=True):
        clonProcesador = ProcesadorVariantes(wpantalla, self.liKibitzersActivas, self.xtutor)

        clonProcesador.gestor = GestorSolo.GestorSolo(clonProcesador)
        clonProcesador.gestor.inicio(pgn=pgn, jugadaInicial=jugadaInicial, siGrabar=siGrabar)

        clonProcesador.pantalla.muestraVariantes(clonProcesador.gestor.tituloVentanaPGN())

        return getattr(clonProcesador, "valorPGN", (None, None, None))

    def gestorPartida(self, wpantalla, partidaCompleta, siCompleta, tableroFather):
        clonProcesador = ProcesadorVariantes(wpantalla, self.liKibitzersActivas, self.xtutor)

        clonProcesador.gestor = GestorPartida.GestorPartida(clonProcesador)
        clonProcesador.gestor.inicio(partidaCompleta, siCompleta)

        tablero = clonProcesador.pantalla.tablero
        tablero.dbVisual_setFichero(tableroFather.nomdbVisual)
        tablero.dbVisual_setShowAllways(tableroFather.dbVisual_showAllways)

        resp = clonProcesador.pantalla.muestraVariantes(clonProcesador.gestor.tituloVentana())
        tableroFather.dbVisual_setFichero(tableroFather.nomdbVisual)
        tableroFather.dbVisual_setShowAllways(tableroFather.dbVisual_showAllways)

        if resp:
            return clonProcesador.gestor.partida
        else:
            return None

    def saveAsPKS(self, estado, partida, pgn):
        dic = GestorSolo.pgn_pks(estado, pgn)
        dic["PARTIDA"] = partida.guardaEnTexto()
        return dic


class ProcesadorVariantes(Procesador):

    def __init__(self, wpantalla, liKibitzersActivas, xtutor):
        self.liKibitzersActivas = liKibitzersActivas

        # self.configuracion = copy.deepcopy( VarGen.configuracion )
        self.configuracion = VarGen.configuracion

        self.liOpcionesInicio = [k_terminar, k_play, k_entrenamiento, k_competir,
                                 k_tools, k_opciones, k_informacion]  # Lo incluimos aqui porque sino no lo lee, en caso de aplazada

        self.siPresentacion = False

        self.pantalla = Pantalla.PantallaDialog(self, wpantalla)
        self.pantalla.ponGestor(self)

        self.tablero = self.pantalla.tablero

        self.teclaPanico = None
        self.xtutor = xtutor
        self.xrival = None
        self.xanalyzer = None

        self.replayBeep = None

        self.posicionInicial = None

        self.cpu = CPU.CPU(self.pantalla)

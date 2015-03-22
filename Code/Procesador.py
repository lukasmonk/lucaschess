import sys
import os
import random

from Code.Constantes import *
import Code.Configuracion as Configuracion
import Code.VarGen as VarGen
import Code.LCOS as LCOS
import Code.ControlPosicion as ControlPosicion
import Code.XGestorMotor as XGestorMotor
import Code.Util as Util
import Code.CPU as CPU
import Code.Presentacion as Presentacion
import Code.Albums as Albums
import Code.Entrenamientos as Entrenamientos
import Code.GestorMateMap as GestorMateMap
import Code.GestorNueva as GestorNueva
import Code.GestorEntPos as GestorEntPos
import Code.GestorElo as GestorElo
import Code.GestorMicElo as GestorMicElo
import Code.GestorFideFics as GestorFideFics
import Code.GestorSolo as GestorSolo
# import Code.GestorXFCC as GestorXFCC
import Code.GestorPGN as GestorPGN
import Code.GestorEntMaq as GestorEntMaq
import Code.GestorTorneo as GestorTorneo
import Code.GestorAlbum as GestorAlbum
import Code.QT.Piezas as Piezas
import Code.QT.DatosNueva as DatosNueva
import Code.QT.Pantalla as Pantalla
import Code.QT.QTUtil as QTUtil
import Code.QT.QTUtil2 as QTUtil2
import Code.QT.QTVarios as QTVarios
import Code.QT.Iconos as Iconos
import Code.QT.PantallaConfig as PantallaConfig
import Code.QT.PantallaMotores as PantallaMotores
import Code.QT.PantallaSonido as PantallaSonido
import Code.QT.PantallaColores as PantallaColores
# import Code.QT.PantallaXFCC as PantallaXFCC
import Code.QT.Info as Info
import Code.QT.PantallaEntMaq as PantallaEntMaq
import Code.QT.PantallaUsuarios as PantallaUsuarios
import Code.QT.PantallaAperturas as PantallaAperturas
import Code.QT.PantallaFavoritos as PantallaFavoritos
import Code.QT.PantallaTorneos as PantallaTorneos
import Code.QT.PantallaSTS as PantallaSTS
import Code.QT.PantallaAlbumes as PantallaAlbumes
import Code.QT.WBGuide as WBGuide
import Code.QT.WBDatabase as WBDatabase
import Code.QT.WBDatabaseFEN as WBDatabaseFEN
import Code.QT.PantallaWorkMap as PantallaWorkMap
import Code.QT.PantallaVoice as PantallaVoice

class Procesador():
    """
    Vinculo entre pantalla y gestores
    """

    def iniciaConUsuario(self, user):

        self.user = user

        self.web = "http://www-lucaschess.rhcloud.com"
        self.blog = "http://lucaschess.blogspot.com"

        self.liOpcionesInicio = [   k_terminar, k_play, k_competicion, k_elo,
                                    k_entrenamiento, k_tools, k_opciones, k_informacion]  # Lo incluimos aqui porque sino no lo lee, en caso de aplazada

        self.configuracion = Configuracion.Configuracion(user)
        self.configuracion.start(self.version)
        VarGen.configuracion = self.configuracion

        VarGen.todasPiezas = Piezas.TodasPiezas()

        self.gestor = None
        self.teclaPanico = 32  # necesario

        self.siPrimeraVez = True
        self.siPresentacion = False  # si esta funcionando la presentacion

        self.posicionInicial = ControlPosicion.ControlPosicion()
        self.posicionInicial.posInicial()

        self.xrival = None
        self.xtutor = None  # creaTutor lo usa asi que hay que definirlo antes

        self.replay = None
        self.replayBeep = None

        self.liKibitzersActivas = []

        self.liEngines = []
        self.xtutor = None

    def setVersion(self, version):
        self.version = version

    def quitaKibitzers(self):
        if hasattr(self, "liKibitzersActivas"):
            for xkibitzer in self.liKibitzersActivas:
                xkibitzer.terminar()

    def iniciarGUI(self):
        self.pantalla = Pantalla.Pantalla(self)
        self.pantalla.ponGestor(self)  # antes que muestra
        self.pantalla.muestra()

        self.tablero = self.pantalla.tablero

        self.entrenamientos = Entrenamientos.Entrenamientos(self)

        if self.configuracion.siAplazada:
            aplazamiento = self.configuracion.aplazamiento
            self.juegaAplazada(aplazamiento)
        else:
            if len(sys.argv) > 1:
                comando = sys.argv[1]
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
                elif comandoL.endswith(".lcf"):
                    self.externDatabaseFEN(comando)
                    return
                elif comando == "-play":
                    fen = sys.argv[2]
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
            self.gM1 = Presentacion.GestorM1(self)

            # self.cpu.start()

    def juegaAplazada(self, aplazamiento):
        self.cpu = CPU.CPU(self.pantalla)

        tipoJuego = aplazamiento["TIPOJUEGO"]
        siBlancas = aplazamiento["SIBLANCAS"]

        if tipoJuego == kJugNueva:
            categoria = self.configuracion.rival.categorias.segunClave(aplazamiento["CATEGORIA"])
            nivel = aplazamiento["NIVEL"]
            puntos = aplazamiento["PUNTOS"]
            self.gestor = GestorNueva.GestorNueva(self)
            self.gestor.inicio(categoria, nivel, siBlancas, puntos, aplazamiento)
        elif tipoJuego == kJugEntMaq:
            self.entrenaMaquina(None, aplazamiento)
        elif tipoJuego == kJugElo:
            self.gestor = GestorElo.GestorElo(self)
            self.gestor.inicio(None, aplazamiento["SICOMPETITIVO"], aplazamiento)
        elif tipoJuego == kJugMicElo:
            self.gestor = GestorMicElo.GestorMicElo(self)
            self.gestor.inicio(None, 0, 0, aplazamiento["SICOMPETITIVO"], aplazamiento)
        elif tipoJuego == kJugAlbum:
            self.gestor = GestorAlbum.GestorAlbum(self)
            self.gestor.inicio(None, None, None, aplazamiento)
        elif tipoJuego == kJugPGN:
            self.visorPGN("pgn_comandoExterno")
        elif tipoJuego == kJugSolo:
            self.jugarSolo(fichero=sys.argv[1])
        elif tipoJuego in (kJugFics, kJugFide):
            self.gestor = GestorFideFics.GestorFideFics(self)
            self.gestor.selecciona("Fics" if tipoJuego == kJugFics else "Fide" )
            self.gestor.inicio(aplazamiento["IDGAME"], aplazamiento["SICOMPETITIVO"], aplazamiento=aplazamiento)

    def XTutor(self):
        if not self.xtutor:
            self.creaXTutor()
        return self.xtutor

    def creaXTutor(self):
        xtutor = XGestorMotor.GestorMotor(self, self.configuracion.tutor)
        xtutor.opciones(self.configuracion.tiempoTutor, None, True)
        if self.configuracion.tutorMultiPV == 0:
            xtutor.maximizaMultiPV()
        else:
            xtutor.actMultiPV(self.configuracion.tutorMultiPV)

        self.xtutor = xtutor
        VarGen.xtutor = xtutor

        self.liEngines.append(xtutor)

    def cambiaXTutor(self):
        if self.xtutor:
            self.xtutor.terminar()
        self.creaXTutor()

    def creaGestorMotor(self, confMotor, tiempo, nivel, siMultiPV=False, priority=LCOS.NORMAL):
        xgestor = XGestorMotor.GestorMotor(self, confMotor)
        xgestor.opciones(tiempo, nivel, siMultiPV)
        xgestor.setPriority(priority)
        self.liEngines.append(xgestor)
        return xgestor

    def pararMotores(self):
        for xmotor in self.liEngines:
            xmotor.terminar()
        self.liEngines = []

    def cambiaRival(self, nuevo):
        """
        Llamado desde DatosNueva, cuando elegimos otro motor para jugar.
        """
        self.configuracion.rival = self.configuracion.buscaRival(nuevo)
        self.configuracion.graba()

    def menuPlay(self):
        menu = QTVarios.LCMenu(self.pantalla)
        menu.opcion(k_libre, _("Play against an engine of your choice"), Iconos.Libre())
        menu.separador()

        # Principiantes ----------------------------------------------------------------------------------------
        menu1 = menu.submenu(_("Opponents for young players"), Iconos.RivalesMP())

        menu1.opcion(1000 + kMP_1, _("Monkey"), Iconos.Monkey())
        menu1.opcion(1000 + kMP_2, _("Donkey"), Iconos.Donkey())
        menu1.opcion(1000 + kMP_3, _("Bull"), Iconos.Bull())
        menu1.opcion(1000 + kMP_4, _("Wolf"), Iconos.Wolf())
        menu1.opcion(1000 + kMP_5, _("Lion"), Iconos.Lion())
        menu1.opcion(1000 + kMP_6, _("Rat"), Iconos.Rat())
        menu1.opcion(1000 + kMP_7, _("Snake"), Iconos.Snake())
        menu1.separador()

        menu2 = menu1.submenu(_("Albums of animals"), Iconos.Penguin())
        albumes = Albums.AlbumesAnimales()
        dic = albumes.listaMenu()
        anterior = None
        for animal in dic:
            siDeshabilitado = False
            if anterior and not dic[anterior]:
                siDeshabilitado = True
            menu2.opcion(( "animales", animal ), _F(animal), Iconos.icono(animal), siDeshabilitado=siDeshabilitado)
            anterior = animal
        menu1.separador()

        menu2 = menu1.submenu(_("Albums of vehicles"), Iconos.Wheel())
        albumes = Albums.AlbumesVehicles()
        dic = albumes.listaMenu()
        anterior = None
        for character in dic:
            siDeshabilitado = False
            if anterior and not dic[anterior]:
                siDeshabilitado = True
            menu2.opcion(( "vehicles", character ), _F(character), Iconos.icono(character),
                         siDeshabilitado=siDeshabilitado)
            anterior = character

        resp = menu.lanza()
        if resp:
            if resp == k_libre:
                self.procesarAccion(resp)

            elif type(resp) == int:
                rival = resp - 1000
                uno = QTVarios.blancasNegrasTiempo(self.pantalla)
                if uno:
                    siBlancas, siTiempo, minutos, segundos = uno
                    if siBlancas is not None:
                        if not siTiempo:
                            minutos = None
                            segundos = None
                        self.entrenaRivalesMPC(siBlancas, rival, rival >= kMP_6, minutos, segundos)
            else:
                tipo, cual = resp
                if tipo == "animales":
                    self.albumAnimales(cual)
                elif tipo == "vehicles":
                    self.albumVehicles(cual)

    def reabrirAlbum(self, album):
        tipo, nombre = album.claveDB.split("_")
        if tipo == "animales":
            self.albumAnimales(nombre)
        elif tipo == "vehicles":
            self.albumVehicles(nombre)

    def albumAnimales(self, animal):
        albumes = Albums.AlbumesAnimales()
        album = albumes.getAlbum(animal)
        album.compruebaTerminado()
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
        album = albumes.getAlbum(character)
        album.compruebaTerminado()
        cromo, siRebuild = PantallaAlbumes.eligeCromo(self.pantalla, self, album)
        if cromo is None:
            if siRebuild:
                albumes.reset(character)
                self.albumVehicles(character)
            return

        self.gestor = GestorAlbum.GestorAlbum(self)
        self.gestor.inicio(album, cromo)

    def procesarAccion(self, clave):
        if self.siPresentacion:
            self.presentacion(False)

        if clave == k_terminar:
            if hasattr(self, "cpu"):
                self.cpu.stop()
            self.pantalla.guardarVideo()
            self.pantalla.accept()

        elif clave == k_play:
            self.menuPlay()

        elif clave == k_libre:
            self.libre()

        elif clave == k_competicion:
            self.competicion()

        elif clave == k_entrenamiento:
            self.entrenamientos.lanza()

        elif clave == k_opciones:
            self.opciones()

        elif clave == k_tools:
            self.tools()

        elif clave == k_elo:
            self.elo()

        elif clave == k_informacion:
            self.informacion()

    def opciones(self):
        menu = QTVarios.LCMenu(self.pantalla)

        menu.opcion(self.cambiaConfiguracion, _("Configuration"), Iconos.Opciones())
        menu.separador()

        menu1 = menu.submenu(_("Colors"), Iconos.Colores())
        menu1.opcion(self.editaColoresTablero, _("Main board"), Iconos.EditarColores())
        menu1.separador()
        menu1.opcion(self.cambiaColoresPGN, _("PGN"), Iconos.Vista())
        menu.separador()

        menu1 = menu.submenu(_("Change board size"), Iconos.TamTablero())
        menu1.opcion(self.size_main, _("Main board"), Iconos.PuntoVerde())
        menu1.separador()
        menu2 = menu1.submenu(_("Tutor board"), Iconos.PuntoAzul())
        for txt, size in (   ( _("Large"), 64 ),
                             ( _("Medium"), 48 ),
                             ( _("Medium-small"), 32 ),
                             ( _("Small"), 24 ),
                             ( _("Very small"), 16 ) ):
            menu2.opcion( (self.size_tutor, size), txt, Iconos.PuntoNaranja())
            menu2.separador()

        menu.separador()
        menu1 = menu.submenu(_("Sound"), Iconos.SoundTool())
        menu1.opcion(self.sonidos, _("Custom sounds"), Iconos.S_Play())
        if self.configuracion.voice:
            menu1.separador()
            menu2 = menu1.submenu(_("Test voice"), Iconos.S_Microfono())
            menu2.opcion( (self.voice, "word"), _("Words"), Iconos.Words())
            menu2.opcion( (self.voice, "position"), _("Positions"), Iconos.Voyager())
            menu2.opcion( (self.voice, "pgn"), _("Games"), Iconos.InformacionPGN())
        menu1.separador()
        menu2 = menu1.submenu(_("Import voices"), Iconos.Importar())
        menu2.opcion( (self.voiceImport, "en_us"), _("English") + " US", Iconos.PuntoVerde() )
        menu2.opcion( (self.voiceImport, "es"), _("Spanish"), Iconos.PuntoRojo() )

        menu.separador()
        menu.opcion(self.favoritos, _("Training favorites"), Iconos.Corazon())

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

    def cambiaColoresPGN(self):
        PantallaColores.cambiaColoresPGN(self.pantalla, self.configuracion)

    def size_main(self):
        self.tablero.cambiaSize()

    def size_tutor(self, tam):
        confTablero = self.configuracion.confTablero("TUTOR", 16)
        confTablero.anchoPieza(tam)
        confTablero.guardaEnDisco()

    def sonidos(self):
        w = PantallaSonido.WSonidos(self)
        w.exec_()

    def voice(self, tipo):
        if tipo == "word":
            w = PantallaVoice.WVoiceWordsTest(self)
        elif tipo == "position":
            w = PantallaVoice.WVoicePositionsTest(self)
        elif tipo == "pgn":
            w = PantallaVoice.WVoicePGNsTest(self)
        w.exec_()

    def voiceImport( self, lng):
        PantallaVoice.importVoice(self.pantalla, lng, self.configuracion)

    def favoritos(self):
        PantallaFavoritos.miraFavoritos(self.entrenamientos)

    def folder_change(self):
        carpeta = QTUtil2.leeCarpeta(self.pantalla, self.configuracion.carpeta,
                                     _("Change the folder where all data is saved") + "\n" + _(
                                         "Be careful please"))
        if carpeta:
            if os.path.isdir(carpeta):
                self.configuracion.changeFolder(carpeta)
                self.reiniciar()

    def folder_default(self):
        self.configuracion.changeFolder(None)
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
        id = self.gestor.eligeJuego(siCompetitivo, nivel)
        self.gestor.inicio(id, siCompetitivo)

    def fideelo(self, siCompetitivo, nivel):
        self.gestor = GestorFideFics.GestorFideFics(self)
        self.gestor.selecciona("Fide")
        id = self.gestor.eligeJuego(siCompetitivo, nivel)
        self.gestor.inicio(id, siCompetitivo)

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
        menu1.opcion("databaseFEN", _("Positions"), Iconos.DatabaseF())
        menu.separador()

        menu1 = menu.submenu(_("Engines"), Iconos.Motores())
        menu1.opcion("torneos", _("Tournaments between engines"), Iconos.Torneos())
        menu1.separador()
        menu1.opcion("motores", _("External engines"), Iconos.Motores())
        menu1.separador()
        menu1.opcion("sts", _("STS: Strategic Test Suite"), Iconos.STS())
        menu.separador()

        menu1 = menu.submenu(_("Openings"), Iconos.Aperturas())
        menu1.opcion("aperturaspers", _("Custom openings"), Iconos.Apertura())
        menu1.separador()
        menu1.opcion("bookguide", _("Personal Opening Guide"), Iconos.BookGuide())
        menu.separador()
        menu.separador()

        # menu1 = menu.submenu(_("Correspondence Chess"), Iconos.XFCC())
        # liRemoves = []
        # for f in Util.listfiles(self.configuracion.carpeta, "*.xfcc"):
            # nomf = os.path.basename(f)[:-5]
            # x = nomf.rfind("_")
            # if x > 0:
                # user = nomf[x + 1:].lower()
                # server = nomf[:x]
                # menu1.opcion("xfcc|%s|%s|%s" % (user, server, f), "%s: %s" % (server, user), Iconos.PuntoAzul())
                # menu1.separador()
                # liRemoves.append((user, server, f))

        # menu1.opcion("xfcc_nuevo", _("New link"), Iconos.Mas())
        # if liRemoves:
            # menu1.separador()
            # menu2 = menu1.submenu(_("Remove"), Iconos.Delete())
            # for user, server, f in liRemoves:
                # menu2.opcion("del_xfcc|%s|%s|%s" % (user, server, f), "%s: %s" % (server, user), Iconos.PuntoNaranja())
                # menu2.separador()

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

            elif resp == "database":
                self.database()
            elif resp == "databaseFEN":
                self.databaseFEN()

            elif resp == "aperturaspers":
                self.aperturaspers()
            elif resp == "bookguide":
                w = WBGuide.WBGuide(self.pantalla, self)
                w.exec_()

            # elif resp.startswith("xfcc"):
                # self.xfcc(resp)

            # elif resp.startswith("del_xfcc"):
                # self.xfccDel(resp)

    def externDatabase(self, fichero):
        self.configuracion.ficheroDBgames = fichero
        self.database()
        self.procesarAccion(k_terminar)

    def database(self):
        w = WBDatabase.WBDatabase(self.pantalla, self)
        w.exec_()

    def externDatabaseFEN(self, fichero):
        self.configuracion.ficheroDBgamesFEN = fichero
        self.databaseFEN()
        self.procesarAccion(k_terminar)

    def databaseFEN(self):
        w = WBDatabaseFEN.WBDatabaseFEN(self.pantalla, self)
        w.exec_()

    # def xfcc(self, orden):
        # dicServ = GestorXFCC.dicServers()
        # if orden == "xfcc_nuevo":
            # resp = PantallaXFCC.newServerUser(self.pantalla, dicServ)
            # if resp:
                # server, user, password = resp
                # db = GestorXFCC.DB_XFCC(self.configuracion.carpeta, server, user, dicServ[server], password)
            # else:
                # return
        # else:
            # nada, user, server, fich = orden.split("|")
            # for k in dicServ:
                # if k.lower() == server.lower():
                    # server = k
                    # break
            # db = GestorXFCC.DB_XFCC(self.configuracion.carpeta, server, user, dicServ[server])
        # if PantallaXFCC.pantallaXFCC(self, db):
            # self.gestor = GestorXFCC.GestorXFCC(self)
            # self.gestor.inicio(db)

    # def xfccDel(self, orden):
        # nada, user, server, fich = orden.split("|")
        # if QTUtil2.pregunta(self.pantalla, _X(_("Delete %1?"), "%s: %s" % (server, user))):
            # os.remove(fich)

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

    def jugarSolo(self, fichero=None, pgn=None):
        self.gestor = GestorSolo.GestorSolo(self)
        if pgn:
            dic = GestorSolo.pgn_pks(kJugando, pgn)
            self.gestor.inicio(dic)
        else:
            self.gestor.inicio(fichero=fichero)

    def entrenaPos(self, posicion, nPosiciones, titentreno, liEntrenamientos, entreno, jump):
        self.tipoJuego = kJugEntPos
        self.estado = kJugando
        self.gestor = GestorEntPos.GestorEntPos(self)
        self.gestor.ponEntreno(entreno)
        self.gestor.inicio(posicion, nPosiciones, titentreno, liEntrenamientos, jump=jump)

    def informacion(self):

        liBlog = (
            ( "Director ", "http://lucaschess.blogspot.com.es/2012/05/director.html" ),
            ("Tactical training with your own blunders",
             "http://lucaschess.blogspot.com.es/2011/11/tactical-training-with-your-own.html" ),
            ( "Announcements sounds", "http://lucaschess.blogspot.com.es/2011/10/announcements-sounds.html" ),
            ("Personalities in Game against an engine of your choice",
             "http://lucaschess.blogspot.com.es/2011/09/version-60-beta-1-personalities.html" ),
            ("Training favourites and Your daily test",
             "http://lucaschess.blogspot.com.es/2011/09/version-60-dev4-with-favourites-and.html" ),
            (
                "Captured material panel",
                "http://lucaschess.blogspot.com.es/2011/06/version-53-captures-and-more.html" ),
            ("Learn openings by repetition",
             "http://lucaschess.blogspot.com.es/2011/06/version-52-standard-openings.html" ),
            ( "Kibitzers", "http://lucaschess.blogspot.com.es/2011/06/version-51-with-kibitzers.html" ),
            ("Training mate positions",
             "http://lucaschess.blogspot.com.es/2011/03/new-option-training-mate-positions.html" ),
        )

        menu = QTVarios.LCMenu(self.pantalla)

        menu.opcion( "docs", _("Documents"), Iconos.Ayuda())
        menu.separador()
        menu.opcion("web", _("Homepage"), Iconos.Web())
        menu.separador()
        menu1 = menu.submenu("Fresh news", Iconos.Blog())
        menu1.opcion("blog", "Fresh news", Iconos.Blog())
        menu1.separador()
        for txt, lnk in liBlog:
            menu1.opcion(lnk, txt, Iconos.PuntoAzul())
        menu.separador()
        # menu.opcion("downloads", _("Downloads"), Iconos.Downloads())
        # menu.separador()
        menu.opcion("mail", _("Contact") + " (%s)" % "lukasmonk@gmail.com", Iconos.Mail())
        menu.separador()

        menu.opcion("acercade", _("About"), Iconos.Aplicacion())

        resp = menu.lanza()
        if resp is None:
            return
        elif resp == "acercade":
            self.acercade()
        elif resp == "docs":
            VarGen.startfile("%s/docs" % self.web)
        elif resp == "blog":
            VarGen.startfile(self.blog)
        elif resp.startswith("http"):
            VarGen.startfile(resp)
        elif resp == "web":
            VarGen.startfile("%s/index?lang=%s" % (self.web, self.configuracion.traductor))
        # elif resp == "downloads":
            # VarGen.startfile("https://2dc90e9d4d8c66f3ab71f42ff9cd1b6ab1f26543.googledrive.com/host/0B0D6J3YCrUoublFqc0VGZWw3VVU/release/")
        elif resp == "mail":
            VarGen.startfile("mailto:lukasmonk@gmail.com")


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

            self.gestor = GestorNueva.GestorNueva(self)
            self.gestor.inicio(categoria, nivel, siBlancas, puntos)

    def finalX(self):
        return True

    def finalX0(self):
        return True

    def clonVariantes(self, wpantalla, liKibitzersActivas=None, xtutor=None):
        if liKibitzersActivas is None:
            liKibitzersActivas = []
        return ProcesadorVariantes(wpantalla, liKibitzersActivas, xtutor, self.liEngines)

    def gestorUnPGN(self, wpantalla, pgn, jugadaInicial=None, siGrabar=True):

        clonProcesador = ProcesadorVariantes(wpantalla, self.liKibitzersActivas, self.xtutor, self.liEngines)

        clonProcesador.gestor = GestorSolo.GestorSolo(clonProcesador)
        clonProcesador.gestor.inicio(pgn=pgn, jugadaInicial=jugadaInicial, siGrabar=siGrabar)

        clonProcesador.pantalla.muestraVariantes(clonProcesador.gestor.tituloVentanaPGN())

        return getattr(clonProcesador, "valorPGN", (None, None, None))

    def entrenaRivalesMPC(self, siBlancas, rival, siApertura, nMinutos, nSegundos):

        dic = {}
        dic["SIBLANCAS"] = siBlancas
        dic["COLOR"] = "B" if siBlancas else "N"
        dic["FEN"] = ""
        dic["AYUDAS"] = 0
        dic["APERTURA"] = None
        dic["SIAPERTURA"] = siApertura

        dr = dic["RIVAL"] = {}
        dr["MOTOR"] = rival
        dr["TIEMPO"] = 0
        dr["PROFUNDIDAD"] = 0

        dr = dic["TUTOR"] = {}

        dr["MOTOR"] = self.configuracion.tutor.claveReal()
        dr["TIEMPO"] = self.configuracion.tiempoTutor / 100
        dr["PROFUNDIDAD"] = 0

        dic["SITIEMPO"] = nMinutos > 0
        if dic["SITIEMPO"]:
            dic["MINUTOS"] = nMinutos
            dic["SEGUNDOS"] = nSegundos

        dic["ATRAS"] = True

        self.entrenaMaquina(dic)

    def saveAsPKS(self, estado, partida, pgn):
        dic = GestorSolo.pgn_pks(estado, pgn)
        dic["PARTIDA"] = partida.guardaEnTexto()

        return dic

        # def saveAsJSON(self, estado, partida, pgn):
        # dic = GestorSolo.pgn_json(estado, pgn)
        # return dic

class ProcesadorVariantes(Procesador):
    def __init__(self, wpantalla, liKibitzersActivas, xtutor, liEngines):
        self.liKibitzersActivas = liKibitzersActivas

        # self.configuracion = copy.deepcopy( VarGen.configuracion )
        self.configuracion = VarGen.configuracion

        self.pantalla = Pantalla.Pantalla(self, wpantalla)
        self.pantalla.ponGestor(self)

        self.tablero = self.pantalla.tablero

        self.teclaPanico = None
        self.xtutor = xtutor
        self.xrival = None

        self.replayBeep = None

        self.cpu = CPU.CPU(self.pantalla)
        self.liEngines = liEngines


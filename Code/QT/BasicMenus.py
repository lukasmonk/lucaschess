from Code import VarGen
from Code import Util
from Code import Albums
from Code.QT import Colocacion
from Code.QT import Columnas
from Code.QT import Controles
from Code.QT import Grid
from Code.QT import QTVarios
from Code.QT import Iconos
from Code.QT import Delegados


class SaveMenu:
    def __init__(self, dicDatos, launcher, rotulo=None, icono=None):
        self.liopciones = []
        self.rotulo = rotulo
        self.icono = icono
        self.dicDatos = {} if dicDatos is None else dicDatos
        self.launcher = launcher

    def opcion(self, clave, rotulo, icono, siDeshabilitado=None):
        self.liopciones.append(("opc", (clave, rotulo, icono, siDeshabilitado)))
        self.dicDatos[clave] = (self.launcher, rotulo, icono, siDeshabilitado)

    def separador(self):
        self.liopciones.append(("sep", None))

    def submenu(self, rotulo, icono):
        sm = SaveMenu(self.dicDatos, self.launcher, rotulo, icono)
        self.liopciones.append(("sub", sm))
        return sm

    def _menu(self, menu):
        for tipo, datos in self.liopciones:
            if tipo == "opc":
                (clave, rotulo, icono, siDeshabilitado) = datos
                menu.opcion(clave, rotulo, icono, siDeshabilitado=siDeshabilitado)
            elif tipo == "sep":
                menu.separador()
            elif tipo == "sub":
                sm = datos
                submenu = menu.submenu(sm.rotulo, sm.icono)
                sm._menu(submenu)

    def lanza(self, procesador):
        menu = QTVarios.LCMenu(procesador.pantalla)
        self._menu(menu)
        return menu.lanza()


def menuTools_savemenu(procesador, dicDatos=None):
    savemenu = SaveMenu(dicDatos, procesador.menuTools_run)

    savemenu.opcion("juega_solo", _("Create your own game"), Iconos.JuegaSolo())
    savemenu.separador()

    menu1 = savemenu.submenu(_("PGN viewer"), Iconos.PGN())
    menu1.opcion("pgn_paste", _("Paste PGN"), Iconos.Pegar())
    menu1.separador()
    menu1.opcion("pgn_fichero", _("Read PGN"), Iconos.Fichero())
    menu1.separador()
    menu1.opcion("pgn_miniatura", _("Miniature of the day"), Iconos.Miniatura())
    menu1.separador()
    if procesador.configuracion.liTrasteros:
        menu1.opcion("pgn_trasteros", _("Boxrooms PGN"), Iconos.Trasteros())
        menu1.separador()
    if procesador.configuracion.salvarFichero and Util.existeFichero(procesador.configuracion.salvarFichero):
        menu1.opcion("pgn_nuestroFichero", _("My games"), Iconos.NuestroFichero())
    savemenu.separador()

    menu1 = savemenu.submenu(_("Database"), Iconos.Database())
    menu1.opcion("database", _("Complete games"), Iconos.DatabaseC())
    menu1.separador()
    menu1.opcion("databaseFEN", _("Positions"), Iconos.DatabaseF())
    savemenu.separador()

    savemenu.opcion("manual_save", _("Save positions to FNS/PGN"), Iconos.ManualSave())
    savemenu.separador()

    menu1 = savemenu.submenu(_("Openings"), Iconos.Aperturas())
    menu1.opcion("openings", _("Opening lines"), Iconos.OpeningLines())
    menu1.separador()
    menu1.opcion("bookguide", _("Personal Opening Guide"), Iconos.BookGuide())
    menu1.separador()
    menu1.opcion("aperturaspers", _("Custom openings"), Iconos.Apertura())
    savemenu.separador()

    menu1 = savemenu.submenu(_("Engines"), Iconos.Motores())
    menu1.opcion("torneos", _("Tournaments between engines"), Iconos.Torneos())
    menu1.separador()
    menu1.opcion("sts", _("STS: Strategic Test Suite"), Iconos.STS())
    menu1.separador()
    menu1.opcion("motores", _("External engines"), Iconos.Motores())
    menu1.separador()
    menu1.opcion("kibitzers", _("Kibitzers"), Iconos.Kibitzer())
    savemenu.separador()
    return savemenu


def menuTools(procesador):
    savemenu = menuTools_savemenu(procesador)
    return savemenu.lanza(procesador)


def menuPlay_savemenu(procesador, dicDatos=None):
    savemenu = SaveMenu(dicDatos, procesador.menuPlay_run)

    savemenu.opcion(("free", None), _("Play against an engine of your choice"), Iconos.Libre())
    savemenu.separador()

    # Principiantes ----------------------------------------------------------------------------------------
    menu1 = savemenu.submenu(_("Opponents for young players"), Iconos.RivalesMP())

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

    return savemenu


def menuPlay(procesador):
    savemenu = menuPlay_savemenu(procesador)
    return savemenu.lanza(procesador)


def menuCompetir_savemenu(procesador, dicDatos=None):
    savemenu = SaveMenu(dicDatos, procesador.menuCompetir_run)
    savemenu.opcion(("competition", None), _("Competition with tutor"), Iconos.NuevaPartida())
    savemenu.separador()

    submenu = savemenu.submenu(_("Elo-Rating"), Iconos.Elo())
    submenu.opcion(("lucaselo",0), "%s (%d)" % (_("Lucas-Elo"), procesador.configuracion.elo), Iconos.Elo())
    submenu.separador()
    if VarGen.isWindows or VarGen.isWine:
        submenu.opcion(("micelo",0), "%s (%d)" % (_("Tourney-Elo"), procesador.configuracion.michelo), Iconos.EloTimed())
        submenu.separador()
    fics = procesador.configuracion.fics
    menuf = submenu.submenu("%s (%d)" % (_("Fics-Elo"), fics), Iconos.Fics())
    rp = QTVarios.rondoPuntos()
    for elo in range(900, 2800, 100):
        if (elo == 900) or (0 <= (elo + 99 - fics) <= 400 or 0 <= (fics - elo) <= 400):
            menuf.opcion(("fics", elo / 100), "%d-%d" % (elo, elo + 99), rp.otro())
    submenu.separador()
    fide = procesador.configuracion.fide
    menuf = submenu.submenu("%s (%d)" % (_("Fide-Elo"), fide), Iconos.Fide())
    for elo in range(1500, 2700, 100):
        if (elo == 1500) or (0 <= (elo + 99 - fide) <= 400 or 0 <= (fide - elo) <= 400):
            menuf.opcion(("fide", elo / 100), "%d-%d" % (elo, elo + 99), rp.otro())
    lichess = procesador.configuracion.lichess
    submenu.separador()
    menuf = submenu.submenu("%s (%d)" % (_("Lichess-Elo"), lichess), Iconos.Lichess())
    rp = QTVarios.rondoPuntos()
    for elo in range(800, 2700, 100):
        if (elo == 800) or (0 <= (elo + 99 - lichess) <= 400 or 0 <= (lichess - elo) <= 400):
            menuf.opcion(("lichess", elo / 100), "%d-%d" % (elo, elo + 99), rp.otro())
    savemenu.separador()
    submenu = savemenu.submenu(_("Singular moves"), Iconos.Singular())
    submenu.opcion(("strenght101", 0), _("Calculate your strength"), Iconos.Strength())
    submenu.separador()
    submenu.opcion(("challenge101",0), _("Challenge 101"), Iconos.Wheel())

    return savemenu


def menuCompetir(procesador):
    savemenu = menuCompetir_savemenu(procesador)
    return savemenu.lanza(procesador)


class WAtajos(QTVarios.WDialogo):
    def __init__(self, procesador, dicDatos):
        entrenamientos = procesador.entrenamientos
        entrenamientos.comprueba()
        self.entrenamientos = entrenamientos
        self.procesador = procesador
        self.liFavoritos = self.procesador.configuracion.liFavoritos
        self.dicDatos = dicDatos

        QTVarios.WDialogo.__init__(self, self.procesador.pantalla, _("Shortcuts"), Iconos.Atajos(), "atajos")

        liAcciones = [
            (_("Close"), Iconos.MainMenu(), self.terminar), None,
            ("+" + _("Play"), Iconos.Libre(), self.masplay),
            ("+" + _("Train"), Iconos.Entrenamiento(), self.masentrenamiento),
            ("+" + _("Compete"), Iconos.NuevaPartida(), self.mascompete),
            ("+" + _("Tools"), Iconos.Tools(), self.mastools), None,
            (_("Remove"), Iconos.Borrar(), self.borrar), None,
            (_("Up"), Iconos.Arriba(), self.arriba),
            (_("Down"), Iconos.Abajo(), self.abajo), None,
        ]
        tb = Controles.TBrutina(self, liAcciones, puntos=procesador.configuracion.puntosTB)

        # Lista
        oColumnas = Columnas.ListaColumnas()
        oColumnas.nueva("OPCION", _("Option"), 300)
        oColumnas.nueva("LABEL", _("Label"), 300, edicion=Delegados.LineaTextoUTF8(siPassword=False))

        self.grid = Grid.Grid(self, oColumnas, siSelecFilas=True, siEditable=True)
        self.grid.setMinimumWidth(self.grid.anchoColumnas() + 20)
        f = Controles.TipoLetra(puntos=10, peso=75)
        self.grid.setFont(f)

        # Layout
        layout = Colocacion.V().control(tb).control(self.grid).relleno().margen(3)
        self.setLayout(layout)

        self.recuperarVideo(siTam=True)

        self.grid.gotop()

    def terminar(self):
        self.guardarVideo()
        self.accept()

    def masplay(self):
        self.nuevo(menuPlay(self.procesador))

    def mascompete(self):
        self.nuevo(menuCompetir(self.procesador))

    def masentrenamiento(self):
        self.nuevo(self.entrenamientos.menu.lanza())

    def mastools(self):
        self.nuevo(menuTools(self.procesador))

    def gridNumDatos(self, grid):
        return len(self.liFavoritos)

    def gridDato(self, grid, fila, oColumna):
        dic = self.liFavoritos[fila]
        opcion = dic["OPCION"]
        if opcion in self.dicDatos:
            menu_run, nombre, icono, siDeshabilitado = self.dicDatos[opcion]
        else:
            nombre = "???"
        return dic.get("LABEL", nombre) if oColumna.clave == "LABEL" else nombre

    def gridPonValor(self, grid, fila, oColumna, valor):  # ? necesario al haber delegados
        dic = self.liFavoritos[fila]
        dato = self.dicDatos.get(dic["OPCION"], None)
        if dato is not None:
            if valor:
                dic["LABEL"] = valor.strip()
            else:
                if "LABEL" in dic:
                    del dic["LABEL"]

            self.graba(fila)

    def graba(self, fila):
        self.procesador.configuracion.liFavoritos = self.liFavoritos
        self.procesador.configuracion.graba()
        self.grid.refresh()
        if fila >= len(self.liFavoritos):
            fila = len(self.liFavoritos) - 1
        self.grid.goto(fila, 0)

    def nuevo(self, resp):
        if resp:
            resp = {"OPCION": resp}
            fila = self.grid.recno()
            tam = len(self.liFavoritos)
            if fila < tam - 1:
                fila += 1
                self.liFavoritos.insert(fila, resp)
            else:
                self.liFavoritos.append(resp)
                fila = len(self.liFavoritos) - 1
            self.graba(fila)

    def borrar(self):
        fila = self.grid.recno()
        if fila >= 0:
            del self.liFavoritos[fila]
            self.graba(fila)

    def arriba(self):
        fila = self.grid.recno()
        if fila >= 1:
            self.liFavoritos[fila], self.liFavoritos[fila - 1] = self.liFavoritos[fila - 1], self.liFavoritos[fila]
            self.graba(fila - 1)

    def abajo(self):
        fila = self.grid.recno()
        if fila < len(self.liFavoritos) - 1:
            self.liFavoritos[fila], self.liFavoritos[fila + 1] = self.liFavoritos[fila + 1], self.liFavoritos[fila]
            self.graba(fila + 1)


def atajos(procesador):
    procesador.entrenamientos.comprueba()
    dicDatos = procesador.entrenamientos.dicMenu
    menuPlay_savemenu(procesador, dicDatos)
    menuCompetir_savemenu(procesador, dicDatos)
    menuTools_savemenu(procesador, dicDatos)
    liFavoritos = procesador.configuracion.liFavoritos

    menu = QTVarios.LCMenu(procesador.pantalla)
    nx = 1
    for dic in liFavoritos:
        clave = dic["OPCION"]
        if clave in dicDatos:
            launcher, rotulo, icono, siDeshabilitado = dicDatos[clave]
            label = dic.get("LABEL", rotulo)
            if nx <= 9:
                label += "  [%s-%d]" % (_("Alt"), nx)
            menu.opcion(clave, label, icono, siDeshabilitado)
            nx += 1
            menu.separador()
    menu.separador()
    menu.opcion("ed_atajos", _("Add new shortcuts"), Iconos.Mas())
    resp = menu.lanza()
    if resp == "ed_atajos":
        w = WAtajos(procesador, dicDatos)
        w.exec_()
    elif resp is not None and resp in dicDatos:
        launcher, rotulo, icono, siDeshabilitado = dicDatos[resp]
        launcher(resp)


def atajosALT(procesador, num):
    procesador.entrenamientos.comprueba()
    dicDatos = procesador.entrenamientos.dicMenu
    menuPlay_savemenu(procesador, dicDatos)
    menuCompetir_savemenu(procesador, dicDatos)
    menuTools_savemenu(procesador, dicDatos)
    liFavoritos = procesador.configuracion.liFavoritos

    nx = 1
    for dic in liFavoritos:
        clave = dic["OPCION"]
        if clave in dicDatos:
            launcher, rotulo, icono, siDeshabilitado = dicDatos[clave]
            if nx == num:
                launcher(clave)
                return
            nx += 1


def menuInformacion(procesador):
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

    menu = QTVarios.LCMenu(procesador.pantalla)

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
    submenu = menu.submenu("%s (%s)" % (_("Scores"), _("will be removed in the next version")), Iconos.EstrellaAzul())  # EXE
    submenu.opcion("puntuacionPost", _("Post your score"), Iconos.Puntuacion())  # EXE
    submenu.opcion("puntuacionConsulta", _("Check your scores in time"), Iconos.EstrellaAzul())  # EXE
    menu.separador()  # EXE
    if procesador.configuracion.siMain:  # EXE
        menu.opcion("actualiza", _("Search for updates"), Iconos.Actualiza())  # EXE
        menu.separador()  # EXE

    menu.opcion("acercade", _("About"), Iconos.Aplicacion())

    return menu.lanza()

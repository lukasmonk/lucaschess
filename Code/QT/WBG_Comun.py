from Code import ControlPosicion
from Code import Partida
from Code.QT import Iconos
from Code.QT import PantallaAnalisisParam
from Code.QT import QTUtil2
from Code.QT import QTVarios
from Code import VarGen


class Analisis:
    def __init__(self, wowner, bookGuide, dispatchAnalisis, procesador):
        self.pantalla = wowner
        self.dbAnalisis = bookGuide.dbAnalisis
        self.dispatchAnalisis = dispatchAnalisis
        self.procesador = procesador

    def menuAnalizar(self, fenM2, menu, siShowMoves):

        liAnalisis, nActivo = self.dbAnalisis.lista(fenM2)

        # Si tiene ya analisis, lo pedimos o nuevo
        if liAnalisis:
            if menu is None:
                submenu = QTVarios.LCMenu(self.pantalla)
            else:
                submenu = menu.submenu(_("Analyze"), Iconos.Analizar())
            if nActivo is not None:
                submenu.opcion("an_use_%d" % nActivo, liAnalisis[nActivo].rotulo, Iconos.Seleccionado(),
                               siDeshabilitado=True)
            submenu.separador()
            for n, mrm in enumerate(liAnalisis):
                if n != nActivo:
                    submenu.opcion("an_use_%d" % n, mrm.rotulo, Iconos.PuntoAzul())
            submenu.separador()

            submenu.opcion("an_new", _("New analysis"), Iconos.Mas())
            submenu.separador()

            if self.dbAnalisis.activo(fenM2) is not None:
                submenu.opcion("an_hide", _("Hide analysis"), Iconos.Ocultar())
                submenu.separador()
                if siShowMoves:
                    submenu.opcion("an_show", _("Show moves"), Iconos.Pelicula())
                    submenu.separador()

            menu1 = submenu.submenu(_("Remove"), Iconos.Delete())
            for n, mrm in enumerate(liAnalisis):
                menu1.opcion("an_rem_%d" % n, mrm.rotulo, Iconos.PuntoRojo())
                menu1.separador()

            if menu is None:
                return submenu.lanza()

        else:
            if menu is None:
                return "an_new"
            else:
                menu.opcion("an_new", _("Analyze"), Iconos.Analizar())

    def exeAnalizar(self, fenM2, resp, infoDispatch, fen, pv, rm):

        if resp == "an_new":
            self.nuevoAnalisis(fenM2, infoDispatch)

        elif resp == "an_show":
            self.showAnalisis(fen, pv, rm)

        elif resp.startswith("an_use_"):
            self.dbAnalisis.pon(fenM2, int(resp[7:]))
            self.dispatchAnalisis(infoDispatch)

        elif resp == "an_hide":
            self.dbAnalisis.pon(fenM2, None)
            self.dispatchAnalisis(infoDispatch)

        elif resp.startswith("an_rem_"):
            liAnalisis = self.dbAnalisis.lista(fenM2)[0]
            num = int(resp[7:])
            if QTUtil2.pregunta(self.pantalla, _X(_("Delete %1?"), liAnalisis[num].rotulo)):
                self.dbAnalisis.quita(fenM2, num)
                self.dispatchAnalisis(infoDispatch)
            return

    def nuevoAnalisis(self, fenM2, infoDispatch):
        alm = PantallaAnalisisParam.paramAnalisis(self.pantalla, VarGen.configuracion, False, siTodosMotores=True)
        if alm is None:
            return
        configuracion = VarGen.configuracion
        confMotor = configuracion.buscaMotor(alm.motor)
        confMotor.actMultiPV(alm.multiPV)

        xmotor = self.procesador.creaGestorMotor(confMotor, alm.tiempo, alm.depth, siMultiPV=True)

        me = QTUtil2.analizando(self.pantalla)
        mrm = xmotor.analiza(fenM2 + " 0 1")
        mrm.tiempo = alm.tiempo / 1000.0

        mrm.rotulo = ("%s %.0f\"" % (mrm.nombre, mrm.tiempo))
        xmotor.terminar()
        me.final()

        self.dbAnalisis.nuevo(fenM2, mrm)

        self.dispatchAnalisis(infoDispatch)

    def showAnalisis(self, fen, pv, rm):

        cp = ControlPosicion.ControlPosicion()
        cp.leeFen(fen)
        partida = Partida.Partida(cp)
        partida.leerPV(pv)
        lineaPGN = partida.pgnBaseRAW()
        import Code.Variantes as Variantes

        Variantes.editaVarianteMoves(self.procesador, self.pantalla, True, fen, lineaPGN,
                                     titulo="%s %s" % (rm.nombre, rm.texto()))

    def siMiraKibitzers(self):
        return False  # Compatibilidad para que funcione showAnalisis

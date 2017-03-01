from Code.QT import Controles
from Code.QT import FormLayout
from Code.QT import Iconos
from Code.QT import QTUtil2
from Code.QT import QTVarios
from Code.Constantes import *


class Personalidades:
    def __init__(self, owner, configuracion):
        self.owner = owner
        self.configuracion = configuracion

    def listaAjustes(self, siTodos):
        liAjustes = [
            (_("Best move"), kAjustarMejor),
            (_("Somewhat better") + "++", kAjustarSuperiorMM),
            (_("Somewhat better") + "+", kAjustarSuperiorM),
            (_("Somewhat better"), kAjustarSuperior),
            (_("Similar to the player"), kAjustarSimilar),
            (_("Somewhat worse"), kAjustarInferior),
            (_("Somewhat worse") + "-", kAjustarInferiorM),
            (_("Somewhat worse") + "--", kAjustarInferiorMM),
            (_("Worst move"), kAjustarPeor),
            ("-" * 30, None),
            (_("High level"), kAjustarNivelAlto),
            (_("Intermediate level"), kAjustarNivelMedio),
            (_("Low level"), kAjustarNivelBajo),
            ("-" * 30, None),
            (_("Move selected by the player"), kAjustarPlayer),
        ]
        if siTodos and self.configuracion.liPersonalidades:
            liAjustes.append(("-" * 30, None))
            for num, una in enumerate(self.configuracion.liPersonalidades):
                liAjustes.append((una["NOMBRE"], 1000 + num))
        return liAjustes

    def label(self, nAjuste):
        for lb, n in self.listaAjustes(True):
            if n == nAjuste:
                return lb
        return ""

    def editar(self, una, icono):
        if una is None:
            una = {}

        # Datos basicos
        liGen = [(None, None)]
        liGen.append((FormLayout.Editbox(_("Name")), una.get("NOMBRE", "")))

        liGen.append((None, None))

        config = FormLayout.Fichero(_("Debug file"), "txt", True)
        liGen.append((config, una.get("DEBUG", "")))

        liGen.append((None, None))

        liGen.append((None, _("Serious errors, select the best move if:")))
        liGen.append(
                (FormLayout.Editbox(_("Mate is less than or equal to"), tipo=int, ancho=50), una.get("MAXMATE", 0)))
        liGen.append((
            FormLayout.Editbox(_("The loss of points is greater than"), tipo=int, ancho=50),
            una.get("MINDIFPUNTOS", 0)))
        liGen.append((None, None))
        liGen.append((
            FormLayout.Editbox(_("Max. loss of points per move by the <br> engine to reach a leveled evaluation"),
                               tipo=int,
                               ancho=50), una.get("ATERRIZAJE", 50)))

        # Apertura
        liA = [(None, None)]

        config = FormLayout.Fichero(_("Polyglot book"), "bin", False)
        liA.append((config, una.get("BOOK", "")))

        # Medio juego
        liMJ = [(None, None)]

        # # Ajustar
        liMJ.append((FormLayout.Combobox(_("Strength"), self.listaAjustes(False)), una.get("AJUSTAR", kAjustarMejor)))

        # Movimiento siguiente
        liMJ.append((None, _("In the next move")))

        trlistaSG = [_("To move a pawn"), _("Advance piece"), _("Make check"), _("Capture")]
        listaSG = ["MOVERPEON", "AVANZARPIEZA", "JAQUE", "CAPTURAR"]
        for n, opcion in enumerate(listaSG):
            liMJ.append((FormLayout.Spinbox(trlistaSG[n], -2000, +2000, 50), una.get(opcion, 0)))

        # Movimientos previstos
        liMJ.append((None, _("In the expected moves")))
        trlistaPR = [_("Keep the two bishops"), _("Advance"), _("Make check"), _("Capture")]
        listaPR = ["2B", "AVANZAR", "JAQUE", "CAPTURAR"]
        for n, opcion in enumerate(listaPR):
            liMJ.append((FormLayout.Spinbox(trlistaPR[n], -2000, +2000, 50), una.get(opcion + "PR", 0)))

        # Final
        liF = [(None, None)]

        # Ajustar
        liF.append(
                (FormLayout.Combobox(_("Strength"), self.listaAjustes(False)), una.get("AJUSTARFINAL", kAjustarMejor)))

        liF.append((FormLayout.Spinbox(_("Maximum pieces at this stage"), 0, 32, 50), una.get("MAXPIEZASFINAL", 0)))
        liF.append((None, None))

        # Movimiento siguiente
        liF.append((None, _("In the next move")))
        for n, opcion in enumerate(listaSG):
            liF.append((FormLayout.Spinbox(trlistaSG[n], -2000, +2000, 50), una.get(opcion + "F", 0)))

        # Movimientos previstos
        liF.append((None, _("In the expected moves")))
        for n, opcion in enumerate(listaPR):
            liF.append((FormLayout.Spinbox(trlistaPR[n], -2000, +2000, 50), una.get(opcion + "PRF", 0)))

        while True:
            lista = []
            lista.append((liGen, _("Basic data"), ""))
            lista.append((liA, _("Opening"), ""))
            lista.append((liMJ, _("Middlegame"), ""))
            lista.append((liF, _("Endgame"), ""))
            resultado = FormLayout.fedit(lista, title=_("Personalities"), parent=self.owner, anchoMinimo=460,
                                         icon=icono)
            if resultado:
                accion, liResp = resultado
                liGenR, liAR, liMJR, liFR = liResp

                nombre = liGenR[0].strip()

                if not nombre:
                    QTUtil2.mensError(self.owner, _("Missing name"))
                    continue

                una = {}
                # Base
                una["NOMBRE"] = nombre
                una["DEBUG"] = liGenR[1]
                una["MAXMATE"] = liGenR[2]
                una["MINDIFPUNTOS"] = liGenR[3]
                una["ATERRIZAJE"] = liGenR[4]

                # Apertura
                una["BOOK"] = liAR[0]

                # Medio
                una["AJUSTAR"] = liMJR[0]

                for num, opcion in enumerate(listaSG):
                    una[opcion] = liMJR[num + 1]

                nSG = len(listaSG) + 1
                for num, opcion in enumerate(listaPR):
                    una[opcion + "PR"] = liMJR[num + nSG]

                # Final
                una["AJUSTARFINAL"] = liFR[0]
                una["MAXPIEZASFINAL"] = liFR[1]

                for num, opcion in enumerate(listaSG):
                    una[opcion + "F"] = liFR[num + 2]

                nSG = len(listaSG) + 2
                for num, opcion in enumerate(listaPR):
                    una[opcion + "PRF"] = liFR[num + nSG]

                return una

            return None

    def lanzaMenu(self):
        menu = QTVarios.LCMenu(self.owner)
        f = Controles.TipoLetra(puntos=8, peso=75)
        menu.ponFuente(f)
        icoCrear = Iconos.Mas()
        icoEditar = Iconos.ModificarP()
        icoBorrar = Iconos.Delete()
        icoVerde = Iconos.PuntoVerde()
        icoRojo = Iconos.PuntoNaranja()

        menu.opcion(("c", None), _("New personality"), icoCrear)

        liPersonalidades = self.configuracion.liPersonalidades
        if liPersonalidades:
            menu.separador()
            menuMod = menu.submenu(_("Edit"), icoEditar)
            for num, una in enumerate(liPersonalidades):
                menuMod.opcion(("e", num), una["NOMBRE"], icoVerde)
            menu.separador()
            menuBor = menu.submenu(_("Delete"), icoBorrar)
            for num, una in enumerate(liPersonalidades):
                menuBor.opcion(("b", num), una["NOMBRE"], icoRojo)
        resp = menu.lanza()
        if resp:
            siRehacer = False
            accion, num = resp
            if accion == "c":
                una = self.editar(None, icoCrear)
                if una:
                    liPersonalidades.append(una)
                    siRehacer = True
            elif accion == "e":
                una = self.editar(liPersonalidades[num], icoEditar)
                if una:
                    liPersonalidades[num] = una
                    siRehacer = True
            elif accion == "b":
                if QTUtil2.pregunta(self.owner, _X(_("Delete %1?"), liPersonalidades[num]["NOMBRE"])):
                    del liPersonalidades[num]
                    siRehacer = True

            if siRehacer:
                self.configuracion.graba()
                return True
        return False

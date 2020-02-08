from Code import TrListas

from Code.QT import Colocacion
from Code.QT import Columnas
from Code.QT import Delegados
from Code.QT import Grid
from Code.QT import Iconos
from Code.QT import QTVarios
from Code.QT import FormLayout


class WEtiquetasPGN(QTVarios.WDialogo):
    def __init__(self, procesador, liPGN):
        titulo = _("Edit PGN labels")
        icono = Iconos.PGN()
        extparam = "editlabels"
        self.listandard = ("Event", "Site", "Date", "Round", "White", "Black", "Result", "ECO", "WhiteElo", "BlackElo")

        QTVarios.WDialogo.__init__(self, procesador.pantalla, titulo, icono, extparam)
        self.procesador = procesador
        self.liPGN = self.creaLista(liPGN)

        # Toolbar
        liAccionesWork = (
            (_("Accept"), Iconos.Aceptar(), self.aceptar),
            None,
            (_("Cancel"), Iconos.Cancelar(), self.cancelar),
            None,
            (_("Up"), Iconos.Arriba(), self.arriba),
            None,
            (_("Down"), Iconos.Abajo(), self.abajo),
            None,
        )
        tbWork = QTVarios.LCTB(self, liAccionesWork, tamIcon=24)

        # Lista
        oColumnas = Columnas.ListaColumnas()
        oColumnas.nueva("ETIQUETA", _("Label"), 150, edicion=Delegados.LineaTextoUTF8())
        oColumnas.nueva("VALOR", _("Value"), 400, edicion=Delegados.LineaTextoUTF8())

        self.grid = Grid.Grid(self, oColumnas, siEditable=True)
        n = self.grid.anchoColumnas()
        self.grid.setFixedWidth(n + 20)
        self.registrarGrid(self.grid)

        # Layout
        layout = Colocacion.V().control(tbWork).control(self.grid).margen(3)
        self.setLayout(layout)

        self.recuperarVideo()

    def creaLista(self, liPGN):
        st = {eti for eti, val in liPGN}

        li = [[k, v] for k, v in liPGN]
        for eti in self.listandard:
            if eti not in st:
                li.append([eti, ""])
        while len(li) < 30:
            li.append(["", ""])
        return li

    def aceptar(self):
        dic_rev = {}
        for eti in self.listandard:
            dic_rev[TrListas.pgnLabel(eti.upper())] = eti

        for n, (eti, val) in enumerate(self.liPGN):
            if eti in dic_rev:
                self.liPGN[n][0] = dic_rev[eti]

        li = []
        st = set()
        for n, (eti, val) in enumerate(self.liPGN):
            val = val.strip()
            if eti not in st and val:
                st.add(eti)
                li.append((eti, val))
        self.liPGN = li

        self.guardarVideo()
        self.accept()

    def closeEvent(self):
        self.guardarVideo()

    def cancelar(self):
        self.guardarVideo()
        self.reject()

    def gridNumDatos(self, grid):
        return len(self.liPGN)

    def gridPonValor(self, grid, fila, oColumna, valor):
        col = 0 if oColumna.clave == "ETIQUETA" else 1
        self.liPGN[fila][col] = valor

    def gridDato(self, grid, fila, oColumna):
        if oColumna.clave == "ETIQUETA":
            lb = self.liPGN[fila][0]
            ctra = lb.upper()
            trad = TrListas.pgnLabel(lb)
            if trad != ctra:
                clave = trad
            else:
                if lb:
                    clave = lb  # [0].upper()+lb[1:].lower()
                else:
                    clave = ""
            return clave
        return self.liPGN[fila][1]

    def arriba(self):
        recno = self.grid.recno()
        if recno:
            self.liPGN[recno], self.liPGN[recno - 1] = self.liPGN[recno - 1], self.liPGN[recno]
            self.grid.goto(recno - 1, 0)
            self.grid.refresh()

    def abajo(self):
        n0 = self.grid.recno()
        if n0 < len(self.liPGN) - 1:
            n1 = n0 + 1
            self.liPGN[n0], self.liPGN[n1] = self.liPGN[n1], self.liPGN[n0]
            self.grid.goto(n1, 0)
            self.grid.refresh()


def editarEtiquetasPGN(procesador, liPGN):
    w = WEtiquetasPGN(procesador, liPGN)
    if w.exec_():
        li = []
        for eti, valor in w.liPGN:
            if (len(eti.strip()) > 0) and (len(valor.strip()) > 0):
                li.append([eti, valor])
        return li
    else:
        return None


def massive_change_tags(owner, configuracion, num_selected, si_games):
    APPLY_ALL, APPLY_SELECTED = range(2)
    SAVE_NOTHING, SAVE_LABELS, SAVE_LABELS_VALUES = range(3)
    NUM_OTHERS = 4

    dic = configuracion.leeVariables("MASSIVE_CHANGE_TAGS")
    if not dic:
        dic = {}

    liBase = []
    sep = (None, None)

    def sepBase():
        liBase.append(sep)

    sepBase()

    liBase.append((None, _("Basic tags")))
    liBase.append(("%s:" % _("Event"), dic.get("EVENT", "")))
    liBase.append(("%s:" % _("Site"), dic.get("SITE", "")))
    liBase.append(("%s:" % _("Date"), dic.get("DATE", "")))

    sepBase()

    liBaseOther = [
        "Round",
        "White",
        "Black",
        "Result",
        "WhiteTitle",
        "BlackTitle",
        "WhiteElo",
        "BlackElo",
        "WhiteUSCF",
        "BlackUSCF",
        "WhiteNA",
        "BlackNA",
        "WhiteType",
        "BlackType",
        "EventDate",
        "EventSponsor",
        "Section",
        "Stage",
        "Board",
        "Opening",
        "Variation",
        "SubVariation",
        "ECO",
        "Time",
        "UTCTime",
        "UTCDate",
        "TimeControl",
        "SetUp",
        "Termination",
    ]
    li0 = ["", "Event", "Site", "Date"]
    li0.extend(liBaseOther)
    li = [[uno, uno] for uno in li0]
    combo = FormLayout.Combobox(_("Remove the tag (in English)"), li, si_editable=True)
    liBase.append((combo, ""))

    sepBase()
    if si_games:
        liBase.append((_("Set Opening, ECO, PlyCount") + ": ", False))
        sepBase()

    liBase.append((None, _("Configuration")))

    liA = [(_("All read"), APPLY_ALL), ("%s [%d]" % (_("Only selected"), num_selected), APPLY_SELECTED)]
    config = FormLayout.Combobox(_("Apply to"), liA)
    liBase.append((config, dic.get("APPLY", APPLY_SELECTED if num_selected > 1 else APPLY_ALL)))
    sepBase()

    liBase.append((_("Overwrite"), dic.get("OVERWRITE", True)))
    sepBase()

    liS = [
        (_("Nothing"), SAVE_NOTHING),
        (_("Labels"), SAVE_LABELS),
        ("%s+%s" % (_("Labels"), _("Values")), SAVE_LABELS_VALUES),
    ]
    config = FormLayout.Combobox(_("Save as default"), liS)
    liBase.append((config, dic.get("SAVE", SAVE_LABELS_VALUES)))

    liOther = [sep]

    for x in range(NUM_OTHERS):
        li = [[uno, uno] for uno in liBaseOther]
        previo = dic.get("OTHERS_TAG_%d" % x, "")
        li.insert(0, [previo, previo])
        if previo:
            li.insert(0, ["", ""])
        combo = FormLayout.Combobox("%s %d" % (_("Tag"), x + 1), li, si_editable=True)
        liOther.append((combo, previo))

        previo_value = dic.get("OTHERS_VALUE_%d" % x)
        rotulo = "%s %d" % (_("Value"), x + 1)
        if previo_value:
            li = [["", ""], [previo_value, previo_value]]
            combo = FormLayout.Combobox(rotulo, li, si_editable=True)
            liOther.append((combo, ""))
        else:
            liOther.append((rotulo, ""))
        liOther.append(sep)

    liOther.append((None, "** %s" % _("All official tags must be indicated with their name in English.")))

    lista = []
    lista.append((liBase, _("Base"), ""))
    lista.append((liOther, _("Others"), ""))

    # Editamos
    resultado = FormLayout.fedit(
        lista, title=_("Massive change of tags"), parent=owner, anchoMinimo=640, icon=Iconos.PGN()
    )
    if resultado:
        accion, resp = resultado
        liBase, liOther = resp

        dic = {}
        set_extend = False
        if si_games:
            dic["EVENT"], dic["SITE"], dic["DATE"], dic["REMOVE"], set_extend, dic["APPLY"], dic["OVERWRITE"], dic[
                "SAVE"
            ] = liBase
        else:
            dic["EVENT"], dic["SITE"], dic["DATE"], dic["REMOVE"], dic["APPLY"], dic["OVERWRITE"], dic["SAVE"] = liBase

        liTags = []
        for tag in ("Event", "Site", "Date"):
            tagu = tag.upper()
            if dic[tagu]:
                liTags.append((tag, dic[tagu]))

        for x in range(0, NUM_OTHERS * 2, 2):
            tag = liOther[x]
            if tag and tag.upper != "FEN":
                val = liOther[x + 1]
                ntag = x / 2
                dic["OTHERS_TAG_%d" % ntag] = tag
                dic["OTHERS_VALUE_%d" % ntag] = val
                if val:
                    liTags.append((tag, val))

        save = dic["SAVE"]

        if save == SAVE_NOTHING or save == SAVE_LABELS:
            for x in ("EVENT", "SITE", "DATE"):
                del dic[x]
            for x in range(NUM_OTHERS):
                key = "OTHERS_VALUE_%d" % x
                if key in dic:
                    del dic[key]
                if save == SAVE_NOTHING:
                    key = "OTHERS_TAG_%d" % x
                    if key in dic:
                        del dic[key]

        configuracion.escVariables("MASSIVE_CHANGE_TAGS", dic)

        if liTags or dic["REMOVE"] or set_extend:
            if si_games:
                return (liTags, dic["REMOVE"], dic["OVERWRITE"], dic["APPLY"] == APPLY_ALL, set_extend)
            else:
                return (liTags, dic["REMOVE"], dic["OVERWRITE"], dic["APPLY"] == APPLY_ALL)

    return None

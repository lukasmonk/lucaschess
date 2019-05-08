import codecs
import encodings
import os

import chardet.universaldetector
from PyQt4 import QtGui

from Code import PGN
from Code.QT import Colocacion
from Code.QT import Columnas
from Code.QT import Controles
from Code.QT import Delegados
from Code.QT import Grid
from Code.QT import Iconos
from Code.QT import QTUtil
from Code.QT import QTUtil2
from Code.QT import QTVarios
from Code import TrListas
from Code import Util
from Code import VarGen


class WBaseSave(QtGui.QWidget):
    def __init__(self, wowner, configuracion, with_remcomments=False):
        QtGui.QWidget.__init__(self, wowner)

        self.wowner = wowner
        self.file = ""
        self.configuracion = configuracion
        self.with_remcomments = with_remcomments
        self.vars_read()

        lb_file = Controles.LB(self, _("File to save") + ": ")
        bt_history = Controles.PB(self, "", self.history).ponIcono(Iconos.Favoritos(), 24).ponToolTip(_("Previous"))
        bt_boxrooms = Controles.PB(self, "", self.boxrooms).ponIcono(Iconos.Trasteros(), 24).ponToolTip(_("Boxrooms PGN"))
        self.bt_file = Controles.PB(self, "", self.file_select, plano=False).anchoMinimo(300)

        # Codec
        lb_codec = Controles.LB(self, _("Encoding") + ": ")
        liCodecs = [k for k in set(v for k, v in encodings.aliases.aliases.iteritems())]
        liCodecs.sort()
        liCodecs = [(k, k) for k in liCodecs]
        liCodecs.insert(0, (_("Same as file"), "file"))
        liCodecs.insert(0, ("%s: %s" % (_("By default"), _("UTF-8")), "default"))
        self.cb_codecs = Controles.CB(self, liCodecs, self.codec)

        # Rest
        self.chb_overwrite = Controles.CHB(self, _("Overwrite"), False)
        if with_remcomments:
            self.chb_remove_c_v = Controles.CHB(self, _("Remove comments and variations"), self.remove_c_v)

        lyF = Colocacion.H().control(lb_file).control(self.bt_file).control(bt_history).control(bt_boxrooms).relleno(1)
        lyC = Colocacion.H().control(lb_codec).control(self.cb_codecs).relleno(1)
        ly = Colocacion.V().espacio(15).otro(lyF).otro(lyC).control(self.chb_overwrite)
        if with_remcomments:
            ly.control(self.chb_remove_c_v)
        ly.relleno(1)

        self.chb_overwrite.hide()

        self.setLayout(ly)

    def file_select(self):
        last_dir = ""
        if self.file:
            last_dir = os.path.dirname(self.file)
        elif self.history_list:
            last_dir = os.path.dirname(self.history_list[0])
            if not os.path.isdir(last_dir):
                last_dir = ""

        fich = QTUtil2.leeCreaFichero(self, last_dir, "pgn")
        if fich:
            if not fich.lower().endswith(".pgn"):
                fich += ".pgn"
            self.file = fich
            self.show_file()

    def show_file(self):
        self.file = os.path.normpath(self.file)
        self.bt_file.ponTexto(self.file)
        if os.path.isfile(self.file):
            self.chb_overwrite.show()
        else:
            self.chb_overwrite.hide()
        self.wowner.check_toolbar()

    def vars_read(self):
        dicVariables = self.configuracion.leeVariables("SAVEPGN")
        self.history_list = dicVariables.get("LIHISTORICO", [])
        self.codec = dicVariables.get("CODEC", "default")

    def vars_save(self):
        if self.file:
            dicVariables = self.configuracion.leeVariables("SAVEPGN")
            if self.file in self.history_list:
                del self.history_list[self.history_list.index(self.file)]
            self.history_list.insert(0, self.file)
            dicVariables["LIHISTORICO"] = self.history_list
            dicVariables["CODEC"] = self.cb_codecs.valor()
            self.configuracion.escVariables("SAVEPGN", dicVariables)

    def history(self):
        menu = QTVarios.LCMenu(self, puntos=9)
        menu.setToolTip(_("To choose: <b>left button</b> <br>To erase: <b>right button</b>"))
        rp = QTVarios.rondoPuntos()
        for pos, txt in enumerate(self.history_list):
            menu.opcion(pos, txt, rp.otro())
        pos = menu.lanza()
        if pos is not None:
            if menu.siIzq:
                self.file = self.history_list[pos]
                self.show_file()
            elif menu.siDer:
                del self.history_list[pos]
        self.wowner.check_toolbar()

    def boxrooms(self):
        menu = QTVarios.LCMenu(self, puntos=9)
        menu.setToolTip(_("To choose: <b>left button</b> <br>To erase: <b>right button</b>"))

        icoTras = Iconos.Trastero()
        liTras = self.configuracion.liTrasteros
        for ntras, uno in enumerate(liTras):
            carpeta, trastero = uno
            menu.opcion((0, ntras), "%s  (%s)" % (trastero, carpeta), icoTras)
        menu.separador()
        menu.opcion((1, 0), _("New boxroom"), Iconos.Trastero_Nuevo())

        resp = menu.lanza()
        if resp is not None:

            op, ntras = resp
            if op == 0:
                if menu.siIzq:
                    carpeta, trastero = liTras[ntras]
                    self.file = os.path.join(carpeta, trastero)
                    self.show_file()
                elif menu.siDer:
                    del self.configuracion.liTrasteros[ntras]
                    self.configuracion.graba()

            elif op == 1:
                resp = QTUtil2.salvaFichero(self, _("Boxrooms PGN"), self.configuracion.dirSalvados +"/",
                                            _("File") + " pgn (*.pgn)", False)
                if resp:
                    resp = os.path.normpath(resp)
                    carpeta, trastero = os.path.split(resp)
                    if carpeta != self.configuracion.dirSalvados:
                        self.configuracion.dirSalvados = carpeta
                        self.configuracion.graba()

                    orden = None
                    for n, (carpeta1, trastero1) in enumerate(self.configuracion.liTrasteros):
                        if carpeta1.lower() == carpeta.lower() and trastero1.lower() == trastero.lower():
                            orden = len(self.configuracion.liTrasteros) - 1
                            break

                    if orden is None:
                        self.configuracion.liTrasteros.append((carpeta, trastero))
                        self.configuracion.graba()
        self.wowner.check_toolbar()

    def terminar(self):
        self.dic_result = {
            "FILE": self.file,
            "CODEC": self.cb_codecs.valor(),
            "OVERWRITE": self.chb_overwrite.valor(),
            "REMCOMMENTSVAR": self.chb_remove_c_v.valor() if self.with_remcomments else False
        }
        self.vars_save()


class WSave(QTVarios.WDialogo):
    def __init__(self, owner, pgn, configuracion):
        titulo = _("Save to PGN")
        icono = Iconos.PGN()
        extparam = "savepgn"
        QTVarios.WDialogo.__init__(self, owner, titulo, icono, extparam)

        self.pgn_read(pgn)
        self.configuracion = configuracion
        self.file = ""
        self.vars_read()

        # Opciones
        liOpciones = [
            (_("Save"), Iconos.GrabarFichero(), self.save), None,
            (_("Cancel"), Iconos.Cancelar(), self.terminar), None,
            (_("Clipboard"), Iconos.Clip(), self.portapapeles), None,
            (_("Reinit"), Iconos.Reiniciar(), self.reinit), None,
        ]
        tb = QTVarios.LCTB(self, liOpciones)

        tabs = Controles.Tab(self)

        # Tab-fichero -----------------------------------------------------------------------------------------------
        lb_file = Controles.LB(self, _("File to save") + ": ")
        bt_history = Controles.PB(self, "", self.history).ponIcono(Iconos.Favoritos(), 24).ponToolTip(_("Previous"))
        bt_boxrooms = Controles.PB(self, "", self.boxrooms).ponIcono(Iconos.Trasteros(), 24).ponToolTip(_("Boxrooms PGN"))
        self.bt_file = Controles.PB(self, "", self.file_select, plano=False).anchoMinimo(300)

        # Codec
        lb_codec = Controles.LB(self, _("Encoding") + ": ")
        liCodecs = [k for k in set(v for k, v in encodings.aliases.aliases.iteritems())]
        liCodecs.sort()
        liCodecs = [(k, k) for k in liCodecs]
        liCodecs.insert(0, (_("Same as file"), "file"))
        liCodecs.insert(0, ("%s: %s" % (_("By default"), _("UTF-8")), "default"))
        self.cb_codecs = Controles.CB(self, liCodecs, self.codec)

        # Rest
        self.chb_overwrite = Controles.CHB(self, _("Overwrite"), False)
        self.chb_remove_c_v = Controles.CHB(self, _("Remove comments and variations"), self.remove_c_v)

        lyF = Colocacion.H().control(lb_file).control(self.bt_file).control(bt_history).control(bt_boxrooms).relleno(1)
        lyC = Colocacion.H().control(lb_codec).control(self.cb_codecs).relleno(1)
        ly = Colocacion.V().espacio(15).otro(lyF).otro(lyC).control(self.chb_overwrite).control(self.chb_remove_c_v).relleno(1)
        w = QtGui.QWidget()
        w.setLayout(ly)
        tabs.nuevaTab(w, _("File"))
        self.chb_overwrite.hide()

        # Tab-labels -----------------------------------------------------------------------------------------------
        liAccionesWork = (
            ("", Iconos.Mas22(), self.labels_more), None,
            ("", Iconos.Menos22(), self.labels_less), None,
            ("", Iconos.Arriba(), self.labels_up), None,
            ("", Iconos.Abajo(), self.labels_down), None,
        )
        tb_labels = Controles.TBrutina(self, liAccionesWork, tamIcon=16, siTexto=False)

        # Lista
        oColumnas = Columnas.ListaColumnas()
        oColumnas.nueva("ETIQUETA", _("Label"), 150, edicion=Delegados.LineaTextoUTF8())
        oColumnas.nueva("VALOR", _("Value"), 400, edicion=Delegados.LineaTextoUTF8())

        self.grid_labels = Grid.Grid(self, oColumnas, siEditable=True)
        n = self.grid_labels.anchoColumnas()
        self.grid_labels.setFixedWidth(n + 20)

        # Layout
        ly = Colocacion.V().control(tb_labels).control(self.grid_labels).margen(3)
        w = QtGui.QWidget()
        w.setLayout(ly)
        tabs.nuevaTab(w, _("Labels"))

        # Tab-Body -----------------------------------------------------------------------------------------------
        self.em_body = Controles.EM(self, self.body, siHTML=False)
        tabs.nuevaTab(self.em_body, _("Body"))

        layout = Colocacion.V().control(tb).control(tabs)

        self.setLayout(layout)

        if self.history_list:
            fich = self.history_list[0]
            if os.path.isfile(fich):
                self.file = fich
                self.show_file()

        self.registrarGrid(self.grid_labels)
        self.recuperarVideo()

    def pgn_read(self, pgn):
        self.pgn_init = pgn
        is_label = True
        self.li_labels = []
        self.body = ""
        for linea in pgn.split("\n"):
            linea = linea.strip()
            if linea:
                if is_label:
                    if linea.startswith("["):
                        li = linea.split('"')
                        if len(li) == 3:
                            key = li[0][1:].strip()
                            value = li[1]
                            self.li_labels.append([key, value])
                        continue
                    else:
                        is_label = False
                self.body += linea + "\n"

    def file_select(self):
        last_dir = ""
        if self.file:
            last_dir = os.path.dirname(self.file)
        elif self.history_list:
            last_dir = os.path.dirname(self.history_list[0])
            if not os.path.isdir(last_dir):
                last_dir = ""

        fich = QTUtil2.leeCreaFichero(self, last_dir, "pgn")
        if fich:
            if not fich.lower().endswith(".pgn"):
                fich += ".pgn"
            self.file = fich
            self.show_file()

    def show_file(self):
        self.bt_file.ponTexto(self.file)
        if os.path.isfile(self.file):
            self.chb_overwrite.show()
        else:
            self.chb_overwrite.hide()

    def vars_read(self):
        dicVariables = self.configuracion.leeVariables("SAVEPGN")
        self.history_list = dicVariables.get("LIHISTORICO", [])
        self.codec = dicVariables.get("CODEC", "default")
        self.remove_c_v = dicVariables.get("REMCOMMENTSVAR", False)

    def vars_save(self):
        dicVariables = {}
        dicVariables["LIHISTORICO"] = self.history_list
        dicVariables["CODEC"] = self.cb_codecs.valor()
        dicVariables["REMCOMMENTSVAR"] = self.chb_remove_c_v.isChecked()
        self.configuracion.escVariables("SAVEPGN", dicVariables)

    def history(self):
        menu = QTVarios.LCMenu(self, puntos=9)
        menu.setToolTip(_("To choose: <b>left button</b> <br>To erase: <b>right button</b>"))
        rp = QTVarios.rondoPuntos()
        for pos, txt in enumerate(self.history_list):
            menu.opcion(pos, txt, rp.otro())
        pos = menu.lanza()
        if pos is not None:
            if menu.siIzq:
                self.file = self.history_list[pos]
                self.show_file()
            elif menu.siDer:
                del self.history_list[pos]

    def boxrooms(self):
        menu = QTVarios.LCMenu(self, puntos=9)
        menu.setToolTip(_("To choose: <b>left button</b> <br>To erase: <b>right button</b>"))

        icoTras = Iconos.Trastero()
        liTras = self.configuracion.liTrasteros
        for ntras, uno in enumerate(liTras):
            carpeta, trastero = uno
            menu.opcion((0, ntras), "%s  (%s)" % (trastero, carpeta), icoTras)
        menu.separador()
        menu.opcion((1, 0), _("New boxroom"), Iconos.Trastero_Nuevo())

        resp = menu.lanza()
        if resp is not None:

            op, ntras = resp
            if op == 0:
                if menu.siIzq:
                    carpeta, trastero = liTras[ntras]
                    self.file = os.path.join(carpeta, trastero)
                    self.show_file()
                elif menu.siDer:
                    del self.configuracion.liTrasteros[ntras]
                    self.configuracion.graba()

            elif op == 1:
                resp = QTUtil2.salvaFichero(self, _("Boxrooms PGN"), self.configuracion.dirSalvados,
                                            _("File") + " pgn (*.pgn)", False)
                if resp:
                    carpeta, trastero = os.path.split(resp)
                    if carpeta != self.configuracion.dirSalvados:
                        self.configuracion.dirSalvados = carpeta
                        self.configuracion.graba()

                    orden = None
                    for n, (carpeta1, trastero1) in enumerate(self.configuracion.liTrasteros):
                        if carpeta1.lower() == carpeta.lower() and trastero1.lower() == trastero.lower():
                            orden = len(self.configuracion.liTrasteros) - 1
                            break

                    if orden is None:
                        self.configuracion.liTrasteros.append((carpeta, trastero))
                        self.configuracion.graba()

    def current_pgn(self):
        pgn = ""
        for key, value in self.li_labels:
            key = key.strip()
            value = value.strip()
            if key and value:
                pgn += '[%s "%s"]\n' % (key, value)
        pgn += "\n%s\n" % self.em_body.texto().strip()
        if "\r\n" in pgn:
            pgn = pgn.replace("\r\n", "\n")
        if VarGen.isWindows:
            pgn = pgn.replace("\n", "\r\n")

        if self.chb_remove_c_v.isChecked():
            pgn = PGN.rawPGN(pgn)

        return pgn

    def save(self):
        pgn = self.current_pgn()
        modo = "w"
        if os.path.isfile(self.file):
            if not self.chb_overwrite.isChecked():
                modo = "a"
                pgn = ("\r\n\r\n") + pgn

        codec = self.cb_codecs.valor()

        if codec == "default":
            codec = "utf-8"
        elif codec == "file":
            codec = "utf-8"
            if Util.existeFichero(self.file):
                with open(self.file) as f:
                    u = chardet.universaldetector.UniversalDetector()
                    for n, x in enumerate(f):
                        u.feed(x)
                        if n == 1000:
                            break
                    u.close()
                    codec = u.result.get("encoding", "utf-8")

        try:
            f = codecs.open(self.file, modo, codec, 'ignore')
            if modo == "a":
                f.write("\r\n\r\n" if VarGen.isWindows else "\n\n")
            f.write(pgn)
            f.close()
            if self.file in self.history_list:
                self.history_list.remove(self.file)
            self.history_list.insert(0, self.file)
            QTUtil2.mensajeTemporal(self.parent(), _("Saved"), 0.6)
            self.terminar()
        except:
            QTUtil2.mensError(self, _("Unable to save"))

    def portapapeles(self):
        pgn = self.current_pgn()
        QTUtil.ponPortapapeles(pgn)
        self.terminar()

    def terminar(self):
        self.vars_save()
        self.guardarVideo()
        self.accept()

    def reinit(self):
        self.vars_read()
        self.pgn_read(self.pgn_init)
        self.grid_labels.refresh()
        self.em_body.ponTexto(self.body)

    def labels_more(self):
        self.li_labels.append(["", ""])
        self.grid_labels.goto(len(self.li_labels) - 1, 0)
        self.grid_labels.refresh()

    def labels_less(self):
        recno = self.grid_labels.recno()
        if recno > -1:
            del self.li_labels[recno]
            self.grid_labels.refresh()

    def labels_up(self):
        recno = self.grid_labels.recno()
        if recno:
            self.li_labels[recno], self.li_labels[recno - 1] = self.li_labels[recno - 1], self.li_labels[recno]
            self.grid_labels.goto(recno - 1, 0)
            self.grid_labels.refresh()

    def labels_down(self):
        n0 = self.grid_labels.recno()
        if n0 < len(self.li_labels) - 1:
            n1 = n0 + 1
            self.li_labels[n0], self.li_labels[n1] = self.li_labels[n1], self.li_labels[n0]
            self.grid_labels.goto(n1, 0)
            self.grid_labels.refresh()

    def gridNumDatos(self, grid):
        return len(self.li_labels)

    def gridPonValor(self, grid, fila, oColumna, valor):
        col = 0 if oColumna.clave == "ETIQUETA" else 1
        self.li_labels[fila][col] = valor

    def gridDato(self, grid, fila, oColumna):
        if oColumna.clave == "ETIQUETA":
            lb = self.li_labels[fila][0]
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
        return self.li_labels[fila][1]


class WSaveVarios(QTVarios.WDialogo):
    def __init__(self, owner, configuracion):
        titulo = _("Save to PGN")
        icono = Iconos.PGN()
        extparam = "savepgnvarios"
        QTVarios.WDialogo.__init__(self, owner, titulo, icono, extparam)

        self.configuracion = configuracion

        # Opciones
        liOpciones = [
            (_("Save"), Iconos.GrabarFichero(), self.aceptar), None,
            (_("Cancel"), Iconos.Cancelar(), self.reject), None,
        ]
        self.tb = QTVarios.LCTB(self, liOpciones)

        self.wbase = WBaseSave(self, configuracion)

        layout = Colocacion.V().control(self.tb).control(self.wbase)

        self.setLayout(layout)

        self.recuperarVideo()

        self.check_toolbar()

    def check_toolbar(self):
        self.tb.setPosVisible(0, len(self.wbase.file) > 0)

    def aceptar(self):
        if self.wbase.file:
            self.wbase.terminar()
            self.dic_result = self.wbase.dic_result
            self.guardarVideo()
            self.accept()
        else:
            self.reject()


class FileSavePGN:
    def __init__(self, owner, dic_vars):
        self.owner = owner
        self.file = dic_vars["FILE"]
        self.overwrite = dic_vars["OVERWRITE"]
        self.codec = dic_vars["CODEC"]
        if self.codec == "default" or self.codec is None:
            self.codec = "utf-8"
        elif self.codec == "file":
            self.codec = "utf-8"
            if Util.existeFichero(self.file):
                with open(self.file) as f:
                    u = chardet.universaldetector.UniversalDetector()
                    for n, x in enumerate(f):
                        u.feed(x)
                        if n == 1000:
                            break
                    u.close()
                    self.codec = u.result.get("encoding", "utf-8")
        self.xum = None

    def open(self):
        try:
            modo = "w" if self.overwrite else "a"
            self.f = codecs.open(self.file, modo, self.codec)
            self.is_new = self.overwrite or not os.path.isfile(self.file)
            return True
        except:
            QTUtil2.mensError(self.owner, "%s : %s\n" % (_("Unable to save"), self.file))
            return False

    def write(self, pgn):
        self.f.write(pgn)

    def close(self):
        self.f.close()

    def um(self):
        self.xum = QTUtil2.unMomento(self.owner, _("Saving..."))

    def um_final(self):
        if self.xum:
            self.xum.final()
        QTUtil2.mensaje(self.owner, _X(_("Saved to %1"), self.file))

    def pb(self, total):
        self._pb = QTUtil2.BarraProgreso(self.owner, self.file, "", total)
        self._pb_total = total
        self.pb_pos(0)

    def pb_pos(self, pos):
        self._pb.pon(pos)
        self._pb.mensaje("%d/%d" % (pos, self._pb_total))

    def pb_cancel(self):
        return self._pb.siCancelado()

    def pb_close(self):
        self._pb.cerrar()


import os.path
import shutil
import zipfile
import urllib

import Code.Util as Util
import Code.Voice as Voice
import Code.Sonido as Sonido
import Code.ControlPosicion as ControlPosicion
import Code.Partida as Partida
import Code.QT.QTUtil as QTUtil
import Code.QT.QTUtil2 as QTUtil2
import Code.QT.Colocacion as Colocacion
import Code.QT.Iconos as Iconos
import Code.QT.Controles as Controles
import Code.QT.QTVarios as QTVarios
import Code.QT.Columnas as Columnas
import Code.QT.Grid as Grid

class WVoiceTest(QTVarios.WDialogo):
    def __init__(self, procesador, titulo, tipo):
        QTVarios.WDialogo.__init__(self, procesador.pantalla, titulo, Iconos.S_Microfono(), "voicetest%s"%tipo)
        self.procesador = procesador
        conf = self.configuracion = procesador.configuracion
        q = QTUtil.qtColor
        self.dicColor = { "G":q(conf.color_nag1), "B":q(conf.color_nag2), "VG":q(conf.color_nag3), "VB":q(conf.color_nag4), "0":q("black") }

        self.voice = self.configuracion.voice
        self.configVoice = Voice.readConfigExt(self.voice)
        self.dicSamples, self.sTime = self.getDicSamplesTime()
        self.keyDB = self.voice+tipo
        self.liRegs = self.leeRegistros()

        self.dKeyLiWords = {}
        for w,k in self.configVoice.dic.iteritems():
            if k not in self.dKeyLiWords:
                self.dKeyLiWords[k] = [w]
            else:
                self.dKeyLiWords[k].append(w)

        # Tool bar
        liAcciones = [
            ( _("Quit"), Iconos.MainMenu(), self.terminar ), None,
            ( _("Train"), Iconos.Empezar(), self.test ), None,
        ]
        if tipo != "WORDS":
            liAcciones.append( (_("Words"), Iconos.Words(), self.words ) )
            liAcciones.append( None )
        if tipo != "POSITIONS":
            liAcciones.append( (_("Positions"), Iconos.Voyager(), self.positions ) )
            liAcciones.append( None )
        if tipo != "PGNS":
            liAcciones.append( (_("Games"), Iconos.InformacionPGN(), self.pgns ) )
            liAcciones.append( None )
        if len(self.dicSamples) > 3:
            liAcciones.append( ( _("Recompile"), Iconos.AdaptVoice(), self.recompile ) )
            liAcciones.append( None )

        self.tb = Controles.TBrutina(self, liAcciones)

        self.lbRecorded = Controles.LB(self)
        self.actualizaTime()

    def init2(self, oColumnas):
        self.gridDatos = Grid.Grid(self, oColumnas, siSelecFilas=True, siSeleccionMultiple=True)
        self.gridDatos.setMinimumWidth(self.gridDatos.anchoColumnas() + 20)

        # Colocamos
        lytr = Colocacion.H().relleno(1).control(self.lbRecorded).relleno(1)
        ly = Colocacion.V().control(self.tb).otro(lytr).control(self.gridDatos).margen(3)

        self.setLayout(ly)

        self.registrarGrid(self.gridDatos)
        self.recuperarVideo()
        self.gridDatos.gotop()

    def leeRegistros(self):
        return None

    def getDB(self):
        db = Util.DicSQL(self.configuracion.ficheroVoice)
        v = db[self.keyDB]
        db.close()
        return v if v else {}

    def setDB(self, value):
        db = Util.DicSQL(self.configuracion.ficheroVoice)
        db[self.keyDB] = value
        db.close()

    def terminar(self):
        self.cerrar()
        self.reject()

    def test(self):
        pass

    def getDicSamplesTime(self):
        lista = os.listdir(self.configuracion.folderVoiceWavs)
        d = {}
        for fich in lista:
            n_ = fich.find("_")
            if n_ > 0:
                key = fich[:n_]
                tipo = fich[-3:]
                if key not in d:
                    d[key] = { "ini":set(), "wav":set() }
                if tipo in d[key]:
                    d[key][tipo].add(fich[:-4])

        ds = {}
        stime = 0.0
        for k in d:
            st = d[k]["ini"] & d[k]["wav"]
            for x in st:
                n = Util.tamFichero(os.path.join(self.configuracion.folderVoiceWavs,x+".wav"))
                stime += (n-44)*1.0/32000.0

            ds[k] = len(st)
        return ds, stime

    def gridNumDatos(self, grid):
        return len(self.liRegs)

    def gridDato(self, grid, fila, oColumna):
        col = oColumna.clave
        dReg = self.liRegs[fila]
        if col == "CORRECT":
            return "%0.02f%%"%dReg[col]
        return dReg[col]

    def gridDobleClick(self, grid, fila, columna):
        self.test()

    def gridBold(self, grid, fila, columna):
        dReg = self.liRegs[fila]
        return dReg["CORRECT"] > 0.0

    def gridColorTexto(self, grid, fila, columna):
        dReg = self.liRegs[fila]
        t = dReg["CORRECT"]
        if t > 0.0:
            if t > 90:
                r = "VG"
            elif t > 75:
                r = "G"
            elif t > 40:
                r = "B"
            else:
                r = "VB"
        else:
            r = "0"
        return self.dicColor[r]

    def closeEvent(self, event):  # Cierre con X
        self.cerrar()

    def cerrar(self):
        self.guardarVideo()

    def grabaRegistros(self):
        datos = {}
        for reg in self.liRegs:
            key = reg["KEY"]
            datos[key] = reg["CORRECT"]
        self.setDB(datos)

    def actualizaRatio(self, key, ratio):
        for reg in self.liRegs:
            if reg["KEY"] == key:
                reg["CORRECT"] = ratio
                self.grabaRegistros()
                self.gridDatos.refresh()
                return

    def actualizaSamples(self, key):
        self.dicSamples, self.sTime = self.getDicSamplesTime()
        self.actualizaTime()
        self.liRegs = self.leeRegistros()
        self.gridDatos.refresh()

    def actualizaTime(self):
        sTime = int(round(self.sTime,0))
        sMin = sTime/60
        sSecs = sTime%60
        sHour = sMin/60
        sMin = sMin%60
        self.lbRecorded.ponTexto( "%s: %02d:%02d:%02d"%(_("Time recorded"),sHour,sMin, sSecs))

    def recompile(self):
        step = _("Step %d/%d done")
        um = QTUtil2.unMomento(self)
        rec = Voice.Recompile()
        rec.sphinx_fe()
        um.rotulo( step%(1,4) )
        rec.bw()
        um.rotulo( step%(2,4) )
        rec.mllr_solve()
        um.rotulo( step%(3,4) )
        if self.sTime > 600:
            rec.map_adapt()
            um.rotulo( step%(4,4) )
        rec.limpia()
        um.final()

        mens = "%s\n"%self.lbRecorded.texto()
        mens += _("It has been taken into account")

        QTUtil2.mensaje(self, mens, titulo=_("Done"))

    def words(self):
        self.terminar()
        self.procesador.voice("word")

    def positions(self):
        self.terminar()
        self.procesador.voice("position")

    def pgns(self):
        self.terminar()
        self.procesador.voice("pgn")

class WVoiceWordsTest(WVoiceTest):
    def __init__(self, procesador):
        WVoiceTest.__init__(self, procesador, _("Words test"), "WORDS")

        oColumnas = Columnas.ListaColumnas()
        oColumnas.nueva("KEY", _("Key"), 100, siCentrado=True)
        oColumnas.nueva("WORD", _("Word"), 120, siCentrado=True)
        oColumnas.nueva("PHONEMES", _("Phonemes"), 120, siCentrado=True)
        oColumnas.nueva("SAMPLES", _("Samples"), 80, siCentrado=True)
        oColumnas.nueva("CORRECT", "%%%s"%_("Correct"), 80, siCentrado=True)

        self.init2(oColumnas)

    def leeRegistros(self):
        datos = self.getDB()
        liWords = self.configVoice.dic.keys()
        dPhonemes = self.configVoice.dicPhonemes
        li = []
        for w in liWords:
            d = {}
            key = d["KEY"] = self.configVoice.dic[w]
            d["WORD"] = w
            d["PHONEMES"] = dPhonemes[w]
            d["SAMPLES"] = self.dicSamples.get(key,0)
            d["CORRECT"] = datos.get(key,0.0)
            li.append(d)
        return li

    def test(self):
        fila = self.gridDatos.recno()
        if fila < 0:
            fila = 0

        dReg = self.liRegs[fila]

        target = [dReg["WORD"]]*5
        key = dReg["KEY"]

        w = WTestFrase(self, target, key)
        w.exec_()

class WVoicePositionsTest(WVoiceTest):
    def __init__(self, procesador):
        WVoiceTest.__init__(self, procesador, _("Positions test"), "POSITIONS")

        oColumnas = Columnas.ListaColumnas()
        oColumnas.nueva("KEY", _("Key"), 60, siCentrado=True)
        oColumnas.nueva("PIECES", _("Pieces"), 80, siCentrado=True)
        oColumnas.nueva("SAMPLES", _("Samples"), 80, siCentrado=True)
        oColumnas.nueva("CORRECT", "%%%s"%_("Correct"), 80, siCentrado=True)

        self.init2(oColumnas)

    def leeRegistros(self):
        datos = self.getDB()
        liPositions = self.getPositions()
        li = []
        for cp in liPositions:
            d = {}
            key = d["KEY"] = cp.voice_key
            d["PIECES"] = cp.voice_pieces
            d["SAMPLES"] = self.dicSamples.get(key,0)
            d["CORRECT"] = datos.get(key,0.0)
            d["__CP__"] = cp
            li.append(d)
        return li

    def getTarget(self, cp):
        li = []
        for col in "abcdefgh":
            for fil in "12345678":
                pieza = cp.casillas[col+fil]
                if pieza:
                    li.append( (pieza,col+fil,pieza.isupper()))

        li.sort( key=lambda x: 0 if x[2] else 1)

        target = []
        ultPieza = ""
        for pieza, pos, siW in li:
            if pieza != ultPieza:
                if not ultPieza or siW != ultPieza.isupper():
                    target.append(self.dKeyLiWords["WHITE" if siW else "BLACK"][0])
                target.append(self.dKeyLiWords[pieza.upper()][0])
            ultPieza = pieza
            target.append(self.dKeyLiWords[pos[0]][0])
            target.append(self.dKeyLiWords[pos[1]][0])
        return target

    def getPositions(self):
        li =   (
                ("8/4P3/1P1k4/6Q1/4K3/8/8/8", "PS001"),
                ("8/5P2/4k1B1/1Q6/1K6/8/8/8", "PS002"),
                ("8/1Q3P2/4k1B1/8/1P6/8/4K3/8", "PS003"),
                ("3Q4/3B4/2PkP1n1/8/3K4/8/8/8", "PS004"),
                ("5QKB/2P5/8/3k4/8/1R4N1/8/8", "PS005"),
                ("6QN/5B2/2r4k/8/2b4K/8/8/8", "PS006"),
                ("4N3/1R2P3/2nPkP2/4N3/4K3/8/8/8", "PS007"),
                ("Rq6/3krP2/2b5/1N4K1/8/Q7/8/8", "PS008"),
                ("4N3/8/4p3/4k3/1p5p/4K3/3P1P2/4Q3", "PS009"),
                ("3KN3/2B2R2/6n1/8/4k2n/4p3/4B3/3R4", "PS010"),
                ("8/3b4/Rp1B1b2/3p4/2k1p3/2p5/2K5/Q7", "PS011"),
                ("3B4/7R/7b/3QNbpk/8/7p/7P/2K5", "PS012"),
                ("2R4B/8/1KPp4/5P2/b1k3N1/8/6n1/1Q6", "PS013"),
                ("8/6P1/2p5/1Pqk4/6P1/2P1RKP1/4P1P1/8", "PS014"),
                ("6r1/4p3/4N1p1/7k/5B2/4PpK1/2Qp1N2/8", "PS015"),
                ("8/8/1PPkPB2/3P2pn/n2K3p/2Nb4/5Q2/8", "PS016"),
                ("8/8/b5BQ/1p1p4/4pp2/2N5/1BkP4/1RN1K3", "PS017"),
                ("2B5/1K5b/QR6/2p1p3/2P1k2P/pR3N1N/r7/2b5", "PS018"),
                ("3n4/3B4/3p4/pP1k1P2/Pb3K2/1NpP4/2P5/3N1n2", "PS019"),
                ("8/1Rp5/K3P3/2B2Q2/n1kP4/P3r3/P3PN2/1N2bB2", "PS020"),
                ("q1b5/p1pP2r1/2p2p2/2K1k3/p1Q5/2n1N3/5r2/1B3R2", "PS021"),
                ("BB6/8/1Q3P2/R2NN2b/4k2K/R2r1p2/2P2p2/3q1n2", "PS022"),
                ("3N3b/p1pR4/2B3p1/2k2qQr/KR6/p2p1p2/7B/4r3", "PS023"),
                ("2R5/B1pp1K2/2Pk4/3np3/1b2B3/p1NP2p1/P5P1/4Q3", "PS024"),
                ("7B/3p4/K2P2P1/Rn1k1N1R/b7/1p1Np1p1/1P2QPP1/8", "PS025"),
                ("b6B/2pQ3q/3b4/3r2p1/R1P1N2p/3k4/1P1pNP2/3K3B", "PS026"),
                ("2b3q1/pN6/p1k1pN2/P3Bp2/KQ1Rr3/2P5/2R3r1/7B", "PS027"),
                ("R6n/2ppB1p1/2rb2k1/3B2P1/1p4P1/1P1PK2N/2Q4R/8", "PS028"),
                ("5B2/3p1n2/R2p4/1P1NRBQ1/1KPkrb2/1p6/2Pp1Pn1/4r3", "PS029"),
                ("rqr5/1B1N3p/2n1p2K/b2P4/RB1Nkn1R/1P6/2P2PP1/1Q6", "PS030"),
        )
        li1 = []
        for fen, key in li:
            fen = "%s w - - 0 1"%fen
            cp = ControlPosicion.ControlPosicion().leeFen(fen)
            cp.voice_key = key
            cp.voice_pieces = cp.totalPiezas()
            cp.voice_fen = fen
            li1.append(cp)
        return li1

    def test(self):
        fila = self.gridDatos.recno()
        if fila < 0:
            fila = 0
        cp = self.liRegs[fila]["__CP__"]
        target = self.getTarget(cp)
        key = cp.voice_key
        w = WTestFrase(self, target, key)
        w.exec_()

class WVoicePGNsTest(WVoiceTest):
    def __init__(self, procesador):
        WVoiceTest.__init__(self, procesador, _("PGNs test"), "PGNS")

        oColumnas = Columnas.ListaColumnas()
        oColumnas.nueva("KEY", _("Key"), 60, siCentrado=True)
        oColumnas.nueva("MOVES", _("Moves"), 80, siCentrado=True)
        oColumnas.nueva("SAMPLES", _("Samples"), 80, siCentrado=True)
        oColumnas.nueva("CORRECT", "%%%s"%_("Correct"), 80, siCentrado=True)

        self.init2(oColumnas)

    def leeRegistros(self):
        datos = self.getDB()
        liPGNs = self.getPGNs()
        li = []
        for partida in liPGNs:
            d = {}
            key = d["KEY"] = partida.voice_key
            d["MOVES"] = partida.voice_moves
            d["SAMPLES"] = self.dicSamples.get(key,0)
            d["CORRECT"] = datos.get(key,0.0)
            d["__P__"] = partida
            li.append(d)
        return li

    def getTarget(self, p):
        target = []
        def x(txt):
            target.append( self.dKeyLiWords[txt][0] )
        for jg in p.liJugadas:
            if target:
                target.append( " " )
            pgn = jg.pgnBase
            raiz = pgn[0]
            if raiz == "O":
                x( "CASTLE" if pgn == "O-O" else "LONGCASTLE" )
            else:
                liFinal = []
                if "e.p." in pgn:
                    pgn = pgn.replace("e.p.","")
                    liFinal.append("ENPASSANT")
                if "#" in pgn:
                    pgn = pgn.replace("#","")
                    liFinal.append("CHECKMATE")
                if "+" in pgn:
                    pgn = pgn.replace("+","")
                    liFinal.append("CHECK")
                for c in pgn:
                    if c == "=":
                        x("PROMOTEDTO")
                    elif c == "x":
                        x("CAPTURE")
                    else:
                        x(c)
                for uno in liFinal:
                    x(uno)
        return target

    def getPGNs(self):
        li = (
                ( "01", "d2d4 g8f6 b1c3 d7d5 c1g5 c7c6 e2e3 b8d7 f1d3 g7g6 f2f4 f8g7 g1f3 d7f8 e1g1 c8f5 f3e5 f6e4 c3e4 d5e4 d3c4 f5e6 f4f5 e6c4 e5c4 f7f6 g5h4 d8d5 c4d2 e8c8 d1g4 e7e6 f5e6 f6f5 e6e7 f5g4 e7d8Q d5d8 h4d8 c8d8 f1f7"),
                ( "02", "e2e4 e7e5 g1f3 b8c6 f1b5 f8c5 c2c3 g8f6 d2d4 e5d4 e4e5 f6e4 d1e2 d7d5 e5d6 e8g8 d6c7 d8e7 e1g1 c8g4 b5c6 b7c6 c3d4 c5d6 h2h3 g4h5 f1e1 a8e8 c7c8Q e8c8 e2e4 e7f6 b1c3 h5f3 e4f3 c8e8 c1e3 f6e6 d4d5 e6e5 g2g3"),
                ( "03", "d2d4 d7d5 g1f3 g8f6 c1g5 e7e6 b1d2 f8e7 g5f6 e7f6 e2e4 d5e4 d2e4 e8g8 c2c3 b7b6 f1d3 c8b7 d1e2 b8d7 e1c1 a7a5 h2h4 b6b5 f3g5 h7h6 e2h5 b7d5 h1h3 d8e7 h3g3 a5a4 g5h7 a4a3 e4f6 d7f6 h5h6 a3b2 c1d2 b2b1Q d3b1"),
                ( "04", "e2e4 c7c5 g1f3 d7d6 d2d4 c5d4 f3d4 g8f6 b1c3 a7a6 c1e3 e7e6 f1e2 f8e7 e1g1 d8c7 f2f4 c8d7 a2a4 b8c6 d4b3 c6a5 e4e5 d6e5 f4e5 a5b3 e5f6 e7c5 e3c5 c7c5 g1h1 b3a1 c3e4 c5d5 f6g7 e8c8 g7h8Q d8h8 e4f6 d5d1 f1d1"),
                ( "05", "g2g3 d7d5 g1f3 c7c5 f1g2 b8c6 e1g1 g8f6 d2d4 c8f5 c2c4 e7e6 c4d5 e6d5 b1c3 a7a6 c1g5 f8e7 d4c5 d5d4 c3a4 h7h6 g5f4 f6h5 f4e5 c6e5 f3e5 d8c7 e2e4 d4e3 e5f7 h5f4 f7d6 e7d6 c5d6 e3e2 d6c7 e2d1Q f1d1 f4g2 a4b6"),
                ( "06", "g1f3 c7c5 c2c4 b8c6 b1c3 e7e5 d2d3 g7g6 g2g3 f8g7 f1g2 g8e7 h2h4 d7d6 h4h5 a7a6 c3e4 d6d5 h5h6 d5e4 h6g7 h8g8 f3g5 g8g7 g5h7 e7f5 c1g5 d8a5 e1f1 g7h7 h1h7 e4e3 h7h8 e8d7 g2h3 e3f2 e2e4 a5e1 d1e1 f2e1Q a1e1"),
                ( "07", "f2f4 g8f6 g1f3 g7g6 b2b3 f8g7 c1b2 e8g8 e2e3 c7c5 f1e2 b8c6 e1g1 d7d5 d1e1 d5d4 b1a3 f6d5 a1d1 d4d3 b2g7 d3e2 g7f8 e2d1Q e1d1 d8f8 c2c4 d5f6 d2d4 c8f5 d4d5 c6b4 h2h3 a8d8 g2g4 f5d3 f1f2 e7e6 a3b5 f6d5 a2a3"),
                ( "08", "d2d4 f7f5 g2g3 g8f6 f1g2 g7g6 g1h3 f8g7 b1d2 c7c6 c2c4 d7d6 d4d5 e8g8 h3f4 c8d7 h2h4 g7h8 e2e4 f5e4 d2e4 f6e4 g2e4 b8a6 h4h5 g6g5 f4g6 h7g6 h5g6 g5g4 h1h8 g8h8 d1d2 h8g7 d2h6 g7f6 g6g7 f6e5 g7f8Q d8f8 h6e3"),
                ( "09", "d2d4 g8f6 c2c4 e7e6 g1f3 b7b6 e2e3 c8b7 f1d3 f8b4 c1d2 b4d2 b1d2 d7d5 a1c1 b8d7 e1g1 e8g8 b2b4 d8e7 c4c5 c7c6 f1e1 a7a5 a2a3 a5b4 a3b4 b7a6 b4b5 a6b5 d3b5 c6b5 c5c6 d7b8 d1e2 a8a5 c6c7 e7d7 c7b8Q f8b8 f3e5"),
                ( "10", "e2e4 c7c6 d2d4 g7g6 b1c3 f8g7 f1c4 d7d6 g1f3 g8f6 c1g5 b7b5 c4d3 h7h6 g5h4 e8g8 e1g1 d8c7 d1d2 g8h7 f1e1 b5b4 c3e2 c6c5 c2c3 b4c3 e2c3 c8a6 e4e5 c5d4 e5f6 d4c3 f6g7 c3d2 g7f8Q d2e1Q a1e1 a6d3 f8f7 h7h8 h4f6"),
                ( "11", "e2e4 e7e5 g1f3 d7d6 d2d4 e5d4 f3d4 g8f6 b1c3 f8e7 g2g3 e8g8 f1g2 b8c6 d4c6 b7c6 e1g1 a8b8 b2b3 f6d7 c1b2 e7f6 f2f4 f8e8 e4e5 d6e5 c3e4 f6e7 f4e5 c8a6 e5e6 a6f1 e6d7 f1g2 d7e8Q d8e8 g1g2 e7a3 e4f6 g7f6 d1g4"),
                ( "12", "e2e4 c7c5 c2c3 g7g6 d2d4 c5d4 c3d4 d7d5 e4d5 g8f6 f1b5 b8d7 b1c3 a7a6 b5a4 b7b5 a4b3 d7b6 g1f3 f8g7 f3e5 e8g8 e1g1 c8b7 e5c6 d8d6 d1f3 e7e6 c1f4 d6d7 c6e5 d7e8 d5e6 b7f3 e6f7 g8h8 f7e8Q f6e8 e5f7 h8g8 g2f3"),
                ( "13", "e2e4 e7e5 g1f3 g8f6 f3e5 d7d6 e5f3 f6e4 b1c3 e4f6 d2d4 f8e7 f1d3 e8g8 c3e2 c7c5 h2h3 b8c6 c2c3 f8e8 e1g1 d6d5 c1g5 c5c4 d3c2 f6e4 c2e4 d5e4 g5e7 e4f3 e7d8 f3e2 d1a4 e2f1Q a1f1 e8d8 a4c4 c8e6 c4b5 d8d7 f1e1"),
                ( "14", "e2e4 e7e5 g1f3 b8c6 f1b5 a7a6 b5a4 g8f6 e1g1 f8e7 f1e1 b7b5 a4b3 d7d6 c2c3 e8g8 d2d4 c8g4 c1e3 e5d4 c3d4 c6a5 b3c2 c7c5 d4c5 d6c5 b1c3 a5c6 e4e5 d8d1 a1d1 g4f3 e5f6 f3d1 f6e7 d1c2 e7f8Q a8f8 e3c5 f8c8 c3d5"),
                ( "15", "d2d4 g8f6 c2c4 e7e6 b1c3 d7d5 g1f3 c7c6 e2e3 f8d6 f1d3 b8d7 e1g1 d5c4 d3c4 b7b5 c4d3 e8g8 e3e4 e6e5 c1g5 d8c7 d4d5 b5b4 d5c6 b4c3 c6d7 c3b2 d7c8Q f8c8 a1b1 f6d7 b1b2 f7f6 g5e3 a8b8 b2c2 c7b7 d3c4 g8f8 d1d6"),
                # ( "16", "d2d4 g8f6 g1f3 e7e6 c2c4 b7b6 e2e3 c8b7 f1d3 f8b4 b1d2 f6e4 e1g1 b4d2 f3d2 e4d2 c1d2 e8g8 e3e4 d7d6 f2f4 b8d7 d1g4 d7f6 g4e2 d6d5 c4d5 e6d5 e4e5 f6e4 f4f5 f7f6 a1c1 a8c8 e5e6 e4d2 e6e7 d8d7 e7f8Q c8f8 e2d2"),
                # ( "17", "d2d4 g8f6 c2c4 g7g6 g1f3 f8g7 g2g3 e8g8 f1g2 d7d6 b1c3 b8c6 e1g1 a7a6 d4d5 c6a5 f3d2 c7c5 d1c2 a8b8 b2b3 b7b5 c1b2 e7e5 a1e1 h7h5 c4b5 a6b5 f2f4 h5h4 f4e5 f6g4 e5e6 g4e3 c2e4 e3f1 e6e7 f1d2 e7d8Q f8d8 e4h4"),
                # ( "18", "e2e4 g8f6 e4e5 f6d5 d2d4 d5b6 c2c4 d7d6 e5d6 c7d6 b1c3 g7g6 h2h3 f8g7 g1f3 e8g8 f1e2 b8c6 e1g1 c8f5 a2a3 a8c8 b2b3 e7e5 d4d5 e5e4 d5c6 e4f3 c6b7 f3e2 b7c8Q e2d1Q c8d8 d1f1 g1f1 f8d8 c1d2 f5c2 a1c1 c2b3"),
                # ( "19", "d2d4 g8f6 c1f4 e7e6 e2e3 c7c5 c2c3 d7d5 f1d3 b8c6 g1f3 f8e7 b1d2 e8g8 d1e2 c8d7 g2g4 c5c4 d3c2 b7b5 f3e5 b5b4 g4g5 b4c3 g5f6 c3b2 c2h7 g8h7 e2h5 h7g8 f6g7 b2a1Q e1e2 g8g7 h5h6 g7g8 h1a1 c6e5 f4e5 f7f6 a1g1"),
                # ( "20", "d2d4 d7d5 c2c4 d5c4 g1f3 g8f6 e2e3 e7e6 f1c4 c7c5 e1g1 a7a6 c4b3 b8c6 b1c3 b7b5 d1e2 c8b7 f1d1 d8c7 d4d5 e6d5 e3e4 d5d4 c3d5 c7d8 c1f4 a8c8 a2a4 c5c4 a4b5 d4d3 b5c6 d3e2 d5f6 d8f6 c6b7 e2d1Q a1d1 f6c6 b3a4"),
                # ( "21", "e2e4 c7c5 g1f3 d7d6 d2d4 c5d4 f3d4 g8f6 b1c3 e7e6 c1g5 f8e7 f2f4 b8c6 f1c4 e8g8 c4b3 a7a6 d1d3 c6b4 d3f3 h7h6 g5h4 e6e5 f4e5 c8g4 e5f6 g4f3 f6e7 d8b6 e7f8Q a8f8 d4f3 b6e3 e1f1 g8h8 h4f2 e3f4 f2g3 f4e3 g3d6"),
                # ( "22", "d2d3 d7d5 g1f3 g8f6 b1d2 e7e6 g2g3 c7c5 f1g2 b8c6 e1g1 f8e7 c2c3 e8g8 a2a3 b7b5 b2b4 c8b7 a3a4 c5b4 a4b5 b4c3 b5c6 c3d2 c6b7 d2c1Q b7a8Q c1d1 a8d8 d1f1 g2f1 f8d8 a1a7 e7c5 a7c7 c5d6 c7a7 d6c5 a7c7 c5d6 c7a7"),
                # ( "23", "d2d4 d7d5 c2c4 b8c6 b1c3 d5c4 g1f3 g8f6 e2e3 e7e5 f1c4 e5d4 e3d4 c8g4 e1g1 f8e7 d4d5 c6e5 c4b5 e5d7 h2h3 g4h5 g2g4 h5g6 g4g5 f6e4 c3e4 g6e4 f3e5 c7c6 d5c6 d7e5 c6c7 e8f8 c7d8Q a8d8 d1e2 e5f3 g1g2 d8d4 e2f3"),
                # ( "24", "c2c4 e7e6 b1c3 g8f6 d2d4 c7c5 g1f3 a7a6 g2g3 d7d5 c4d5 f6d5 c3d5 d8d5 c1e3 c5d4 d1d4 d5d4 e3d4 b8c6 a2a3 c6d4 f3d4 e6e5 d4b3 c8e6 b3d2 e6d5 e2e4 d5c6 f2f3 e8c8 d2c4 f7f6 c4a5 c6a4 b2b3 a4d7 f1c4 c8b8 c4d5"),
                # ( "25", "f2f4 g8f6 g1f3 e7e6 b2b3 f8e7 d2d3 b8c6 e2e4 d7d6 c1b2 e8g8 b1d2 f6g4 d1e2 f8e8 e1c1 c6b4 a2a3 b4a6 h2h3 g4f6 g2g4 h7h6 e2g2 f6h7 h3h4 e7f6 e4e5 d6e5 f4e5 f6e7 d3d4 b7b6 f1d3 c8b7 g4g5 h6h5 g5g6 h7f8 g6f7"),
                # ( "26", "b1c3 g8f6 e2e4 d7d6 d2d4 g7g6 f1c4 f8g7 d1e2 b8c6 e4e5 d6e5 d4e5 f6g4 e5e6 c8e6 c4e6 f7e6 e2g4 c6d4 g4e4 e8g8 c1e3 c7c5 e1c1 d8b6 d1d2 a8d8 g1f3 d4f3 g2f3 d8d2 e3d2 g7d4 h2h4 f8f5 h4h5 g6g5 f3f4 b6c6 e4c6"),
                # ( "27", "f2f4 c7c5 g1f3 b7b6 e2e4 c8b7 b1c3 a7a6 d2d4 c5d4 f3d4 e7e6 f1d3 b6b5 d1e2 f8c5 d4b3 c5b6 c1e3 d7d6 e3b6 d8b6 e4e5 g8e7 e1c1 d6e5 f4e5 e8g8 h2h4 b8c6 h4h5 c6b4 d3e4 b7e4 e2e4 b4d5 c3d5 e7d5 h1h3 b6f2 c1b1"),
                # ( "28", "c2c4 g8f6 b1c3 e7e6 e2e4 d7d5 e4e5 f6e4 c3e4 d5e4 d1g4 d8d4 g1f3 d4b6 g4e4 f8c5 e4h4 b8c6 f1d3 c6d4 f3g5 c5e7 h4h5 g7g6 h5h6 c8d7 h6g7 e8c8 g5f7 h8g8 g7h7 d8f8 f7h6 e7h4 h6g8 h4f2 e1f1 f8f4 g8e7 c8d8 e7g6"),
                # ( "29", "f2f4 d7d6 d2d4 g8f6 e2e3 b8d7 g1f3 c7c6 b2b3 e7e5 f4e5 d6e5 d4e5 f6e4 c1b2 d8a5 c2c3 d7e5 f3e5 a5e5 d1f3 e4g5 f3f4 f8d6 f4e5 d6e5 f1d3 c8g4 e1g1 e8c8 d3c2 h8e8 b1a3 e5c7 e3e4 d8d2 b2c1 g5h3 g2h3 c7h2 g1h1"),
                # ( "30", "b1c3 c7c5 d2d4 c5d4 d1d4 b8c6 d4d3 e7e6 g1f3 g8f6 c1g5 f8e7 e2e4 d7d5 e1c1 d5e4 d3c4 d8a5 g5f6 e4f3 f6e7 c6e7 g2f3 e8g8 f1d3 a5h5 h1g1 e6e5 c3e4 g8h8 c4c5 f8e8 d3b5 h5h6 c1b1 e7c6 e4d6 e8f8 b5c6 b7c6 d6f7"),
        )

        def una(li1, cp, lipv, desde, hasta, pkey, num):
            p = Partida.Partida(cp)
            pv = " ".join(lipv[desde:hasta])
            p.leerPV(pv)
            p.voice_key = "PG%s%02d"%(pkey, num)
            p.voice_moves = p.numJugadas()
            li1.append(p)
            return p

        li1 = []
        for pkey, pv in li:
            lipv = pv.split(" ")
            p = una(li1, None,          lipv,  0,      10, pkey, 1)
            p = una(li1, p.ultPosicion, lipv, 10,      20, pkey, 2)
            p = una(li1, p.ultPosicion, lipv, 20,      30, pkey, 3)
            p = una(li1, p.ultPosicion, lipv, 30, len(pv), pkey, 4)

        return li1

    def test(self):
        fila = self.gridDatos.recno()
        if fila < 0:
            fila = 0
        p = self.liRegs[fila]["__P__"]
        xtarget = self.getTarget(p)

        key = p.voice_key
        w = WTestFrase(self, xtarget, key)
        w.exec_()

class WTestFrase(QTVarios.WDialogo):
    ST_NEW, ST_RECORDING, ST_RECORDED, ST_LISTENING, ST_SAMPLE=range(5)
    def __init__(self, owner, target, key ):
        self.target = target
        self.txtTarget = " ".join(target)
        self.realTarget = self.txtTarget
        while "  " in self.realTarget:
            self.realTarget = self.realTarget.replace("  ", " ")
            self.target = self.realTarget.split(" ")
        self.key = key
        self.dKeyLiWords = owner.dKeyLiWords
        self.actualizaRatio = owner.actualizaRatio
        self.actualizaSamples = owner.actualizaSamples
        self.configuracion = owner.configuracion

        QTVarios.WDialogo.__init__(self, owner, _("Testing %s")%key, Iconos.S_Microfono(), "testfrase")

        ft = Controles.TipoLetra(puntos=11, peso=75)
        flb = Controles.TipoLetra(puntos=10)
        fr = Controles.TipoLetra(puntos=14, peso=75)
        # fp = Controles.TipoLetra(puntos=10)

        # st = "* { border-style: solid; border-width: 4px; border-color: LightSlateGray ;}"
        slb = "* { border-style: solid; border-width: 1px; border-color: black ;}"
        sr = "* { border-style: ridge; border-width: 2px; border-color: teal;}"

        self.lbTargetTit = Controles.LB(self, _("Target")).alinCentrado().anchoMinimo(600).altoMinimo(30).ponFuente(ft).ponFondoN("Gainsboro")
        self.lbTarget = Controles.LB(self, self.txtTarget).alinCentrado().anchoMinimo(600).altoMinimo(80).ponFuente(flb).ponWrap()
        self.lbTarget.setStyleSheet(slb)
        self.lbDecodedTit = Controles.LB(self, _("Decoded")).alinCentrado().anchoMinimo(600).altoMinimo(30).ponFuente(ft).ponFondoN("#848484")
        self.lbDecoded = Controles.LB(self).alinCentrado().anchoMinimo(600).altoMinimo(80).ponFuente(flb).ponWrap()
        self.lbDecoded.setStyleSheet(slb)
        self.lbResult = Controles.LB(self).alinCentrado().anchoMinimo(200).altoMinimo(60).ponFuente(fr)
        self.lbResult.setStyleSheet(sr)

        # Tool bar
        liAcciones = (
            ( _("Quit"), Iconos.Cancelar(), self.terminar ),
            ( _("Record"), Iconos.S_Microfono(), self.beginRecord ),
            ( _("End"), Iconos.S_StopMicrofono(), self.endRecord ),
            ( _("Listen"), Iconos.S_Play(), self.beginListen ),
            ( _("End"), Iconos.S_StopPlay(), self.endListen ),
            ( _("Save"), Iconos.Grabar(), self.save ),
            ( _("Delete"), Iconos.S_Limpiar(), self.delete ),
        )
        self.tb = Controles.TBrutina(self, liAcciones)

        self.estado = None
        self.setEstado( self.ST_NEW )

        ly = Colocacion.V()
        ly.control(self.lbTargetTit)
        ly.control(self.lbTarget)
        ly.control(self.lbDecodedTit)
        ly.control(self.lbDecoded)
        ly.control(self.lbResult)
        ly.margen(15)

        lyT = Colocacion.V().control(self.tb).otro(ly).relleno(1).margen(0)

        self.setLayout(lyT)

        self.recuperarVideo()

        self.runVoiceTmp = Voice.RunVoice()
        self.runVoiceTmp.setConf(self, True)

    def setEstado(self, estado):
        self.estadoPrevio = self.estado
        self.estado = estado
        d = {
                self.ST_NEW: (self.terminar, self.beginRecord),
                self.ST_RECORDING: (self.endRecord,),
                self.ST_RECORDED: (self.terminar, self.beginListen, self.save, self.delete),
                self.ST_LISTENING: (self.endListen,),
                self.ST_SAMPLE: (self.terminar, self.beginListen),
            }
        li = d[estado]
        for key in self.tb.dicTB:
            self.tb.setAccionVisible( key, key in li )

    def voice(self, txt):
        self.liDecoded.extend(txt.split(" "))
        self.lbDecoded.ponTexto(" ".join( self.liDecoded ))

    def terminar(self):
        self.cerrar()
        self.reject()

    def beginRecord(self):
        self.liDecoded = []
        self.lbDecoded.ponTexto("")
        self.lbResult.ponTexto("")
        self.runVoiceTmp.start(self.voice)
        self.setEstado(self.ST_RECORDING)

    def endRecord(self):
        self.ficheroWav = self.ficheroTmp = self.configuracion.ficheroTemporal("wav")

        self.runVoiceTmp.stop(self.ficheroTmp)

        ndeco = len(self.liDecoded)
        ntarget = len(self.target)
        estado = self.ST_RECORDED if abs(ndeco-ntarget) < 4 else self.ST_NEW
        self.setEstado(estado)

        aciertos = 0
        for n in range(min(ndeco,ntarget)):
            if self.liDecoded[n] == self.target[n]:
                aciertos += 1
        self.ratio = aciertos*100.0/ntarget
        self.lbResult.ponTexto("%0.02f%%"%self.ratio)
        self.actualizaRatio(self.key, self.ratio)

    def beginListen(self):
        self.setEstado(self.ST_LISTENING)

        self.siSeguimos = True
        tallerSonido = Sonido.TallerSonido(None)
        tallerSonido.leeWAV(self.ficheroWav)
        tallerSonido.playInicio(0,tallerSonido.centesimas)
        siSeguir = True
        while self.siSeguimos and siSeguir:
            siSeguir, pos = tallerSonido.play()
            QTUtil.refreshGUI()
        tallerSonido.playFinal()
        self.endListen()

    def endListen(self):
        self.siSeguimos = False
        self.setEstado(self.estadoPrevio)

    def save(self):
        n = 1
        while True:
            self.ficheroWav = os.path.join(self.configuracion.folderVoiceWavs, "%s_%d.wav"%(self.key,n))
            if os.path.isfile(self.ficheroWav):
                n += 1
            else:
                break
        shutil.copy(self.ficheroTmp, self.ficheroWav)
        ficheroTxt = self.ficheroWav[:-3] + "ini"
        with open(ficheroTxt,"wb") as f:
            f.write( "TARGET=%s\nRATIO=%0.02f"%(self.realTarget, self.ratio))
        self.actualizaSamples(self.key)
        self.setEstado(self.ST_SAMPLE)

    def delete(self):
        self.ficheroWav = self.ficheroTmp = self.configuracion.ficheroTemporal("wav")
        self.lbDecoded.ponTexto("")
        self.setEstado(self.ST_NEW)

    def cerrar(self):
        self.guardarVideo()
        self.runVoiceTmp.close()

    def closeEvent(self):
        self.cerrar()

def importVoice(owner, lng, configuracion):
    web = "https://3692fcee60669d89a7756cf7c38e045d23151cfc.googledrive.com/host/0B0D6J3YCrUoudHh3S3RxdGM1TlE"
    me = QTUtil2.mensEspera.inicio(owner, _("Reading the voice from the web"))
    fichz = "%s.zip"%lng

    siError = False
    # try:
    urllib.urlretrieve("%s/%s" % (web, fichz), fichz)

    zipFich = zipfile.ZipFile(fichz)
    zipFich.extractall("Voice")
    zipFich.close()
    os.remove(fichz)
    # except:
        # siError = True
    me.final()

    if siError:
        QTUtil2.mensError(owner, _("Voice currently unavailable; please check Internet connectivity"))
        return False

    QTUtil2.mensaje(owner, _("Voice installed"))
    configuracion.voice = lng
    configuracion.graba()


# -*- coding: latin-1 -*-

import Code.EnginesMicElo as EnginesMicElo
import Code.MotoresExternos as MotoresExternos
import Code.QT.QTVarios as QTVarios
import Code.QT.Iconos as Iconos
import Code.QT.Controles as Controles

INTERNO, EXTERNO, MICGM = range(3)

class Motores:
    def __init__(self, configuracion):
        self.configuracion = configuracion
        self.dicIconos = {INTERNO: Iconos.Motor(), EXTERNO: Iconos.MotoresExternos(), MICGM: Iconos.GranMaestro()}
        self.dicMotoresGM = EnginesMicElo.dicGM()
        self.liMotoresInternos = configuracion.listaMotoresInternos()
        self.rehazMotoresExternos()

    def rehazMotoresExternos(self):
        self.liMotoresExternos = [MotoresExternos.ConfigMotor(cm) for cm in self.configuracion.listaMotoresExternos()]
        self.liMotoresClavePV = self.configuracion.comboMotoresMultiPV10()

    def menu(self, parent):
        menu = Controles.Menu(parent)

        rp = QTVarios.rondoPuntos()

        submenu = menu.submenu(_("Internal engines"), self.dicIconos[INTERNO])
        for cm in self.liMotoresInternos:
            clave = INTERNO, cm
            texto = cm.nombre
            icono = rp.otro()
            elo = cm.elo
            submenu.opcion(clave, "%s (%d)" % (texto, elo), icono)
        menu.separador()
        submenu = menu.submenu(_("External engines"), self.dicIconos[EXTERNO])
        for cm in self.liMotoresExternos:
            clave = EXTERNO, cm
            texto = cm.clave
            icono = rp.otro()
            submenu.opcion(clave, texto, icono)
        submenu.separador()
        clave = EXTERNO, None
        texto = _("Edition")
        icono = Iconos.Mas()
        submenu.opcion(clave, texto, icono)

        if self.dicMotoresGM:
            menu.separador()
            submenu = menu.submenu(_("GM engines"), self.dicIconos[MICGM])
            for gm, li in self.dicMotoresGM.iteritems():
                icono = rp.otro()
                submenuGM = submenu.submenu(gm, icono)
                for cm in li:
                    clave = MICGM, cm
                    texto = cm.rotmenu
                    submenuGM.opcion(clave, texto, icono)
                submenuGM.separador()

        return menu.lanza()

    def busca(self, tipo, clave):
        if tipo is None:
            if clave.startswith("*"):
                clave = clave[1:]
                tipo = EXTERNO
            else:
                tipo = INTERNO

        rival = None
        if tipo == EXTERNO:
            for cm in self.liMotoresExternos:
                if cm.clave == clave:
                    rival = cm
                    break
            if not rival:
                tipo = INTERNO
                clave = self.configuracion.rivalInicial

        if tipo == MICGM:
            for nom, li in self.dicMotoresGM.iteritems():
                for cm in li:
                    if cm.clave == clave:
                        rival = cm
                        break
                if rival:
                    break
            if not rival:
                tipo = INTERNO
                clave = self.configuracion.rivalInicial

        if tipo == INTERNO:
            for cm in self.liMotoresInternos:
                if cm.clave == clave:
                    rival = cm
                    break
            if not rival:
                rival = self.liMotoresInternos[0]

        return tipo, rival


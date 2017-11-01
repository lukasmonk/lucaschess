from Code import VarGen
if VarGen.isLinux32:
    import Code.EnginesLinux32 as Engines
elif VarGen.isLinux64:
    import Code.EnginesLinux64 as Engines
else:
    import Code.EnginesWindows as Engines

from Code import BaseConfig
from Code import EnginesMicElo
from Code import MotoresExternos
from Code import GestorElo
from Code.QT import Controles
from Code.QT import Iconos
from Code.QT import QTVarios

INTERNO, EXTERNO, MICGM, MICPER, FIXED, IRINA, ELO = range(7)


class Motores:
    def __init__(self, configuracion):
        self.configuracion = configuracion
        self.dicIconos = {INTERNO: Iconos.Motor(), EXTERNO: Iconos.MotoresExternos(),
                          MICGM: Iconos.GranMaestro(), MICPER: Iconos.EloTimed(),
                          FIXED: Iconos.FixedElo(), IRINA: Iconos.RivalesMP(),
                          ELO: Iconos.Elo()}
        self.liMotoresGM = EnginesMicElo.listaGM()
        self.liMotoresInternos = configuracion.listaMotoresInternos()
        self.dicMotoresFixedElo = configuracion.dicMotoresFixedElo()
        self.rehazMotoresExternos()

        self.liIrina = self.genEnginesIrina()

        self.liElo = self.genEnginesElo()

    def rehazMotoresExternos(self):
        self.liMotoresExternos = [MotoresExternos.ConfigMotor(cm) for cm in self.configuracion.listaMotoresExternos()]
        self.liMotoresClavePV = self.configuracion.comboMotoresMultiPV10()

    def genEnginesIrina(self):
        cmbase = self.configuracion.buscaRival("irina")
        li = []
        for name, trans, ico in QTVarios.list_irina():
            cm = BaseConfig.ConfigMotor(name, cmbase.autor, cmbase.version, cmbase.url)
            cm.nombre = trans
            cm.icono = ico
            cm.carpeta = cmbase.carpeta
            cm.path = cmbase.path
            cm.ordenUCI("Personality", name)
            li.append(cm)
        return li

    def genEnginesElo(self):
        d = Engines.leeRivales()
        li = []
        for elo, clave, depth in GestorElo.listaMotoresElo():
            if clave in d:
                cm = d[clave].clona()
                cm.nombre = "%d - %s (%s %d)" % (elo, cm.nombre, _("depth"), depth)
                cm.clave = cm.nombre
                cm.fixed_depth = depth
                cm.elo = elo
                li.append(cm)
        li.sort(key=lambda x:x.elo)
        return li

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

        menu.separador()
        submenu = menu.submenu(_("GM engines"), self.dicIconos[MICGM])
        for cm in self.liMotoresGM:
            icono = rp.otro()
            clave = MICGM, cm
            texto = cm.nombre
            submenu.opcion(clave, texto, icono)
            submenu.separador()

        menu.separador()
        menu.opcion((MICPER, None), _("Tourney engines"), self.dicIconos[MICPER])

        menu.separador()
        submenu = menu.submenu(_("Engines with fixed elo"), self.dicIconos[FIXED])
        li = self.dicMotoresFixedElo.keys()
        li.sort()
        for elo in li:
            icono = rp.otro()
            submenuElo = submenu.submenu(str(elo), icono)
            lien = self.dicMotoresFixedElo[elo]
            lien.sort(key=lambda x: x.nombre)
            for cm in lien:
                clave = FIXED, cm
                texto = cm.nombre
                submenuElo.opcion(clave, texto, icono)
            submenuElo.separador()

        menu.separador()
        menu1 = menu.submenu(_("Opponents for young players"), Iconos.RivalesMP())
        for cm in self.liIrina:
            menu1.opcion((IRINA, cm), cm.nombre, cm.icono)

        menu.separador()
        menu1 = menu.submenu(_("Lucas-Elo"), Iconos.Elo())
        ico = QTVarios.rondoPuntos()
        limenus = []
        for x in range(1000, 2250, 250):
            limenus.append(menu1.submenu("%d - %d"%(x,x+250), ico.otro()))

        for cm in self.liElo:
            submenu = limenus[cm.elo/250-4]
            submenu.opcion((ELO, cm), cm.nombre, ico.otro())

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
            for cm in self.liMotoresGM:
                if cm.clave == clave:
                    rival = cm
                    break
            if not rival:
                tipo = INTERNO
                clave = self.configuracion.rivalInicial

        if tipo == MICPER:
            liMotores = EnginesMicElo.listaCompleta()

            for cm in liMotores:
                if cm.clave == clave:
                    rival = cm
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

        if tipo == FIXED:
            rival = None
            for elo, lista in self.dicMotoresFixedElo.iteritems():
                for cm in lista:
                    if cm.clave == clave:
                        rival = cm
                        break
                if rival:
                    break
            if not rival:
                rival = self.liMotoresInternos[0]

        if tipo == IRINA:
            rival = None
            for cm in self.liIrina:
                if cm.clave == clave:
                    rival = cm
                    break
            if not rival:
                rival = self.liMotoresInternos[0]

        if tipo == ELO:
            rival = None
            for cm in self.liElo:
                if cm.clave == clave:
                    rival = cm
                    break
            if not rival:
                rival = self.liMotoresInternos[0]

        return tipo, rival

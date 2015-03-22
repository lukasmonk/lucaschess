from Code.Constantes import *

import Code.QT.Iconos as Iconos

import Code.QT.FormLayout as FormLayout

def paramPelicula(parent):
    # Datos
    liGen = [(None, None)]

    # # Segundos
    liGen.append(( _("Number of seconds between moves") + ":", 2 ))

    # # Si desde el principi
    liGen.append(( _("Start from first move") + ":", True ))

    # Editamos
    resultado = FormLayout.fedit(liGen, title=_("Replay game"), parent=parent, anchoMinimo=460, icon=Iconos.Pelicula())

    if resultado:
        accion, liResp = resultado

        segundos = liResp[0]
        siPrincipio = liResp[1]
        return segundos, siPrincipio
    else:
        return None

class Pelicula:
    def __init__(self, gestor, segundos, siInicio):
        self.gestor = gestor
        self.procesador = gestor.procesador
        self.pantalla = gestor.pantalla
        self.siEmpiezaConNegras = gestor.partida.siEmpiezaConNegras
        self.tablero = gestor.tablero
        self.pgn = gestor.pgn
        self.segundos = segundos
        self.siInicio = siInicio
        self.rapidez = 1.0

        liAcciones = (
            k_peliculaTerminar, k_peliculaLento, k_peliculaPausa, k_peliculaSeguir, k_peliculaRapido,
            k_peliculaRepetir )

        self.antAcciones = self.pantalla.dameToolBar()
        self.pantalla.ponToolBar(liAcciones)

        self.gestor.ponRutinaAccionDef(self.procesarTB)

        self.muestraPausa(True)

        self.numJugadas, self.jugInicial, self.filaInicial, self.siBlancas = self.gestor.jugadaActual()

        self.liJugadas = self.gestor.partida.liJugadas
        self.posActual = 0 if siInicio else self.jugInicial

        self.siStop = False

        self.muestraActual()

    def muestraActual(self):
        if self.siStop:
            return

        jg = self.liJugadas[self.posActual]
        self.tablero.ponPosicion(jg.posicionBase)
        liMovs = [( "b", jg.hasta ), ( "m", jg.desde, jg.hasta )]
        if jg.posicion.liExtras:
            liMovs.extend(jg.posicion.liExtras)
        self.movimientosPiezas(liMovs)

        self.skip()

    def movimientosPiezas(self, liMovs):
        cpu = self.procesador.cpu
        cpu.reset()
        segundos = None

        jg = self.liJugadas[self.posActual]
        num = self.posActual
        if self.siEmpiezaConNegras:
            num += 1
        fila = int(num / 2)
        self.pantalla.pgnColocate(fila, jg.posicionBase.siBlancas)
        self.pantalla.base.pgnRefresh()

        # primero los movimientos
        for movim in liMovs:
            if movim[0] == "m":
                if segundos is None:
                    desde, hasta = movim[1], movim[2]
                    dc = ord(desde[0]) - ord(hasta[0])
                    df = int(desde[1]) - int(hasta[1])
                    # Maxima distancia = 9.9 ( 9,89... sqrt(7**2+7**2)) = 4 segundos
                    dist = ( dc ** 2 + df ** 2 ) ** 0.5
                    rp = self.rapidez if self.rapidez > 1.0 else 1.0
                    segundos = 4.0 * dist / (9.9 * rp)

                cpu.muevePieza(movim[1], movim[2], siExclusiva=False, segundos=segundos)

        if segundos is None:
            segundos = 1.0

        # segundo los borrados
        for movim in liMovs:
            if movim[0] == "b":
                n = cpu.duerme(segundos * 0.80)
                cpu.borraPieza(movim[1], padre=n)

        # tercero los cambios
        for movim in liMovs:
            if movim[0] == "c":
                cpu.cambiaPieza(movim[1], movim[2], siExclusiva=True)
        cpu.runLineal()
        self.gestor.ponFlechaSC(jg.desde, jg.hasta)

        self.tablero.ponPosicion(jg.posicion)

        # self.gestor.pgnMueve( fila, jg.posicion.siBlancas )

        cpu.reset()
        cpu.duerme(self.segundos / self.rapidez)
        cpu.runLineal()

    def muestraPausa(self, siPausa):
        self.pantalla.mostrarOpcionToolbar(k_peliculaPausa, siPausa)
        self.pantalla.mostrarOpcionToolbar(k_peliculaSeguir, not siPausa)

    def procesarTB(self, clave):
        if clave == k_peliculaTerminar:
            self.terminar()
        elif clave == k_peliculaLento:
            self.lento()
        elif clave == k_peliculaPausa:
            self.pausa()
        elif clave == k_peliculaSeguir:
            self.seguir()
        elif clave == k_peliculaRapido:
            self.rapido()
        elif clave == k_peliculaRepetir:
            self.repetir()

    def terminar(self):
        self.siStop = True
        self.pantalla.ponToolBar(self.antAcciones)
        self.gestor.ponRutinaAccionDef(None)
        self.gestor.xpelicula = None

    def lento(self):
        self.rapidez /= 1.2

    def rapido(self):
        self.rapidez *= 1.2

    def pausa(self):
        self.siStop = True
        self.muestraPausa(False)

    def seguir(self):
        numJugadas, self.posActual, filaInicial, siBlancas = self.gestor.jugadaActual()
        self.siStop = False
        self.muestraPausa(True)
        self.muestraActual()

    def repetir(self):
        self.posActual = self.jugInicial
        self.siStop = False
        self.muestraPausa(True)
        self.muestraActual()

    def skip(self):
        if self.siStop:
            return
        self.posActual += 1
        if self.posActual == self.numJugadas:
            self.pausa()
        else:
            self.muestraActual()

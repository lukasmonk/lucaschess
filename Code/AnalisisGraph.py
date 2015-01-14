# -*- coding: utf-8 -*-

from PyQt4.QtSvg import QSvgWidget

import pygal
import Code.Util as Util
import Code.VarGen as VarGen

def wHistogram(ncolor, siDiferencias, x, y):
    colors = ("#348f00", "#780098", "#b300a0", "#0181b2", "#00b2f0", "#e8537a", "#e95355", "#e87653")
    estilo = pygal.style.Style(
        background='transparent',
        plot_background='transparent',
        foreground='#53E89B',
        foreground_light='#53A0E8',
        foreground_dark='#630C0D',
        opacity='0.8',
        opacity_hover='.9',
        transition='250ms ease-in',
        colors=( colors[ncolor], )
    )
    chart = pygal.Line(x_label_rotation=90,
                       show_dots=False,
                       interpolation_parameters={'type': 'kochanek_bartels', 'b': -1, 'c': 1, 't': 1},
                       interpolate='hermite' if siDiferencias else 'cubic',
                       style=estilo,
                       fill=True,
                       show_legend=False,
                       css=['IntFiles/PyGal/style.css', 'IntFiles/PyGal/graph.css'],
                       label_font_size=7)
    chart.x_labels = x
    chart.add("0", y)
    cr = chart.render()
    fich = VarGen.configuracion.ficheroTemporal("svg")

    f = open(fich, "wb")
    f.write(cr.replace('x="-5"', 'x="-25"'))
    f.close()
    return QSvgWidget(fich)

def genSVG(partida):
    siPawns = not VarGen.configuracion.centipawns
    game = []
    gameDif = []
    white = []
    black = []
    whiteDif = []
    blackDif = []

    axis_x = []
    axis_xW = []
    axis_xB = []

    lijg = []
    lijgW = []
    lijgB = []

    # complexity = {True:[], False:[], None:[]}
    # winprobability = {True:[], False:[], None:[]}
    # narrowness = {True:[], False:[], None:[]}
    # efficientmobility = {True:[], False:[], None:[]}
    # piecesactivity = {True:[], False:[], None:[]}
    # exchangetendency = {True:[], False:[], None:[]}

    for num, jg in enumerate(partida.liJugadas):
        if jg.analisis:
            mrm, pos = jg.analisis
            siBlancas = jg.siBlancas()
            pts = mrm.liMultiPV[pos].puntosABS_5()
            pts0 = mrm.liMultiPV[0].puntosABS_5()
            if not siBlancas:
                pts = -pts
                pts0 = -pts0
            if siPawns:
                pts = pts / 100.0
                pts0 = pts0 / 100.0

            nj = num / 2.0 + 1.0

            rotulo = "%d." % int(nj)
            if not siBlancas:
                rotulo += ".."
            jg.xnum = rotulo
            jg.xsiW = siBlancas
            rotulo += jg.pgnSP()
            axis_x.append(rotulo)
            lijg.append(jg)

            game.append(pts)
            gameDif.append(pts0 - pts)

            if siBlancas:
                axis_xW.append(rotulo)
                white.append(pts)
                whiteDif.append(pts0 - pts)
                lijgW.append(jg)
            else:
                axis_xB.append(rotulo)
                black.append(-pts)
                blackDif.append(-pts0 + pts)
                lijgB.append(jg)

                # if not hasattr(jg,"complexity"):
                # cp = jg.posicionBase
                # jg.complexity = Analisis.calc_complexity(cp, mrm)
                # jg.winprobability = Analisis.calc_winprobability(cp, mrm)
                # jg.narrowness = Analisis.calc_narrowness(cp, mrm)
                # jg.efficientmobility = Analisis.calc_efficientmobility(cp, mrm)
                # jg.piecesactivity = Analisis.calc_piecesactivity(cp, mrm)
                # jg.exchangetendency = Analisis.calc_exchangetendency(cp, mrm)

                # complexity[siBlancas].append(jg.complexity)
                # winprobability[siBlancas].append(jg.winprobability)
                # narrowness[siBlancas].append(jg.narrowness)
                # efficientmobility[siBlancas].append(jg.efficientmobility)
                # piecesactivity[siBlancas].append(jg.piecesactivity)
                # exchangetendency[siBlancas].append(jg.exchangetendency)
                # complexity[None].append(jg.complexity)
                # winprobability[None].append(jg.winprobability)
                # narrowness[None].append(jg.narrowness)
                # efficientmobility[None].append(jg.efficientmobility)
                # piecesactivity[None].append(jg.piecesactivity)
                # exchangetendency[None].append(jg.exchangetendency)

    alm = Util.Almacen()

    alm.lijg = lijg
    alm.lijgW = lijgW
    alm.lijgB = lijgB
    alm.axis_x = axis_x

    alm.game = wHistogram(6, False, axis_x, game)
    alm.gameDif = wHistogram(7, True, axis_x, gameDif)

    alm.white = wHistogram(2, False, axis_xW, white)
    alm.whiteDif = wHistogram(3, True, axis_xW, whiteDif)

    alm.black = wHistogram(4, False, axis_xB, black)
    alm.blackDif = wHistogram(5, True, axis_xB, blackDif)

    # alm.complexity = {}
    # alm.winprobability = {}
    # alm.narrowness = {}
    # alm.efficientmobility = {}
    # alm.piecesactivity = {}
    # alm.exchangetendency = {}

    # axis = { True:axis_xW, False:axis_xB, None:axis_x}
    # for quien in (True,False,None):
    # ax = axis[quien]
    # alm.complexity[quien] = wHistogram( 0, True, ax, complexity[quien] )
    # alm.winprobability[quien] = wHistogram( 1, True, ax, winprobability[quien] )
    # alm.narrowness[quien] = wHistogram( 2, True, ax, narrowness[quien] )
    # alm.efficientmobility[quien] = wHistogram( 3, True, ax, efficientmobility[quien] )
    # alm.piecesactivity[quien] = wHistogram( 4, True, ax, piecesactivity[quien] )
    # alm.exchangetendency[quien] = wHistogram( 5, True, ax, exchangetendency[quien] )

    return alm

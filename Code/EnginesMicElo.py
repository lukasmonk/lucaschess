from Code import MotoresExternos


def leeMicEngines():
    lme = MotoresExternos.ListaMotoresExternos("./IntFiles/michele.pkt")
    lme.leer()
    li = []
    for mt in lme.liMotores:
        mt1 = MotoresExternos.ConfigMotor(mt)
        if mt.alias.isupper():
            mt1.alias = mt1.nombre = mt.alias[0] + mt.alias[1:].lower()
            mt1.book = "Openings/%s.bin" % mt.alias.lower()
        else:
            mt1.alias = mt1.nombre = mt.alias
            mt1.book = None

        mt1.elo = mt.elo
        mt1.idInfo = mt.idInfo
        li.append(mt1)
    return li


def listaGM():
    li = [mtl for mtl in leeMicEngines() if mtl.nombre[0].isupper()]
    li.sort(key=lambda uno: uno.nombre)
    return li


def listaCompleta():
    li = leeMicEngines()
    li.sort(key=lambda uno: uno.elo)
    return li

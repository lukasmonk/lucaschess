import __builtin__
import gettext
import os


def _F(txt):
    return _(txt) if txt else ""


def _SP(clave):
    if not clave:
        return ""
    clave = clave.strip()
    t = _F(clave)
    if t == clave:
        li = []
        for x in clave.split(" "):
            if x:
                li.append(_F(x))
        return " ".join(li)
    else:
        return t


def _X(clave, op1, op2=None, op3=None):
    if not clave:
        return ""
    resp = clave.replace("%1", op1)
    if op2:
        resp = resp.replace("%2", op2)
        if op3:
            resp = resp.replace("%3", op3)
    return resp

DOMAIN = "lucaschess"
DIR_LOCALE = "Locale"


def install(lang=None):
    if lang and os.path.isfile("%s/%s/LC_MESSAGES/%s.mo" % (DIR_LOCALE, lang, DOMAIN)):
        t = gettext.translation(DOMAIN, DIR_LOCALE, languages=[lang])
        t.install(True)
    else:
        gettext.install(DOMAIN, DIR_LOCALE)

    __builtin__.__dict__['_X'] = _X
    __builtin__.__dict__['_F'] = _F
    __builtin__.__dict__['_SP'] = _SP

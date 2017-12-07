import os
import codecs
import random
import datetime

from Code import PGNreader
from Code import Util
from Code import VarGen

QUALIFICATIONS = (
            ("Mind-bending", "7", 0.7, 0.5),
            ("Grandmaster", "6", 1.0, 0.8),
            ("International Master", "5", 1.5, 1.0),
            ("Master", "4", 2.2, 1.3),
            ("Candidate Master", "3", 3.1, 1.6),
            ("Amateur", "2", 4.2, 2.0),
            ("Beginner", "1", 5.5, 3.0),
            ("Future beginner", "0", 99.0, 99.0),
)


def qualification(seconds, think_mode):
    txt, ico, secs, secsThink = QUALIFICATIONS[-1]
    if seconds is not None:
        for xtxt, xico, xsecs, xsecsThink in QUALIFICATIONS[:-1]:
            if think_mode:
                if seconds < xsecsThink:
                    txt, ico = xtxt, xico
                    break
            else:
                if seconds < xsecs:
                    txt, ico = xtxt, xico
                    break
    return _F(txt), ico


class TOL_Block:
    def __init__(self):
        self.lines = []
        self.times = []
        self.reinits = []

    def update(self):
        if hasattr(self, "reinits"):
            return False
        self.reinits = []
        return True

    def penaltyError(self, think_mode):
        if think_mode:
            return 8.0 + len(self.times)*2.0
        else:
            return 5.0

    def penaltyHelp(self, think_mode):
        if think_mode:
            return 10.0 + len(self.times)*5.0
        else:
            return 10.0

    def add_line(self, line):
        self.lines.append(line)

    def new_result(self, seconds):
        today = datetime.datetime.now()
        self.times.append((seconds, today))
        self.times.sort(key=lambda x: x[0])

    def new_reinit(self, seconds):
        today = datetime.datetime.now()
        self.reinits.append((seconds, today))

    def av_seconds(self):
        return self.times[0][0] if self.times else None

    def num_moves(self):
        nm = 0
        for line in self.lines:
            nm += line.num_moves
        return nm

    def shuffle(self):
        random.shuffle(self.lines)

    def __len__(self):
        return len(self.lines)

    def line(self, num):
        return self.lines[num]

    def qualification(self, think_mode):
        av_seconds = self.av_seconds()
        txt, val = qualification(av_seconds, think_mode)
        return val

    def cqualification(self, think_mode):
        av_seconds = self.av_seconds()
        return qualification(av_seconds, think_mode)

    def calc_current(self, current_line, current_secs, errores, ayudas, think_mode):
        nmoves = 0
        for x in range(current_line+1):
            nmoves += self.lines[x].num_moves
        current_secs += errores*self.penaltyError(think_mode) + ayudas*self.penaltyHelp(think_mode)
        av_secs = current_secs/nmoves
        return av_secs, qualification(av_secs, think_mode)[0]


class TOL_Line:
    def __init__(self):
        self.fen = None
        self.label = None
        self.pgn = None
        self.num_moves = 0
        self.pv = None
        self.limoves = None

    def read_line_fich(self, line):
        li = line.strip().split("|")
        self.fen, self.label, pgn_moves = li[0], li[1], li[2]

        self.pgn = '[FEN "%s"]\n\n%s' % (self.fen, pgn_moves)
        g = PGNreader.read1Game(self.pgn)
        self.pv = g.pv()
        self.limoves = self.pv.split(" ")

        nmoves = len(self.limoves)
        self.num_moves = int(len(self.limoves)/2)
        if nmoves%2 == 1:
            self.num_moves += 1
        return self

    def get_move(self, num_move):
        return self.limoves[num_move]

    def total_moves(self):
        return len(self.limoves)


class TOL_level:
    def __init__(self, lines_per_block, num_level):
        self.themes_blocks = []
        self.lines_per_block = lines_per_block
        self.num_level = num_level

    def update(self):
        for liblock in self.themes_blocks:
            for block in liblock:
                if not block.update():
                    break

    def set_theme_blocks(self, theme):
        theme_blocks = []
        li = range(len(theme.lines))
        while li:
            li1 = random.sample(li, self.lines_per_block)
            li = [x for x in li if x not in li1]
            tol_block = TOL_Block()
            for i in li1:
                tol_block.add_line(theme.lines[i])
            theme_blocks.append(tol_block)
        self.themes_blocks.append(theme_blocks)

    def num_blocks(self):
        return len(self.themes_blocks[0])

    def done(self, think_mode):
        return int(self.get_cat_num(think_mode)[1]) > min(self.num_level, 2)

    def get_cat_num(self, think_mode):
        qmin = "?", "7"
        for theme_blocks in self.themes_blocks:
            for block in theme_blocks:
                q = block.cqualification(think_mode)
                if q[1] < qmin[1]:
                    qmin = q
        return qmin

    def val_theme_block(self, num_theme, num_block, think_mode):
        block = self.themes_blocks[num_theme][num_block]
        return block.qualification(think_mode)

    def get_theme_block(self, num_theme, num_block):
        return self.themes_blocks[num_theme][num_block]


class TOL_Theme:
    def __init__(self, folder, nameFNS, num_pos):
        self.name = nameFNS[:-4]
        self.lines = []
        path = os.path.join(folder, nameFNS)
        with codecs.open(path, encoding="utf-8", errors="ignore") as f:
            li = [linea.strip() for linea in f if linea.strip()]
            if len(li) != num_pos:
                li = random.sample(li, num_pos)
            for linea in li:
                tol_line = TOL_Line().read_line_fich(linea)
                self.lines.append(tol_line)


class TurnOnLights:
    def __init__(self, name, title, folder, li_tam_blocks):
        self.name = name
        self.title = title
        self.folder = folder
        self.themes = []
        self.levels = []
        self.work_level = 0
        self.li_tam_blocks = li_tam_blocks
        self.num_pos = li_tam_blocks[-1]
        self.go_fast = False
        self.calculation_mode = name.endswith("_calc")

    def is_calculation_mode(self):
        return self.calculation_mode

    def recupera(self):
        filepath = os.path.join(VarGen.configuracion.carpeta, "%s.tol" % self.name)
        tolr = Util.recuperaVar(filepath)
        if tolr is None:
            self.new()
        else:
            self.themes = tolr.themes
            self.levels = tolr.levels
            for level in self.levels:
                level.update()

            self.work_level = tolr.work_level
            self.num_pos = tolr.num_pos
            self.go_fast = tolr.go_fast

    def new(self):
        liFich = os.listdir(self.folder)
        liFich.sort()

        for fich in liFich:
            if fich.lower().endswith(".fns"):
                theme = TOL_Theme(self.folder, fich, self.num_pos)
                self.themes.append(theme)

        for num_level, lines_per_block in enumerate(self.li_tam_blocks):
            level = TOL_level(lines_per_block, num_level)
            for theme in self.themes:
                level.set_theme_blocks(theme)
            self.levels.append(level)

    def prev_next(self):
        prev, next = False, False
        if self.work_level > 0:
            prev = True
        if self.work_level < (len(self.levels)-1):
            level = self.levels[self.work_level]
            if level.done(self.calculation_mode):
                next = True
        return prev, next

    @property
    def num_blocks(self):
        return self.levels[self.work_level].num_blocks()

    @property
    def num_themes(self):
        return len(self.themes)

    @property
    def num_levels(self):
        return len(self.levels)

    def nom_theme(self, num):
        tname = self.themes[num].name
        return _F(tname)

    def val_block(self, num_theme, num_block):
        return self.levels[self.work_level].val_theme_block(num_theme, num_block, self.calculation_mode)

    def get_block(self, num_theme, num_block):
        return self.levels[self.work_level].get_theme_block(num_theme, num_block)

    def done_level(self):
        return self.levels[self.work_level].done(self.calculation_mode)

    def cat_num_level(self):
        return self.levels[self.work_level].get_cat_num(self.calculation_mode)

    def islast_level(self):
        return self.work_level == (len(self.levels)-1)

    def cat_global(self):
        cat, num, nada, nada = QUALIFICATIONS[0]

        for level in self.levels:
            cat1, num1 = level.get_cat_num(self.calculation_mode)
            if num1 < num:
                cat, num = cat1, num1
                if num == "0":
                    break

        return cat


def read_tol(name, title, folder, li_tam_blocks):
    tol = TurnOnLights(name, title, folder, li_tam_blocks)
    tol.recupera()
    return tol


def write_tol(tol):
    filepath = os.path.join(VarGen.configuracion.carpeta, "%s.tol" % tol.name)
    Util.guardaVar(filepath, tol)


def remove_tol(tol):
    filepath = os.path.join(VarGen.configuracion.carpeta, "%s.tol" % tol.name)
    Util.borraFichero(filepath)


def numColorMinimum(tol):
    num = tol.levels[tol.work_level].num_level+1
    if num >= 3:
        num = 3
    return num, tol.work_level == tol.num_levels-1


def compruebaUweEasy(configuracion, name):
    file = os.path.join(configuracion.carpeta, "%s.tol" % name)
    if Util.existeFichero(file):
        return
    folderDest = configuracion.carpetaTemporal()
    configuracion.limpiaTemporal()
    folderOri = "Trainings/Tactics by Uwe Auerswald"
    for fich in os.listdir(folderOri):
        if fich.endswith(".fns"):
            with open(os.path.join(folderOri, fich)) as f, open(os.path.join(folderDest, fich), "wb") as q:
                for linea in f:
                    if linea.count("*") < 3:
                        q.write(linea)

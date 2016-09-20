import os
import codecs
import random
import datetime

from Code import PGNreader
from Code import Util
from Code import VarGen

QUALIFICATIONS = (
            ("Mind-bending", "7", 0.7),
            ("Grandmaster", "6", 1.0),
            ("International Master", "5", 1.5),
            ("Master", "4", 2.2),
            ("Candidate Master", "3", 3.1),
            ("Amateur", "2", 4.2),
            ("Beginner", "1", 5.5),
            ("Future beginner", "0", 99.0),
)

def qualification(seconds):
    if seconds is None:
        txt, ico, secs = QUALIFICATIONS[-1]
    else:
        for txt, ico, secs in QUALIFICATIONS[:-1]:
            if seconds < secs:
                break
    return _F(txt), ico

class TOL_Block:
    def __init__(self):
        self.lines = []
        self.times = []

    def add_line(self, line):
        self.lines.append(line)

    def new_result(self, seconds):
        today = datetime.datetime.now()
        self.times.append((seconds, today))
        self.times.sort(key=lambda x: x[0])

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

    def qualification(self):
        av_seconds = self.av_seconds()
        txt, val = qualification(av_seconds)
        return val

    def cqualification(self):
        av_seconds = self.av_seconds()
        return qualification(av_seconds)

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

    def done(self):
        return int(self.get_cat_num()[1]) > min(self.num_level, 2)

    def get_cat_num(self):
        qmin = "?", "7"
        for theme_blocks in self.themes_blocks:
            for block in theme_blocks:
                q = block.cqualification()
                if q[1] < qmin[1]:
                    qmin = q
        return qmin

    def val_theme_block(self, num_theme, num_block):
        block = self.themes_blocks[num_theme][num_block]
        return block.qualification()

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
            if level.done():
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
        return self.levels[self.work_level].val_theme_block(num_theme, num_block)

    def get_block(self, num_theme, num_block):
        return self.levels[self.work_level].get_theme_block(num_theme, num_block)

    def done_level(self):
        return self.levels[self.work_level].done()

    def cat_num_level(self):
        return self.levels[self.work_level].get_cat_num()

    def islast_level(self):
        return self.work_level == (len(self.levels)-1)

    def cat_global(self):
        cat, num, nada = QUALIFICATIONS[0]

        for level in self.levels:
            cat1, num1 = level.get_cat_num()
            if num1 < num:
                cat, num = cat1, num1
                if num == "0":
                    break

        return cat

def read_tol(name, title, folder, li_tam_blocks):
    filepath = os.path.join(VarGen.configuracion.carpeta, "%s.tol" % name)
    tol = Util.recuperaVar(filepath)
    if tol is None:
        tol = TurnOnLights(name, title, folder, li_tam_blocks)
        tol.new()

    # for num_level, level in enumerate(tol.levels):
    #     for theme_blocks in level.themes_blocks:
    #         for block in theme_blocks:
    #             tm = random.randint(6, 55-5*num_level)
    #             block.new_result(tm*1.0/10.0)
    # write_tol(tol)

    return tol

def write_tol(tol):
    filepath = os.path.join(VarGen.configuracion.carpeta, "%s.tol" % tol.name)
    Util.guardaVar(filepath, tol)

def remove_tol(tol):
    filepath = os.path.join(VarGen.configuracion.carpeta, "%s.tol" % tol.name)
    Util.borraFichero(filepath)

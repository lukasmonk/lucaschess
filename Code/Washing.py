import os
import random
import datetime

from Code import Util
from Code import PGNreader

INACTIVE, CREATING, REPLAY, TACTICS, ENDED = range(5)


class WLine:
    def __init__(self):
        self.fen = None
        self.label = None
        self.pgn = None
        self.num_moves = 0
        self.pv = None
        self.limoves = None

    def read_line(self, line):
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


class WEngine:
    def __init__(self, clave=None, nombre=None, elo=0, color=True):
        self.clave = clave
        self.nombre = nombre
        self.elo = elo
        self.color = color
        self.secs = 0
        self.hints = 0
        self.hints_current = 0
        self.games = 0
        self.state = CREATING
        self.liNumTactics = []
        self.date = None

    def lbState(self):
        st = self.state
        if st == INACTIVE:
            return ""
        if st == CREATING:
            return _("Create the game")
        if st == REPLAY:
            return _("Replay the game")
        if st == TACTICS:
            return "%s (%d)" % (_("Play tactics"), len(self.liNumTactics))
        if st == ENDED:
            return _("Finished")

    def index(self):
        x = self.elo - self.hints*3 - self.games*23
        return x*100.0/self.elo if x > 0 else 0.0

    def cdate(self):
        if self.date:
            return Util.localDateT(self.date)
        else:
            return "-"

    def cindex(self):
        return "%0.02f" % self.index()
        #x = self.elo - self.hints*5 - self.games*47
        #return x*100.0/self.elo if x > 0 else 0.0

    def lbTime(self):
        return Util.secs2str(self.secs)

    def keyGame(self):
        return "%s-%d-game" %(self.clave, 0 if self.color else 1)

    def restoreGame(self, db):
        return db[self.keyGame()]

    def saveGame(self, db, game):
        db[self.keyGame()] = game

    def toDic(self):
        d = dict(
            clave=self.clave,
            nombre=self.nombre,
            elo=self.elo,
            color=self.color,
            secs=self.secs,
            hints=self.hints,
            hints_current=self.hints_current,
            games=self.games,
            state=self.state,
            liNumTactics=self.liNumTactics,
            date=self.date,
        )
        return d

    def fromDic(self, dic):
        get = dic.get
        self.clave = get("clave")
        self.nombre = get("nombre")
        self.elo = get("elo")
        self.color = get("color")
        self.secs = get("secs")
        self.hints = get("hints")
        self.hints_current = get("hints_current")
        self.games = get("games")
        self.state = get("state")
        self.liNumTactics = get("liNumTactics")
        self.date = get("date")

    def numTactics(self):
        return len(self.liNumTactics)

    def assign_date(self):
        self.date = datetime.datetime.now()


class Washing:
    def __init__(self):
        self.liEngines = []
        self.liTactics = []
        self.posTactics = 0

    def numEngines(self):
        return len(self.liEngines)

    def totalEngines(self, configuracion):
        n = 0
        for k, m in configuracion.dicRivales.iteritems():
            if m.elo < 3000:
                n += 2
        return n

    def totals(self):
        h = t = g = 0
        for eng in self.liEngines:
            h += eng.hints
            t += eng.secs
            g += eng.games
        return h, t, g

    def lastEngine(self, configuracion):
        st = set()
        if self.liEngines:
            eng = self.liEngines[-1]
            if eng.state != ENDED:
                return eng
            for eng in self.liEngines:
                st.add((eng.clave, eng.color))

        li = []
        for k, m in configuracion.dicRivales.iteritems():
            if m.elo < 3000:
                for color in (True, False):
                    if (m.clave, color) not in st:
                        engine = WEngine(m.clave, m.nombre, m.elo, color)
                        li.append(engine)
        li.sort( key=lambda x: "%4d%s" % (x.elo, "0" if x.color else "1" ))
        if li:
            eng = li[0]
            self.liEngines.append(eng)
            return eng
        return None

    def save(self, db):
        db["ENGINES"] = [ eng.toDic() for eng in self.liEngines ]
        db["POSTACTICS"] = self.posTactics

    def restore(self, db):
        liE = db.get("ENGINES", [])
        li = []
        for dic in liE:
            eng = WEngine()
            eng.fromDic(dic)
            li.append(eng)
        self.liEngines = li
        self.posTactics = db["POSTACTICS"]
        self.liTactics = db["TACTICS"]

    def addHint(self, hint=1):
        eng = self.liEngines[-1]
        eng.hints += hint
        eng.hints_current += hint

    def addTime(self, secs):
        eng = self.liEngines[-1]
        eng.secs += secs

    def addGame(self):
        eng = self.liEngines[-1]
        eng.games += 1

    def assign_tactics(self, eng):
        if eng.hints_current:
            eng.state = TACTICS
            eng.pos_tactics = self.posTactics
            ntactics = len(self.liTactics)
            self.posTactics += eng.hints_current
            if self.posTactics >= ntactics:
                self.posTactics -= ntactics
            eng.liNumTactics = []
            kp = eng.pos_tactics
            for x in range(eng.hints_current):
                p = kp + x
                eng.liNumTactics.append(p%ntactics)
        else:
            eng.state = ENDED

    def saveGame(self, db, game, siFinal):
        eng = self.liEngines[-1]
        if siFinal:
            self.assign_tactics(eng)
            if eng.state == ENDED:
                eng.assign_date()
        eng.saveGame(db, game)
        self.save(db)

    def createTactics(self, db, tipo):
        if tipo == "UNED":
            self.createTacticsUNED(db)
        elif tipo == "UWE":
            self.createTacticsUWE(db)
        elif tipo == "SM":
            self.createTacticsSM(db)
        else:
            self.createTacticsUNED(db)

    def createTacticsUNED(self, db):
        folder = "./Trainings/Tactics by UNED chess school"
        li = []
        for fns in os.listdir(folder):
            if fns.endswith(".fns"):
                fns = os.path.join(folder, fns)
                with open(fns) as f:
                    for linea in f:
                        li.append(linea)
        random.shuffle(li)
        self.liTactics = li
        db["TACTICS"] = self.liTactics
        self.posTactics = 0
        db["POSTACTICS"] = 0

    def createTacticsUWE(self, db):
        folder = "./Trainings/Tactics by Uwe Auerswald"
        d = {}
        for x in range(1, 6):
            d[x] = []
        for fns in os.listdir(folder):
            if fns.endswith(".fns"):
                fns = os.path.join(folder, fns)
                with open(fns) as f:
                    for linea in f:
                        l = linea.split("|")
                        n = l[1].count("*")
                        d[n].append(linea.strip())
        li = []
        for x in range(1, 6):
            random.shuffle(d[x])
            li.extend(d[x])

        self.liTactics = li
        db["TACTICS"] = li
        self.posTactics = 0
        db["POSTACTICS"] = 0

    def createTacticsSM(self, db):
        file = "./IntFiles/tactic0.bm"
        li = []
        with open(file) as f:
            for linea in f:
                linea = linea.strip()
                if linea:
                    fen, a8, mov, pgn, dif = linea.split("|")
                    li.append("%s|%s %s|%s" %(fen, _("Difficulty"), dif, mov))

        li = random.sample(li, 1000)
        self.liTactics = li
        db["TACTICS"] = li
        self.posTactics = 0
        db["POSTACTICS"] = 0

    def index_average(self):
        li = [eng.index() for eng in self.liEngines if eng.state == ENDED]
        return sum(li)/len(li) if li else 0.0


class DBWashing:
    def __init__(self, configuracion):
        self.configuracion = configuracion
        self.filename = "washing.wsm"
        self.file = os.path.join(configuracion.carpeta, self.filename)
        self.washing = self.washingRestore()

    def new(self, tactic):
        Util.borraFichero(self.file)
        self.washing = self.washingRestore(tactic)

    def washingRestore(self, tactic=None):
        with Util.DicRaw(self.file) as db:
            w = Washing()
            if "TACTICS" not in db:
                w.createTactics(db, tactic)
            else:
                w.restore(db)
        self.washing = w
        return w

    def saveWashing(self):
        with Util.DicRaw(self.file) as db:
            self.washing.save(db)

    def addHint(self, hint=1):
        self.washing.addHint(hint)
        self.saveWashing()

    def addTime(self, secs):
        self.washing.addTime(secs)
        self.saveWashing()

    def addGame(self):
        self.washing.addGame()
        self.saveWashing()

    def saveGame(self, game, siFinal):
        with Util.DicRaw(self.file) as db:
            self.washing.saveGame(db, game, siFinal)
            if siFinal:
                db.pack()

    def restoreGame(self, engine):
        with Util.DicRaw(self.file) as db:
            return engine.restoreGame(db)

    def next_tactic(self, engine):
        if engine.liNumTactics:
            n = engine.liNumTactics[0]
            linea = self.washing.liTactics[n]
            line = WLine()
            line.read_line(linea)
        else:
            line = None
        return line

    def done_tactic(self, engine, result):
        n = engine.liNumTactics[0]
        del engine.liNumTactics[0]
        if result:
            if len(engine.liNumTactics) == 0:
                engine.state = REPLAY
                engine.hints_current = 0
        else:
            engine.liNumTactics.append(n)
        self.saveWashing()

    def done_reinit(self, engine):
        self.washing.assign_tactics(engine)
        if engine.state == ENDED:
            engine.assign_date()

            with Util.DicRaw(self.file) as db:
                db.pack()
        self.saveWashing()


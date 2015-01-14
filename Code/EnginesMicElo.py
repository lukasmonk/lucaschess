# -*- coding: latin-1 -*-

import collections

import VarGen

class MotorMicElo:
    def __init__(self, nombre, exe):
        self.nombre = nombre[0].upper() + nombre[1:]
        self.clave = nombre
        self.exe = exe
        self.elo = 0
        self.liUCI = []
        self.book = "Openings/%s.bin" % nombre.split(" ")[0].lower()
        self.multiPV = 0
        self.siDebug = False

    def ejecutable(self):
        return self.exe

    def rotulo(self):
        return self.nombre

    def opcionUCI(self, nombre, valor):
        self.liUCI.append((nombre, valor))
        if nombre == "MultiPV":
            self.multiPV = int(valor)

    def guardaUCI(self):
        return str(self.liUCI)

    def recuperaUCI(self, txt):
        self.liUCI = eval(txt)

def listaGreko():
    txtGreko71 = """-adam-
1470
Hash 3
MultiPV 3
NodesPerSecondLimit 200
-albert-
1840
Hash 3
MultiPV 3
WeightMaterial 95
WeightPSQ 95
WeightMobility 95
WeightKingSafety 95
WeightPawns 95
WeightPassedPawns 95
NodesPerSecondLimit 700
-boris-
1540
Hash 3
MultiPV 3
WeightMaterial 90
WeightPSQ 90
WeightMobility 150
WeightKingSafety 90
WeightPawns 80
WeightPassedPawns 80
NodesPerSecondLimit 200
-brian-
1530
Hash 3
MultiPV 3
WeightMaterial 85
WeightPSQ 85
WeightMobility 115
WeightKingSafety 85
WeightPawns 85
WeightPassedPawns 85
NodesPerSecondLimit 230
-carlos-
1770
Hash 3
MultiPV 3
WeightMaterial 95
WeightPSQ 200
WeightMobility 95
WeightPawns 95
WeightPassedPawns 200
NodesPerSecondLimit 400
-cristina-
1480
Hash 3
MultiPV 3
WeightMobility 60
WeightKingSafety 200
NodesPerSecondLimit 230
-david-
1810
Hash 3
MultiPV 3
WeightMaterial 110
WeightPSQ 110
WeightMobility 75
WeightKingSafety 150
WeightPawns 150
WeightPassedPawns 75
NodesPerSecondLimit 900
-diana-
1970
Hash 3
MultiPV 3
WeightMaterial 95
WeightPSQ 95
WeightPawns 105
WeightPassedPawns 105
NodesPerSecondLimit 3000
-emily-
1690
Hash 3
MultiPV 3
WeightMaterial 70
WeightPSQ 70
WeightMobility 120
WeightKingSafety 70
WeightPawns 70
WeightPassedPawns 70
NodesPerSecondLimit 500
-erald-
1410
Hash 3
MultiPV 3
WeightMaterial 50
WeightPSQ 50
WeightMobility 50
WeightKingSafety 50
WeightPawns 50
WeightPassedPawns 50
NodesPerSecondLimit 230
-fabian-
1780
Hash 3
MultiPV 3
WeightMaterial 110
WeightPSQ 90
WeightMobility 130
WeightKingSafety 90
WeightPassedPawns 40
NodesPerSecondLimit 800
-ferenc-
1630
Hash 3
MultiPV 3
NodesPerSecondLimit 300
-goran-
1760
Hash 3
MultiPV 3
WeightKingSafety 105
WeightPawns 105
WeightPassedPawns 105
NodesPerSecondLimit 1300
-horst-
1530
Hash 3
MultiPV 3
WeightMaterial 95
WeightPSQ 95
WeightMobility 95
WeightKingSafety 95
WeightPawns 105
WeightPassedPawns 105
NodesPerSecondLimit 200
-irina-
1550
Hash 3
MultiPV 3
WeightMaterial 200
WeightPawns 250
NodesPerSecondLimit 300
-jacob-
1720
Hash 3
MultiPV 3
WeightMobility 70
WeightKingSafety 170
NodesPerSecondLimit 300
-kim-
1860
Hash 3
MultiPV 3
WeightMaterial 110
WeightPSQ 90
WeightMobility 110
WeightKingSafety 90
WeightPawns 90
WeightPassedPawns 40
NodesPerSecondLimit 1000
-lars-
1860
Hash 3
MultiPV 3
WeightMaterial 90
WeightPSQ 90
WeightMobility 300
WeightKingSafety 50
WeightPawns 50
WeightPassedPawns 90
NodesPerSecondLimit 1500
-monique-
1460
Hash 3
MultiPV 3
WeightMobility 60
WeightKingSafety 200
NodesPerSecondLimit 200
-nikola-
1950
Hash 3
MultiPV 3
WeightMaterial 95
WeightPSQ 95
WeightPawns 105
WeightPassedPawns 105
NodesPerSecondLimit 2400
-oleg-
1390
Hash 3
MultiPV 3
WeightMaterial 50
WeightPSQ 50
WeightMobility 50
WeightKingSafety 50
WeightPawns 50
WeightPassedPawns 50
NodesPerSecondLimit 200
-pavel-
1680
Hash 3
MultiPV 3
NodesPerSecondLimit 400
-qaled-
1740
Hash 3
MultiPV 3
WeightMaterial 95
WeightPSQ 95
WeightMobility 300
WeightKingSafety 50
WeightPawns 40
WeightPassedPawns 30
NodesPerSecondLimit 500
-raul-
2120
Hash 3
MultiPV 3
WeightPSQ 250
WeightKingSafety 150
WeightPawns 150
WeightPassedPawns 150
NodesPerSecondLimit 2200
-said-
1560
Hash 3
MultiPV 3
WeightMobility 150
WeightKingSafety 90
NodesPerSecondLimit 200
-tamara-
1830
Hash 3
MultiPV 3
WeightMaterial 95
WeightPSQ 95
WeightMobility 95
WeightKingSafety 95
WeightPawns 95
WeightPassedPawns 95
NodesPerSecondLimit 600
-umberto-
1510
Hash 3
MultiPV 3
WeightMaterial 85
WeightPSQ 85
WeightMobility 115
WeightKingSafety 85
WeightPawns 85
WeightPassedPawns 85
NodesPerSecondLimit 200
-viorel
1410
Hash 3
MultiPV 3
WeightMaterial 95
WeightPSQ 350
WeightMobility 95
WeightKingSafety 120
WeightPawns 110
WeightPassedPawns 95
NodesPerSecondLimit 300
-werner-
1460
Hash 3
MultiPV 3
WeightPSQ 20
WeightPassedPawns 20
NodesPerSecondLimit 200
-xavier-
1580
Hash 3
MultiPV 3
WeightMobility 150
WeightKingSafety 90
NodesPerSecondLimit 230
-yasser-
1430
Hash 3
MultiPV 3
WeightMaterial 90
WeightPSQ 20
WeightMobility 150
WeightKingSafety 90
WeightPawns 90
WeightPassedPawns 20
NodesPerSecondLimit 200
-zeliko-
1590
Hash 3
MultiPV 3
WeightMaterial 65
WeightPSQ 110
WeightMobility 250
WeightKingSafety 35
WeightPawns 65
WeightPassedPawns 35
NodesPerSecondLimit 500"""

    li = []
    tipo = "Windows" if VarGen.isWindows else "Linux"
    exe = "Engines%s/greko/7.1/Greko.exe" % tipo
    for linea in txtGreko71.split("\n"):
        linea = linea.strip()
        if linea.startswith("-"):
            mt = MotorMicElo(linea.replace("-", ""), exe)
            li.append(mt)
        elif linea.isdigit():
            mt.elo = int(linea)
        else:
            nom, valor = linea.split(" ")
            mt.opcionUCI(nom, valor)
    return li

def listaGM():
    txtToga = """-Alekhine champion-
2440
Root Alpha True
BookFile
Positional History Value 50
History Drop Depth 5
Mate Threat Depth 9
JD's Hangers Depth 9
Tactical Depth 9
King Attack 130
King Safety 110
Passed Pawns 110
Piece Activity 130
Piece Invasion 120
Material 105
Material Imbalance 20
Bishop Pair Opening 30
Bishop Pair Endgame 30
MovesToGo Max 40
Delta Pruning True
Futility Margin 1 50
Futility Margin 2 75
Futility Margin 3 75
Futility Margin 4 100
Futility Margin 5 100
Max NPS 30000
-Alekhine master-
2220
Root Alpha True
BookFile
Positional History Value 50
History Drop Depth 5
Mate Threat Depth 5
JD's Hangers Depth 5
Tactical Depth 5
King Attack 130
King Safety 110
Passed Pawns 110
Piece Activity 130
Piece Invasion 120
Material 105
Material Imbalance 20
Bishop Pair Opening 30
Bishop Pair Endgame 30
MovesToGo Max 40
Delta Pruning True
Delta Margin 150
Futility Margin 1 150
Futility Margin 2 200
Futility Margin 3 200
Futility Margin 4 250
Futility Margin 5 250
Max NPS 3000
-Alekhine expert-
2060
Root Alpha True
BookFile
Positional History Value 50
History Drop Depth 5
Mate Threat Depth 2
JD's Hangers Depth 2
Tactical Depth 2
King Attack 130
King Safety 110
Passed Pawns 110
Piece Activity 130
Piece Invasion 120
Material 105
Material Imbalance 20
Bishop Pair Opening 30
Bishop Pair Endgame 30
MovesToGo Max 40
Delta Pruning True
Delta Margin 250
Futility Margin 1 250
Futility Margin 2 350
Futility Margin 3 350
Futility Margin 4 450
Futility Margin 5 450
Max NPS 1000
-Anand champion-
2500
Root Alpha True
BookFile
Positional History Value 50
History Drop Depth 5
Mate Threat Depth 9
JD's Hangers Depth 9
Tactical Depth 9
King Attack 120
Passed Pawns 110
Piece Activity 120
Piece Invasion 120
Material 95
Material Imbalance 10
Bishop Pair Opening 30
Bishop Pair Endgame 30
MovesToGo Max 50
Delta Pruning True
Futility Margin 1 50
Futility Margin 2 75
Futility Margin 3 75
Futility Margin 4 100
Futility Margin 5 100
Max NPS 33000
-Anand master-
2200
Root Alpha True
BookFile
Positional History Value 50
History Drop Depth 5
Mate Threat Depth 5
JD's Hangers Depth 5
Tactical Depth 5
King Attack 120
Passed Pawns 110
Piece Activity 120
Piece Invasion 120
Material 95
Material Imbalance 10
Bishop Pair Opening 30
Bishop Pair Endgame 30
MovesToGo Max 50
Delta Pruning True
Delta Margin 150
Futility Margin 1 150
Futility Margin 2 200
Futility Margin 3 200
Futility Margin 4 250
Futility Margin 5 250
Max NPS 3300
-Anand expert-
1980
Root Alpha True
BookFile
Positional History Value 50
History Drop Depth 5
Mate Threat Depth 2
JD's Hangers Depth 2
Tactical Depth 2
King Attack 120
Passed Pawns 110
Piece Activity 120
Piece Invasion 120
Material 95
Material Imbalance 10
Bishop Pair Opening 30
Bishop Pair Endgame 30
MovesToGo Max 50
Delta Pruning True
Delta Margin 250
Futility Margin 1 250
Futility Margin 2 350
Futility Margin 3 350
Futility Margin 4 450
Futility Margin 5 450
Max NPS 1100
-Aronian champion-
2530
Root Alpha True
BookFile
Positional History Value 50
History Drop Depth 5
Mate Threat Depth 9
JD's Hangers Depth 9
Tactical Depth 9
King Attack 120
Pawn Structure 80
Passed Pawns 90
Piece Square Tables 90
Piece Activity 130
Piece Invasion 120
Material 90
Material Imbalance 30
Toga Exchange Bonus 10
Bishop Pair Opening 30
Bishop Pair Endgame 30
MovesToGo Max 50
Delta Pruning True
Delta Margin 60
Futility Margin 1 60
Futility Margin 2 90
Futility Margin 3 90
Futility Margin 4 120
Futility Margin 5 120
Max NPS 33000
-Aronian master-
2210
Root Alpha True
BookFile
Positional History Value 50
History Drop Depth 5
Mate Threat Depth 5
JD's Hangers Depth 5
Tactical Depth 5
King Attack 120
Pawn Structure 80
Passed Pawns 90
Piece Square Tables 90
Piece Activity 130
Piece Invasion 120
Material 90
Material Imbalance 30
Toga Exchange Bonus 10
Bishop Pair Opening 30
Bishop Pair Endgame 30
MovesToGo Max 50
Delta Pruning True
Delta Margin 180
Futility Margin 1 180
Futility Margin 2 240
Futility Margin 3 240
Max NPS 3300
-Aronian expert-
1990
Root Alpha True
BookFile
Positional History Value 50
History Drop Depth 5
Mate Threat Depth 2
JD's Hangers Depth 2
Tactical Depth 2
King Attack 120
Pawn Structure 80
Passed Pawns 90
Piece Square Tables 90
Piece Activity 130
Piece Invasion 120
Material 90
Material Imbalance 30
Toga Exchange Bonus 10
Bishop Pair Opening 30
Bishop Pair Endgame 30
MovesToGo Max 50
Delta Pruning True
Delta Margin 300
Futility Margin 1 300
Futility Margin 2 420
Futility Margin 3 420
Futility Margin 4 540
Futility Margin 5 540
Max NPS 1100
-Botvinnik champion-
2430
Root Alpha True
BookFile
Playing Style Positional
Positional History Value 50
History Drop Depth 5
Mate Threat Depth 7
JD's Hangers Depth 7
Tactical Depth 7
Toga Exchange Bonus 30
Bishop Pair Opening 30
Bishop Pair Endgame 40
MovesToGo Max 30
Delta Pruning True
Delta Margin 70
Futility Margin 1 70
Futility Margin 2 105
Futility Margin 3 105
Futility Margin 4 140
Futility Margin 5 140
Max NPS 33000
-Botvinnik master-
2110
Root Alpha True
BookFile
Playing Style Positional
Positional History Value 50
History Drop Depth 5
Mate Threat Depth 3
JD's Hangers Depth 3
Tactical Depth 3
Toga Exchange Bonus 30
Bishop Pair Opening 30
Bishop Pair Endgame 40
MovesToGo Max 30
Delta Pruning True
Delta Margin 210
Futility Margin 1 210
Futility Margin 2 280
Futility Margin 3 280
Futility Margin 4 350
Futility Margin 5 350
Max NPS 3300
-Botvinnik expert-
1650
Root Alpha True
BookFile
Playing Style Positional
Positional History Value 50
History Drop Depth 5
Mate Threat Depth 0
Tactical Depth 0
Toga Exchange Bonus 30
Bishop Pair Opening 30
Bishop Pair Endgame 40
MovesToGo Max 30
Delta Pruning True
Delta Margin 350
Futility Margin 1 350
Futility Margin 2 490
Futility Margin 3 490
Futility Margin 4 630
Futility Margin 5 630
Max NPS 1100
-Bronstein champion-
2380
Root Alpha True
BookFile
Positional History Value 50
History Drop Depth 5
Mate Threat Depth 9
JD's Hangers Depth 9
Tactical Depth 9
King Attack 140
King Safety 80
Pawn Structure 80
Passed Pawns 90
Piece Square Tables 80
Piece Activity 140
Piece Invasion 120
Material 90
Material Imbalance 40
Toga Exchange Bonus 10
Bishop Pair Opening 30
Bishop Pair Endgame 30
MovesToGo Max 30
Delta Pruning True
Delta Margin 60
Futility Margin 1 60
Futility Margin 2 90
Futility Margin 3 90
Futility Margin 4 120
Futility Margin 5 120
Max NPS 30000
-Bronstein master-
2140
Root Alpha True
BookFile
Positional History Value 50
History Drop Depth 5
Mate Threat Depth 5
JD's Hangers Depth 5
Tactical Depth 5
King Attack 140
King Safety 80
Pawn Structure 80
Passed Pawns 90
Piece Square Tables 80
Piece Activity 140
Piece Invasion 120
Material 90
Material Imbalance 40
Toga Exchange Bonus 10
Bishop Pair Opening 30
Bishop Pair Endgame 30
MovesToGo Max 30
Delta Pruning True
Delta Margin 180
Futility Margin 1 180
Futility Margin 2 240
Futility Margin 3 240
Max NPS 3000
-Bronstein expert-
1820
Root Alpha True
BookFile
Positional History Value 50
History Drop Depth 5
Mate Threat Depth 2
JD's Hangers Depth 2
Tactical Depth 2
King Attack 140
King Safety 80
Pawn Structure 80
Passed Pawns 90
Piece Square Tables 80
Piece Activity 140
Piece Invasion 120
Material 90
Material Imbalance 40
Toga Exchange Bonus 10
Bishop Pair Opening 30
Bishop Pair Endgame 30
MovesToGo Max 30
Delta Pruning True
Delta Margin 300
Futility Margin 1 300
Futility Margin 2 420
Futility Margin 3 420
Futility Margin 4 540
Futility Margin 5 540
Max NPS 1000
-Capablanca champion-
2420
Root Alpha True
BookFile
Playing Style Positional
Positional History Value 50
History Drop Depth 5
Mate Threat Depth 8
JD's Hangers Depth 8
Tactical Depth 8
King Attack 80
King Safety 120
Pawn Structure 90
Piece Activity 80
Piece Invasion 90
Toga Exchange Bonus 30
Bishop Pair Opening 30
Bishop Pair Endgame 30
MovesToGo Max 40
Delta Pruning True
Delta Margin 60
Futility Margin 1 60
Futility Margin 2 90
Futility Margin 3 90
Futility Margin 4 120
Futility Margin 5 120
Max NPS 30000
-Capablanca master-
2160
Root Alpha True
BookFile
Playing Style Positional
Positional History Value 50
History Drop Depth 5
JD's Hangers Depth 4
Tactical Depth 4
King Attack 80
King Safety 120
Pawn Structure 90
Piece Activity 80
Piece Invasion 90
Toga Exchange Bonus 30
Bishop Pair Opening 30
Bishop Pair Endgame 30
MovesToGo Max 40
Delta Pruning True
Delta Margin 180
Futility Margin 1 180
Futility Margin 2 240
Futility Margin 3 240
Max NPS 3000
-Capablanca expert-
1880
Root Alpha True
BookFile
Playing Style Positional
Positional History Value 50
History Drop Depth 5
Mate Threat Depth 1
JD's Hangers Depth 1
Tactical Depth 1
King Attack 80
King Safety 120
Pawn Structure 90
Piece Activity 80
Piece Invasion 90
Toga Exchange Bonus 30
Bishop Pair Opening 30
Bishop Pair Endgame 30
MovesToGo Max 40
Delta Pruning True
Delta Margin 300
Futility Margin 1 300
Futility Margin 2 420
Futility Margin 3 420
Futility Margin 4 540
Futility Margin 5 540
Max NPS 1000
-Carlsen champion-
2450
Root Alpha True
BookFile
Playing Style Positional
Positional History Value 50
History Drop Depth 5
Mate Threat Depth 8
JD's Hangers Depth 8
Tactical Depth 8
King Attack 90
Piece Square Tables 110
Piece Activity 90
Material Imbalance 10
Toga Exchange Bonus 30
Bishop Pair Opening 20
Bishop Pair Endgame 20
MovesToGo Max 60
Delta Pruning True
Delta Margin 60
Futility Margin 1 60
Futility Margin 2 90
Futility Margin 3 90
Futility Margin 4 120
Futility Margin 5 120
Max NPS 33000
-Carlsen master-
2190
Root Alpha True
BookFile
Playing Style Positional
Positional History Value 50
History Drop Depth 5
JD's Hangers Depth 4
Tactical Depth 4
King Attack 90
Piece Square Tables 110
Piece Activity 90
Material Imbalance 10
Toga Exchange Bonus 30
Bishop Pair Opening 20
Bishop Pair Endgame 20
MovesToGo Max 60
Delta Pruning True
Delta Margin 180
Futility Margin 1 180
Futility Margin 2 240
Futility Margin 3 240
Max NPS 3300
-Carlsen expert-
2000
Root Alpha True
BookFile
Playing Style Positional
Positional History Value 50
History Drop Depth 5
Mate Threat Depth 1
JD's Hangers Depth 1
Tactical Depth 1
King Attack 90
Piece Square Tables 110
Piece Activity 90
Material Imbalance 10
Toga Exchange Bonus 30
Bishop Pair Opening 20
Bishop Pair Endgame 20
MovesToGo Max 60
Delta Pruning True
Delta Margin 300
Futility Margin 1 300
Futility Margin 2 420
Futility Margin 3 420
Futility Margin 4 540
Futility Margin 5 540
Max NPS 1100"""

    dic = {"champion": _("Champion"), "expert": _("Expert"), "master": _("Master")}
    li = []
    tipo = "Windows" if VarGen.isWindows else "Linux"
    exe = "Engines%s/toga/DeepToga1.9.6nps.exe" % tipo
    for linea in txtToga.split("\n"):
        linea = linea.strip()
        if linea.startswith("-"):
            mt = MotorMicElo(linea.replace("-", ""), exe)
            mt.rotmenu = dic[mt.nombre.split(" ")[1]]
            li.append(mt)
        elif linea.isdigit():
            mt.elo = int(linea)
        else:
            sp = linea.rfind(" ")
            if sp == -1:
                nom = linea
                valor = ""
            else:
                nom, valor = linea[:sp], linea[sp + 1:]
            mt.opcionUCI(nom, valor)
    return li

def dicGM():
    dic = collections.OrderedDict()
    if VarGen.isLinux and not VarGen.isWine:
        return dic

    lista = listaGM()
    for cm in lista:
        gm = cm.nombre.split(" ")[0]
        if gm not in dic:
            dic[gm] = []
        dic[gm].append(cm)
    return dic

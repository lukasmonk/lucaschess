

def relaciones(dicRivales):
    result = """
stockfish=b2b3 e2e4 c1d2 e3e4 g1e2 c4c5 c1d2 b3e3 e1g1 b3d1 a4d7 d2c4 f1b5 d1d4 a1c1 c4c5 d4d5 a2a4 d1c1 e2e4 c1f4 e2c1 e1g1 d2d4 a4b5 h2h3 c1g5 d3d4 b2b3 a2a4 f2f4 a2a4 e1g1 d3d4 a2a4 b1c3 f1e1 d1c1 d1a4 b2b3 d4d5 c1d2 a2a3 h2h3 c4b5 c3e2 c3d1 e1g1 f1g2 e3d4 d1e2 f1e1 a4c4 a4b5 a4b5 d2d4 e1g1 c2c3 d2d4 e1g1 a2a4 d4d5 e1g1 d2b3 b1c3 d2d3 g2c6 b2b3 d4d5 c1e3 c4d3 c4d3 a2a4 f1d3 d4e5 b1a3 g2g3 b5e2 d2d3 c1e3 e1g1 d4d5 c4c5 e2f4 c4c5 d2d4 b1c3 e3d4 g2g4 f1e1 c1f4 d2d4 b2b3 c1b2 b5c3 a2a4 d2d3 f1e2 e2e3 f1g2
garbochess=d2b4 e2e4 c4d5 b3a4 g1e2 h1g1 c1d2 h2h3 h2h3 b3d1 a4d7 d2c4 f1e2 d1d4 e1f2 d4d5 c1f4 d4d5 d4d5 d4d5 d4d5 e2f4 e1g1 d2d4 a4b5 f1e1 c1g5 c1e3 c1g5 c1e3 e2f4 f2f3 e1g1 d2b3 d1e1 b1c3 d3b5 a2a4 f3d2 d1d3 d4d5 a2a3 a2a3 f3g5 c4b5 c3e2 c3d1 a1b1 f1d3 e3d4 d4d5 f3e5 e5d7 a4b5 a4b5 d2d4 b1c3 b1c3 d2d4 b1c3 f1d1 e4e5 e1g1 d2b3 b1c3 b1c3 d2c4 b2b3 c1e3 c1f4 c4d3 c4d3 a2a4 f1d3 d4e5 b1a3 c1e3 b5c6 d2d3 c6c2 e1g1 h2h3 c4c5 e2f4 c4d5 d2d4 b1c3 f3g4 e1c1 c1f4 c1e3 b1a3 b2b3 f1e2 c1e3 a2a4 a2a4 f3h4 e2e3 f1h3
rodent=d2b4 c1f4 f1d3 b3a4 g1f3 e3e4 a2a4 b3e3 h2h3 b3d1 a4f4 d2c4 f1e2 d1d4 e1f2 h2h3 e2e4 e3e4 d4d5 c1f4 d4d5 e1c1 h2h3 d2d4 a4b5 d1d3 c1e3 c1e3 c1g5 c1d2 c1e3 a2a3 e1g1 d2c4 h2h3 b1c3 a2a3 a2a4 e3e4 d1d3 d4d5 c1d2 e2e4 e1g1 c4b5 c3e2 c3d1 e1g1 h2h3 e3d4 c1e3 d1c2 a4c4 a4b5 c1e3 d2d4 b1c3 f3e5 d2d4 b1c3 a2a4 a2a3 e1g1 d2b3 b1c3 d2d3 g2c6 f1d1 d4d5 e1g1 c4d3 c4d3 d2d4 f1d3 d4e5 b1a3 c1e3 b5c6 d1a4 c6c2 e1g1 h2h3 g1f3 e2f4 b1c3 d2d4 b1c3 e3d4 e1c1 c1f4 f1d3 d2d4 b2b3 c1b2 d1a4 a2a4 a2a4 f3h4 e2e3 f1h3
zappa=d2b4 e2e4 f1d3 b3a4 g1e2 h1g1 c1d2 h2h3 h2h3 b3a4 a4f4 d2c4 c3b5 d1d4 e1c1 d4d5 c1f4 e3e4 d4d5 c1g5 c1f4 e1c1 a1d1 d2d4 a4b5 f1e1 c1e3 c1e3 c1e3 f1e1 c1f4 e2f4 e1g1 d2b3 c1d2 b1c3 e4e5 g5f6 f3d2 d1d3 d4e5 c1d2 a2a3 e1g1 c4b5 c3e2 c3d1 e1c1 f1g2 e3d4 d4d5 a3a4 a4c4 a4b5 a4b5 d2d4 b1c3 f3e5 d2d4 b1c3 b2b3 c1e3 e1g1 d2b3 b1c3 b1a3 g2c6 f3e5 a2a3 c1f4 c4d3 c4d3 d2d4 f1d3 d4e5 b1c3 e1f2 b5c6 d1a4 c6c2 e1g1 h2h3 g5f6 e2f4 c4d5 d2d4 b1c3 f3g4 e1c1 c1f4 e4e5 d2d4 b1a3 f1e2 d1a4 d2d4 a2a4 f1e2 d1a4 f1h3
rybka=d2b4 g2g4 f1e2 f1e2 g1e2 h1g1 c1d2 c4d5 h2h3 b3d1 a4f4 d2c4 f1e2 d1d4 e1f2 d4d5 c1g5 d4d5 d4d5 e2e4 d4d5 e1f2 a2a4 d2d4 a4b5 d4e5 c1g5 c1e3 c1g5 c1e3 c1e3 d1d2 e1g1 a2a4 c1d2 b1c3 d3e2 d1e2 d1a4 d1d3 d4d5 c1d2 a2a3 e1g1 c4b5 c3e2 e4e5 e1g1 f1d3 e3d4 d4d5 a3a4 e1g1 a4b5 a4b5 d2d4 e1g1 f3e5 d2d4 e1g1 a2a4 e4e5 e1g1 d2b3 b1c3 e1g1 d2c4 b2b3 e4e5 c6c2 c4d3 c4d3 a2a4 f1d3 d4e5 d2d3 e1f2 b5d3 d1c2 c6c2 e1g1 h2h3 c4c5 e2f4 b1c3 d2d4 b1c3 f3g4 e1c1 d1e2 e4e5 d2d4 b2b3 c1b2 c1e3 d2d3 d2d3 f1e2 d1a4 f1g2
umko=d1a4 e2e4 f1e2 f1e2 g1e2 c4c5 c1d2 h2h3 h2h3 b3d1 a4f4 d2c4 c3b5 d1d4 g2g4 d1b3 f1e1 d4d5 d2g5 e2e4 g3g4 g2g4 e1g1 a4b5 a4b5 d1d3 g3g4 g3g4 c1g5 g3g4 g3g4 f2f3 g3g4 a2a4 g1h1 b1c3 e4e5 d1c1 d1a4 d1d3 d4d5 d1e2 f1d1 e1g1 c4b5 c3e2 c3d1 e1g1 f1d3 e1g1 d1e2 a3a4 a4c4 a4b5 a4b5 d2d4 e1g1 b1c3 d2d4 b1c3 a2a4 c1f4 e1g1 d2b3 b1c3 b1c3 g2c6 b2b3 e4e5 c6c2 c4d3 c4d3 b1c3 d1a4 d4e5 b1a3 c2c3 b5c6 d1a4 c6c2 e1g1 c1e3 c4c5 e2f4 b1c3 d2d4 b1c3 e3d4 e1c1 c1f4 e4e5 d2d4 b2b3 f1e2 d1a4 a2a4 a2a4 f3h4 e5c6 f1h3
honey=d2b4 e2e4 h2h3 g2g4 g1e2 c4c5 c1d2 b3e3 e1g1 b3d1 a4d7 d2c4 c3b5 d1d4 a1c1 e2e4 c1g5 d1e2 b2b3 d1d3 e2e4 c3d5 e1g1 d2d4 a4b5 d1e2 a2a4 d3d4 b2b3 a2a4 c1e3 h2h3 e1g1 d3d4 a2a4 b1c3 d3c4 b1d2 d1a4 b2b3 d4d5 a2a3 a2a3 c1e3 c4b5 c3d5 c3d1 e1g1 f1g2 e3d4 d1e2 c1b2 e5d7 a4b5 a4b5 a4b5 b1c3 c2c3 d2d4 e1g1 a2a4 a2a3 e1g1 d2b3 b1c3 d2d3 g2c6 b2b3 d4d5 c1f4 c4e2 c4d3 d2d3 f1e2 d4e5 d2d3 g2g3 b5e2 d2d3 c1e3 a2a4 c1f4 c4c5 a2a3 c4c5 d2d4 b1d2 e3d4 e1c1 f1e1 c1f4 d2d4 b2b3 c1e3 b5c3 a2a4 a2a3 f3h4 e2e3 f1g2
rodentII=d1b3 e2e4 h2h3 f1e2 g1e2 c4c5 e1g1 b3e3 h2h3 b3d1 a4f4 d2c4 f1e2 d1d4 e1f2 h2h3 e2e4 e3e4 d4d5 d4d5 c1f4 e2g3 e1g1 d2d4 a4b5 h2h3 c1e3 c1e3 f2f3 c1e3 f2f4 f2f3 e1g1 h2h3 h2h3 b1c3 d3a6 f3d2 d1a4 d1d3 d4d5 d1c2 f1d1 e1g1 c4b5 c3e2 c3d1 e1g1 f1g2 e1g1 d1e2 a3a4 a4c4 a4b5 a4b5 d2d4 b1c3 f3e5 d2d4 e1g1 a2a4 c1e3 e1g1 d2b3 b1c3 e1g1 g2c6 f3e5 d4d5 e1g1 c4d3 c4d3 d2d4 f1d3 d4e5 e1g1 c1e3 b5c6 d2d3 c6c2 d2d4 h2h3 c4d5 e2f4 c4d5 d2d4 e1g1 f3g4 e1c1 c1f4 f1d3 d2d4 b1a3 c1b2 b5c3 d2d4 a2a4 f1e2 e2e3 f1h3
demolito=d2b4 e1f2 f1e2 h1g1 g1e2 h1g1 c1d2 c4d5 h2h3 b3d1 a4d7 d2c4 e1c1 b3d4 e1c1 d4d5 c1d2 d4d5 d4d5 c1f4 c1f4 e1c1 e1g1 a4b5 a4b5 c1g5 c1e3 c3b5 c1g5 c1e3 c1d2 d3d4 e1g1 d2b3 c1d2 b1c3 c1d2 b1d2 d1a4 f3d4 d4e5 f3d2 f1d1 e1g1 c4c5 c3d5 c3d1 e1c1 f1e2 e1g1 c1e3 c1b2 a4c4 a4b5 a4b5 a4b5 e1g1 b1c3 d2d4 b1c3 b1d2 f1e1 e1g1 d2b3 e1g1 e1g1 g2c6 b1d2 e4e5 e1g1 c4d3 c4d3 d2d4 f1e2 d4e5 b1c3 e2c3 b5c6 f3e1 e1g1 e1g1 h2h3 c4d5 e2f4 c4d5 d2d3 e1g1 e3d4 e1c1 c1f4 f1e2 d2d4 b2b3 f1e2 b1d2 d2d3 b1a3 f3h4 h2h3 f1g2
andscacs=d2g5 e2e4 c1d2 e3e4 g1e2 c4c5 c1d2 b3e3 h2h3 b3d1 a4d7 d2c4 f1e2 d1d4 e1c1 d4d5 c1f4 b2b3 d2g5 c1g5 c1g5 e2g3 a2a4 d2d4 a4b5 c1g5 c1g5 g1h2 a2a3 a2a4 c1g5 e3g5 e1g1 a2a4 c1e3 a2a4 f1e1 a2a4 d1a4 d1d3 d4d5 a2a3 f1d1 e1g1 c4b5 c3d5 c3d1 e1g1 f1g2 e3d4 d4d5 f1e1 a4c4 a4b5 a4b5 d2d4 e1g1 f3e5 d2d4 b1c3 f3e5 a2a3 e1g1 c2c4 b1c3 b1a3 g2c6 b2b3 f1e1 c6c2 c4e2 c4d3 d2d3 f1c4 d4e5 b1a3 g2g3 b5d3 d2d3 c6c2 e1g1 h2h3 c4c5 e1g1 c4c5 d2d4 f2f3 e3d4 e1c1 c1f4 f1e2 d2d4 b2b3 c1b2 b5c3 a2a4 a2a4 f1e2 f2f3 f1h3
gull=b2b3 g2g4 a2a4 e3e4 g1e2 c4c5 a2a4 h2h3 h2h3 b3d1 a4f4 d2c4 c3b5 d1d4 a1c1 a1c1 e2e4 e3e4 a1c1 e2e4 d4d5 a1c1 e1g1 d2d4 a4b5 c1g5 a2a4 d3d4 a2a3 a2a4 a2a4 h2h3 e1g1 d3d4 a2a4 a2a4 g1h1 a2a4 b2b3 d1d3 d4d5 a2a3 e2e3 e1g1 c4b5 c3d5 c3d1 e1g1 f1g2 a1b1 d4d5 f3e5 c1e3 a4b5 c2c3 d2d4 b1c3 c2c3 d2d4 b1c3 a2a4 a2a3 a2a3 d2b3 b1c3 b1a3 d2c4 f1d1 d4d5 c1f4 c4d3 c4d3 d2d4 f1d3 d4e5 b1c3 c1e3 b5d3 d2d3 c6c2 e1g1 h2h3 c4c5 e2f4 c4c5 d2d4 b1c3 f3g4 g2g4 f1e1 f1d3 d2d4 b2b3 c1b2 b5c3 a2a4 a2a3 f1e2 f2f3 f1g2
hamsters=d1b3 e2e4 c4c5 f1e2 g1e2 c4c5 c1d2 h2h3 h2h3 b3a4 a4d7 d2c4 f1e2 d1d4 e1c1 d4d5 c1d2 e3e4 e2e4 d4d5 c1f4 e1c1 e1g1 d2d4 a4b5 d1d3 c1e3 c1e3 d1b3 f1e1 g3g4 a2a3 d3d4 d2b3 f1e1 f1e1 d3c4 f3d2 f3d2 d1d3 d3d1 d1e2 f1d1 f3g5 c4b5 c3d5 c3d1 e1g1 f1d3 e3d4 d4d5 f3e5 a4c4 a4b5 a4b5 d2d4 e1g1 b1c3 d2d4 b1c3 a2a4 c1f4 c1f4 d2b3 b1c3 b1c3 g2c6 b1c3 c1f4 c6c2 c4d3 c4d3 d2d4 f1d3 d4e5 b1a3 c1e3 b5c6 d1a4 c6c2 e1g1 h2h3 c4c5 e2f4 c4d5 d2d4 b1c3 e3d4 e1c1 d1e2 e4e5 d2d3 b1a3 c1b2 d1a4 d2d3 a2a4 f3h4 e2e3 f1h3
ufim=a2a3 g2g4 f1e2 f1e2 g2g4 c4c5 c1d2 b3e3 h2h3 b3d1 a4d7 d2c4 f1b5 d1d4 g5h6 d4d5 c1g5 c1d2 d4d5 c1g5 c1d2 e3h6 g5h6 d2d4 a4b5 d4e5 c1g5 c1d2 c1g5 c1f4 c1e3 a2a3 e1g1 d2c4 c1d2 b1c3 c1d2 b1d2 d1a4 d1d4 d4d5 d1e2 f1d1 e1g1 c4b5 c3e2 c3d1 e1g1 f1g2 e3d4 c1e3 a3a4 a4c4 a4b5 a4b5 d2d4 e1g1 f3e5 d2d4 b1c3 a2a4 e4e5 e1g1 d2b3 e1g1 e1g1 g2c6 b2b3 e4e5 c1f4 c4d3 c4d3 d2d4 f1e2 d4e5 e1g1 c1e3 b5c6 d1a4 b1d2 d2d4 h2h3 c4c5 e2f4 c4d5 d2d4 b1d2 e3d4 e1c1 c1f4 f1e2 d2d4 b1a3 c1b2 f2f4 a2a4 d2d4 f1e2 g2g3 f1g2
cheng=d2e3 e2e4 c4c5 e3e4 g2g4 c4d5 c1e3 h2h3 e1g1 b3d1 a4f4 d2c4 c3b5 d1d4 a2a3 a2a3 e2e3 b2b3 d2g5 c1g5 e2e4 a2a3 e1g1 d2d4 a4b5 f1e1 c1e3 d3d4 a2a3 c1e3 c1e3 a2a3 d3d4 b2b3 f1e1 c2c4 d3a6 b1d2 b2b3 d1d3 d4d5 d1e2 f1d1 c1e3 c4b5 c3e2 c3d1 e1g1 f1g2 e3d4 c1g5 a3a4 a4c4 a4b5 a4b5 d2d4 e1g1 c2c3 d2d4 b1c3 b2b3 e4e5 a2a3 d2b3 b1c3 e1g1 d2c4 a2a4 e4e5 c1f4 c4e2 c4d3 d2d4 f1d3 d4e5 d2d3 g2g3 b5d3 d2d3 e1g1 d2d4 h2h3 c4c5 e2f4 c4d5 d2d4 b1d2 f3g4 e1c1 f1e1 e4e5 b2b3 b2b3 c1b2 b1c3 a2a4 a2a4 f1d3 e2e3 f1h3
spike=d2g5 g2g4 h2h3 b3a4 g1e2 c4c5 c1d2 b3e3 h2h3 b3d1 a4d7 d2c4 c3b5 d1d4 e2g3 b2b3 c1b2 b2b3 b2b3 a1b1 e2e4 h2h4 e1g1 d2d4 a4b5 h2h3 c1e3 a2a3 c1g5 a2a4 f2f4 h2h3 d1d2 f1e1 a2a4 b1c3 e4e5 d1c1 d1a4 d1d3 d4d5 d1c2 e2e3 e1g1 c4b5 c3d5 c3d1 e1g1 f3e2 e1g1 d1e2 f3e5 a4c4 a4b5 a4b5 d2d4 e1g1 f3e5 d2d4 e1g1 a2a4 c1e3 e1g1 d2b3 b1c3 d2d3 g2c6 f1d1 d4d5 c1f4 c4d3 c4d3 d2d4 f1d3 d4e5 d2d3 g2g3 b5d3 d2d3 c6c2 d2d3 d4d5 c4c5 e2f4 c4c5 d2d4 b1d2 f3g4 g2g4 e5f6 c1f4 d2d4 b2b3 c1b2 d1a4 a2a4 a2a4 f1e2 e2e3 f1h3
critter=d2f4 g2g3 f1e2 e3e4 g2g4 c4c5 a2a4 h2h3 h2h3 b3d1 a4d7 d2c4 f1e2 d1d4 b2b3 d4d5 c1b2 d4d5 d4d5 d4d5 d4d5 e2g3 e1g1 d2d4 a4b5 h2h3 c1e3 e2f4 d1b3 c1e3 a2a4 h2h3 e1g1 a2a4 a2a3 b1c3 d3c4 b1c3 d1a4 d1d3 d3d1 d1e2 f1d1 e1g1 c4b5 c3e2 c3d1 a2a3 f1g2 e3d4 b2b3 d1c2 e5d7 a4b5 a4b5 d2d4 b1c3 f3e5 d2d4 e1g1 a2a4 d4d5 c1f4 d2c4 b1c3 d2d3 g2c6 b2b3 f1e1 a2a4 c4d3 c4d3 d2d4 f1d3 d4e5 b1a3 c1e3 b5c6 b2b4 e1g1 e1g1 d4d5 c4d5 e2f4 c4d5 d2d4 e1g1 e3d4 e1c1 c1f4 c1e3 d2d4 b2b3 c1b2 d1a4 d2d3 d2d4 f3h4 h1g1 f1g2
wildcat=d2b4 g2g4 c4c5 f2f3 g1e2 c4c5 a2a4 c4c5 h2h3 b3d1 a4f4 d2c4 f1e2 d1d4 e2g3 f1e1 a2a4 e3e4 d4d5 c1f4 e2e4 e2g3 g5h6 d2d4 a4b5 d4e5 c1g5 a2a4 c1d2 f1e1 c1d2 d1d2 d1d2 d2b3 c1d2 b1c3 c1d2 f3d2 d1a4 d1d3 d4d5 c1d2 c1f4 e1g1 c4b5 c3e2 c3d1 e1g1 g3g4 e3d4 d4d5 a3a4 a4c4 a4b5 a4b5 d2d4 b1c3 f3e5 d2d4 b1c3 a2a4 d4d5 c1f4 d2b3 b1c3 e1g1 g2c6 f3e5 d4d5 c1d2 c4d3 c4d3 f3d4 a2a4 d4e5 e1g1 d4c5 b5c6 d1c2 e1g1 d2d3 h2h3 c4c5 e2f4 c4d5 d2d4 e1g1 f3g4 e1c1 c1f4 f1e2 d2d4 b1a3 f1e2 b1c3 a2a4 a2a4 f3h4 h1g1 f1h3
texel=d2b4 g2g4 c4c5 c3e2 f1e2 c4c5 a2a4 h2h3 e1g1 b3d1 a4d4 d2c4 a2a4 d1d4 g5h4 e2e4 a2a4 c1d2 b2b3 e2e4 e2e4 c3d5 a2a4 d2d4 a4b5 b2b3 a2a4 a2a4 h2h3 f1e1 a2a4 h2h3 e1g1 a2a4 a2a4 a2a4 h2h3 a2a4 d1a4 d1d3 d4d5 c1d2 a2a3 e1g1 c4b5 c3d5 c3d1 e1c1 f1g2 a2a4 f1e1 c1d2 a4c4 a4b5 a4b5 d2d4 b1c3 c2c3 d2d4 b1c3 a2a4 a2a3 c3d5 c2c3 a2a4 d2d3 g2c6 f1d1 d1e2 a2a4 c4e2 c4d3 a2a4 f1d3 d4e5 b1a3 g2g3 b5d3 d1a4 e1g1 d2d3 h2h3 c4c5 a2a3 c4d5 d2d4 b1d2 f3g4 e1c1 c1f4 c1e3 d2d4 b2b3 c1b2 c1e3 a2a4 a2a4 f1d3 e2e3 f1g2
arminius=d2g5 g2g4 f1d3 b3a4 g2g4 c4c5 c1d2 b3e3 h2h3 b3a4 a4f4 d2c4 e1f2 d1d4 e1f2 f4g5 c1a3 d4d5 a2a3 c1g5 c1g5 e2g3 e1g1 d2d4 a4b5 b2b3 c1g5 d3d4 c1g5 a2a4 a2a4 e3g5 e1g1 a2a4 a2a4 a2a4 d3a6 f3d2 d1a4 d1d3 d4d5 c1d2 e2e3 e1g1 c4b5 c3d5 c3d1 e1c1 f1d3 e3d4 d4d5 f3e5 a4c4 a4b5 a4b5 d2d4 b1c3 f3e5 d2d4 b1c3 a2a4 c3e2 a2a3 d2b3 a2a4 d2d3 g2c6 b2b3 d4d5 c1f4 c4e2 c4d3 d2d4 f1d3 d4e5 b1a3 e1f2 b5c6 d2d3 c6c2 d2d3 h2h3 c4c5 e2f4 b1c3 d2d4 e1g1 f3g4 e1c1 c1f4 e4e5 d2d4 b2b3 h2h4 d1a4 a2a4 d2d4 f1d3 e2e3 f1g2
rhetoric=a2a3 e2e4 c1d2 f1e2 g1f3 c4c5 c1d2 h2h3 h2h3 b3d1 a4f4 d2c4 f1e2 d1d4 a2a4 d4d5 c1e3 a2a4 a2a4 e2e4 a2a4 a1c1 e1g1 d2d4 a4b5 d4e5 c1e3 a2a3 a2a4 f1e1 a2a4 h2h3 e1g1 a2a4 a2a4 f1e1 h2h3 a2a4 a2a4 d1d3 d4d5 d1c2 c1e3 a2a3 c4b5 c3e2 c3d1 a2a3 h2h3 e1g1 c1f4 a1b1 a4c4 a4b5 a4b5 d2d4 e1g1 f3e5 d2d4 b1c3 a2a4 e4e5 e1g1 d2b3 e1g1 e1g1 g2c6 f1d1 d4d5 e1g1 c4e2 c4d3 a2a4 f1d3 d4e5 d2d3 d4c5 b5d3 d2d3 c6c2 e1g1 h2h3 c4c5 e2f4 c4c5 d2d4 b1c3 f3g4 e1c1 f1e1 e4e5 d2d4 b2b3 c1b2 b1c3 a2a4 a2a4 f3h4 f2f3 f1g2
fruit=d1b3 e2e4 f1d3 f1e2 g1e2 c4c5 c1d2 h2h3 h2h3 b3d1 a4d7 d2c4 f1b5 d1d4 e1c1 h2h3 f1e1 e3e4 d1c2 e2e4 c1f4 e1c1 e1g1 d2d4 a4b5 f1e1 c1g5 a2a3 c1g5 f1e1 c1d2 a2a3 a2a3 f1e1 g1h1 b1c3 d3e2 f3d2 f3d2 d1d3 d4e5 d1e2 f1d1 c1e3 c4c5 c3d5 c3d1 e1g1 f1e2 e1c1 d1e2 d1c2 e5d7 a4b5 a4b5 d2d4 b1c3 f3e5 d2d4 b1c3 a2a4 c1e3 e1g1 d2b3 b1c3 d2d3 g2c6 b2b3 c1f4 c6c2 c4d3 c4d3 b1c3 f1d3 d4e5 b1a3 c1e3 b5c6 d2d3 c6c2 e1g1 h2h3 c4c5 e2f4 c4d5 d2d4 b1c3 e3d4 e1c1 d1e2 c1e3 d2d3 b2b3 b1c3 c1e3 d2d3 a2a4 f3h4 e2e3 f1h3
simplex=d2b4 g2g4 f1e2 f1e2 g1e2 c4c5 c1d2 h2h3 h2h3 b3d1 a4d7 d2c4 f1e2 d1d4 e1c1 e2e4 e2e4 d4d5 d4d5 d4d5 d4d5 g2g4 e1g1 d2d4 a4b5 d4e5 g3g4 a2a3 a2a3 g3g4 g3g4 e3g5 a2a3 a2a4 a2a3 g3g4 d3c4 f3d2 d1a4 d1d3 d4d5 d1e2 a2a3 e1g1 c4b5 c3e2 c3d1 e1g1 g3g4 e3d4 d4d5 a3a4 a4c4 a4b5 a4b5 a4b5 e1g1 d2d3 d2d4 e1g1 a2a4 e4e5 e1g1 d2b3 b1c3 b1c3 g2c6 a2a4 e4e5 c1f4 c4d3 c4d3 b3c2 f1d3 d4e5 e1g1 c1e3 b5c6 d1a4 c6c2 e1g1 h2h3 c4c5 e2f4 b1c3 d2d4 e1g1 f3g4 e1c1 c1f4 e4e5 b1a3 b1a3 f1e2 d1a4 a2a4 a2a4 f1d3 f2f3 f1h3
komodo=d2e3 g2g4 c4c5 e3e4 g2g4 c4c5 a2a4 c4c5 h2h3 b3d1 a4d7 d2c4 c3b5 d1d4 a2a3 d4d5 c1f4 e3e4 a2a3 d4d5 d4d5 a2a3 e1g1 a4b5 a4b5 c4e2 c1e3 d3d4 c1f4 c1f4 c1g5 a1b1 a2a3 f1e1 c1d2 b1c3 a2a3 a2a4 d1a4 d1d3 d4d5 d1e2 a2a3 e1g1 c4b5 c3d5 c3d1 e1g1 f1g2 e3d4 d1e2 a3a4 a4c4 a4b5 a4b5 d2d4 c1e3 f3e5 d2d4 e1g1 b2b3 c1e3 e1g1 d2b3 b1c3 b1a3 g2c6 b2b3 d4d5 c1f4 c4d3 c4d3 d2d3 f1d3 d4e5 b1a3 d4c5 b5d3 d2d3 c6c2 e1g1 h2h3 c4c5 e2f4 c4c5 d2d4 b1d2 f3g4 e1c1 c1f4 c1e3 d2d4 b2b3 c1e3 c1e3 a2a4 a2a4 f1d3 d1a4 f1h3
gaviota=a2a3 g2g4 h2h3 h2h3 g2g4 c4c5 c1d2 h2h3 h2h3 b3d1 a4d7 d2c4 c3b5 d1d4 a2a3 d1d2 a2a3 c1d2 d4d5 e2e4 a2a3 a2a3 e1g1 d2d4 a4b5 f1e1 a2a4 a2a4 c1g5 a2a4 a2a4 h2h3 a2a3 a2a4 a2a4 g3g4 d3c4 b1d2 d1a4 d1d3 d4d5 a2a3 a2a3 e1g1 c4b5 c3d5 c3d1 a2a3 g3g4 e3d4 b2b3 c1b2 a4c4 a4b5 a4b5 d2d4 e1g1 f3e5 d2d4 b1c3 a2a4 a2a3 a2a3 d2b3 b1c3 b1c3 g2c6 b2b3 e4e5 c1f4 c4d3 c4d3 d2d4 h2h3 d4e5 d2d3 d4c5 b5c6 d2d3 c6c2 d2d3 h2h3 c4c5 e2f4 b1c3 d2d4 b1d2 e3d4 e1c1 b2b3 c1f4 d2d4 b2b3 c1b2 b1c3 d2d4 a2a4 f1e2 h2h3 f1h3
bikjump=d1c2 c1f4 c4d5 b3a4 g2g4 c4d5 e1g1 h2h3 h2h3 b3c2 a4d7 d2c4 d4c5 d1d4 e1c1 c3d5 a2a3 e3e4 h2h3 c3d5 c3d5 e1c1 e1c1 d2d4 a4b5 c1e3 f2f3 f2f3 c1f4 f1e1 f2f3 f2f3 d1d2 h2h3 c1d2 b1c3 f1f2 f3d2 f3d2 d1a4 d4d5 a2a3 f1d1 e1g1 c4b5 c3e2 c3d1 e1g1 e1c1 e1c1 d4d5 c1g5 a4c4 a4b5 a4b5 d2d4 f3g5 f3e5 d2d4 e1g1 b2b3 e4e5 e1g1 d2b3 e1g1 a4a5 g2c6 b2b3 e4e5 e1g1 c4e2 c4d3 b3c2 f1c4 d4e5 e1g1 c1d2 b5c6 d1c2 c1g5 d2d3 h2h3 c4c5 e2f4 c4d5 d2d4 b1d2 e3d4 e1c1 d1e1 c1f4 d2d3 b2b3 f1e2 c1e3 d2d3 d2d3 f1e2 e2e3 f2f3
gaia=d2b4 e2e4 f1d3 c1d2 c1d2 c1d2 c1d2 c4c5 h2h3 b3a4 a4d7 d2c4 f1b5 d1d4 d4d5 d4d5 c1b2 e3e4 d4d5 e2e4 e2e4 g2g4 e1g1 d2d4 a4b5 d4e5 d4c5 d3d4 c1g5 c1e3 c1e3 e3g5 a2a3 d2b3 f1e1 g3g4 d3a6 b1d2 d1a4 d1d3 d4d5 a2a3 a2a3 e1g1 c4c5 c3e2 c3d1 e3d4 f1d3 e3d4 d4d5 c1b2 e5d7 a4b5 a4b5 d2d4 b1c3 f3e5 d2d4 b1c3 a2a4 c1e3 e1g1 d2b3 b1c3 d2d3 g2c6 f3e5 e4e5 c1f4 c4d3 c4d3 f3d4 f1d3 d4e5 d2d3 c1d2 b5d3 d2d3 c6c2 d2d3 h2h3 g5f6 e2f4 c4c5 d2d3 b1d2 e3d4 e1c1 e5f6 f1d3 d2d4 b1a3 c1e3 d1a4 d2d4 d2d3 f1e2 d1a4 c1d2
greko=a2a3 e2e4 c1d2 c1d2 g1e2 c4c5 c1d2 c4c5 h2h3 b3a4 a4d4 d2c4 d4c5 d1d4 e2g3 d4d5 c1e3 d4d5 d4d5 d4d5 c1f4 e2g3 e1g1 d2d4 a4b5 d4e5 c1e3 c1e3 d1b3 c1e3 c1e3 d1d2 a2a3 d2b3 a2a4 b1c3 a2a4 b1c3 f3d2 d1a4 f1d1 d1e2 f1d1 e1g1 c4b5 c3e2 c3d1 a2a3 f1d3 e3d4 c1f4 f3e5 a4c4 a4b5 a4b5 a4b5 b1c3 d2d3 d2d4 b1c3 c1d2 c1e3 e1g1 d2b3 b1c3 d2d3 g2c6 b1c3 c1f4 c1f4 c4d3 c4d3 d2d4 b6b7 d4e5 d2d3 c1e3 b5c6 d2d3 c1f4 d2d3 h2h3 c4c5 e2f4 c4d5 d2d4 f2f3 e3d4 e1c1 f1e1 c1f4 d2d3 b1a3 c1e3 c1e3 a2a4 a2a4 f1d3 e2e3 f1h3
pawny=d2b4 e2e4 f1d3 b3a4 g2g4 h1g1 c1d2 c4c5 h2h3 b3a4 a4d7 d2c4 d4c5 d1d4 e1c1 e2e4 e2e4 d4d5 d4d5 d4d5 d4d5 e1c1 e1g1 d2d4 a4b5 c1e3 d4c5 c1e3 c1g5 f1e1 c1g5 d1d2 d3d4 d2b3 h2h3 g1h2 d3c4 f3d2 d1a4 d1d3 d4e5 f3d2 e2e3 c1e3 c4b5 c3e2 c3d1 e1c1 f1g2 e3d4 d4d5 f3e5 a4c4 a4b5 a4b5 d2d4 b1c3 f3e5 d2d4 b1c3 a2a4 d4d5 e1g1 d2b3 b1c3 b1c3 g2c6 f1d1 d4d5 c1f4 c4d3 c4d3 d2d4 f1e2 d4e5 b1c3 d4c5 b5c6 d1a4 c6c2 d2d3 c1f4 c4c5 e2f4 c4d5 d2d4 b1c3 e3d4 e1c1 c1f4 c1f4 d2d4 b2b3 f1e2 d1a4 d2d3 d2d4 f1d3 e5c6 f1h3
toga=d1b3 g2g4 f1d3 b3a4 g1e2 c4c5 c1d2 h2h3 h2h3 b3d1 a4d7 d2c4 f1e2 d1d4 h2h4 d1d2 c1a3 c1d2 e2e4 e2e4 g3g4 h2h4 e1g1 d2d4 a4b5 h2h3 c1g5 c1e3 c1g5 g3g4 g3g4 a2a3 a2a3 d2c4 g1h1 g3g4 d3c4 d1e2 d1a4 d1d3 d4e5 d1e2 f1d1 e1g1 c4b5 c3d5 c3d1 e1g1 f1d3 e1g1 d1e2 f3e5 e1g1 a4b5 a4b5 d2d4 e1g1 c2c3 d2d4 e1g1 a2a4 e4e5 a2a3 d2b3 b1c3 b1c3 g2c6 b2b3 d1e2 c6c2 c4d3 c4d3 a2a4 f1d3 d4e5 b1a3 c1e3 b5c6 d1a4 c6c2 e1g1 h2h3 c4c5 e2f4 b1c3 d2d4 b1c3 f3g4 e1c1 b2b4 f1d3 d2d4 b2b3 c1b2 d1a4 a2a4 a2a4 f1e2 d1a4 f1h3
cdrill=d1c2 g2g4 c4d5 e3e4 g2g4 a2a4 a2a4 c4c5 h2h3 b3d1 a4d7 b1c3 c3b5 d1d4 g5h6 d1d2 d1d2 d4d5 d4d5 d4d5 d1d2 e3h6 g5h6 d2d4 a4b5 d1d2 d4c5 d1d2 d1d2 f3h4 d1d2 d1d2 d1c1 h2h3 f4f5 g3g4 f4f5 b1d2 a2a4 d1d4 d4d5 d1c2 a2a3 d4c5 c4b5 c3d5 e4e5 e3d4 g3g4 e1g1 e4e5 f3g5 a4c4 a4b5 a4b5 d2d4 d3d4 d2d4 d2d4 d3d4 f3e5 e4e5 c1f4 d2b3 b1c3 d2d4 g2c6 f3e5 e4e5 c1f4 c4b3 c4d3 f3d4 d1a4 d4e5 d2d4 c1e3 b5c6 d2d3 c6c2 c2a4 h2h3 c4c5 e2f4 d1e1 e2e3 b1c3 e3d4 e1c1 d1e2 c1f4 d2d4 b2b3 c1f4 d1h5 d2d4 d2d4 d1b3 d1b3 e2f3
chispa=d1c2 g2g4 b3c2 b3d1 g2g4 c4d5 a2a4 b3d3 b3c2 b3d1 a4d7 d2c4 e1c1 d1d4 e1f2 d4d5 e2e4 d1e2 d4d5 e2e4 d4d5 g2g4 e1g1 d2d4 d1e2 d1e2 c3b5 d3d4 c1e3 d1e2 c3d5 a2a4 e1g1 d1c2 a2a4 b1c3 d3c4 f3d2 d1a4 d1d3 d4d5 a2a3 e2e3 e1g1 c4b5 c3e2 c3d1 e1g1 f3e2 e3d4 e4e5 d1c2 e5d7 c1d2 c2c3 d2d4 e1g1 c2c3 d2d4 e1g1 b2b3 c1e3 e1g1 c2c4 b1c3 b3c2 g2c6 b2b3 f1e1 c1f4 c4d3 c4d3 b3c2 f1d3 d4e5 b3c2 e2c3 b5c6 d1a4 c6c2 d2d4 c1e3 c4c5 e2f4 c4d5 d2d4 e1g1 f3g4 e1c1 d1e2 f1e2 d2d4 b2b3 f1e2 b1c3 d2d3 a2a4 h2h3 e2e3 f1g2
amyan=a2a3 e2e4 f1e2 f1e2 f1e2 c1d2 e1g1 h2h3 h2h3 b3d1 a4f4 d2c4 e1c1 d1d4 e1c1 e2e4 c1f4 e3e4 d2g5 c1f4 e2e4 e1c1 g5h6 a4b5 b1c3 c1e3 f2f4 f2f4 c1g5 c1f4 f2f4 f2f3 e1g1 d3d4 h2h3 b1c3 c3b5 f3d2 f3d2 d1a4 c1g5 d1e2 f1d1 d4c5 c4b5 c3e2 c3d1 e1g1 e1c1 e1g1 d1a4 c1g5 a4c4 a4b5 a4b5 d2d4 e1g1 f1e1 d2d4 e1g1 a2a4 c1f4 e1g1 e2e4 e1g1 e1g1 g2c6 b1c3 c1g5 c1f4 c4d3 c4d3 d2d4 f1d3 d4e5 e1g1 e1f2 b5c6 d2d3 e1g1 e1g1 c1e3 c4c5 e2f4 c4c5 d2d4 b1d2 e3d4 e1c1 c1f4 c1f4 d2d4 b1d2 f1e2 c1d2 d2d4 d2d4 f1d3 e5c6 f1h3
alaric=d2b4 e2e4 c4c5 f1e2 g1e2 c4c5 c1d2 c4c5 h2h3 b3d1 a4d7 d2c4 f1b5 d1d4 g2g4 d4d5 c1d2 d4d5 d2f4 c1f4 d4d5 g2g4 d2c2 d2d4 b1c3 d4e5 c1e3 c1e3 c1g5 f1e1 e2f4 e2f4 a2a3 d2c4 g1h1 b1c3 e4e5 f3d2 d1a4 d1d3 d4e5 c1d2 a2a3 e1g1 c4b5 c3d5 c3d1 e3d4 f1d3 e3d4 c1e3 f3e5 e5d7 a4b5 a4b5 d2d4 b1c3 b1c3 d2d4 b1c3 b2b3 e4e5 e1g1 d2b3 b1c3 b1a3 g2c6 b1c3 e4e5 c6c2 c4d3 c4d3 b1c3 f1d3 d4e5 b1a3 c1e3 b5c6 d1a4 c6c2 e1g1 h2h3 c4c5 e2f4 c4d5 d2d4 b1d2 e3d4 e1c1 c1f4 e4e5 d2d3 b2b3 c1b2 b1d2 d2d3 a2a4 f3h4 e2e3 f1h3
delfi=b1c3 e2e4 f1d3 f1e2 g1f3 h1g1 e1g1 c4d5 e1g1 f1b5 a4d4 d2c4 f1d3 b3d4 e2f4 d1d3 c1f4 e3e4 d1c2 c1f4 c1f4 b2b3 e1g1 d2d4 b1c3 c1d2 c1d2 c1e3 c1e3 c1e3 c1e3 e2f4 e1g1 d2c4 c1e3 b1c3 c1e3 b1c3 e3e4 d1d3 c1g5 c1d2 c1e3 e1g1 c4c5 c3d5 e4e5 e2d4 f1d3 e1g1 c1f4 a1b1 e1g1 b1c3 b1c3 d2d4 e1g1 b1c3 d2d4 e1g1 b1c3 c1f4 e1g1 d2c4 f3d2 b1c3 g2c6 b1c3 c1f4 e1g1 c4e2 c4d3 b1c3 c1f4 d4e5 b1c3 c1e3 b5c6 d2d3 e1g1 e1g1 c1f4 c4c5 e1g1 c4d5 d2d4 b1d2 e3d4 e1c1 c1f4 c1f4 d2d4 b1a3 c1b2 b1c3 d2d4 d2d4 f1d3 d1a4 c1f4
houdini=d1b3 g2g4 f1e2 e3e4 g1e2 c4c5 a2a4 h2h3 h2h3 b3d1 a4d7 d2c4 f1e2 d1d4 a2a3 a1c1 c1d2 d4d5 d4d5 d4d5 c1e3 b2b3 e1g1 d2d4 a4b5 h2h3 c1e3 a2a3 c1g5 c1e3 c1e3 h2h3 e1g1 a2a4 d1e2 b1c3 d3e2 b1d2 d1a4 d1d3 d3d1 d1e2 a2a3 f3g5 c4b5 c3d5 e4e5 a2a3 f1d3 e1g1 c1f4 a3a4 a4c4 a4b5 a4b5 d2d4 e1g1 f3e5 d2d4 b1c3 a2a4 d4d5 e1g1 d2b3 b1c3 d2d3 g2c6 b2b3 e4e5 c1f4 c4d3 c4d3 d2d4 f1c4 d4e5 b1a3 d4c5 b5d3 d1a4 e1g1 e1g1 c1f4 c4c5 e2f4 b1c3 d2d4 e1g1 e3d4 e1c1 c1f4 c1f4 d2d4 b2b3 b1c3 d1a4 d2d3 a2a4 f1d3 e2e3 f1h3
glaurung=d2b4 e2e4 f1d3 f1e2 f1e2 h1g1 e1g1 h2h3 h2h3 b3d1 a4f4 d2c4 e1c1 d1d4 e1c1 e2e4 e2e4 e3e4 d4d5 d4d5 c1f4 e1c1 e1g1 d2d4 a4b5 c1e3 c1g5 c1e3 c1g5 c1e3 c1e3 e2f4 e1g1 d2b3 c1d2 g3g4 a2a3 f3d2 f3d2 d1d3 e2e4 a2a3 f1d1 e1g1 c4b5 c3e2 c3d1 e1g1 f1d3 e1g1 d4d5 f3e5 a4c4 a4b5 b1c3 d2d4 e1g1 b1c3 d2d4 e1g1 a2a4 e4e5 e1g1 d2b3 e1g1 b1a3 g2c6 b1c3 e4e5 c1f4 c4d3 c4b3 f3d4 f1d3 d4e5 b1c3 g2g3 b5c6 d1a4 e1g1 e1g1 h2h3 c4c5 e2f4 c4c5 d2d4 b1d2 f3g4 e1c1 c1f4 e4e5 d2d4 b1a3 f1e2 d1a4 d2d4 d2d4 f1d3 e2e3 f1h3
roce=b2b3 e2e4 f1d3 f1d3 f1e2 c4c5 e1g1 b3e3 b3a3 b3a4 a4d7 d2c4 f1e2 d1d4 e1c1 e2e4 c1g5 e3e4 d4d5 e1f1 e2e4 e1c1 e1g1 a4b5 a4b5 d4e5 c1e3 c1e3 c1e3 g3g4 c1e3 d1d2 d1d2 d3d4 c1d2 b1c3 d3c4 f3d2 f3d2 d1d3 d4e5 c1d2 e2e3 e1g1 c4b5 c3e2 c3d1 e1g1 f1g2 e3d4 d4d5 f3e5 a4c4 a4b5 a4b5 d2d4 e1g1 f3e5 a4b5 e1g1 a2a4 d4d5 e2d4 d2b3 e1g1 e1g1 g2c6 b1c3 e4e5 c6c2 c4d3 c4d3 f3d4 f1d3 d4e5 e1g1 d4c5 b5c6 d1c2 e1g1 e1g1 d4d5 c4c5 e1g1 c4d5 d2d4 e1g1 f3g4 e1c1 e5f6 c1f4 d2d4 b1d2 f1e2 d1a4 d2d4 d2d4 f1e2 e2e3 f2f4
clarabit=d2b4 e2e4 c4d5 b3a4 g1e2 e1g1 e1g1 c4c5 h2h3 b3c2 a4d7 d2c4 f1b5 d1d4 e1c1 d1b3 e2e4 e3e4 d1b3 c1f4 e2e4 e3h6 e1c1 d2d4 b1c3 d1d3 c1e3 d3d4 c1e3 f1e1 c1d2 d3d4 d1d2 f1e1 d1e2 b1c3 c1e3 b1c3 f3d2 d1d3 e2e4 d1e2 c1f4 d4c5 c4b5 c3d5 c3d1 e1c1 e1c1 e3d4 c1e3 c1b2 a4c4 a4b5 a4b5 d2d4 b1c3 d2d3 d2d4 b1c3 a2a4 c1e3 e1g1 d2b3 b1c3 b1c3 g2c6 b1c3 c1e3 e1g1 c4d3 c4d3 d2d4 d1a4 d4e5 e1g1 e2c3 b5c6 d2d3 c6c3 e1g1 h2h3 c4c5 e2f4 c4d5 d2d4 b1d2 f3g4 e1c1 f1e1 f1d3 d2d3 b1a3 c1b2 b1c3 d2d3 d2d4 f3h4 e2e3 f1g2
hannibal=d1c2 e2e4 c1d2 b3a4 g2g4 c4c5 a2a4 h2h3 h2h3 b3d1 a4d4 d2c4 c3b5 d1d4 a1d1 f1e1 c1b2 d4d5 d2f4 c1f4 c1g5 e1c1 a1d1 d2d4 a4b5 f1e1 c1e3 a2a3 f2f3 f1e1 a2a4 f2f3 e1g1 f1e1 g1h1 a2a4 a2a3 a2a4 d1a4 d1d3 d4d5 c1d2 a2a3 e1g1 c4b5 c3e2 c3d1 e1g1 f1d3 e3d4 d1e2 c1e3 a4c4 a4b5 b1c3 d2d4 b1c3 f3e5 d2d4 b1c3 a2a4 c1e3 e1g1 d2b3 b1c3 e1g1 g2c6 b2b3 d1e2 c6c2 c4e2 c4d3 d2d4 b6b7 d4e5 b1a3 c2c3 b5c6 d1a4 c6c3 e1g1 h2h3 c4c5 e1g1 c4d5 d2d4 b1c3 f3g4 e1c1 c1f4 c1e3 d2d4 b2b3 c1b2 b1c3 a2a4 a2a4 f3h4 d1b3 f1h3
irina=d2b4 e2e4 c1d2 e3e4 g2g4 e1g1 e1g1 b3e3 h2h3 b3d1 a4d7 d2c4 d4c5 d1d4 d4d5 d4d5 f3g5 f3d2 d2g5 f3g5 f3g5 e3g5 e1g1 a4b5 a4b5 c1g5 d4c5 c1f4 c1g5 c1g5 c1g5 d3d4 d3d4 h2h3 d3d4 b1c3 e4e5 b1d2 f3d2 d1a4 c1g5 f3g5 c1g5 e1g1 c4b5 c3e2 e4e5 e3d4 d2d3 e3d4 c1g5 c1g5 e5d7 a4b5 a4b5 d2d4 f3g5 f3g5 d2d4 f3g5 a2a4 c1g5 c1g5 d2b3 c1g5 b1a3 g2c6 a2a4 e4e5 c1f4 c4b3 c4b3 f3d4 d1a4 d4e5 b1c3 e2c3 b5c6 f3e1 c1f4 e1g1 h2h3 g5f6 b5c6 c4d5 d2d4 b1d2 f3g4 e3g5 e5f6 e4e5 d2d3 b1d2 c1f4 d1a4 d2d3 d2d3 f4g5 e5c6 c1g5
lime=d2b4 e2e4 f1d3 f1d3 g1e2 c1d2 c1d2 c4c5 h2h3 b3d1 a4f4 d2c4 f1e2 d1d4 g2g3 d4d5 e2e4 e3e4 d4d5 c1f4 e2e4 g2g3 e1g1 d2d4 a4b5 d4e5 c1e3 c1e3 c1f4 c1e3 f1e1 a1b1 d1d2 b2b3 g1h1 b1c3 c1d2 b1d2 d1a4 d1d3 d4e5 d1e2 c2a4 e5d6 c4b5 c3e2 c3d1 e1g1 f1g2 e3d4 d4d5 a3a4 a4c4 a4b5 a4b5 d2d4 e1g1 d2d3 d2d4 b1c3 a2a4 e4e5 e1g1 d2b3 b1c3 e1g1 g2c6 b1c3 e4e5 c1f4 c4d3 c4d3 b1c3 f1d3 d4e5 b1c3 c1e3 b5c6 d1a4 b1d2 e1g1 h2h3 c4c5 e2f4 b1c3 d2d4 b1c3 e3d4 e1c1 c1f4 c1f4 d2d3 b1a3 f1e2 d1a4 d2d3 d2d3 f3h4 e2e3 f1g2
daydreamer=d2g5 e2e4 c1d2 c1d2 g1e2 c4c5 c1d2 f1d1 e1g1 b3d1 a4d7 d2c4 c3b5 d1d4 e1f2 d4d5 d4d5 d4d5 d4d5 c1f4 d1b3 e1f2 e1g1 d2d4 a4b5 d4e5 c1e3 d3d4 d1b3 f1e1 c1g5 a1b1 e1g1 d1e2 f1e1 g1h2 a2a3 f3d2 d1a4 d1d3 d4e5 d1e2 a2a3 e1g1 d1b3 c3e2 c3d1 e1g1 f1g2 e3d4 d4d5 f3e5 e5d7 a4b5 a4b5 d2d4 b1c3 b1c3 d2d4 e1g1 a2a4 a2a3 c1f4 d2b3 e1g1 b1a3 g2c6 a2a4 e4e5 e1g1 c4d3 c4d3 b1c3 f1d3 d4e5 b1a3 e1f2 b5c6 d1a4 c6c2 e1g1 c1e3 c4c5 e2f4 c4d5 d2d4 b1c3 f3g4 e1c1 c1f4 e4e5 d2d4 b2b3 c1b2 c1e3 d2d3 a2a4 f1d3 e5c6 f1g2
tarrasch=d2g5 g2g4 f1d3 g2g4 g2g4 e1g1 c1d2 d2b1 b3a4 b3d1 a4d7 d2c4 f1b5 d1d4 e2f4 d4d5 c1g5 c3d5 d2g5 c1g5 c1g5 e2f4 e1g1 d2d4 a4b5 c1g5 c1g5 c1g5 c1g5 c1g5 c1g5 d3d4 d3d4 d2c4 c3d5 g3g4 d3a6 f3d2 f3d2 d1a4 c1g5 d1c2 c1f4 e1g1 c4b5 c3d5 e4e5 e1g1 e3e4 e1g1 c1g5 c1f4 a4c4 a4b5 a4b5 d2d4 b1c3 d2d3 d2d4 e1g1 f3e5 c1g5 c1g5 d2c4 b3a4 d2d4 g2c6 c1d2 c1g5 b1c3 c4d3 c4d3 d2d4 g2g4 d4e5 d2d4 e2c3 b5c6 d2d3 c1f4 d2d4 c1f4 c4d5 e1g1 c4d5 d2d4 e1g1 e3d4 g2g4 c1f4 c1g5 d2d4 d4a7 f1e2 c1e3 d2d4 d2d4 f1d3 e2e3 c1g5
cinnamon=d2b4 c1g5 c4d5 c1d2 g2g4 c4d5 c1d2 b3e3 b3a4 b3a4 a4f4 d2c4 f1b5 d1d4 a2a3 d4d5 f3g5 d4d5 d4d5 d4d5 c1f4 e2g3 h2h3 d2d4 a4b5 d4e5 c1e3 c1e3 c1g5 c1e3 c1e3 d1d2 d1d2 d2b3 c1d2 g3g4 d3a6 f3d2 f3d2 d1a4 d4e5 d1e2 f1d1 d4c5 c4b5 c3e2 c3d1 e3d4 f1d3 e3d4 c1e3 c1g5 e5d7 a4b5 a4b5 d2d4 b1c3 d2d3 d2d4 d1e2 a2a4 c1g5 c1f4 d2b3 b1a3 b1c3 g2c6 f3g5 e4e5 c1f4 c4d3 c4b3 b3e3 b6b7 d4e5 d2d3 c1e3 b5c6 f3e1 c6c2 d2d3 f1e1 c4c5 a2a3 c4d5 d2d4 b1d2 e3d4 a2a3 c1f4 f1e2 d2d4 b1a3 c1e3 d1a4 d2d4 d2d4 f1e2 e2e3 c1g5
godel=d1c2 e2e4 c4c5 e3e4 g2g4 c4c5 a2a4 h2h3 h2h3 b3d1 a4d7 d2c4 c3b5 d1d4 b2b3 f1e1 f1e1 b2b3 d4d5 a1b1 f1e1 g2g4 e1g1 d2d4 a4b5 f1e1 a2a3 d3d4 h2h3 c1e3 c1e3 h2h3 a2a3 f1e1 c1d2 c2c4 h2h3 d1e2 d1a4 d1d3 d4e5 c1d2 e2e3 e1g1 c4b5 c3d5 c3d1 a1b1 f1e2 e3d4 d4d5 c1b2 e5d7 a4b5 a4b5 d2d4 b1c3 c2c3 d2d4 b1c3 b2b3 c1e3 e1g1 c2c4 b1c3 d2d3 g2c6 b2b3 e4e5 c1f4 c4e2 c4d3 d2d4 f1d3 d4e5 d2d3 e1f2 b5d3 d1a4 c6c2 d2d3 h2h3 c4c5 e2f4 c4c5 d2d4 b1d2 e3d4 e1c1 c1f4 e4e5 d2d4 b2b3 h2h3 c1e3 a2a4 h2h3 f1e2 e2e3 f1g2
rocinante=d1b3 e2e4 f1d3 f1e2 g1f3 c4c5 c1d2 c4c5 h2h3 b3a4 a4d7 d2c4 c3b5 d1d4 a1d1 e2e4 e2e4 e3e4 d4d5 d4d5 c1f4 g2g4 e1g1 d2d4 a4b5 c1d2 d1d3 c1e3 c1d2 c1d2 a2a4 d3d4 d3d4 d2b3 c1e3 b1c3 c1e3 f3d2 d1a4 d1d3 c1d2 f3d2 a2a3 c1e3 d1b3 c3e2 c3d1 a2a3 a1d1 e3d4 a2a4 c1f4 a4c4 a4b5 a4b5 d2d4 b1c3 d1e2 d2d4 b1c3 b2b3 a2a3 c1g5 d2b3 b1a3 b1c3 d2c4 b1c3 a2a3 c1f4 c4d3 c4d3 d2d4 f1d3 d4e5 b1c3 g2g3 b5c6 d1a4 c6c3 c2a4 c3e2 c4c5 e2f4 c4d5 d2d4 b1d2 e3d4 g2g4 c1f4 e4e5 d2d3 b2b3 c1f4 d1a4 d2d4 d2d3 f3h4 e2e3 f1h3
    """
    dic = {}
    for linea in result.split("\n"):
        if linea.strip():
            clave, lista = linea.strip().split("=")
            if clave in dicRivales:
                dic[clave] = lista.split(" ")

    li = dic.keys()
    li.sort(key=lambda x:dicRivales[x].elo)
    d = {}
    lenli = len(li)
    for nn in range(lenli-1):
        ori = li[nn]
        for nz in range(nn+1, lenli):
            dest = li[nz]
            n = 0
            liori = dic[ori]
            lidest = dic[dest]
            for x in range(100):
                if liori[x] == lidest[x]:
                    n += 1
            if ori > dest:
                s = (dest,ori)
            else:
                s = (ori,dest)
            d[s] = n
    return d


def filtra(dicRivales):
    d = relaciones(dicRivales)
    stkeys = set()
    for uno, dos in d:
        stkeys.add(uno)
        stkeys.add(dos)
    dresp = {k:v for k,v in dicRivales.iteritems() if k in stkeys}
    return dresp


def getLista(configuracion, num_elementos):
    def haz(xdrel, clave):
        s = set()
        s.add(clave)
        total = 0
        for repet in range(num_elementos-1):
            resp = None
            mx = 999999999
            for ori, dest in xdrel:
                if ori == clave:
                    otro = dest
                elif dest == clave:
                    otro = ori
                else:
                    continue
                if otro not in s:
                    x = xdrel[(ori,dest)]
                    if x < mx:
                        resp = otro
                        mx = x
            if resp == None:
                pass
            s.add(resp)
            total += mx
        li = list(s)
        return li, total

    dicRivales = configuracion.dicRivales
    drel = relaciones(dicRivales)

    num_rivales = len(dicRivales)
    num_quitar = num_rivales / 2
    if num_elementos > num_quitar:
        num_quitar = num_rivales-num_elementos-1

    usados = {}
    stkeys = set()
    for x, y in drel:
        usados[x] = 0
        usados[y] = 0
        stkeys.add(x)
        stkeys.add(y)

    desde_quitar = num_rivales-num_quitar if num_quitar > 0 else 0
    quitar = set()
    dlista = {}
    for clave in stkeys:
        li = [(k,v) for k,v in usados.iteritems()]
        li.sort(key=lambda x: x[1])
        if desde_quitar > 0:
            quitar = set(x for x,t in li[desde_quitar:])
            if clave in quitar:
                quitar.remove(clave)

        drel1 = {}
        for k in drel:
            if k[0] in quitar or k[1] in quitar:
                continue
            drel1[k] = drel[k]
        s, total = haz(drel1, clave)
        for key in s:
            usados[key] += 1
        s.sort(key=lambda x:dicRivales[x].elo)
        dlista[clave] = s
    return dlista


def getListaClave(configuracion, num_elementos, clave):
    dlista = getLista(configuracion, num_elementos)
    return dlista[clave]

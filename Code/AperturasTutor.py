

class JuegaAperturaStd:
    def __init__(self, bloque):
        self.bloque = bloque
        self.liMovs = bloque.a1h8.split(" ")
        self.activa = True
        self.movActual = -1
        self.ultMov = len(self.liMovs) - 1

    def juegaMotor(self):
        self.movActual += 1
        if self.movActual > self.ultMov:
            self.activa = False
            return False, None, None, None

        if not self.activa:
            return False, None, None, None

        movimiento = self.liMovs[self.movActual]
        return True, movimiento[:2], movimiento[2:], None

    def compruebaHumano(self, desde, hasta):
        if (self.movActual + 1) > self.ultMov:
            self.activa = False
            return False
        movimiento = self.liMovs[self.movActual + 1]
        desdem, hastam = movimiento[:2], movimiento[2:]
        if desde == desdem and hasta == hastam:
            return True
        else:
            return False

    def desdeHastaActual(self):
        movimiento = self.liMovs[self.movActual + 1]
        return movimiento[:2], movimiento[2:]

    def haJugadoHumano(self, desde, hasta):
        self.movActual += 1
        if not self.activa:
            return False
        self.activa = self.ultMov > self.movActual
        return self.activa

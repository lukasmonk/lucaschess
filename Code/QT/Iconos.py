"""Iconos y pixmap usados en el programa"""
from PyQt4 import QtGui

f = open("IntFiles/Iconos.bin","rb")
binIconos = f.read()
f.close()

def icono( name ):
    return eval( "%s()"%name )

def pixmap( name ):
    return eval( "pm%s()"%name )

def PM( desde, hasta ):
    pm = QtGui.QPixmap()
    pm.loadFromData( binIconos[desde:hasta] )
    return pm

def pmLM():
    return PM(0,1033)

def LM():
    return QtGui.QIcon( pmLM() )

def pmAplicacion():
    return PM(1033,1660)

def Aplicacion():
    return QtGui.QIcon( pmAplicacion() )

def pmAplicacion64():
    return PM(1660,5764)

def Aplicacion64():
    return QtGui.QIcon( pmAplicacion64() )

def pmDatos():
    return PM(5764,6496)

def Datos():
    return QtGui.QIcon( pmDatos() )

def pmTutor():
    return PM(6496,8345)

def Tutor():
    return QtGui.QIcon( pmTutor() )

def pmTablero():
    return PM(5764,6496)

def Tablero():
    return QtGui.QIcon( pmTablero() )

def pmPartidaOriginal():
    return PM(8345,10151)

def PartidaOriginal():
    return QtGui.QIcon( pmPartidaOriginal() )

def pmDGT():
    return PM(10151,10981)

def DGT():
    return QtGui.QIcon( pmDGT() )

def pmZonaPrincipiantes():
    return PM(10981,12784)

def ZonaPrincipiantes():
    return QtGui.QIcon( pmZonaPrincipiantes() )

def pmJ60():
    return PM(12784,14114)

def J60():
    return QtGui.QIcon( pmJ60() )

def pmTamTablero():
    return PM(12784,14114)

def TamTablero():
    return QtGui.QIcon( pmTamTablero() )

def pmMensEspera():
    return PM(14114,20267)

def MensEspera():
    return QtGui.QIcon( pmMensEspera() )

def pmUtilidades():
    return PM(20267,25486)

def Utilidades():
    return QtGui.QIcon( pmUtilidades() )

def pmTerminar():
    return PM(25486,27071)

def Terminar():
    return QtGui.QIcon( pmTerminar() )

def pmNuevaPartida():
    return PM(27071,28639)

def NuevaPartida():
    return QtGui.QIcon( pmNuevaPartida() )

def pmOpciones():
    return PM(28639,32809)

def Opciones():
    return QtGui.QIcon( pmOpciones() )

def pmEntrenamiento():
    return PM(6496,8345)

def Entrenamiento():
    return QtGui.QIcon( pmEntrenamiento() )

def pmAplazar():
    return PM(32809,35552)

def Aplazar():
    return QtGui.QIcon( pmAplazar() )

def pmCapturas():
    return PM(35552,37405)

def Capturas():
    return QtGui.QIcon( pmCapturas() )

def pmReiniciar():
    return PM(37405,41724)

def Reiniciar():
    return QtGui.QIcon( pmReiniciar() )

def pmMotores():
    return PM(41724,46636)

def Motores():
    return QtGui.QIcon( pmMotores() )

def pmImportarGM():
    return PM(46636,49059)

def ImportarGM():
    return QtGui.QIcon( pmImportarGM() )

def pmAbandonar():
    return PM(49059,52845)

def Abandonar():
    return QtGui.QIcon( pmAbandonar() )

def pmEmpezar():
    return PM(52845,54649)

def Empezar():
    return QtGui.QIcon( pmEmpezar() )

def pmOtros():
    return PM(54649,57848)

def Otros():
    return QtGui.QIcon( pmOtros() )

def pmAnalizar():
    return PM(57848,59209)

def Analizar():
    return QtGui.QIcon( pmAnalizar() )

def pmMainMenu():
    return PM(59209,63347)

def MainMenu():
    return QtGui.QIcon( pmMainMenu() )

def pmFinPartida():
    return PM(63347,66031)

def FinPartida():
    return QtGui.QIcon( pmFinPartida() )

def pmGrabar():
    return PM(66031,67385)

def Grabar():
    return QtGui.QIcon( pmGrabar() )

def pmGrabarComo():
    return PM(67385,69269)

def GrabarComo():
    return QtGui.QIcon( pmGrabarComo() )

def pmRecuperar():
    return PM(69269,71796)

def Recuperar():
    return QtGui.QIcon( pmRecuperar() )

def pmInformacion():
    return PM(71796,73563)

def Informacion():
    return QtGui.QIcon( pmInformacion() )

def pmNuevo():
    return PM(73563,74387)

def Nuevo():
    return QtGui.QIcon( pmNuevo() )

def pmCopiar():
    return PM(74387,75623)

def Copiar():
    return QtGui.QIcon( pmCopiar() )

def pmModificar():
    return PM(75623,79411)

def Modificar():
    return QtGui.QIcon( pmModificar() )

def pmBorrar():
    return PM(79411,85190)

def Borrar():
    return QtGui.QIcon( pmBorrar() )

def pmMarcar():
    return PM(85190,89160)

def Marcar():
    return QtGui.QIcon( pmMarcar() )

def pmPegar():
    return PM(89160,91203)

def Pegar():
    return QtGui.QIcon( pmPegar() )

def pmFichero():
    return PM(91203,95118)

def Fichero():
    return QtGui.QIcon( pmFichero() )

def pmNuestroFichero():
    return PM(95118,97802)

def NuestroFichero():
    return QtGui.QIcon( pmNuestroFichero() )

def pmFicheroRepite():
    return PM(97802,99145)

def FicheroRepite():
    return QtGui.QIcon( pmFicheroRepite() )

def pmInformacionPGN():
    return PM(99145,102854)

def InformacionPGN():
    return QtGui.QIcon( pmInformacionPGN() )

def pmVer():
    return PM(102854,104161)

def Ver():
    return QtGui.QIcon( pmVer() )

def pmInicio():
    return PM(104161,105774)

def Inicio():
    return QtGui.QIcon( pmInicio() )

def pmFinal():
    return PM(105774,107358)

def Final():
    return QtGui.QIcon( pmFinal() )

def pmFiltrar():
    return PM(107358,113624)

def Filtrar():
    return QtGui.QIcon( pmFiltrar() )

def pmArriba():
    return PM(113624,115440)

def Arriba():
    return QtGui.QIcon( pmArriba() )

def pmAbajo():
    return PM(115440,117266)

def Abajo():
    return QtGui.QIcon( pmAbajo() )

def pmEstadisticas():
    return PM(117266,119731)

def Estadisticas():
    return QtGui.QIcon( pmEstadisticas() )

def pmRendirse():
    return PM(119731,123274)

def Rendirse():
    return QtGui.QIcon( pmRendirse() )

def pmCheck():
    return PM(123274,125489)

def Check():
    return QtGui.QIcon( pmCheck() )

def pmTablas():
    return PM(125489,126887)

def Tablas():
    return QtGui.QIcon( pmTablas() )

def pmAtras():
    return PM(126887,128296)

def Atras():
    return QtGui.QIcon( pmAtras() )

def pmBuscar():
    return PM(128296,130048)

def Buscar():
    return QtGui.QIcon( pmBuscar() )

def pmLibros():
    return PM(130048,132000)

def Libros():
    return QtGui.QIcon( pmLibros() )

def pmAceptar():
    return PM(132000,134790)

def Aceptar():
    return QtGui.QIcon( pmAceptar() )

def pmCancelar():
    return PM(134790,136859)

def Cancelar():
    return QtGui.QIcon( pmCancelar() )

def pmDefecto():
    return PM(136859,139853)

def Defecto():
    return QtGui.QIcon( pmDefecto() )

def pmGenerar():
    return PM(139853,142772)

def Generar():
    return QtGui.QIcon( pmGenerar() )

def pmInsertar():
    return PM(142772,144936)

def Insertar():
    return QtGui.QIcon( pmInsertar() )

def pmJugar():
    return PM(144936,146896)

def Jugar():
    return QtGui.QIcon( pmJugar() )

def pmConfigurar():
    return PM(146896,149753)

def Configurar():
    return QtGui.QIcon( pmConfigurar() )

def pmS_Aceptar():
    return PM(132000,134790)

def S_Aceptar():
    return QtGui.QIcon( pmS_Aceptar() )

def pmS_Cancelar():
    return PM(134790,136859)

def S_Cancelar():
    return QtGui.QIcon( pmS_Cancelar() )

def pmS_Microfono():
    return PM(149753,154759)

def S_Microfono():
    return QtGui.QIcon( pmS_Microfono() )

def pmS_LeerWav():
    return PM(46636,49059)

def S_LeerWav():
    return QtGui.QIcon( pmS_LeerWav() )

def pmS_Play():
    return PM(154759,159814)

def S_Play():
    return QtGui.QIcon( pmS_Play() )

def pmS_StopPlay():
    return PM(159814,160157)

def S_StopPlay():
    return QtGui.QIcon( pmS_StopPlay() )

def pmS_StopMicrofono():
    return PM(159814,160157)

def S_StopMicrofono():
    return QtGui.QIcon( pmS_StopMicrofono() )

def pmS_Record():
    return PM(160157,163138)

def S_Record():
    return QtGui.QIcon( pmS_Record() )

def pmS_Limpiar():
    return PM(79411,85190)

def S_Limpiar():
    return QtGui.QIcon( pmS_Limpiar() )

def pmHistorial():
    return PM(163138,166836)

def Historial():
    return QtGui.QIcon( pmHistorial() )

def pmPegar16():
    return PM(166836,167441)

def Pegar16():
    return QtGui.QIcon( pmPegar16() )

def pmRivalesMP():
    return PM(167441,168261)

def RivalesMP():
    return QtGui.QIcon( pmRivalesMP() )

def pmCamara():
    return PM(168261,172152)

def Camara():
    return QtGui.QIcon( pmCamara() )

def pmUsuarios():
    return PM(172152,173048)

def Usuarios():
    return QtGui.QIcon( pmUsuarios() )

def pmResistencia():
    return PM(173048,175637)

def Resistencia():
    return QtGui.QIcon( pmResistencia() )

def pmRemoto():
    return PM(175637,179511)

def Remoto():
    return QtGui.QIcon( pmRemoto() )

def pmRemotoServidor():
    return PM(179511,180227)

def RemotoServidor():
    return QtGui.QIcon( pmRemotoServidor() )

def pmRemotoCliente():
    return PM(180227,181134)

def RemotoCliente():
    return QtGui.QIcon( pmRemotoCliente() )

def pmCebra():
    return PM(181134,183341)

def Cebra():
    return QtGui.QIcon( pmCebra() )

def pmGafas():
    return PM(183341,184097)

def Gafas():
    return QtGui.QIcon( pmGafas() )

def pmPuente():
    return PM(184097,184492)

def Puente():
    return QtGui.QIcon( pmPuente() )

def pmWeb():
    return PM(184492,185294)

def Web():
    return QtGui.QIcon( pmWeb() )

def pmMail():
    return PM(185294,185874)

def Mail():
    return QtGui.QIcon( pmMail() )

def pmAyuda():
    return PM(185874,186723)

def Ayuda():
    return QtGui.QIcon( pmAyuda() )

def pmFAQ():
    return PM(186723,187855)

def FAQ():
    return QtGui.QIcon( pmFAQ() )

def pmPuntuacion():
    return PM(187855,188464)

def Puntuacion():
    return QtGui.QIcon( pmPuntuacion() )

def pmActualiza():
    return PM(188464,189038)

def Actualiza():
    return QtGui.QIcon( pmActualiza() )

def pmRefresh():
    return PM(189038,191220)

def Refresh():
    return QtGui.QIcon( pmRefresh() )

def pmJuegaSolo():
    return PM(191220,192051)

def JuegaSolo():
    return QtGui.QIcon( pmJuegaSolo() )

def pmPlayer():
    return PM(191220,192051)

def Player():
    return QtGui.QIcon( pmPlayer() )

def pmJS_Rotacion():
    return PM(192051,193669)

def JS_Rotacion():
    return QtGui.QIcon( pmJS_Rotacion() )

def pmCoordina():
    return PM(192051,193669)

def Coordina():
    return QtGui.QIcon( pmCoordina() )

def pmEstrellaAzul():
    return PM(193669,194987)

def EstrellaAzul():
    return QtGui.QIcon( pmEstrellaAzul() )

def pmElo():
    return PM(193669,194987)

def Elo():
    return QtGui.QIcon( pmElo() )

def pmMate():
    return PM(194987,195386)

def Mate():
    return QtGui.QIcon( pmMate() )

def pmEloTimed():
    return PM(195386,196518)

def EloTimed():
    return QtGui.QIcon( pmEloTimed() )

def pmPGN():
    return PM(196518,198294)

def PGN():
    return QtGui.QIcon( pmPGN() )

def pmPGN_Importar():
    return PM(198294,198723)

def PGN_Importar():
    return QtGui.QIcon( pmPGN_Importar() )

def pmAyudaGR():
    return PM(198723,204314)

def AyudaGR():
    return QtGui.QIcon( pmAyudaGR() )

def pmBotonAyuda():
    return PM(204314,206626)

def BotonAyuda():
    return QtGui.QIcon( pmBotonAyuda() )

def pmColores():
    return PM(206626,207511)

def Colores():
    return QtGui.QIcon( pmColores() )

def pmEditarColores():
    return PM(207511,212263)

def EditarColores():
    return QtGui.QIcon( pmEditarColores() )

def pmGranMaestro():
    return PM(212263,212808)

def GranMaestro():
    return QtGui.QIcon( pmGranMaestro() )

def pmFavoritos():
    return PM(212808,214369)

def Favoritos():
    return QtGui.QIcon( pmFavoritos() )

def pmCarpeta():
    return PM(198294,198723)

def Carpeta():
    return QtGui.QIcon( pmCarpeta() )

def pmDivision():
    return PM(214369,214837)

def Division():
    return QtGui.QIcon( pmDivision() )

def pmDivisionF():
    return PM(214837,215602)

def DivisionF():
    return QtGui.QIcon( pmDivisionF() )

def pmKibitzer():
    return PM(215602,215896)

def Kibitzer():
    return QtGui.QIcon( pmKibitzer() )

def pmKibitzer_Pausa():
    return PM(215896,216494)

def Kibitzer_Pausa():
    return QtGui.QIcon( pmKibitzer_Pausa() )

def pmKibitzer_Continuar():
    return PM(216494,217096)

def Kibitzer_Continuar():
    return QtGui.QIcon( pmKibitzer_Continuar() )

def pmKibitzer_Terminar():
    return PM(217096,217748)

def Kibitzer_Terminar():
    return QtGui.QIcon( pmKibitzer_Terminar() )

def pmDelete():
    return PM(217096,217748)

def Delete():
    return QtGui.QIcon( pmDelete() )

def pmModificarP():
    return PM(217748,218523)

def ModificarP():
    return QtGui.QIcon( pmModificarP() )

def pmGrupo_Si():
    return PM(218523,218752)

def Grupo_Si():
    return QtGui.QIcon( pmGrupo_Si() )

def pmGrupo_No():
    return PM(218752,218924)

def Grupo_No():
    return QtGui.QIcon( pmGrupo_No() )

def pmMotor_Si():
    return PM(218924,219159)

def Motor_Si():
    return QtGui.QIcon( pmMotor_Si() )

def pmMotor_No():
    return PM(217096,217748)

def Motor_No():
    return QtGui.QIcon( pmMotor_No() )

def pmMotor_Actual():
    return PM(219159,219879)

def Motor_Actual():
    return QtGui.QIcon( pmMotor_Actual() )

def pmMotor():
    return PM(219879,220338)

def Motor():
    return QtGui.QIcon( pmMotor() )

def pmMoverInicio():
    return PM(220338,220942)

def MoverInicio():
    return QtGui.QIcon( pmMoverInicio() )

def pmMoverFinal():
    return PM(220942,221563)

def MoverFinal():
    return QtGui.QIcon( pmMoverFinal() )

def pmMoverAdelante():
    return PM(221563,222170)

def MoverAdelante():
    return QtGui.QIcon( pmMoverAdelante() )

def pmMoverAtras():
    return PM(222170,222784)

def MoverAtras():
    return QtGui.QIcon( pmMoverAtras() )

def pmMoverLibre():
    return PM(222784,223322)

def MoverLibre():
    return QtGui.QIcon( pmMoverLibre() )

def pmMoverTiempo():
    return PM(223322,224148)

def MoverTiempo():
    return QtGui.QIcon( pmMoverTiempo() )

def pmMoverMas():
    return PM(224148,224896)

def MoverMas():
    return QtGui.QIcon( pmMoverMas() )

def pmMoverGrabar():
    return PM(224896,225481)

def MoverGrabar():
    return QtGui.QIcon( pmMoverGrabar() )

def pmMoverGrabarTodos():
    return PM(225481,226129)

def MoverGrabarTodos():
    return QtGui.QIcon( pmMoverGrabarTodos() )

def pmMoverJugar():
    return PM(216494,217096)

def MoverJugar():
    return QtGui.QIcon( pmMoverJugar() )

def pmPelicula():
    return PM(226129,228080)

def Pelicula():
    return QtGui.QIcon( pmPelicula() )

def pmPelicula_Pausa():
    return PM(228080,232219)

def Pelicula_Pausa():
    return QtGui.QIcon( pmPelicula_Pausa() )

def pmPelicula_Seguir():
    return PM(232219,236453)

def Pelicula_Seguir():
    return QtGui.QIcon( pmPelicula_Seguir() )

def pmPelicula_Rapido():
    return PM(236453,240688)

def Pelicula_Rapido():
    return QtGui.QIcon( pmPelicula_Rapido() )

def pmPelicula_Lento():
    return PM(240688,244819)

def Pelicula_Lento():
    return QtGui.QIcon( pmPelicula_Lento() )

def pmPelicula_Repetir():
    return PM(37405,41724)

def Pelicula_Repetir():
    return QtGui.QIcon( pmPelicula_Repetir() )

def pmMemoria():
    return PM(244819,246577)

def Memoria():
    return QtGui.QIcon( pmMemoria() )

def pmEntrenar():
    return PM(246577,247931)

def Entrenar():
    return QtGui.QIcon( pmEntrenar() )

def pmEnviar():
    return PM(246577,247931)

def Enviar():
    return QtGui.QIcon( pmEnviar() )

def pmTrasteros():
    return PM(247931,252462)

def Trasteros():
    return QtGui.QIcon( pmTrasteros() )

def pmTrastero():
    return PM(252462,252692)

def Trastero():
    return QtGui.QIcon( pmTrastero() )

def pmTrastero_Quitar():
    return PM(217096,217748)

def Trastero_Quitar():
    return QtGui.QIcon( pmTrastero_Quitar() )

def pmTrastero_Nuevo():
    return PM(252692,254030)

def Trastero_Nuevo():
    return QtGui.QIcon( pmTrastero_Nuevo() )

def pmNuevoMas():
    return PM(252692,254030)

def NuevoMas():
    return QtGui.QIcon( pmNuevoMas() )

def pmTemas():
    return PM(254030,256058)

def Temas():
    return QtGui.QIcon( pmTemas() )

def pmTutorialesCrear():
    return PM(256058,263378)

def TutorialesCrear():
    return QtGui.QIcon( pmTutorialesCrear() )

def pmMover():
    return PM(263378,263724)

def Mover():
    return QtGui.QIcon( pmMover() )

def pmSeleccionado():
    return PM(263378,263724)

def Seleccionado():
    return QtGui.QIcon( pmSeleccionado() )

def pmSeleccionar():
    return PM(263724,268545)

def Seleccionar():
    return QtGui.QIcon( pmSeleccionar() )

def pmVista():
    return PM(268545,270291)

def Vista():
    return QtGui.QIcon( pmVista() )

def pmInformacionPGNUno():
    return PM(270291,271383)

def InformacionPGNUno():
    return QtGui.QIcon( pmInformacionPGNUno() )

def pmNivel():
    return PM(271383,273453)

def Nivel():
    return QtGui.QIcon( pmNivel() )

def pmJuegaPorMi():
    return PM(273453,274925)

def JuegaPorMi():
    return QtGui.QIcon( pmJuegaPorMi() )

def pmArbol():
    return PM(274925,276441)

def Arbol():
    return QtGui.QIcon( pmArbol() )

def pmGrabarFichero():
    return PM(66031,67385)

def GrabarFichero():
    return QtGui.QIcon( pmGrabarFichero() )

def pmClip():
    return PM(276441,278398)

def Clip():
    return QtGui.QIcon( pmClip() )

def pmFics():
    return PM(278398,278799)

def Fics():
    return QtGui.QIcon( pmFics() )

def pmFide():
    return PM(8345,10151)

def Fide():
    return QtGui.QIcon( pmFide() )

def pmFlechas():
    return PM(278799,281628)

def Flechas():
    return QtGui.QIcon( pmFlechas() )

def pmMarcos():
    return PM(281628,282865)

def Marcos():
    return QtGui.QIcon( pmMarcos() )

def pmSVGs():
    return PM(282865,286150)

def SVGs():
    return QtGui.QIcon( pmSVGs() )

def pmAmarillo():
    return PM(286150,286911)

def Amarillo():
    return QtGui.QIcon( pmAmarillo() )

def pmNaranja():
    return PM(286911,287704)

def Naranja():
    return QtGui.QIcon( pmNaranja() )

def pmVerde():
    return PM(287704,288560)

def Verde():
    return QtGui.QIcon( pmVerde() )

def pmAzul():
    return PM(288560,289290)

def Azul():
    return QtGui.QIcon( pmAzul() )

def pmMagenta():
    return PM(289290,290158)

def Magenta():
    return QtGui.QIcon( pmMagenta() )

def pmRojo():
    return PM(290158,290950)

def Rojo():
    return QtGui.QIcon( pmRojo() )

def pmGris():
    return PM(290950,291745)

def Gris():
    return QtGui.QIcon( pmGris() )

def pmEstrella():
    return PM(187855,188464)

def Estrella():
    return QtGui.QIcon( pmEstrella() )

def pmVerde32():
    return PM(291745,293676)

def Verde32():
    return QtGui.QIcon( pmVerde32() )

def pmRojo32():
    return PM(293676,295302)

def Rojo32():
    return QtGui.QIcon( pmRojo32() )

def pmPuntoBlanco():
    return PM(295302,295503)

def PuntoBlanco():
    return QtGui.QIcon( pmPuntoBlanco() )

def pmPuntoAmarillo():
    return PM(218523,218752)

def PuntoAmarillo():
    return QtGui.QIcon( pmPuntoAmarillo() )

def pmPuntoNaranja():
    return PM(295503,295730)

def PuntoNaranja():
    return QtGui.QIcon( pmPuntoNaranja() )

def pmPuntoVerde():
    return PM(218924,219159)

def PuntoVerde():
    return QtGui.QIcon( pmPuntoVerde() )

def pmPuntoAzul():
    return PM(252462,252692)

def PuntoAzul():
    return QtGui.QIcon( pmPuntoAzul() )

def pmPuntoMagenta():
    return PM(295730,296024)

def PuntoMagenta():
    return QtGui.QIcon( pmPuntoMagenta() )

def pmPuntoRojo():
    return PM(296024,296311)

def PuntoRojo():
    return QtGui.QIcon( pmPuntoRojo() )

def pmPuntoNegro():
    return PM(218752,218924)

def PuntoNegro():
    return QtGui.QIcon( pmPuntoNegro() )

def pmPuntoEstrella():
    return PM(296311,296529)

def PuntoEstrella():
    return QtGui.QIcon( pmPuntoEstrella() )

def pmComentario():
    return PM(296529,296873)

def Comentario():
    return QtGui.QIcon( pmComentario() )

def pmComentarioMas():
    return PM(296873,297366)

def ComentarioMas():
    return QtGui.QIcon( pmComentarioMas() )

def pmComentarioEditar():
    return PM(224896,225481)

def ComentarioEditar():
    return QtGui.QIcon( pmComentarioEditar() )

def pmApertura():
    return PM(297366,298117)

def Apertura():
    return QtGui.QIcon( pmApertura() )

def pmAperturaComentario():
    return PM(298117,298904)

def AperturaComentario():
    return QtGui.QIcon( pmAperturaComentario() )

def pmBookGuide():
    return PM(298904,299575)

def BookGuide():
    return QtGui.QIcon( pmBookGuide() )

def pmMas():
    return PM(299575,299889)

def Mas():
    return QtGui.QIcon( pmMas() )

def pmMasR():
    return PM(299889,300289)

def MasR():
    return QtGui.QIcon( pmMasR() )

def pmMasDoc():
    return PM(300289,303481)

def MasDoc():
    return QtGui.QIcon( pmMasDoc() )

def pmPotencia():
    return PM(188464,189038)

def Potencia():
    return QtGui.QIcon( pmPotencia() )

def pmSorpresa():
    return PM(303481,304222)

def Sorpresa():
    return QtGui.QIcon( pmSorpresa() )

def pmSonrisa():
    return PM(304222,304947)

def Sonrisa():
    return QtGui.QIcon( pmSonrisa() )

def pmBMT():
    return PM(304947,305570)

def BMT():
    return QtGui.QIcon( pmBMT() )

def pmCorazon():
    return PM(305570,307289)

def Corazon():
    return QtGui.QIcon( pmCorazon() )

def pmOjo():
    return PM(307289,308087)

def Ojo():
    return QtGui.QIcon( pmOjo() )

def pmOcultar():
    return PM(307289,308087)

def Ocultar():
    return QtGui.QIcon( pmOcultar() )

def pmMostrar():
    return PM(308087,308869)

def Mostrar():
    return QtGui.QIcon( pmMostrar() )

def pmBlog():
    return PM(308869,309200)

def Blog():
    return QtGui.QIcon( pmBlog() )

def pmVariantes():
    return PM(309200,309875)

def Variantes():
    return QtGui.QIcon( pmVariantes() )

def pmVariantesG():
    return PM(309875,312132)

def VariantesG():
    return QtGui.QIcon( pmVariantesG() )

def pmCambiar():
    return PM(312132,316549)

def Cambiar():
    return QtGui.QIcon( pmCambiar() )

def pmAnterior():
    return PM(316549,320750)

def Anterior():
    return QtGui.QIcon( pmAnterior() )

def pmSiguiente():
    return PM(320750,324956)

def Siguiente():
    return QtGui.QIcon( pmSiguiente() )

def pmSiguienteF():
    return PM(324956,326890)

def SiguienteF():
    return QtGui.QIcon( pmSiguienteF() )

def pmAnteriorF():
    return PM(326890,328832)

def AnteriorF():
    return QtGui.QIcon( pmAnteriorF() )

def pmX():
    return PM(328832,329663)

def X():
    return QtGui.QIcon( pmX() )

def pmTools():
    return PM(329663,331852)

def Tools():
    return QtGui.QIcon( pmTools() )

def pmTacticas():
    return PM(331852,334257)

def Tacticas():
    return QtGui.QIcon( pmTacticas() )

def pmCancelarPeque():
    return PM(334257,334844)

def CancelarPeque():
    return QtGui.QIcon( pmCancelarPeque() )

def pmAceptarPeque():
    return PM(219159,219879)

def AceptarPeque():
    return QtGui.QIcon( pmAceptarPeque() )

def pmP_16c():
    return PM(334844,335270)

def P_16c():
    return QtGui.QIcon( pmP_16c() )

def pmLibre():
    return PM(335270,337488)

def Libre():
    return QtGui.QIcon( pmLibre() )

def pmEnBlanco():
    return PM(337488,338019)

def EnBlanco():
    return QtGui.QIcon( pmEnBlanco() )

def pmDirector():
    return PM(338019,340707)

def Director():
    return QtGui.QIcon( pmDirector() )

def pmTorneos():
    return PM(340707,342052)

def Torneos():
    return QtGui.QIcon( pmTorneos() )

def pmAperturas():
    return PM(342052,342744)

def Aperturas():
    return QtGui.QIcon( pmAperturas() )

def pmV_Blancas():
    return PM(342744,342895)

def V_Blancas():
    return QtGui.QIcon( pmV_Blancas() )

def pmV_Blancas_Mas():
    return PM(342895,343046)

def V_Blancas_Mas():
    return QtGui.QIcon( pmV_Blancas_Mas() )

def pmV_Blancas_Mas_Mas():
    return PM(343046,343190)

def V_Blancas_Mas_Mas():
    return QtGui.QIcon( pmV_Blancas_Mas_Mas() )

def pmV_Negras():
    return PM(343190,343341)

def V_Negras():
    return QtGui.QIcon( pmV_Negras() )

def pmV_Negras_Mas():
    return PM(343341,343490)

def V_Negras_Mas():
    return QtGui.QIcon( pmV_Negras_Mas() )

def pmV_Negras_Mas_Mas():
    return PM(343490,343634)

def V_Negras_Mas_Mas():
    return QtGui.QIcon( pmV_Negras_Mas_Mas() )

def pmV_Blancas_Igual_Negras():
    return PM(343634,343804)

def V_Blancas_Igual_Negras():
    return QtGui.QIcon( pmV_Blancas_Igual_Negras() )

def pmMezclar():
    return PM(142772,144936)

def Mezclar():
    return QtGui.QIcon( pmMezclar() )

def pmVoyager():
    return PM(343804,345486)

def Voyager():
    return QtGui.QIcon( pmVoyager() )

def pmReindexar():
    return PM(345486,347127)

def Reindexar():
    return QtGui.QIcon( pmReindexar() )

def pmRename():
    return PM(347127,348504)

def Rename():
    return QtGui.QIcon( pmRename() )

def pmJG_Mala():
    return PM(348504,349221)

def JG_Mala():
    return QtGui.QIcon( pmJG_Mala() )

def pmJG_Buena():
    return PM(349221,349625)

def JG_Buena():
    return QtGui.QIcon( pmJG_Buena() )

def pmJG_MuyBuena():
    return PM(349625,350322)

def JG_MuyBuena():
    return QtGui.QIcon( pmJG_MuyBuena() )

def pmJG_MuyMala():
    return PM(350322,351183)

def JG_MuyMala():
    return QtGui.QIcon( pmJG_MuyMala() )

def pmJG_Interesante():
    return PM(351183,352064)

def JG_Interesante():
    return QtGui.QIcon( pmJG_Interesante() )

def pmJG_Dudosa():
    return PM(352064,352998)

def JG_Dudosa():
    return QtGui.QIcon( pmJG_Dudosa() )

def pmJG_SinValoracion():
    return PM(352998,353224)

def JG_SinValoracion():
    return QtGui.QIcon( pmJG_SinValoracion() )

def pmAdd():
    return PM(353224,353890)

def Add():
    return QtGui.QIcon( pmAdd() )

def pmMas22():
    return PM(353890,354352)

def Mas22():
    return QtGui.QIcon( pmMas22() )

def pmMenos22():
    return PM(354352,354602)

def Menos22():
    return QtGui.QIcon( pmMenos22() )

def pmTransposition():
    return PM(354602,355232)

def Transposition():
    return QtGui.QIcon( pmTransposition() )

def pmRat():
    return PM(355232,360936)

def Rat():
    return QtGui.QIcon( pmRat() )

def pmAlligator():
    return PM(360936,365928)

def Alligator():
    return QtGui.QIcon( pmAlligator() )

def pmAnt():
    return PM(365928,372626)

def Ant():
    return QtGui.QIcon( pmAnt() )

def pmBat():
    return PM(372626,375580)

def Bat():
    return QtGui.QIcon( pmBat() )

def pmBear():
    return PM(375580,382859)

def Bear():
    return QtGui.QIcon( pmBear() )

def pmBee():
    return PM(382859,387861)

def Bee():
    return QtGui.QIcon( pmBee() )

def pmBird():
    return PM(387861,393920)

def Bird():
    return QtGui.QIcon( pmBird() )

def pmBull():
    return PM(393920,400889)

def Bull():
    return QtGui.QIcon( pmBull() )

def pmBulldog():
    return PM(400889,407780)

def Bulldog():
    return QtGui.QIcon( pmBulldog() )

def pmButterfly():
    return PM(407780,415154)

def Butterfly():
    return QtGui.QIcon( pmButterfly() )

def pmCat():
    return PM(415154,421426)

def Cat():
    return QtGui.QIcon( pmCat() )

def pmChicken():
    return PM(421426,427237)

def Chicken():
    return QtGui.QIcon( pmChicken() )

def pmCow():
    return PM(427237,433980)

def Cow():
    return QtGui.QIcon( pmCow() )

def pmCrab():
    return PM(433980,439569)

def Crab():
    return QtGui.QIcon( pmCrab() )

def pmCrocodile():
    return PM(439569,445710)

def Crocodile():
    return QtGui.QIcon( pmCrocodile() )

def pmDeer():
    return PM(445710,452017)

def Deer():
    return QtGui.QIcon( pmDeer() )

def pmDog():
    return PM(452017,458620)

def Dog():
    return QtGui.QIcon( pmDog() )

def pmDonkey():
    return PM(458620,464267)

def Donkey():
    return QtGui.QIcon( pmDonkey() )

def pmDuck():
    return PM(464267,470810)

def Duck():
    return QtGui.QIcon( pmDuck() )

def pmEagle():
    return PM(470810,475628)

def Eagle():
    return QtGui.QIcon( pmEagle() )

def pmElephant():
    return PM(475628,482109)

def Elephant():
    return QtGui.QIcon( pmElephant() )

def pmFish():
    return PM(482109,488950)

def Fish():
    return QtGui.QIcon( pmFish() )

def pmFox():
    return PM(488950,495733)

def Fox():
    return QtGui.QIcon( pmFox() )

def pmFrog():
    return PM(495733,502149)

def Frog():
    return QtGui.QIcon( pmFrog() )

def pmGiraffe():
    return PM(502149,509327)

def Giraffe():
    return QtGui.QIcon( pmGiraffe() )

def pmGorilla():
    return PM(509327,515866)

def Gorilla():
    return QtGui.QIcon( pmGorilla() )

def pmHippo():
    return PM(515866,522987)

def Hippo():
    return QtGui.QIcon( pmHippo() )

def pmHorse():
    return PM(522987,529534)

def Horse():
    return QtGui.QIcon( pmHorse() )

def pmInsect():
    return PM(529534,535469)

def Insect():
    return QtGui.QIcon( pmInsect() )

def pmLion():
    return PM(535469,544379)

def Lion():
    return QtGui.QIcon( pmLion() )

def pmMonkey():
    return PM(544379,552058)

def Monkey():
    return QtGui.QIcon( pmMonkey() )

def pmMoose():
    return PM(552058,558682)

def Moose():
    return QtGui.QIcon( pmMoose() )

def pmMouse():
    return PM(355232,360936)

def Mouse():
    return QtGui.QIcon( pmMouse() )

def pmOwl():
    return PM(558682,565388)

def Owl():
    return QtGui.QIcon( pmOwl() )

def pmPanda():
    return PM(565388,569422)

def Panda():
    return QtGui.QIcon( pmPanda() )

def pmPenguin():
    return PM(569422,574971)

def Penguin():
    return QtGui.QIcon( pmPenguin() )

def pmPig():
    return PM(574971,583011)

def Pig():
    return QtGui.QIcon( pmPig() )

def pmRabbit():
    return PM(583011,590312)

def Rabbit():
    return QtGui.QIcon( pmRabbit() )

def pmRhino():
    return PM(590312,596699)

def Rhino():
    return QtGui.QIcon( pmRhino() )

def pmRooster():
    return PM(596699,601962)

def Rooster():
    return QtGui.QIcon( pmRooster() )

def pmShark():
    return PM(601962,607732)

def Shark():
    return QtGui.QIcon( pmShark() )

def pmSheep():
    return PM(607732,611563)

def Sheep():
    return QtGui.QIcon( pmSheep() )

def pmSnake():
    return PM(611563,617588)

def Snake():
    return QtGui.QIcon( pmSnake() )

def pmTiger():
    return PM(617588,625625)

def Tiger():
    return QtGui.QIcon( pmTiger() )

def pmTurkey():
    return PM(625625,633039)

def Turkey():
    return QtGui.QIcon( pmTurkey() )

def pmTurtle():
    return PM(633039,639760)

def Turtle():
    return QtGui.QIcon( pmTurtle() )

def pmWolf():
    return PM(639760,642855)

def Wolf():
    return QtGui.QIcon( pmWolf() )

def pmWheel():
    return PM(642855,650766)

def Wheel():
    return QtGui.QIcon( pmWheel() )

def pmWheelchair():
    return PM(650766,659453)

def Wheelchair():
    return QtGui.QIcon( pmWheelchair() )

def pmTouringMotorcycle():
    return PM(659453,665629)

def TouringMotorcycle():
    return QtGui.QIcon( pmTouringMotorcycle() )

def pmContainer():
    return PM(665629,670817)

def Container():
    return QtGui.QIcon( pmContainer() )

def pmBoatEquipment():
    return PM(670817,676174)

def BoatEquipment():
    return QtGui.QIcon( pmBoatEquipment() )

def pmCar():
    return PM(676174,680662)

def Car():
    return QtGui.QIcon( pmCar() )

def pmLorry():
    return PM(680662,686542)

def Lorry():
    return QtGui.QIcon( pmLorry() )

def pmCarTrailer():
    return PM(686542,690492)

def CarTrailer():
    return QtGui.QIcon( pmCarTrailer() )

def pmTowTruck():
    return PM(690492,695084)

def TowTruck():
    return QtGui.QIcon( pmTowTruck() )

def pmQuadBike():
    return PM(695084,700923)

def QuadBike():
    return QtGui.QIcon( pmQuadBike() )

def pmRecoveryTruck():
    return PM(700923,705766)

def RecoveryTruck():
    return QtGui.QIcon( pmRecoveryTruck() )

def pmContainerLoader():
    return PM(705766,710742)

def ContainerLoader():
    return QtGui.QIcon( pmContainerLoader() )

def pmPoliceCar():
    return PM(710742,715414)

def PoliceCar():
    return QtGui.QIcon( pmPoliceCar() )

def pmExecutiveCar():
    return PM(715414,719928)

def ExecutiveCar():
    return QtGui.QIcon( pmExecutiveCar() )

def pmTruck():
    return PM(719928,725236)

def Truck():
    return QtGui.QIcon( pmTruck() )

def pmExcavator():
    return PM(725236,729988)

def Excavator():
    return QtGui.QIcon( pmExcavator() )

def pmCabriolet():
    return PM(729988,734662)

def Cabriolet():
    return QtGui.QIcon( pmCabriolet() )

def pmMixerTruck():
    return PM(734662,740815)

def MixerTruck():
    return QtGui.QIcon( pmMixerTruck() )

def pmForkliftTruckLoaded():
    return PM(740815,746821)

def ForkliftTruckLoaded():
    return QtGui.QIcon( pmForkliftTruckLoaded() )

def pmAmbulance():
    return PM(746821,752707)

def Ambulance():
    return QtGui.QIcon( pmAmbulance() )

def pmDieselLocomotiveBoxcar():
    return PM(752707,756543)

def DieselLocomotiveBoxcar():
    return QtGui.QIcon( pmDieselLocomotiveBoxcar() )

def pmTractorUnit():
    return PM(756543,761854)

def TractorUnit():
    return QtGui.QIcon( pmTractorUnit() )

def pmFireTruck():
    return PM(761854,768032)

def FireTruck():
    return QtGui.QIcon( pmFireTruck() )

def pmCargoShip():
    return PM(768032,772216)

def CargoShip():
    return QtGui.QIcon( pmCargoShip() )

def pmSubwayTrain():
    return PM(772216,776951)

def SubwayTrain():
    return QtGui.QIcon( pmSubwayTrain() )

def pmTruckMountedCrane():
    return PM(776951,782528)

def TruckMountedCrane():
    return QtGui.QIcon( pmTruckMountedCrane() )

def pmAirAmbulance():
    return PM(782528,787501)

def AirAmbulance():
    return QtGui.QIcon( pmAirAmbulance() )

def pmAirplane():
    return PM(787501,792249)

def Airplane():
    return QtGui.QIcon( pmAirplane() )

def pmCaracol():
    return PM(792249,794282)

def Caracol():
    return QtGui.QIcon( pmCaracol() )

def pmDownloads():
    return PM(794282,795961)

def Downloads():
    return QtGui.QIcon( pmDownloads() )

def pmXFCC():
    return PM(795961,796932)

def XFCC():
    return QtGui.QIcon( pmXFCC() )

def pmLastXFCC():
    return PM(796932,797488)

def LastXFCC():
    return QtGui.QIcon( pmLastXFCC() )

def pmUno():
    return PM(797488,800227)

def Uno():
    return QtGui.QIcon( pmUno() )

def pmMotoresExternos():
    return PM(800227,802292)

def MotoresExternos():
    return QtGui.QIcon( pmMotoresExternos() )

def pmDatabase():
    return PM(802292,802837)

def Database():
    return QtGui.QIcon( pmDatabase() )

def pmDatabaseC():
    return PM(802837,803229)

def DatabaseC():
    return QtGui.QIcon( pmDatabaseC() )

def pmDatabaseF():
    return PM(803229,803670)

def DatabaseF():
    return QtGui.QIcon( pmDatabaseF() )

def pmAtacante():
    return PM(803670,804006)

def Atacante():
    return QtGui.QIcon( pmAtacante() )

def pmAtacada():
    return PM(804006,804306)

def Atacada():
    return QtGui.QIcon( pmAtacada() )

def pmGoToNext():
    return PM(804306,804474)

def GoToNext():
    return QtGui.QIcon( pmGoToNext() )

def pmBlancas():
    return PM(804474,804614)

def Blancas():
    return QtGui.QIcon( pmBlancas() )

def pmNegras():
    return PM(804614,804704)

def Negras():
    return QtGui.QIcon( pmNegras() )

def pmFolderChange():
    return PM(69269,71796)

def FolderChange():
    return QtGui.QIcon( pmFolderChange() )

def pmMarkers():
    return PM(804704,806243)

def Markers():
    return QtGui.QIcon( pmMarkers() )

def pmTop():
    return PM(806243,806877)

def Top():
    return QtGui.QIcon( pmTop() )

def pmBottom():
    return PM(806877,807481)

def Bottom():
    return QtGui.QIcon( pmBottom() )

def pmSTS():
    return PM(807481,809604)

def STS():
    return QtGui.QIcon( pmSTS() )

def pmRun():
    return PM(809604,813810)

def Run():
    return QtGui.QIcon( pmRun() )

def pmWorldMap():
    return PM(813810,816951)

def WorldMap():
    return QtGui.QIcon( pmWorldMap() )

def pmAfrica():
    return PM(816951,819293)

def Africa():
    return QtGui.QIcon( pmAfrica() )

def pmMaps():
    return PM(819293,820633)

def Maps():
    return QtGui.QIcon( pmMaps() )

def pmSol():
    return PM(820633,826855)

def Sol():
    return QtGui.QIcon( pmSol() )

def pmSolNubes():
    return PM(826855,832590)

def SolNubes():
    return QtGui.QIcon( pmSolNubes() )

def pmNubes():
    return PM(832590,835590)

def Nubes():
    return QtGui.QIcon( pmNubes() )

def pmTormenta():
    return PM(835590,840110)

def Tormenta():
    return QtGui.QIcon( pmTormenta() )


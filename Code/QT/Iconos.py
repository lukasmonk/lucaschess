"""Iconos y pixmap usados en el programa"""
from PyQt4 import QtGui

f = open("./IntFiles/Iconos.bin","rb")
binIconos = f.read()
f.close()


def icono(name):
    return eval( "%s()"%name )


def pixmap(name):
    return eval( "pm%s()"%name )


def PM(desde, hasta):
    pm = QtGui.QPixmap()
    pm.loadFromData( binIconos[desde:hasta] )
    return pm


def pmLM():
    return PM(0,1248)


def LM():
    return QtGui.QIcon(pmLM())


def pmAplicacion():
    return PM(1248,2025)


def Aplicacion():
    return QtGui.QIcon(pmAplicacion())


def pmAplicacion64():
    return PM(2025,6294)


def Aplicacion64():
    return QtGui.QIcon(pmAplicacion64())


def pmDatos():
    return PM(6294,7481)


def Datos():
    return QtGui.QIcon(pmDatos())


def pmTutor():
    return PM(7481,9510)


def Tutor():
    return QtGui.QIcon(pmTutor())


def pmTablero():
    return PM(6294,7481)


def Tablero():
    return QtGui.QIcon(pmTablero())


def pmPartidaOriginal():
    return PM(9510,11487)


def PartidaOriginal():
    return QtGui.QIcon(pmPartidaOriginal())


def pmDGT():
    return PM(11487,12481)


def DGT():
    return QtGui.QIcon(pmDGT())


def pmZonaPrincipiantes():
    return PM(12481,14515)


def ZonaPrincipiantes():
    return QtGui.QIcon(pmZonaPrincipiantes())


def pmJ60():
    return PM(14515,16111)


def J60():
    return QtGui.QIcon(pmJ60())


def pmTamTablero():
    return PM(14515,16111)


def TamTablero():
    return QtGui.QIcon(pmTamTablero())


def pmMensEspera():
    return PM(16111,23085)


def MensEspera():
    return QtGui.QIcon(pmMensEspera())


def pmUtilidades():
    return PM(23085,29514)


def Utilidades():
    return QtGui.QIcon(pmUtilidades())


def pmTerminar():
    return PM(29514,31264)


def Terminar():
    return QtGui.QIcon(pmTerminar())


def pmNuevaPartida():
    return PM(31264,33012)


def NuevaPartida():
    return QtGui.QIcon(pmNuevaPartida())


def pmOpciones():
    return PM(33012,34740)


def Opciones():
    return QtGui.QIcon(pmOpciones())


def pmEntrenamiento():
    return PM(7481,9510)


def Entrenamiento():
    return QtGui.QIcon(pmEntrenamiento())


def pmAplazar():
    return PM(34740,37807)


def Aplazar():
    return QtGui.QIcon(pmAplazar())


def pmCapturas():
    return PM(37807,39848)


def Capturas():
    return QtGui.QIcon(pmCapturas())


def pmReiniciar():
    return PM(39848,42142)


def Reiniciar():
    return QtGui.QIcon(pmReiniciar())


def pmMotores():
    return PM(42142,48041)


def Motores():
    return QtGui.QIcon(pmMotores())


def pmImportarGM():
    return PM(48041,50641)


def ImportarGM():
    return QtGui.QIcon(pmImportarGM())


def pmAbandonar():
    return PM(50641,54641)


def Abandonar():
    return QtGui.QIcon(pmAbandonar())


def pmEmpezar():
    return PM(54641,56677)


def Empezar():
    return QtGui.QIcon(pmEmpezar())


def pmOtros():
    return PM(56677,61147)


def Otros():
    return QtGui.QIcon(pmOtros())


def pmAnalizar():
    return PM(61147,62684)


def Analizar():
    return QtGui.QIcon(pmAnalizar())


def pmMainMenu():
    return PM(62684,66994)


def MainMenu():
    return QtGui.QIcon(pmMainMenu())


def pmFinPartida():
    return PM(66994,69942)


def FinPartida():
    return QtGui.QIcon(pmFinPartida())


def pmGrabar():
    return PM(69942,71405)


def Grabar():
    return QtGui.QIcon(pmGrabar())


def pmGrabarComo():
    return PM(71405,73457)


def GrabarComo():
    return QtGui.QIcon(pmGrabarComo())


def pmRecuperar():
    return PM(73457,76215)


def Recuperar():
    return QtGui.QIcon(pmRecuperar())


def pmInformacion():
    return PM(76215,78174)


def Informacion():
    return QtGui.QIcon(pmInformacion())


def pmNuevo():
    return PM(78174,78928)


def Nuevo():
    return QtGui.QIcon(pmNuevo())


def pmCopiar():
    return PM(78928,80109)


def Copiar():
    return QtGui.QIcon(pmCopiar())


def pmModificar():
    return PM(80109,84506)


def Modificar():
    return QtGui.QIcon(pmModificar())


def pmBorrar():
    return PM(84506,89497)


def Borrar():
    return QtGui.QIcon(pmBorrar())


def pmMarcar():
    return PM(89497,94426)


def Marcar():
    return QtGui.QIcon(pmMarcar())


def pmPegar():
    return PM(94426,96737)


def Pegar():
    return QtGui.QIcon(pmPegar())


def pmFichero():
    return PM(96737,101422)


def Fichero():
    return QtGui.QIcon(pmFichero())


def pmNuestroFichero():
    return PM(101422,104469)


def NuestroFichero():
    return QtGui.QIcon(pmNuestroFichero())


def pmFicheroRepite():
    return PM(104469,105965)


def FicheroRepite():
    return QtGui.QIcon(pmFicheroRepite())


def pmInformacionPGN():
    return PM(105965,106983)


def InformacionPGN():
    return QtGui.QIcon(pmInformacionPGN())


def pmVer():
    return PM(106983,108437)


def Ver():
    return QtGui.QIcon(pmVer())


def pmInicio():
    return PM(108437,110451)


def Inicio():
    return QtGui.QIcon(pmInicio())


def pmFinal():
    return PM(110451,112445)


def Final():
    return QtGui.QIcon(pmFinal())


def pmFiltrar():
    return PM(112445,118935)


def Filtrar():
    return QtGui.QIcon(pmFiltrar())


def pmArriba():
    return PM(118935,121088)


def Arriba():
    return QtGui.QIcon(pmArriba())


def pmAbajo():
    return PM(121088,123196)


def Abajo():
    return QtGui.QIcon(pmAbajo())


def pmEstadisticas():
    return PM(123196,125335)


def Estadisticas():
    return QtGui.QIcon(pmEstadisticas())


def pmRendirse():
    return PM(125335,129181)


def Rendirse():
    return QtGui.QIcon(pmRendirse())


def pmCheck():
    return PM(129181,132405)


def Check():
    return QtGui.QIcon(pmCheck())


def pmTablas():
    return PM(132405,134028)


def Tablas():
    return QtGui.QIcon(pmTablas())


def pmAtras():
    return PM(134028,135547)


def Atras():
    return QtGui.QIcon(pmAtras())


def pmBuscar():
    return PM(135547,137532)


def Buscar():
    return QtGui.QIcon(pmBuscar())


def pmLibros():
    return PM(137532,139660)


def Libros():
    return QtGui.QIcon(pmLibros())


def pmAceptar():
    return PM(139660,143007)


def Aceptar():
    return QtGui.QIcon(pmAceptar())


def pmCancelar():
    return PM(143007,144990)


def Cancelar():
    return QtGui.QIcon(pmCancelar())


def pmDefecto():
    return PM(144990,148309)


def Defecto():
    return QtGui.QIcon(pmDefecto())


def pmGenerar():
    return PM(148309,151525)


def Generar():
    return QtGui.QIcon(pmGenerar())


def pmInsertar():
    return PM(151525,153921)


def Insertar():
    return QtGui.QIcon(pmInsertar())


def pmJugar():
    return PM(153921,156130)


def Jugar():
    return QtGui.QIcon(pmJugar())


def pmConfigurar():
    return PM(156130,159214)


def Configurar():
    return QtGui.QIcon(pmConfigurar())


def pmS_Aceptar():
    return PM(139660,143007)


def S_Aceptar():
    return QtGui.QIcon(pmS_Aceptar())


def pmS_Cancelar():
    return PM(143007,144990)


def S_Cancelar():
    return QtGui.QIcon(pmS_Cancelar())


def pmS_Microfono():
    return PM(159214,164655)


def S_Microfono():
    return QtGui.QIcon(pmS_Microfono())


def pmS_LeerWav():
    return PM(48041,50641)


def S_LeerWav():
    return QtGui.QIcon(pmS_LeerWav())


def pmS_Play():
    return PM(164655,169993)


def S_Play():
    return QtGui.QIcon(pmS_Play())


def pmS_StopPlay():
    return PM(169993,170603)


def S_StopPlay():
    return QtGui.QIcon(pmS_StopPlay())


def pmS_StopMicrofono():
    return PM(169993,170603)


def S_StopMicrofono():
    return QtGui.QIcon(pmS_StopMicrofono())


def pmS_Record():
    return PM(170603,173836)


def S_Record():
    return QtGui.QIcon(pmS_Record())


def pmS_Limpiar():
    return PM(84506,89497)


def S_Limpiar():
    return QtGui.QIcon(pmS_Limpiar())


def pmHistorial():
    return PM(173836,175099)


def Historial():
    return QtGui.QIcon(pmHistorial())


def pmPegar16():
    return PM(175099,176093)


def Pegar16():
    return QtGui.QIcon(pmPegar16())


def pmRivalesMP():
    return PM(176093,177219)


def RivalesMP():
    return QtGui.QIcon(pmRivalesMP())


def pmCamara():
    return PM(177219,178741)


def Camara():
    return QtGui.QIcon(pmCamara())


def pmUsuarios():
    return PM(178741,179981)


def Usuarios():
    return QtGui.QIcon(pmUsuarios())


def pmResistencia():
    return PM(179981,183043)


def Resistencia():
    return QtGui.QIcon(pmResistencia())


def pmRemoto():
    return PM(183043,187165)


def Remoto():
    return QtGui.QIcon(pmRemoto())


def pmRemotoServidor():
    return PM(187165,188189)


def RemotoServidor():
    return QtGui.QIcon(pmRemotoServidor())


def pmRemotoCliente():
    return PM(188189,189433)


def RemotoCliente():
    return QtGui.QIcon(pmRemotoCliente())


def pmCebra():
    return PM(189433,191886)


def Cebra():
    return QtGui.QIcon(pmCebra())


def pmGafas():
    return PM(191886,192870)


def Gafas():
    return QtGui.QIcon(pmGafas())


def pmPuente():
    return PM(192870,193506)


def Puente():
    return QtGui.QIcon(pmPuente())


def pmWeb():
    return PM(193506,194688)


def Web():
    return QtGui.QIcon(pmWeb())


def pmMail():
    return PM(194688,195648)


def Mail():
    return QtGui.QIcon(pmMail())


def pmAyuda():
    return PM(195648,196829)


def Ayuda():
    return QtGui.QIcon(pmAyuda())


def pmFAQ():
    return PM(196829,198150)


def FAQ():
    return QtGui.QIcon(pmFAQ())


def pmPuntuacion():
    return PM(198150,199078)


def Puntuacion():
    return QtGui.QIcon(pmPuntuacion())


def pmActualiza():
    return PM(199078,199944)


def Actualiza():
    return QtGui.QIcon(pmActualiza())


def pmRefresh():
    return PM(199944,202336)


def Refresh():
    return QtGui.QIcon(pmRefresh())


def pmJuegaSolo():
    return PM(202336,203518)


def JuegaSolo():
    return QtGui.QIcon(pmJuegaSolo())


def pmPlayer():
    return PM(202336,203518)


def Player():
    return QtGui.QIcon(pmPlayer())


def pmJS_Rotacion():
    return PM(203518,205428)


def JS_Rotacion():
    return QtGui.QIcon(pmJS_Rotacion())


def pmCoordina():
    return PM(203518,205428)


def Coordina():
    return QtGui.QIcon(pmCoordina())


def pmEstrellaAzul():
    return PM(205428,206934)


def EstrellaAzul():
    return QtGui.QIcon(pmEstrellaAzul())


def pmElo():
    return PM(205428,206934)


def Elo():
    return QtGui.QIcon(pmElo())


def pmMate():
    return PM(206934,207495)


def Mate():
    return QtGui.QIcon(pmMate())


def pmEloTimed():
    return PM(207495,208979)


def EloTimed():
    return QtGui.QIcon(pmEloTimed())


def pmPGN():
    return PM(208979,210977)


def PGN():
    return QtGui.QIcon(pmPGN())


def pmPGN_Importar():
    return PM(210977,212567)


def PGN_Importar():
    return QtGui.QIcon(pmPGN_Importar())


def pmAyudaGR():
    return PM(212567,218445)


def AyudaGR():
    return QtGui.QIcon(pmAyudaGR())


def pmBotonAyuda():
    return PM(218445,220905)


def BotonAyuda():
    return QtGui.QIcon(pmBotonAyuda())


def pmColores():
    return PM(220905,222136)


def Colores():
    return QtGui.QIcon(pmColores())


def pmEditarColores():
    return PM(222136,224439)


def EditarColores():
    return QtGui.QIcon(pmEditarColores())


def pmGranMaestro():
    return PM(224439,225295)


def GranMaestro():
    return QtGui.QIcon(pmGranMaestro())


def pmWGranMaestro():
    return PM(225295,225754)


def WGranMaestro():
    return QtGui.QIcon(pmWGranMaestro())


def pmFavoritos():
    return PM(225754,227520)


def Favoritos():
    return QtGui.QIcon(pmFavoritos())


def pmCarpeta():
    return PM(227520,228224)


def Carpeta():
    return QtGui.QIcon(pmCarpeta())


def pmDivision():
    return PM(228224,228889)


def Division():
    return QtGui.QIcon(pmDivision())


def pmDivisionF():
    return PM(228889,230003)


def DivisionF():
    return QtGui.QIcon(pmDivisionF())


def pmKibitzer():
    return PM(230003,230542)


def Kibitzer():
    return QtGui.QIcon(pmKibitzer())


def pmKibitzer_Pausa():
    return PM(230542,231406)


def Kibitzer_Pausa():
    return QtGui.QIcon(pmKibitzer_Pausa())


def pmKibitzer_Continuar():
    return PM(231406,232237)


def Kibitzer_Continuar():
    return QtGui.QIcon(pmKibitzer_Continuar())


def pmKibitzer_Terminar():
    return PM(232237,233161)


def Kibitzer_Terminar():
    return QtGui.QIcon(pmKibitzer_Terminar())


def pmDelete():
    return PM(232237,233161)


def Delete():
    return QtGui.QIcon(pmDelete())


def pmModificarP():
    return PM(233161,234227)


def ModificarP():
    return QtGui.QIcon(pmModificarP())


def pmGrupo_Si():
    return PM(234227,234689)


def Grupo_Si():
    return QtGui.QIcon(pmGrupo_Si())


def pmGrupo_No():
    return PM(234689,235012)


def Grupo_No():
    return QtGui.QIcon(pmGrupo_No())


def pmMotor_Si():
    return PM(235012,235474)


def Motor_Si():
    return QtGui.QIcon(pmMotor_Si())


def pmMotor_No():
    return PM(232237,233161)


def Motor_No():
    return QtGui.QIcon(pmMotor_No())


def pmMotor_Actual():
    return PM(235474,236491)


def Motor_Actual():
    return QtGui.QIcon(pmMotor_Actual())


def pmMotor():
    return PM(236491,237118)


def Motor():
    return QtGui.QIcon(pmMotor())


def pmMoverInicio():
    return PM(237118,237971)


def MoverInicio():
    return QtGui.QIcon(pmMoverInicio())


def pmMoverFinal():
    return PM(237971,238847)


def MoverFinal():
    return QtGui.QIcon(pmMoverFinal())


def pmMoverAdelante():
    return PM(238847,239708)


def MoverAdelante():
    return QtGui.QIcon(pmMoverAdelante())


def pmMoverAtras():
    return PM(239708,240576)


def MoverAtras():
    return QtGui.QIcon(pmMoverAtras())


def pmMoverLibre():
    return PM(240576,241396)


def MoverLibre():
    return QtGui.QIcon(pmMoverLibre())


def pmMoverTiempo():
    return PM(241396,242589)


def MoverTiempo():
    return QtGui.QIcon(pmMoverTiempo())


def pmMoverMas():
    return PM(242589,243628)


def MoverMas():
    return QtGui.QIcon(pmMoverMas())


def pmMoverGrabar():
    return PM(243628,244484)


def MoverGrabar():
    return QtGui.QIcon(pmMoverGrabar())


def pmMoverGrabarTodos():
    return PM(244484,245528)


def MoverGrabarTodos():
    return QtGui.QIcon(pmMoverGrabarTodos())


def pmMoverJugar():
    return PM(231406,232237)


def MoverJugar():
    return QtGui.QIcon(pmMoverJugar())


def pmPelicula():
    return PM(245528,247662)


def Pelicula():
    return QtGui.QIcon(pmPelicula())


def pmPelicula_Pausa():
    return PM(247662,249421)


def Pelicula_Pausa():
    return QtGui.QIcon(pmPelicula_Pausa())


def pmPelicula_Seguir():
    return PM(249421,251510)


def Pelicula_Seguir():
    return QtGui.QIcon(pmPelicula_Seguir())


def pmPelicula_Rapido():
    return PM(251510,253569)


def Pelicula_Rapido():
    return QtGui.QIcon(pmPelicula_Rapido())


def pmPelicula_Lento():
    return PM(253569,255444)


def Pelicula_Lento():
    return QtGui.QIcon(pmPelicula_Lento())


def pmPelicula_Repetir():
    return PM(39848,42142)


def Pelicula_Repetir():
    return QtGui.QIcon(pmPelicula_Repetir())


def pmPelicula_PGN():
    return PM(255444,256352)


def Pelicula_PGN():
    return QtGui.QIcon(pmPelicula_PGN())


def pmMemoria():
    return PM(256352,258293)


def Memoria():
    return QtGui.QIcon(pmMemoria())


def pmEntrenar():
    return PM(258293,259832)


def Entrenar():
    return QtGui.QIcon(pmEntrenar())


def pmEnviar():
    return PM(258293,259832)


def Enviar():
    return QtGui.QIcon(pmEnviar())


def pmTrasteros():
    return PM(259832,264635)


def Trasteros():
    return QtGui.QIcon(pmTrasteros())


def pmTrastero():
    return PM(264635,265097)


def Trastero():
    return QtGui.QIcon(pmTrastero())


def pmTrastero_Quitar():
    return PM(232237,233161)


def Trastero_Quitar():
    return QtGui.QIcon(pmTrastero_Quitar())


def pmTrastero_Nuevo():
    return PM(265097,266605)


def Trastero_Nuevo():
    return QtGui.QIcon(pmTrastero_Nuevo())


def pmNuevoMas():
    return PM(265097,266605)


def NuevoMas():
    return QtGui.QIcon(pmNuevoMas())


def pmTemas():
    return PM(266605,268828)


def Temas():
    return QtGui.QIcon(pmTemas())


def pmTutorialesCrear():
    return PM(268828,275097)


def TutorialesCrear():
    return QtGui.QIcon(pmTutorialesCrear())


def pmMover():
    return PM(275097,275679)


def Mover():
    return QtGui.QIcon(pmMover())


def pmSeleccionado():
    return PM(275097,275679)


def Seleccionado():
    return QtGui.QIcon(pmSeleccionado())


def pmSeleccionar():
    return PM(275679,281383)


def Seleccionar():
    return QtGui.QIcon(pmSeleccionar())


def pmVista():
    return PM(281383,283307)


def Vista():
    return QtGui.QIcon(pmVista())


def pmInformacionPGNUno():
    return PM(283307,284685)


def InformacionPGNUno():
    return QtGui.QIcon(pmInformacionPGNUno())


def pmDailyTest():
    return PM(284685,287025)


def DailyTest():
    return QtGui.QIcon(pmDailyTest())


def pmJuegaPorMi():
    return PM(287025,288745)


def JuegaPorMi():
    return QtGui.QIcon(pmJuegaPorMi())


def pmArbol():
    return PM(288745,289379)


def Arbol():
    return QtGui.QIcon(pmArbol())


def pmGrabarFichero():
    return PM(69942,71405)


def GrabarFichero():
    return QtGui.QIcon(pmGrabarFichero())


def pmClip():
    return PM(289379,291525)


def Clip():
    return QtGui.QIcon(pmClip())


def pmFics():
    return PM(291525,291942)


def Fics():
    return QtGui.QIcon(pmFics())


def pmFide():
    return PM(9510,11487)


def Fide():
    return QtGui.QIcon(pmFide())


def pmFichPGN():
    return PM(9510,11487)


def FichPGN():
    return QtGui.QIcon(pmFichPGN())


def pmFlechas():
    return PM(291942,295294)


def Flechas():
    return QtGui.QIcon(pmFlechas())


def pmMarcos():
    return PM(295294,296741)


def Marcos():
    return QtGui.QIcon(pmMarcos())


def pmSVGs():
    return PM(296741,300310)


def SVGs():
    return QtGui.QIcon(pmSVGs())


def pmAmarillo():
    return PM(300310,301562)


def Amarillo():
    return QtGui.QIcon(pmAmarillo())


def pmNaranja():
    return PM(301562,302794)


def Naranja():
    return QtGui.QIcon(pmNaranja())


def pmVerde():
    return PM(302794,304070)


def Verde():
    return QtGui.QIcon(pmVerde())


def pmAzul():
    return PM(304070,305158)


def Azul():
    return QtGui.QIcon(pmAzul())


def pmMagenta():
    return PM(305158,306446)


def Magenta():
    return QtGui.QIcon(pmMagenta())


def pmRojo():
    return PM(306446,307665)


def Rojo():
    return QtGui.QIcon(pmRojo())


def pmGris():
    return PM(307665,308623)


def Gris():
    return QtGui.QIcon(pmGris())


def pmEstrella():
    return PM(198150,199078)


def Estrella():
    return QtGui.QIcon(pmEstrella())


def pmAmarillo32():
    return PM(308623,310603)


def Amarillo32():
    return QtGui.QIcon(pmAmarillo32())


def pmNaranja32():
    return PM(310603,312727)


def Naranja32():
    return QtGui.QIcon(pmNaranja32())


def pmVerde32():
    return PM(312727,314848)


def Verde32():
    return QtGui.QIcon(pmVerde32())


def pmAzul32():
    return PM(314848,317227)


def Azul32():
    return QtGui.QIcon(pmAzul32())


def pmMagenta32():
    return PM(317227,319678)


def Magenta32():
    return QtGui.QIcon(pmMagenta32())


def pmRojo32():
    return PM(319678,321493)


def Rojo32():
    return QtGui.QIcon(pmRojo32())


def pmGris32():
    return PM(321493,323407)


def Gris32():
    return QtGui.QIcon(pmGris32())


def pmPuntoBlanco():
    return PM(323407,323756)


def PuntoBlanco():
    return QtGui.QIcon(pmPuntoBlanco())


def pmPuntoAmarillo():
    return PM(234227,234689)


def PuntoAmarillo():
    return QtGui.QIcon(pmPuntoAmarillo())


def pmPuntoNaranja():
    return PM(323756,324218)


def PuntoNaranja():
    return QtGui.QIcon(pmPuntoNaranja())


def pmPuntoVerde():
    return PM(235012,235474)


def PuntoVerde():
    return QtGui.QIcon(pmPuntoVerde())


def pmPuntoAzul():
    return PM(264635,265097)


def PuntoAzul():
    return QtGui.QIcon(pmPuntoAzul())


def pmPuntoMagenta():
    return PM(324218,324717)


def PuntoMagenta():
    return QtGui.QIcon(pmPuntoMagenta())


def pmPuntoRojo():
    return PM(324717,325216)


def PuntoRojo():
    return QtGui.QIcon(pmPuntoRojo())


def pmPuntoNegro():
    return PM(234689,235012)


def PuntoNegro():
    return QtGui.QIcon(pmPuntoNegro())


def pmPuntoEstrella():
    return PM(325216,325643)


def PuntoEstrella():
    return QtGui.QIcon(pmPuntoEstrella())


def pmComentario():
    return PM(325643,326280)


def Comentario():
    return QtGui.QIcon(pmComentario())


def pmComentarioMas():
    return PM(326280,327219)


def ComentarioMas():
    return QtGui.QIcon(pmComentarioMas())


def pmComentarioEditar():
    return PM(243628,244484)


def ComentarioEditar():
    return QtGui.QIcon(pmComentarioEditar())


def pmApertura():
    return PM(327219,328185)


def Apertura():
    return QtGui.QIcon(pmApertura())


def pmAperturaComentario():
    return PM(328185,329181)


def AperturaComentario():
    return QtGui.QIcon(pmAperturaComentario())


def pmBookGuide():
    return PM(329181,330058)


def BookGuide():
    return QtGui.QIcon(pmBookGuide())


def pmMas():
    return PM(330058,330567)


def Mas():
    return QtGui.QIcon(pmMas())


def pmMasR():
    return PM(330567,331055)


def MasR():
    return QtGui.QIcon(pmMasR())


def pmMasDoc():
    return PM(331055,331856)


def MasDoc():
    return QtGui.QIcon(pmMasDoc())


def pmNuevaDB():
    return PM(331856,336490)


def NuevaDB():
    return QtGui.QIcon(pmNuevaDB())


def pmPotencia():
    return PM(199078,199944)


def Potencia():
    return QtGui.QIcon(pmPotencia())


def pmSorpresa():
    return PM(336490,337549)


def Sorpresa():
    return QtGui.QIcon(pmSorpresa())


def pmSonrisa():
    return PM(337549,338631)


def Sonrisa():
    return QtGui.QIcon(pmSonrisa())


def pmBMT():
    return PM(338631,339509)


def BMT():
    return QtGui.QIcon(pmBMT())


def pmCorazon():
    return PM(339509,341387)


def Corazon():
    return QtGui.QIcon(pmCorazon())


def pmOjo():
    return PM(341387,342509)


def Ojo():
    return QtGui.QIcon(pmOjo())


def pmOcultar():
    return PM(341387,342509)


def Ocultar():
    return QtGui.QIcon(pmOcultar())


def pmMostrar():
    return PM(342509,343565)


def Mostrar():
    return QtGui.QIcon(pmMostrar())


def pmBlog():
    return PM(343565,344087)


def Blog():
    return QtGui.QIcon(pmBlog())


def pmVariantes():
    return PM(344087,344994)


def Variantes():
    return QtGui.QIcon(pmVariantes())


def pmVariantesG():
    return PM(344994,347421)


def VariantesG():
    return QtGui.QIcon(pmVariantesG())


def pmCambiar():
    return PM(347421,349135)


def Cambiar():
    return QtGui.QIcon(pmCambiar())


def pmAnterior():
    return PM(349135,351189)


def Anterior():
    return QtGui.QIcon(pmAnterior())


def pmSiguiente():
    return PM(351189,353259)


def Siguiente():
    return QtGui.QIcon(pmSiguiente())


def pmSiguienteF():
    return PM(353259,355434)


def SiguienteF():
    return QtGui.QIcon(pmSiguienteF())


def pmAnteriorF():
    return PM(355434,357628)


def AnteriorF():
    return QtGui.QIcon(pmAnteriorF())


def pmX():
    return PM(357628,358910)


def X():
    return QtGui.QIcon(pmX())


def pmTools():
    return PM(358910,361511)


def Tools():
    return QtGui.QIcon(pmTools())


def pmTacticas():
    return PM(361511,364084)


def Tacticas():
    return QtGui.QIcon(pmTacticas())


def pmCancelarPeque():
    return PM(364084,364945)


def CancelarPeque():
    return QtGui.QIcon(pmCancelarPeque())


def pmAceptarPeque():
    return PM(235474,236491)


def AceptarPeque():
    return QtGui.QIcon(pmAceptarPeque())


def pmP_16c():
    return PM(364945,365469)


def P_16c():
    return QtGui.QIcon(pmP_16c())


def pmLibre():
    return PM(365469,367861)


def Libre():
    return QtGui.QIcon(pmLibre())


def pmEnBlanco():
    return PM(367861,368587)


def EnBlanco():
    return QtGui.QIcon(pmEnBlanco())


def pmDirector():
    return PM(368587,371561)


def Director():
    return QtGui.QIcon(pmDirector())


def pmTorneos():
    return PM(371561,373299)


def Torneos():
    return QtGui.QIcon(pmTorneos())


def pmAperturas():
    return PM(373299,374224)


def Aperturas():
    return QtGui.QIcon(pmAperturas())


def pmV_Blancas():
    return PM(374224,374504)


def V_Blancas():
    return QtGui.QIcon(pmV_Blancas())


def pmV_Blancas_Mas():
    return PM(374504,374784)


def V_Blancas_Mas():
    return QtGui.QIcon(pmV_Blancas_Mas())


def pmV_Blancas_Mas_Mas():
    return PM(374784,375056)


def V_Blancas_Mas_Mas():
    return QtGui.QIcon(pmV_Blancas_Mas_Mas())


def pmV_Negras():
    return PM(375056,375331)


def V_Negras():
    return QtGui.QIcon(pmV_Negras())


def pmV_Negras_Mas():
    return PM(375331,375606)


def V_Negras_Mas():
    return QtGui.QIcon(pmV_Negras_Mas())


def pmV_Negras_Mas_Mas():
    return PM(375606,375875)


def V_Negras_Mas_Mas():
    return QtGui.QIcon(pmV_Negras_Mas_Mas())


def pmV_Blancas_Igual_Negras():
    return PM(375875,376177)


def V_Blancas_Igual_Negras():
    return QtGui.QIcon(pmV_Blancas_Igual_Negras())


def pmMezclar():
    return PM(151525,153921)


def Mezclar():
    return QtGui.QIcon(pmMezclar())


def pmVoyager():
    return PM(376177,378139)


def Voyager():
    return QtGui.QIcon(pmVoyager())


def pmReindexar():
    return PM(378139,379956)


def Reindexar():
    return QtGui.QIcon(pmReindexar())


def pmRename():
    return PM(379956,380940)


def Rename():
    return QtGui.QIcon(pmRename())


def pmAdd():
    return PM(380940,381893)


def Add():
    return QtGui.QIcon(pmAdd())


def pmMas22():
    return PM(381893,382557)


def Mas22():
    return QtGui.QIcon(pmMas22())


def pmMenos22():
    return PM(382557,383001)


def Menos22():
    return QtGui.QIcon(pmMenos22())


def pmTransposition():
    return PM(383001,383520)


def Transposition():
    return QtGui.QIcon(pmTransposition())


def pmRat():
    return PM(383520,389224)


def Rat():
    return QtGui.QIcon(pmRat())


def pmAlligator():
    return PM(389224,394216)


def Alligator():
    return QtGui.QIcon(pmAlligator())


def pmAnt():
    return PM(394216,400914)


def Ant():
    return QtGui.QIcon(pmAnt())


def pmBat():
    return PM(400914,403868)


def Bat():
    return QtGui.QIcon(pmBat())


def pmBear():
    return PM(403868,411147)


def Bear():
    return QtGui.QIcon(pmBear())


def pmBee():
    return PM(411147,416149)


def Bee():
    return QtGui.QIcon(pmBee())


def pmBird():
    return PM(416149,422208)


def Bird():
    return QtGui.QIcon(pmBird())


def pmBull():
    return PM(422208,429177)


def Bull():
    return QtGui.QIcon(pmBull())


def pmBulldog():
    return PM(429177,436068)


def Bulldog():
    return QtGui.QIcon(pmBulldog())


def pmButterfly():
    return PM(436068,443442)


def Butterfly():
    return QtGui.QIcon(pmButterfly())


def pmCat():
    return PM(443442,449714)


def Cat():
    return QtGui.QIcon(pmCat())


def pmChicken():
    return PM(449714,455525)


def Chicken():
    return QtGui.QIcon(pmChicken())


def pmCow():
    return PM(455525,462268)


def Cow():
    return QtGui.QIcon(pmCow())


def pmCrab():
    return PM(462268,467857)


def Crab():
    return QtGui.QIcon(pmCrab())


def pmCrocodile():
    return PM(467857,473998)


def Crocodile():
    return QtGui.QIcon(pmCrocodile())


def pmDeer():
    return PM(473998,480305)


def Deer():
    return QtGui.QIcon(pmDeer())


def pmDog():
    return PM(480305,486908)


def Dog():
    return QtGui.QIcon(pmDog())


def pmDonkey():
    return PM(486908,492555)


def Donkey():
    return QtGui.QIcon(pmDonkey())


def pmDuck():
    return PM(492555,499098)


def Duck():
    return QtGui.QIcon(pmDuck())


def pmEagle():
    return PM(499098,503916)


def Eagle():
    return QtGui.QIcon(pmEagle())


def pmElephant():
    return PM(503916,510397)


def Elephant():
    return QtGui.QIcon(pmElephant())


def pmFish():
    return PM(510397,517238)


def Fish():
    return QtGui.QIcon(pmFish())


def pmFox():
    return PM(517238,524021)


def Fox():
    return QtGui.QIcon(pmFox())


def pmFrog():
    return PM(524021,530437)


def Frog():
    return QtGui.QIcon(pmFrog())


def pmGiraffe():
    return PM(530437,537615)


def Giraffe():
    return QtGui.QIcon(pmGiraffe())


def pmGorilla():
    return PM(537615,544154)


def Gorilla():
    return QtGui.QIcon(pmGorilla())


def pmHippo():
    return PM(544154,551275)


def Hippo():
    return QtGui.QIcon(pmHippo())


def pmHorse():
    return PM(551275,557822)


def Horse():
    return QtGui.QIcon(pmHorse())


def pmInsect():
    return PM(557822,563757)


def Insect():
    return QtGui.QIcon(pmInsect())


def pmLion():
    return PM(563757,572667)


def Lion():
    return QtGui.QIcon(pmLion())


def pmMonkey():
    return PM(572667,580346)


def Monkey():
    return QtGui.QIcon(pmMonkey())


def pmMoose():
    return PM(580346,586970)


def Moose():
    return QtGui.QIcon(pmMoose())


def pmMouse():
    return PM(383520,389224)


def Mouse():
    return QtGui.QIcon(pmMouse())


def pmOwl():
    return PM(586970,593676)


def Owl():
    return QtGui.QIcon(pmOwl())


def pmPanda():
    return PM(593676,597710)


def Panda():
    return QtGui.QIcon(pmPanda())


def pmPenguin():
    return PM(597710,603259)


def Penguin():
    return QtGui.QIcon(pmPenguin())


def pmPig():
    return PM(603259,611299)


def Pig():
    return QtGui.QIcon(pmPig())


def pmRabbit():
    return PM(611299,618600)


def Rabbit():
    return QtGui.QIcon(pmRabbit())


def pmRhino():
    return PM(618600,624987)


def Rhino():
    return QtGui.QIcon(pmRhino())


def pmRooster():
    return PM(624987,630250)


def Rooster():
    return QtGui.QIcon(pmRooster())


def pmShark():
    return PM(630250,636020)


def Shark():
    return QtGui.QIcon(pmShark())


def pmSheep():
    return PM(636020,639851)


def Sheep():
    return QtGui.QIcon(pmSheep())


def pmSnake():
    return PM(639851,645876)


def Snake():
    return QtGui.QIcon(pmSnake())


def pmTiger():
    return PM(645876,653913)


def Tiger():
    return QtGui.QIcon(pmTiger())


def pmTurkey():
    return PM(653913,661327)


def Turkey():
    return QtGui.QIcon(pmTurkey())


def pmTurtle():
    return PM(661327,668048)


def Turtle():
    return QtGui.QIcon(pmTurtle())


def pmWolf():
    return PM(668048,671143)


def Wolf():
    return QtGui.QIcon(pmWolf())


def pmSteven():
    return PM(671143,678295)


def Steven():
    return QtGui.QIcon(pmSteven())


def pmWheel():
    return PM(678295,686360)


def Wheel():
    return QtGui.QIcon(pmWheel())


def pmWheelchair():
    return PM(686360,695164)


def Wheelchair():
    return QtGui.QIcon(pmWheelchair())


def pmTouringMotorcycle():
    return PM(695164,701476)


def TouringMotorcycle():
    return QtGui.QIcon(pmTouringMotorcycle())


def pmContainer():
    return PM(701476,706811)


def Container():
    return QtGui.QIcon(pmContainer())


def pmBoatEquipment():
    return PM(706811,712334)


def BoatEquipment():
    return QtGui.QIcon(pmBoatEquipment())


def pmCar():
    return PM(712334,716980)


def Car():
    return QtGui.QIcon(pmCar())


def pmLorry():
    return PM(716980,723016)


def Lorry():
    return QtGui.QIcon(pmLorry())


def pmCarTrailer():
    return PM(723016,727113)


def CarTrailer():
    return QtGui.QIcon(pmCarTrailer())


def pmTowTruck():
    return PM(727113,731871)


def TowTruck():
    return QtGui.QIcon(pmTowTruck())


def pmQuadBike():
    return PM(731871,737840)


def QuadBike():
    return QtGui.QIcon(pmQuadBike())


def pmRecoveryTruck():
    return PM(737840,742837)


def RecoveryTruck():
    return QtGui.QIcon(pmRecoveryTruck())


def pmContainerLoader():
    return PM(742837,747979)


def ContainerLoader():
    return QtGui.QIcon(pmContainerLoader())


def pmPoliceCar():
    return PM(747979,752811)


def PoliceCar():
    return QtGui.QIcon(pmPoliceCar())


def pmExecutiveCar():
    return PM(752811,757489)


def ExecutiveCar():
    return QtGui.QIcon(pmExecutiveCar())


def pmTruck():
    return PM(757489,762952)


def Truck():
    return QtGui.QIcon(pmTruck())


def pmExcavator():
    return PM(762952,767843)


def Excavator():
    return QtGui.QIcon(pmExcavator())


def pmCabriolet():
    return PM(767843,772681)


def Cabriolet():
    return QtGui.QIcon(pmCabriolet())


def pmMixerTruck():
    return PM(772681,778991)


def MixerTruck():
    return QtGui.QIcon(pmMixerTruck())


def pmForkliftTruckLoaded():
    return PM(778991,785139)


def ForkliftTruckLoaded():
    return QtGui.QIcon(pmForkliftTruckLoaded())


def pmAmbulance():
    return PM(785139,791189)


def Ambulance():
    return QtGui.QIcon(pmAmbulance())


def pmDieselLocomotiveBoxcar():
    return PM(791189,795195)


def DieselLocomotiveBoxcar():
    return QtGui.QIcon(pmDieselLocomotiveBoxcar())


def pmTractorUnit():
    return PM(795195,800662)


def TractorUnit():
    return QtGui.QIcon(pmTractorUnit())


def pmFireTruck():
    return PM(800662,807001)


def FireTruck():
    return QtGui.QIcon(pmFireTruck())


def pmCargoShip():
    return PM(807001,811342)


def CargoShip():
    return QtGui.QIcon(pmCargoShip())


def pmSubwayTrain():
    return PM(811342,816232)


def SubwayTrain():
    return QtGui.QIcon(pmSubwayTrain())


def pmTruckMountedCrane():
    return PM(816232,821973)


def TruckMountedCrane():
    return QtGui.QIcon(pmTruckMountedCrane())


def pmAirAmbulance():
    return PM(821973,827086)


def AirAmbulance():
    return QtGui.QIcon(pmAirAmbulance())


def pmAirplane():
    return PM(827086,831974)


def Airplane():
    return QtGui.QIcon(pmAirplane())


def pmCaracol():
    return PM(831974,833790)


def Caracol():
    return QtGui.QIcon(pmCaracol())


def pmDownloads():
    return PM(833790,835632)


def Downloads():
    return QtGui.QIcon(pmDownloads())


def pmUno():
    return PM(835632,838094)


def Uno():
    return QtGui.QIcon(pmUno())


def pmMotoresExternos():
    return PM(838094,839996)


def MotoresExternos():
    return QtGui.QIcon(pmMotoresExternos())


def pmDatabase():
    return PM(839996,840539)


def Database():
    return QtGui.QIcon(pmDatabase())


def pmDatabaseC():
    return PM(840539,840964)


def DatabaseC():
    return QtGui.QIcon(pmDatabaseC())


def pmDatabaseF():
    return PM(840964,841428)


def DatabaseF():
    return QtGui.QIcon(pmDatabaseF())


def pmDatabaseCNew():
    return PM(841428,842283)


def DatabaseCNew():
    return QtGui.QIcon(pmDatabaseCNew())


def pmDatabaseMas():
    return PM(842283,843906)


def DatabaseMas():
    return QtGui.QIcon(pmDatabaseMas())


def pmAtacante():
    return PM(843906,844511)


def Atacante():
    return QtGui.QIcon(pmAtacante())


def pmAtacada():
    return PM(844511,845077)


def Atacada():
    return QtGui.QIcon(pmAtacada())


def pmGoToNext():
    return PM(845077,845489)


def GoToNext():
    return QtGui.QIcon(pmGoToNext())


def pmBlancas():
    return PM(845489,845840)


def Blancas():
    return QtGui.QIcon(pmBlancas())


def pmNegras():
    return PM(845840,846086)


def Negras():
    return QtGui.QIcon(pmNegras())


def pmFolderChange():
    return PM(73457,76215)


def FolderChange():
    return QtGui.QIcon(pmFolderChange())


def pmMarkers():
    return PM(846086,847781)


def Markers():
    return QtGui.QIcon(pmMarkers())


def pmTop():
    return PM(847781,848365)


def Top():
    return QtGui.QIcon(pmTop())


def pmBottom():
    return PM(848365,848954)


def Bottom():
    return QtGui.QIcon(pmBottom())


def pmSTS():
    return PM(848954,851145)


def STS():
    return QtGui.QIcon(pmSTS())


def pmRun():
    return PM(851145,852869)


def Run():
    return QtGui.QIcon(pmRun())


def pmRun2():
    return PM(852869,854389)


def Run2():
    return QtGui.QIcon(pmRun2())


def pmWorldMap():
    return PM(854389,857130)


def WorldMap():
    return QtGui.QIcon(pmWorldMap())


def pmAfrica():
    return PM(857130,859616)


def Africa():
    return QtGui.QIcon(pmAfrica())


def pmMaps():
    return PM(859616,860560)


def Maps():
    return QtGui.QIcon(pmMaps())


def pmSol():
    return PM(860560,861486)


def Sol():
    return QtGui.QIcon(pmSol())


def pmSolNubes():
    return PM(861486,862349)


def SolNubes():
    return QtGui.QIcon(pmSolNubes())


def pmSolNubesLluvia():
    return PM(862349,863309)


def SolNubesLluvia():
    return QtGui.QIcon(pmSolNubesLluvia())


def pmLluvia():
    return PM(863309,864148)


def Lluvia():
    return QtGui.QIcon(pmLluvia())


def pmInvierno():
    return PM(864148,865724)


def Invierno():
    return QtGui.QIcon(pmInvierno())


def pmWords():
    return PM(865724,869509)


def Words():
    return QtGui.QIcon(pmWords())


def pmAdaptVoice():
    return PM(368587,371561)


def AdaptVoice():
    return QtGui.QIcon(pmAdaptVoice())


def pmFixedElo():
    return PM(173836,175099)


def FixedElo():
    return QtGui.QIcon(pmFixedElo())


def pmX_Microfono():
    return PM(869509,871962)


def X_Microfono():
    return QtGui.QIcon(pmX_Microfono())


def pmSoundTool():
    return PM(871962,874421)


def SoundTool():
    return QtGui.QIcon(pmSoundTool())


def pmImportar():
    return PM(874421,877089)


def Importar():
    return QtGui.QIcon(pmImportar())


def pmVoyager1():
    return PM(877089,879539)


def Voyager1():
    return QtGui.QIcon(pmVoyager1())


def pmTrain():
    return PM(879539,880909)


def Train():
    return QtGui.QIcon(pmTrain())


def pmPlay():
    return PM(249421,251510)


def Play():
    return QtGui.QIcon(pmPlay())


def pmMeasure():
    return PM(132405,134028)


def Measure():
    return QtGui.QIcon(pmMeasure())


def pmPlayGame():
    return PM(880909,885267)


def PlayGame():
    return QtGui.QIcon(pmPlayGame())


def pmScanner():
    return PM(885267,885608)


def Scanner():
    return QtGui.QIcon(pmScanner())


def pmMenos():
    return PM(885608,886133)


def Menos():
    return QtGui.QIcon(pmMenos())


def pmSchool():
    return PM(886133,886674)


def School():
    return QtGui.QIcon(pmSchool())


def pmLaw():
    return PM(886674,887290)


def Law():
    return QtGui.QIcon(pmLaw())


def pmLearnGame():
    return PM(887290,887723)


def LearnGame():
    return QtGui.QIcon(pmLearnGame())


def pmUniversity():
    return PM(887723,888143)


def University():
    return QtGui.QIcon(pmUniversity())


def pmLonghaul():
    return PM(888143,889069)


def Longhaul():
    return QtGui.QIcon(pmLonghaul())


def pmTrekking():
    return PM(889069,889763)


def Trekking():
    return QtGui.QIcon(pmTrekking())


def pmPassword():
    return PM(889763,890216)


def Password():
    return QtGui.QIcon(pmPassword())


def pmSQL_RAW():
    return PM(880909,885267)


def SQL_RAW():
    return QtGui.QIcon(pmSQL_RAW())


def pmSun():
    return PM(338631,339509)


def Sun():
    return QtGui.QIcon(pmSun())


def pmLight():
    return PM(342509,343565)


def Light():
    return QtGui.QIcon(pmLight())


def pmLight32():
    return PM(890216,891916)


def Light32():
    return QtGui.QIcon(pmLight32())


def pmTOL():
    return PM(891916,892625)


def TOL():
    return QtGui.QIcon(pmTOL())


def pmUned():
    return PM(887723,888143)


def Uned():
    return QtGui.QIcon(pmUned())


def pmUwe():
    return PM(892625,893594)


def Uwe():
    return QtGui.QIcon(pmUwe())


def pmThinking():
    return PM(893594,893966)


def Thinking():
    return QtGui.QIcon(pmThinking())


def pmWashingMachine():
    return PM(893966,894629)


def WashingMachine():
    return QtGui.QIcon(pmWashingMachine())


def pmTerminal():
    return PM(894629,898173)


def Terminal():
    return QtGui.QIcon(pmTerminal())


def pmManualSave():
    return PM(898173,898756)


def ManualSave():
    return QtGui.QIcon(pmManualSave())


def pmSettings():
    return PM(898756,899194)


def Settings():
    return QtGui.QIcon(pmSettings())


def pmStrength():
    return PM(899194,899865)


def Strength():
    return QtGui.QIcon(pmStrength())


def pmSingular():
    return PM(899865,900720)


def Singular():
    return QtGui.QIcon(pmSingular())


def pmScript():
    return PM(900720,901289)


def Script():
    return QtGui.QIcon(pmScript())


def pmScriptFree():
    return PM(901289,901849)


def ScriptFree():
    return QtGui.QIcon(pmScriptFree())


def pmTexto():
    return PM(901849,904694)


def Texto():
    return QtGui.QIcon(pmTexto())


def pmLampara():
    return PM(904694,905403)


def Lampara():
    return QtGui.QIcon(pmLampara())


def pmFile():
    return PM(905403,907703)


def File():
    return QtGui.QIcon(pmFile())


def pmCalculo():
    return PM(907703,908629)


def Calculo():
    return QtGui.QIcon(pmCalculo())


def pmOpeningLines():
    return PM(908629,909307)


def OpeningLines():
    return QtGui.QIcon(pmOpeningLines())


def pmStudy():
    return PM(909307,910220)


def Study():
    return QtGui.QIcon(pmStudy())


def pmLichess():
    return PM(910220,911110)


def Lichess():
    return QtGui.QIcon(pmLichess())


def pmMiniatura():
    return PM(911110,912037)


def Miniatura():
    return QtGui.QIcon(pmMiniatura())


def pmLocomotora():
    return PM(912037,912818)


def Locomotora():
    return QtGui.QIcon(pmLocomotora())


def pmPositions():
    return PM(912818,914409)


def Positions():
    return QtGui.QIcon(pmPositions())


def pmTrainSequential():
    return PM(914409,915550)


def TrainSequential():
    return QtGui.QIcon(pmTrainSequential())


def pmTrainStatic():
    return PM(915550,916510)


def TrainStatic():
    return QtGui.QIcon(pmTrainStatic())


def pmTrainPositions():
    return PM(916510,917491)


def TrainPositions():
    return QtGui.QIcon(pmTrainPositions())


def pmTrainEngines():
    return PM(917491,918925)


def TrainEngines():
    return QtGui.QIcon(pmTrainEngines())


def pmError():
    return PM(50641,54641)


def Error():
    return QtGui.QIcon(pmError())


def pmAtajos():
    return PM(918925,920104)


def Atajos():
    return QtGui.QIcon(pmAtajos())


def pmTOLline():
    return PM(920104,921208)


def TOLline():
    return QtGui.QIcon(pmTOLline())


def pmTOLchange():
    return PM(921208,923430)


def TOLchange():
    return QtGui.QIcon(pmTOLchange())


def pmPack():
    return PM(923430,924095)


def Pack():
    return QtGui.QIcon(pmPack())


def pmHome():
    return PM(193506,194688)


def Home():
    return QtGui.QIcon(pmHome())


def pmImport8():
    return PM(924095,925305)


def Import8():
    return QtGui.QIcon(pmImport8())


def pmExport8():
    return PM(925305,925930)


def Export8():
    return QtGui.QIcon(pmExport8())


def pmTablas8():
    return PM(925930,926722)


def Tablas8():
    return QtGui.QIcon(pmTablas8())


def pmBlancas8():
    return PM(926722,927752)


def Blancas8():
    return QtGui.QIcon(pmBlancas8())


def pmNegras8():
    return PM(927752,928591)


def Negras8():
    return QtGui.QIcon(pmNegras8())


def pmBook():
    return PM(928591,929165)


def Book():
    return QtGui.QIcon(pmBook())


def pmWrite():
    return PM(929165,930071)


def Write():
    return QtGui.QIcon(pmWrite())


def pmAlt():
    return PM(930071,930513)


def Alt():
    return QtGui.QIcon(pmAlt())


def pmShift():
    return PM(930513,930853)


def Shift():
    return QtGui.QIcon(pmShift())


def pmRightMouse():
    return PM(930853,931653)


def RightMouse():
    return QtGui.QIcon(pmRightMouse())


def pmControl():
    return PM(931653,932178)


def Control():
    return QtGui.QIcon(pmControl())


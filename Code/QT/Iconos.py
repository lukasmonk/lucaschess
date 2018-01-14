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
    return PM(288745,290430)

def Arbol():
    return QtGui.QIcon(pmArbol())

def pmGrabarFichero():
    return PM(69942,71405)

def GrabarFichero():
    return QtGui.QIcon(pmGrabarFichero())

def pmClip():
    return PM(290430,292576)

def Clip():
    return QtGui.QIcon(pmClip())

def pmFics():
    return PM(292576,292993)

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
    return PM(292993,296345)

def Flechas():
    return QtGui.QIcon(pmFlechas())

def pmMarcos():
    return PM(296345,297792)

def Marcos():
    return QtGui.QIcon(pmMarcos())

def pmSVGs():
    return PM(297792,301361)

def SVGs():
    return QtGui.QIcon(pmSVGs())

def pmAmarillo():
    return PM(301361,302613)

def Amarillo():
    return QtGui.QIcon(pmAmarillo())

def pmNaranja():
    return PM(302613,303845)

def Naranja():
    return QtGui.QIcon(pmNaranja())

def pmVerde():
    return PM(303845,305121)

def Verde():
    return QtGui.QIcon(pmVerde())

def pmAzul():
    return PM(305121,306209)

def Azul():
    return QtGui.QIcon(pmAzul())

def pmMagenta():
    return PM(306209,307497)

def Magenta():
    return QtGui.QIcon(pmMagenta())

def pmRojo():
    return PM(307497,308716)

def Rojo():
    return QtGui.QIcon(pmRojo())

def pmGris():
    return PM(308716,309674)

def Gris():
    return QtGui.QIcon(pmGris())

def pmEstrella():
    return PM(198150,199078)

def Estrella():
    return QtGui.QIcon(pmEstrella())

def pmAmarillo32():
    return PM(309674,311654)

def Amarillo32():
    return QtGui.QIcon(pmAmarillo32())

def pmNaranja32():
    return PM(311654,313778)

def Naranja32():
    return QtGui.QIcon(pmNaranja32())

def pmVerde32():
    return PM(313778,315899)

def Verde32():
    return QtGui.QIcon(pmVerde32())

def pmAzul32():
    return PM(315899,318278)

def Azul32():
    return QtGui.QIcon(pmAzul32())

def pmMagenta32():
    return PM(318278,320729)

def Magenta32():
    return QtGui.QIcon(pmMagenta32())

def pmRojo32():
    return PM(320729,322544)

def Rojo32():
    return QtGui.QIcon(pmRojo32())

def pmGris32():
    return PM(322544,324458)

def Gris32():
    return QtGui.QIcon(pmGris32())

def pmPuntoBlanco():
    return PM(324458,324807)

def PuntoBlanco():
    return QtGui.QIcon(pmPuntoBlanco())

def pmPuntoAmarillo():
    return PM(234227,234689)

def PuntoAmarillo():
    return QtGui.QIcon(pmPuntoAmarillo())

def pmPuntoNaranja():
    return PM(324807,325269)

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
    return PM(325269,325768)

def PuntoMagenta():
    return QtGui.QIcon(pmPuntoMagenta())

def pmPuntoRojo():
    return PM(325768,326267)

def PuntoRojo():
    return QtGui.QIcon(pmPuntoRojo())

def pmPuntoNegro():
    return PM(234689,235012)

def PuntoNegro():
    return QtGui.QIcon(pmPuntoNegro())

def pmPuntoEstrella():
    return PM(326267,326694)

def PuntoEstrella():
    return QtGui.QIcon(pmPuntoEstrella())

def pmComentario():
    return PM(326694,327331)

def Comentario():
    return QtGui.QIcon(pmComentario())

def pmComentarioMas():
    return PM(327331,328270)

def ComentarioMas():
    return QtGui.QIcon(pmComentarioMas())

def pmComentarioEditar():
    return PM(243628,244484)

def ComentarioEditar():
    return QtGui.QIcon(pmComentarioEditar())

def pmApertura():
    return PM(328270,329236)

def Apertura():
    return QtGui.QIcon(pmApertura())

def pmAperturaComentario():
    return PM(329236,330232)

def AperturaComentario():
    return QtGui.QIcon(pmAperturaComentario())

def pmBookGuide():
    return PM(330232,331109)

def BookGuide():
    return QtGui.QIcon(pmBookGuide())

def pmMas():
    return PM(331109,331618)

def Mas():
    return QtGui.QIcon(pmMas())

def pmMasR():
    return PM(331618,332106)

def MasR():
    return QtGui.QIcon(pmMasR())

def pmMasDoc():
    return PM(332106,332907)

def MasDoc():
    return QtGui.QIcon(pmMasDoc())

def pmNuevaDB():
    return PM(332907,337541)

def NuevaDB():
    return QtGui.QIcon(pmNuevaDB())

def pmPotencia():
    return PM(199078,199944)

def Potencia():
    return QtGui.QIcon(pmPotencia())

def pmSorpresa():
    return PM(337541,338600)

def Sorpresa():
    return QtGui.QIcon(pmSorpresa())

def pmSonrisa():
    return PM(338600,339682)

def Sonrisa():
    return QtGui.QIcon(pmSonrisa())

def pmBMT():
    return PM(339682,340560)

def BMT():
    return QtGui.QIcon(pmBMT())

def pmCorazon():
    return PM(340560,342438)

def Corazon():
    return QtGui.QIcon(pmCorazon())

def pmOjo():
    return PM(342438,343560)

def Ojo():
    return QtGui.QIcon(pmOjo())

def pmOcultar():
    return PM(342438,343560)

def Ocultar():
    return QtGui.QIcon(pmOcultar())

def pmMostrar():
    return PM(343560,344616)

def Mostrar():
    return QtGui.QIcon(pmMostrar())

def pmBlog():
    return PM(344616,345138)

def Blog():
    return QtGui.QIcon(pmBlog())

def pmVariantes():
    return PM(345138,346045)

def Variantes():
    return QtGui.QIcon(pmVariantes())

def pmVariantesG():
    return PM(346045,348472)

def VariantesG():
    return QtGui.QIcon(pmVariantesG())

def pmCambiar():
    return PM(348472,350186)

def Cambiar():
    return QtGui.QIcon(pmCambiar())

def pmAnterior():
    return PM(350186,352240)

def Anterior():
    return QtGui.QIcon(pmAnterior())

def pmSiguiente():
    return PM(352240,354310)

def Siguiente():
    return QtGui.QIcon(pmSiguiente())

def pmSiguienteF():
    return PM(354310,356485)

def SiguienteF():
    return QtGui.QIcon(pmSiguienteF())

def pmAnteriorF():
    return PM(356485,358679)

def AnteriorF():
    return QtGui.QIcon(pmAnteriorF())

def pmX():
    return PM(358679,359961)

def X():
    return QtGui.QIcon(pmX())

def pmTools():
    return PM(359961,362562)

def Tools():
    return QtGui.QIcon(pmTools())

def pmTacticas():
    return PM(362562,365135)

def Tacticas():
    return QtGui.QIcon(pmTacticas())

def pmCancelarPeque():
    return PM(365135,365996)

def CancelarPeque():
    return QtGui.QIcon(pmCancelarPeque())

def pmAceptarPeque():
    return PM(235474,236491)

def AceptarPeque():
    return QtGui.QIcon(pmAceptarPeque())

def pmP_16c():
    return PM(365996,366520)

def P_16c():
    return QtGui.QIcon(pmP_16c())

def pmLibre():
    return PM(366520,368912)

def Libre():
    return QtGui.QIcon(pmLibre())

def pmEnBlanco():
    return PM(368912,369638)

def EnBlanco():
    return QtGui.QIcon(pmEnBlanco())

def pmDirector():
    return PM(369638,372612)

def Director():
    return QtGui.QIcon(pmDirector())

def pmTorneos():
    return PM(372612,374350)

def Torneos():
    return QtGui.QIcon(pmTorneos())

def pmAperturas():
    return PM(374350,375275)

def Aperturas():
    return QtGui.QIcon(pmAperturas())

def pmV_Blancas():
    return PM(375275,375555)

def V_Blancas():
    return QtGui.QIcon(pmV_Blancas())

def pmV_Blancas_Mas():
    return PM(375555,375835)

def V_Blancas_Mas():
    return QtGui.QIcon(pmV_Blancas_Mas())

def pmV_Blancas_Mas_Mas():
    return PM(375835,376107)

def V_Blancas_Mas_Mas():
    return QtGui.QIcon(pmV_Blancas_Mas_Mas())

def pmV_Negras():
    return PM(376107,376382)

def V_Negras():
    return QtGui.QIcon(pmV_Negras())

def pmV_Negras_Mas():
    return PM(376382,376657)

def V_Negras_Mas():
    return QtGui.QIcon(pmV_Negras_Mas())

def pmV_Negras_Mas_Mas():
    return PM(376657,376926)

def V_Negras_Mas_Mas():
    return QtGui.QIcon(pmV_Negras_Mas_Mas())

def pmV_Blancas_Igual_Negras():
    return PM(376926,377228)

def V_Blancas_Igual_Negras():
    return QtGui.QIcon(pmV_Blancas_Igual_Negras())

def pmMezclar():
    return PM(151525,153921)

def Mezclar():
    return QtGui.QIcon(pmMezclar())

def pmVoyager():
    return PM(377228,379190)

def Voyager():
    return QtGui.QIcon(pmVoyager())

def pmReindexar():
    return PM(379190,381007)

def Reindexar():
    return QtGui.QIcon(pmReindexar())

def pmRename():
    return PM(381007,381991)

def Rename():
    return QtGui.QIcon(pmRename())

def pmAdd():
    return PM(381991,382944)

def Add():
    return QtGui.QIcon(pmAdd())

def pmMas22():
    return PM(382944,383608)

def Mas22():
    return QtGui.QIcon(pmMas22())

def pmMenos22():
    return PM(383608,384052)

def Menos22():
    return QtGui.QIcon(pmMenos22())

def pmTransposition():
    return PM(384052,384571)

def Transposition():
    return QtGui.QIcon(pmTransposition())

def pmRat():
    return PM(384571,390275)

def Rat():
    return QtGui.QIcon(pmRat())

def pmAlligator():
    return PM(390275,395267)

def Alligator():
    return QtGui.QIcon(pmAlligator())

def pmAnt():
    return PM(395267,401965)

def Ant():
    return QtGui.QIcon(pmAnt())

def pmBat():
    return PM(401965,404919)

def Bat():
    return QtGui.QIcon(pmBat())

def pmBear():
    return PM(404919,412198)

def Bear():
    return QtGui.QIcon(pmBear())

def pmBee():
    return PM(412198,417200)

def Bee():
    return QtGui.QIcon(pmBee())

def pmBird():
    return PM(417200,423259)

def Bird():
    return QtGui.QIcon(pmBird())

def pmBull():
    return PM(423259,430228)

def Bull():
    return QtGui.QIcon(pmBull())

def pmBulldog():
    return PM(430228,437119)

def Bulldog():
    return QtGui.QIcon(pmBulldog())

def pmButterfly():
    return PM(437119,444493)

def Butterfly():
    return QtGui.QIcon(pmButterfly())

def pmCat():
    return PM(444493,450765)

def Cat():
    return QtGui.QIcon(pmCat())

def pmChicken():
    return PM(450765,456576)

def Chicken():
    return QtGui.QIcon(pmChicken())

def pmCow():
    return PM(456576,463319)

def Cow():
    return QtGui.QIcon(pmCow())

def pmCrab():
    return PM(463319,468908)

def Crab():
    return QtGui.QIcon(pmCrab())

def pmCrocodile():
    return PM(468908,475049)

def Crocodile():
    return QtGui.QIcon(pmCrocodile())

def pmDeer():
    return PM(475049,481356)

def Deer():
    return QtGui.QIcon(pmDeer())

def pmDog():
    return PM(481356,487959)

def Dog():
    return QtGui.QIcon(pmDog())

def pmDonkey():
    return PM(487959,493606)

def Donkey():
    return QtGui.QIcon(pmDonkey())

def pmDuck():
    return PM(493606,500149)

def Duck():
    return QtGui.QIcon(pmDuck())

def pmEagle():
    return PM(500149,504967)

def Eagle():
    return QtGui.QIcon(pmEagle())

def pmElephant():
    return PM(504967,511448)

def Elephant():
    return QtGui.QIcon(pmElephant())

def pmFish():
    return PM(511448,518289)

def Fish():
    return QtGui.QIcon(pmFish())

def pmFox():
    return PM(518289,525072)

def Fox():
    return QtGui.QIcon(pmFox())

def pmFrog():
    return PM(525072,531488)

def Frog():
    return QtGui.QIcon(pmFrog())

def pmGiraffe():
    return PM(531488,538666)

def Giraffe():
    return QtGui.QIcon(pmGiraffe())

def pmGorilla():
    return PM(538666,545205)

def Gorilla():
    return QtGui.QIcon(pmGorilla())

def pmHippo():
    return PM(545205,552326)

def Hippo():
    return QtGui.QIcon(pmHippo())

def pmHorse():
    return PM(552326,558873)

def Horse():
    return QtGui.QIcon(pmHorse())

def pmInsect():
    return PM(558873,564808)

def Insect():
    return QtGui.QIcon(pmInsect())

def pmLion():
    return PM(564808,573718)

def Lion():
    return QtGui.QIcon(pmLion())

def pmMonkey():
    return PM(573718,581397)

def Monkey():
    return QtGui.QIcon(pmMonkey())

def pmMoose():
    return PM(581397,588021)

def Moose():
    return QtGui.QIcon(pmMoose())

def pmMouse():
    return PM(384571,390275)

def Mouse():
    return QtGui.QIcon(pmMouse())

def pmOwl():
    return PM(588021,594727)

def Owl():
    return QtGui.QIcon(pmOwl())

def pmPanda():
    return PM(594727,598761)

def Panda():
    return QtGui.QIcon(pmPanda())

def pmPenguin():
    return PM(598761,604310)

def Penguin():
    return QtGui.QIcon(pmPenguin())

def pmPig():
    return PM(604310,612350)

def Pig():
    return QtGui.QIcon(pmPig())

def pmRabbit():
    return PM(612350,619651)

def Rabbit():
    return QtGui.QIcon(pmRabbit())

def pmRhino():
    return PM(619651,626038)

def Rhino():
    return QtGui.QIcon(pmRhino())

def pmRooster():
    return PM(626038,631301)

def Rooster():
    return QtGui.QIcon(pmRooster())

def pmShark():
    return PM(631301,637071)

def Shark():
    return QtGui.QIcon(pmShark())

def pmSheep():
    return PM(637071,640902)

def Sheep():
    return QtGui.QIcon(pmSheep())

def pmSnake():
    return PM(640902,646927)

def Snake():
    return QtGui.QIcon(pmSnake())

def pmTiger():
    return PM(646927,654964)

def Tiger():
    return QtGui.QIcon(pmTiger())

def pmTurkey():
    return PM(654964,662378)

def Turkey():
    return QtGui.QIcon(pmTurkey())

def pmTurtle():
    return PM(662378,669099)

def Turtle():
    return QtGui.QIcon(pmTurtle())

def pmWolf():
    return PM(669099,672194)

def Wolf():
    return QtGui.QIcon(pmWolf())

def pmSteven():
    return PM(672194,679346)

def Steven():
    return QtGui.QIcon(pmSteven())

def pmWheel():
    return PM(679346,687411)

def Wheel():
    return QtGui.QIcon(pmWheel())

def pmWheelchair():
    return PM(687411,696215)

def Wheelchair():
    return QtGui.QIcon(pmWheelchair())

def pmTouringMotorcycle():
    return PM(696215,702527)

def TouringMotorcycle():
    return QtGui.QIcon(pmTouringMotorcycle())

def pmContainer():
    return PM(702527,707862)

def Container():
    return QtGui.QIcon(pmContainer())

def pmBoatEquipment():
    return PM(707862,713385)

def BoatEquipment():
    return QtGui.QIcon(pmBoatEquipment())

def pmCar():
    return PM(713385,718031)

def Car():
    return QtGui.QIcon(pmCar())

def pmLorry():
    return PM(718031,724067)

def Lorry():
    return QtGui.QIcon(pmLorry())

def pmCarTrailer():
    return PM(724067,728164)

def CarTrailer():
    return QtGui.QIcon(pmCarTrailer())

def pmTowTruck():
    return PM(728164,732922)

def TowTruck():
    return QtGui.QIcon(pmTowTruck())

def pmQuadBike():
    return PM(732922,738891)

def QuadBike():
    return QtGui.QIcon(pmQuadBike())

def pmRecoveryTruck():
    return PM(738891,743888)

def RecoveryTruck():
    return QtGui.QIcon(pmRecoveryTruck())

def pmContainerLoader():
    return PM(743888,749030)

def ContainerLoader():
    return QtGui.QIcon(pmContainerLoader())

def pmPoliceCar():
    return PM(749030,753862)

def PoliceCar():
    return QtGui.QIcon(pmPoliceCar())

def pmExecutiveCar():
    return PM(753862,758540)

def ExecutiveCar():
    return QtGui.QIcon(pmExecutiveCar())

def pmTruck():
    return PM(758540,764003)

def Truck():
    return QtGui.QIcon(pmTruck())

def pmExcavator():
    return PM(764003,768894)

def Excavator():
    return QtGui.QIcon(pmExcavator())

def pmCabriolet():
    return PM(768894,773732)

def Cabriolet():
    return QtGui.QIcon(pmCabriolet())

def pmMixerTruck():
    return PM(773732,780042)

def MixerTruck():
    return QtGui.QIcon(pmMixerTruck())

def pmForkliftTruckLoaded():
    return PM(780042,786190)

def ForkliftTruckLoaded():
    return QtGui.QIcon(pmForkliftTruckLoaded())

def pmAmbulance():
    return PM(786190,792240)

def Ambulance():
    return QtGui.QIcon(pmAmbulance())

def pmDieselLocomotiveBoxcar():
    return PM(792240,796246)

def DieselLocomotiveBoxcar():
    return QtGui.QIcon(pmDieselLocomotiveBoxcar())

def pmTractorUnit():
    return PM(796246,801713)

def TractorUnit():
    return QtGui.QIcon(pmTractorUnit())

def pmFireTruck():
    return PM(801713,808052)

def FireTruck():
    return QtGui.QIcon(pmFireTruck())

def pmCargoShip():
    return PM(808052,812393)

def CargoShip():
    return QtGui.QIcon(pmCargoShip())

def pmSubwayTrain():
    return PM(812393,817283)

def SubwayTrain():
    return QtGui.QIcon(pmSubwayTrain())

def pmTruckMountedCrane():
    return PM(817283,823024)

def TruckMountedCrane():
    return QtGui.QIcon(pmTruckMountedCrane())

def pmAirAmbulance():
    return PM(823024,828137)

def AirAmbulance():
    return QtGui.QIcon(pmAirAmbulance())

def pmAirplane():
    return PM(828137,833025)

def Airplane():
    return QtGui.QIcon(pmAirplane())

def pmCaracol():
    return PM(833025,834841)

def Caracol():
    return QtGui.QIcon(pmCaracol())

def pmDownloads():
    return PM(834841,836683)

def Downloads():
    return QtGui.QIcon(pmDownloads())

def pmUno():
    return PM(836683,839145)

def Uno():
    return QtGui.QIcon(pmUno())

def pmMotoresExternos():
    return PM(839145,841047)

def MotoresExternos():
    return QtGui.QIcon(pmMotoresExternos())

def pmDatabase():
    return PM(841047,841590)

def Database():
    return QtGui.QIcon(pmDatabase())

def pmDatabaseC():
    return PM(841590,842015)

def DatabaseC():
    return QtGui.QIcon(pmDatabaseC())

def pmDatabaseF():
    return PM(842015,842479)

def DatabaseF():
    return QtGui.QIcon(pmDatabaseF())

def pmDatabaseCNew():
    return PM(842479,843334)

def DatabaseCNew():
    return QtGui.QIcon(pmDatabaseCNew())

def pmDatabaseMas():
    return PM(843334,844957)

def DatabaseMas():
    return QtGui.QIcon(pmDatabaseMas())

def pmAtacante():
    return PM(844957,845562)

def Atacante():
    return QtGui.QIcon(pmAtacante())

def pmAtacada():
    return PM(845562,846128)

def Atacada():
    return QtGui.QIcon(pmAtacada())

def pmGoToNext():
    return PM(846128,846540)

def GoToNext():
    return QtGui.QIcon(pmGoToNext())

def pmBlancas():
    return PM(846540,846891)

def Blancas():
    return QtGui.QIcon(pmBlancas())

def pmNegras():
    return PM(846891,847137)

def Negras():
    return QtGui.QIcon(pmNegras())

def pmFolderChange():
    return PM(73457,76215)

def FolderChange():
    return QtGui.QIcon(pmFolderChange())

def pmMarkers():
    return PM(847137,848832)

def Markers():
    return QtGui.QIcon(pmMarkers())

def pmTop():
    return PM(848832,849416)

def Top():
    return QtGui.QIcon(pmTop())

def pmBottom():
    return PM(849416,850005)

def Bottom():
    return QtGui.QIcon(pmBottom())

def pmSTS():
    return PM(850005,852196)

def STS():
    return QtGui.QIcon(pmSTS())

def pmRun():
    return PM(852196,853920)

def Run():
    return QtGui.QIcon(pmRun())

def pmWorldMap():
    return PM(853920,856661)

def WorldMap():
    return QtGui.QIcon(pmWorldMap())

def pmAfrica():
    return PM(856661,859147)

def Africa():
    return QtGui.QIcon(pmAfrica())

def pmMaps():
    return PM(859147,860091)

def Maps():
    return QtGui.QIcon(pmMaps())

def pmSol():
    return PM(860091,866443)

def Sol():
    return QtGui.QIcon(pmSol())

def pmSolNubes():
    return PM(866443,872300)

def SolNubes():
    return QtGui.QIcon(pmSolNubes())

def pmNubes():
    return PM(872300,875432)

def Nubes():
    return QtGui.QIcon(pmNubes())

def pmTormenta():
    return PM(875432,880083)

def Tormenta():
    return QtGui.QIcon(pmTormenta())

def pmWords():
    return PM(880083,883868)

def Words():
    return QtGui.QIcon(pmWords())

def pmAdaptVoice():
    return PM(369638,372612)

def AdaptVoice():
    return QtGui.QIcon(pmAdaptVoice())

def pmFixedElo():
    return PM(173836,175099)

def FixedElo():
    return QtGui.QIcon(pmFixedElo())

def pmX_Microfono():
    return PM(883868,886321)

def X_Microfono():
    return QtGui.QIcon(pmX_Microfono())

def pmSoundTool():
    return PM(886321,888780)

def SoundTool():
    return QtGui.QIcon(pmSoundTool())

def pmImportar():
    return PM(888780,891448)

def Importar():
    return QtGui.QIcon(pmImportar())

def pmVoyager1():
    return PM(891448,893898)

def Voyager1():
    return QtGui.QIcon(pmVoyager1())

def pmTrain():
    return PM(893898,895268)

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
    return PM(895268,899626)

def PlayGame():
    return QtGui.QIcon(pmPlayGame())

def pmScanner():
    return PM(899626,899967)

def Scanner():
    return QtGui.QIcon(pmScanner())

def pmMenos():
    return PM(899967,900492)

def Menos():
    return QtGui.QIcon(pmMenos())

def pmSchool():
    return PM(900492,901033)

def School():
    return QtGui.QIcon(pmSchool())

def pmLaw():
    return PM(901033,901649)

def Law():
    return QtGui.QIcon(pmLaw())

def pmLearnGame():
    return PM(901649,902082)

def LearnGame():
    return QtGui.QIcon(pmLearnGame())

def pmUniversity():
    return PM(902082,902502)

def University():
    return QtGui.QIcon(pmUniversity())

def pmLonghaul():
    return PM(902502,903428)

def Longhaul():
    return QtGui.QIcon(pmLonghaul())

def pmTrekking():
    return PM(903428,904122)

def Trekking():
    return QtGui.QIcon(pmTrekking())

def pmPassword():
    return PM(904122,904575)

def Password():
    return QtGui.QIcon(pmPassword())

def pmSQL_RAW():
    return PM(895268,899626)

def SQL_RAW():
    return QtGui.QIcon(pmSQL_RAW())

def pmSun():
    return PM(339682,340560)

def Sun():
    return QtGui.QIcon(pmSun())

def pmLight():
    return PM(343560,344616)

def Light():
    return QtGui.QIcon(pmLight())

def pmLight32():
    return PM(904575,906275)

def Light32():
    return QtGui.QIcon(pmLight32())

def pmTOL():
    return PM(906275,906984)

def TOL():
    return QtGui.QIcon(pmTOL())

def pmUned():
    return PM(902082,902502)

def Uned():
    return QtGui.QIcon(pmUned())

def pmUwe():
    return PM(906984,907953)

def Uwe():
    return QtGui.QIcon(pmUwe())

def pmThinking():
    return PM(907953,908325)

def Thinking():
    return QtGui.QIcon(pmThinking())

def pmWashingMachine():
    return PM(908325,908988)

def WashingMachine():
    return QtGui.QIcon(pmWashingMachine())

def pmTerminal():
    return PM(908988,912532)

def Terminal():
    return QtGui.QIcon(pmTerminal())

def pmManualSave():
    return PM(912532,913115)

def ManualSave():
    return QtGui.QIcon(pmManualSave())

def pmSettings():
    return PM(913115,913553)

def Settings():
    return QtGui.QIcon(pmSettings())

def pmStrength():
    return PM(913553,914224)

def Strength():
    return QtGui.QIcon(pmStrength())

def pmSingular():
    return PM(914224,915079)

def Singular():
    return QtGui.QIcon(pmSingular())

def pmScript():
    return PM(915079,915648)

def Script():
    return QtGui.QIcon(pmScript())

def pmScriptFree():
    return PM(915648,916208)

def ScriptFree():
    return QtGui.QIcon(pmScriptFree())

def pmTexto():
    return PM(916208,919053)

def Texto():
    return QtGui.QIcon(pmTexto())

def pmLampara():
    return PM(919053,919762)

def Lampara():
    return QtGui.QIcon(pmLampara())

def pmFile():
    return PM(919762,922062)

def File():
    return QtGui.QIcon(pmFile())

def pmCalculo():
    return PM(922062,922988)

def Calculo():
    return QtGui.QIcon(pmCalculo())

def pmOpeningLines():
    return PM(922988,923666)

def OpeningLines():
    return QtGui.QIcon(pmOpeningLines())

def pmStudy():
    return PM(923666,924703)

def Study():
    return QtGui.QIcon(pmStudy())

def pmLichess():
    return PM(924703,925593)

def Lichess():
    return QtGui.QIcon(pmLichess())

def pmMiniatura():
    return PM(925593,926520)

def Miniatura():
    return QtGui.QIcon(pmMiniatura())


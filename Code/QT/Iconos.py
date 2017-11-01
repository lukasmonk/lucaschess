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


def pmFavoritos():
    return PM(225295,227061)


def Favoritos():
    return QtGui.QIcon(pmFavoritos())


def pmCarpeta():
    return PM(227061,227765)


def Carpeta():
    return QtGui.QIcon(pmCarpeta())


def pmDivision():
    return PM(227765,228430)


def Division():
    return QtGui.QIcon(pmDivision())


def pmDivisionF():
    return PM(228430,229544)


def DivisionF():
    return QtGui.QIcon(pmDivisionF())


def pmKibitzer():
    return PM(229544,230083)


def Kibitzer():
    return QtGui.QIcon(pmKibitzer())


def pmKibitzer_Pausa():
    return PM(230083,230947)


def Kibitzer_Pausa():
    return QtGui.QIcon(pmKibitzer_Pausa())


def pmKibitzer_Continuar():
    return PM(230947,231778)


def Kibitzer_Continuar():
    return QtGui.QIcon(pmKibitzer_Continuar())


def pmKibitzer_Terminar():
    return PM(231778,232702)


def Kibitzer_Terminar():
    return QtGui.QIcon(pmKibitzer_Terminar())


def pmDelete():
    return PM(231778,232702)


def Delete():
    return QtGui.QIcon(pmDelete())


def pmModificarP():
    return PM(232702,233768)


def ModificarP():
    return QtGui.QIcon(pmModificarP())


def pmGrupo_Si():
    return PM(233768,234230)


def Grupo_Si():
    return QtGui.QIcon(pmGrupo_Si())


def pmGrupo_No():
    return PM(234230,234553)


def Grupo_No():
    return QtGui.QIcon(pmGrupo_No())


def pmMotor_Si():
    return PM(234553,235015)


def Motor_Si():
    return QtGui.QIcon(pmMotor_Si())


def pmMotor_No():
    return PM(231778,232702)


def Motor_No():
    return QtGui.QIcon(pmMotor_No())


def pmMotor_Actual():
    return PM(235015,236032)


def Motor_Actual():
    return QtGui.QIcon(pmMotor_Actual())


def pmMotor():
    return PM(236032,236659)


def Motor():
    return QtGui.QIcon(pmMotor())


def pmMoverInicio():
    return PM(236659,237512)


def MoverInicio():
    return QtGui.QIcon(pmMoverInicio())


def pmMoverFinal():
    return PM(237512,238388)


def MoverFinal():
    return QtGui.QIcon(pmMoverFinal())


def pmMoverAdelante():
    return PM(238388,239249)


def MoverAdelante():
    return QtGui.QIcon(pmMoverAdelante())


def pmMoverAtras():
    return PM(239249,240117)


def MoverAtras():
    return QtGui.QIcon(pmMoverAtras())


def pmMoverLibre():
    return PM(240117,240937)


def MoverLibre():
    return QtGui.QIcon(pmMoverLibre())


def pmMoverTiempo():
    return PM(240937,242130)


def MoverTiempo():
    return QtGui.QIcon(pmMoverTiempo())


def pmMoverMas():
    return PM(242130,243169)


def MoverMas():
    return QtGui.QIcon(pmMoverMas())


def pmMoverGrabar():
    return PM(243169,244025)


def MoverGrabar():
    return QtGui.QIcon(pmMoverGrabar())


def pmMoverGrabarTodos():
    return PM(244025,245069)


def MoverGrabarTodos():
    return QtGui.QIcon(pmMoverGrabarTodos())


def pmMoverJugar():
    return PM(230947,231778)


def MoverJugar():
    return QtGui.QIcon(pmMoverJugar())


def pmPelicula():
    return PM(245069,247203)


def Pelicula():
    return QtGui.QIcon(pmPelicula())


def pmPelicula_Pausa():
    return PM(247203,248962)


def Pelicula_Pausa():
    return QtGui.QIcon(pmPelicula_Pausa())


def pmPelicula_Seguir():
    return PM(248962,251051)


def Pelicula_Seguir():
    return QtGui.QIcon(pmPelicula_Seguir())


def pmPelicula_Rapido():
    return PM(251051,253110)


def Pelicula_Rapido():
    return QtGui.QIcon(pmPelicula_Rapido())


def pmPelicula_Lento():
    return PM(253110,254985)


def Pelicula_Lento():
    return QtGui.QIcon(pmPelicula_Lento())


def pmPelicula_Repetir():
    return PM(39848,42142)


def Pelicula_Repetir():
    return QtGui.QIcon(pmPelicula_Repetir())


def pmPelicula_PGN():
    return PM(254985,255893)


def Pelicula_PGN():
    return QtGui.QIcon(pmPelicula_PGN())


def pmMemoria():
    return PM(255893,257834)


def Memoria():
    return QtGui.QIcon(pmMemoria())


def pmEntrenar():
    return PM(257834,259373)


def Entrenar():
    return QtGui.QIcon(pmEntrenar())


def pmEnviar():
    return PM(257834,259373)


def Enviar():
    return QtGui.QIcon(pmEnviar())


def pmTrasteros():
    return PM(259373,264176)


def Trasteros():
    return QtGui.QIcon(pmTrasteros())


def pmTrastero():
    return PM(264176,264638)


def Trastero():
    return QtGui.QIcon(pmTrastero())


def pmTrastero_Quitar():
    return PM(231778,232702)


def Trastero_Quitar():
    return QtGui.QIcon(pmTrastero_Quitar())


def pmTrastero_Nuevo():
    return PM(264638,266146)


def Trastero_Nuevo():
    return QtGui.QIcon(pmTrastero_Nuevo())


def pmNuevoMas():
    return PM(264638,266146)


def NuevoMas():
    return QtGui.QIcon(pmNuevoMas())


def pmTemas():
    return PM(266146,268369)


def Temas():
    return QtGui.QIcon(pmTemas())


def pmTutorialesCrear():
    return PM(268369,274638)


def TutorialesCrear():
    return QtGui.QIcon(pmTutorialesCrear())


def pmMover():
    return PM(274638,275220)


def Mover():
    return QtGui.QIcon(pmMover())


def pmSeleccionado():
    return PM(274638,275220)


def Seleccionado():
    return QtGui.QIcon(pmSeleccionado())


def pmSeleccionar():
    return PM(275220,280924)


def Seleccionar():
    return QtGui.QIcon(pmSeleccionar())


def pmVista():
    return PM(280924,282848)


def Vista():
    return QtGui.QIcon(pmVista())


def pmInformacionPGNUno():
    return PM(282848,284226)


def InformacionPGNUno():
    return QtGui.QIcon(pmInformacionPGNUno())


def pmDailyTest():
    return PM(284226,286566)


def DailyTest():
    return QtGui.QIcon(pmDailyTest())


def pmJuegaPorMi():
    return PM(286566,288286)


def JuegaPorMi():
    return QtGui.QIcon(pmJuegaPorMi())


def pmArbol():
    return PM(288286,289971)


def Arbol():
    return QtGui.QIcon(pmArbol())


def pmGrabarFichero():
    return PM(69942,71405)


def GrabarFichero():
    return QtGui.QIcon(pmGrabarFichero())


def pmClip():
    return PM(289971,292117)


def Clip():
    return QtGui.QIcon(pmClip())


def pmFics():
    return PM(292117,292534)


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
    return PM(292534,295886)


def Flechas():
    return QtGui.QIcon(pmFlechas())


def pmMarcos():
    return PM(295886,297333)


def Marcos():
    return QtGui.QIcon(pmMarcos())


def pmSVGs():
    return PM(297333,300902)


def SVGs():
    return QtGui.QIcon(pmSVGs())


def pmAmarillo():
    return PM(300902,302154)


def Amarillo():
    return QtGui.QIcon(pmAmarillo())


def pmNaranja():
    return PM(302154,303386)


def Naranja():
    return QtGui.QIcon(pmNaranja())


def pmVerde():
    return PM(303386,304662)


def Verde():
    return QtGui.QIcon(pmVerde())


def pmAzul():
    return PM(304662,305750)


def Azul():
    return QtGui.QIcon(pmAzul())


def pmMagenta():
    return PM(305750,307038)


def Magenta():
    return QtGui.QIcon(pmMagenta())


def pmRojo():
    return PM(307038,308257)


def Rojo():
    return QtGui.QIcon(pmRojo())


def pmGris():
    return PM(308257,309215)


def Gris():
    return QtGui.QIcon(pmGris())


def pmEstrella():
    return PM(198150,199078)


def Estrella():
    return QtGui.QIcon(pmEstrella())


def pmAmarillo32():
    return PM(309215,311195)


def Amarillo32():
    return QtGui.QIcon(pmAmarillo32())


def pmNaranja32():
    return PM(311195,313319)


def Naranja32():
    return QtGui.QIcon(pmNaranja32())


def pmVerde32():
    return PM(313319,315440)


def Verde32():
    return QtGui.QIcon(pmVerde32())


def pmAzul32():
    return PM(315440,317819)


def Azul32():
    return QtGui.QIcon(pmAzul32())


def pmMagenta32():
    return PM(317819,320270)


def Magenta32():
    return QtGui.QIcon(pmMagenta32())


def pmRojo32():
    return PM(320270,322085)


def Rojo32():
    return QtGui.QIcon(pmRojo32())


def pmGris32():
    return PM(322085,323999)


def Gris32():
    return QtGui.QIcon(pmGris32())


def pmPuntoBlanco():
    return PM(323999,324348)


def PuntoBlanco():
    return QtGui.QIcon(pmPuntoBlanco())


def pmPuntoAmarillo():
    return PM(233768,234230)


def PuntoAmarillo():
    return QtGui.QIcon(pmPuntoAmarillo())


def pmPuntoNaranja():
    return PM(324348,324810)


def PuntoNaranja():
    return QtGui.QIcon(pmPuntoNaranja())


def pmPuntoVerde():
    return PM(234553,235015)


def PuntoVerde():
    return QtGui.QIcon(pmPuntoVerde())


def pmPuntoAzul():
    return PM(264176,264638)


def PuntoAzul():
    return QtGui.QIcon(pmPuntoAzul())


def pmPuntoMagenta():
    return PM(324810,325309)


def PuntoMagenta():
    return QtGui.QIcon(pmPuntoMagenta())


def pmPuntoRojo():
    return PM(325309,325808)


def PuntoRojo():
    return QtGui.QIcon(pmPuntoRojo())


def pmPuntoNegro():
    return PM(234230,234553)


def PuntoNegro():
    return QtGui.QIcon(pmPuntoNegro())


def pmPuntoEstrella():
    return PM(325808,326235)


def PuntoEstrella():
    return QtGui.QIcon(pmPuntoEstrella())


def pmComentario():
    return PM(326235,326872)


def Comentario():
    return QtGui.QIcon(pmComentario())


def pmComentarioMas():
    return PM(326872,327811)


def ComentarioMas():
    return QtGui.QIcon(pmComentarioMas())


def pmComentarioEditar():
    return PM(243169,244025)


def ComentarioEditar():
    return QtGui.QIcon(pmComentarioEditar())


def pmApertura():
    return PM(327811,328777)


def Apertura():
    return QtGui.QIcon(pmApertura())


def pmAperturaComentario():
    return PM(328777,329773)


def AperturaComentario():
    return QtGui.QIcon(pmAperturaComentario())


def pmBookGuide():
    return PM(329773,330650)


def BookGuide():
    return QtGui.QIcon(pmBookGuide())


def pmMas():
    return PM(330650,331159)


def Mas():
    return QtGui.QIcon(pmMas())


def pmMasR():
    return PM(331159,331647)


def MasR():
    return QtGui.QIcon(pmMasR())


def pmMasDoc():
    return PM(331647,332448)


def MasDoc():
    return QtGui.QIcon(pmMasDoc())


def pmNuevaDB():
    return PM(332448,337082)


def NuevaDB():
    return QtGui.QIcon(pmNuevaDB())


def pmPotencia():
    return PM(199078,199944)


def Potencia():
    return QtGui.QIcon(pmPotencia())


def pmSorpresa():
    return PM(337082,338141)


def Sorpresa():
    return QtGui.QIcon(pmSorpresa())


def pmSonrisa():
    return PM(338141,339223)


def Sonrisa():
    return QtGui.QIcon(pmSonrisa())


def pmBMT():
    return PM(339223,340101)


def BMT():
    return QtGui.QIcon(pmBMT())


def pmCorazon():
    return PM(340101,341979)


def Corazon():
    return QtGui.QIcon(pmCorazon())


def pmOjo():
    return PM(341979,343101)


def Ojo():
    return QtGui.QIcon(pmOjo())


def pmOcultar():
    return PM(341979,343101)


def Ocultar():
    return QtGui.QIcon(pmOcultar())


def pmMostrar():
    return PM(343101,344157)


def Mostrar():
    return QtGui.QIcon(pmMostrar())


def pmBlog():
    return PM(344157,344679)


def Blog():
    return QtGui.QIcon(pmBlog())


def pmVariantes():
    return PM(344679,345586)


def Variantes():
    return QtGui.QIcon(pmVariantes())


def pmVariantesG():
    return PM(345586,348013)


def VariantesG():
    return QtGui.QIcon(pmVariantesG())


def pmCambiar():
    return PM(348013,349727)


def Cambiar():
    return QtGui.QIcon(pmCambiar())


def pmAnterior():
    return PM(349727,351781)


def Anterior():
    return QtGui.QIcon(pmAnterior())


def pmSiguiente():
    return PM(351781,353851)


def Siguiente():
    return QtGui.QIcon(pmSiguiente())


def pmSiguienteF():
    return PM(353851,356026)


def SiguienteF():
    return QtGui.QIcon(pmSiguienteF())


def pmAnteriorF():
    return PM(356026,358220)


def AnteriorF():
    return QtGui.QIcon(pmAnteriorF())


def pmX():
    return PM(358220,359502)


def X():
    return QtGui.QIcon(pmX())


def pmTools():
    return PM(359502,362103)


def Tools():
    return QtGui.QIcon(pmTools())


def pmTacticas():
    return PM(362103,364676)


def Tacticas():
    return QtGui.QIcon(pmTacticas())


def pmCancelarPeque():
    return PM(364676,365537)


def CancelarPeque():
    return QtGui.QIcon(pmCancelarPeque())


def pmAceptarPeque():
    return PM(235015,236032)


def AceptarPeque():
    return QtGui.QIcon(pmAceptarPeque())


def pmP_16c():
    return PM(365537,366061)


def P_16c():
    return QtGui.QIcon(pmP_16c())


def pmLibre():
    return PM(366061,368453)


def Libre():
    return QtGui.QIcon(pmLibre())


def pmEnBlanco():
    return PM(368453,369179)


def EnBlanco():
    return QtGui.QIcon(pmEnBlanco())


def pmDirector():
    return PM(369179,372153)


def Director():
    return QtGui.QIcon(pmDirector())


def pmTorneos():
    return PM(372153,373891)


def Torneos():
    return QtGui.QIcon(pmTorneos())


def pmAperturas():
    return PM(373891,374816)


def Aperturas():
    return QtGui.QIcon(pmAperturas())


def pmV_Blancas():
    return PM(374816,375096)


def V_Blancas():
    return QtGui.QIcon(pmV_Blancas())


def pmV_Blancas_Mas():
    return PM(375096,375376)


def V_Blancas_Mas():
    return QtGui.QIcon(pmV_Blancas_Mas())


def pmV_Blancas_Mas_Mas():
    return PM(375376,375648)


def V_Blancas_Mas_Mas():
    return QtGui.QIcon(pmV_Blancas_Mas_Mas())


def pmV_Negras():
    return PM(375648,375923)


def V_Negras():
    return QtGui.QIcon(pmV_Negras())


def pmV_Negras_Mas():
    return PM(375923,376198)


def V_Negras_Mas():
    return QtGui.QIcon(pmV_Negras_Mas())


def pmV_Negras_Mas_Mas():
    return PM(376198,376467)


def V_Negras_Mas_Mas():
    return QtGui.QIcon(pmV_Negras_Mas_Mas())


def pmV_Blancas_Igual_Negras():
    return PM(376467,376769)


def V_Blancas_Igual_Negras():
    return QtGui.QIcon(pmV_Blancas_Igual_Negras())


def pmMezclar():
    return PM(151525,153921)


def Mezclar():
    return QtGui.QIcon(pmMezclar())


def pmVoyager():
    return PM(376769,378731)


def Voyager():
    return QtGui.QIcon(pmVoyager())


def pmReindexar():
    return PM(378731,380548)


def Reindexar():
    return QtGui.QIcon(pmReindexar())


def pmRename():
    return PM(380548,381532)


def Rename():
    return QtGui.QIcon(pmRename())


def pmAdd():
    return PM(381532,382485)


def Add():
    return QtGui.QIcon(pmAdd())


def pmMas22():
    return PM(382485,383149)


def Mas22():
    return QtGui.QIcon(pmMas22())


def pmMenos22():
    return PM(383149,383593)


def Menos22():
    return QtGui.QIcon(pmMenos22())


def pmTransposition():
    return PM(383593,384443)


def Transposition():
    return QtGui.QIcon(pmTransposition())


def pmRat():
    return PM(384443,390147)


def Rat():
    return QtGui.QIcon(pmRat())


def pmAlligator():
    return PM(390147,395139)


def Alligator():
    return QtGui.QIcon(pmAlligator())


def pmAnt():
    return PM(395139,401837)


def Ant():
    return QtGui.QIcon(pmAnt())


def pmBat():
    return PM(401837,404791)


def Bat():
    return QtGui.QIcon(pmBat())


def pmBear():
    return PM(404791,412070)


def Bear():
    return QtGui.QIcon(pmBear())


def pmBee():
    return PM(412070,417072)


def Bee():
    return QtGui.QIcon(pmBee())


def pmBird():
    return PM(417072,423131)


def Bird():
    return QtGui.QIcon(pmBird())


def pmBull():
    return PM(423131,430100)


def Bull():
    return QtGui.QIcon(pmBull())


def pmBulldog():
    return PM(430100,436991)


def Bulldog():
    return QtGui.QIcon(pmBulldog())


def pmButterfly():
    return PM(436991,444365)


def Butterfly():
    return QtGui.QIcon(pmButterfly())


def pmCat():
    return PM(444365,450637)


def Cat():
    return QtGui.QIcon(pmCat())


def pmChicken():
    return PM(450637,456448)


def Chicken():
    return QtGui.QIcon(pmChicken())


def pmCow():
    return PM(456448,463191)


def Cow():
    return QtGui.QIcon(pmCow())


def pmCrab():
    return PM(463191,468780)


def Crab():
    return QtGui.QIcon(pmCrab())


def pmCrocodile():
    return PM(468780,474921)


def Crocodile():
    return QtGui.QIcon(pmCrocodile())


def pmDeer():
    return PM(474921,481228)


def Deer():
    return QtGui.QIcon(pmDeer())


def pmDog():
    return PM(481228,487831)


def Dog():
    return QtGui.QIcon(pmDog())


def pmDonkey():
    return PM(487831,493478)


def Donkey():
    return QtGui.QIcon(pmDonkey())


def pmDuck():
    return PM(493478,500021)


def Duck():
    return QtGui.QIcon(pmDuck())


def pmEagle():
    return PM(500021,504839)


def Eagle():
    return QtGui.QIcon(pmEagle())


def pmElephant():
    return PM(504839,511320)


def Elephant():
    return QtGui.QIcon(pmElephant())


def pmFish():
    return PM(511320,518161)


def Fish():
    return QtGui.QIcon(pmFish())


def pmFox():
    return PM(518161,524944)


def Fox():
    return QtGui.QIcon(pmFox())


def pmFrog():
    return PM(524944,531360)


def Frog():
    return QtGui.QIcon(pmFrog())


def pmGiraffe():
    return PM(531360,538538)


def Giraffe():
    return QtGui.QIcon(pmGiraffe())


def pmGorilla():
    return PM(538538,545077)


def Gorilla():
    return QtGui.QIcon(pmGorilla())


def pmHippo():
    return PM(545077,552198)


def Hippo():
    return QtGui.QIcon(pmHippo())


def pmHorse():
    return PM(552198,558745)


def Horse():
    return QtGui.QIcon(pmHorse())


def pmInsect():
    return PM(558745,564680)


def Insect():
    return QtGui.QIcon(pmInsect())


def pmLion():
    return PM(564680,573590)


def Lion():
    return QtGui.QIcon(pmLion())


def pmMonkey():
    return PM(573590,581269)


def Monkey():
    return QtGui.QIcon(pmMonkey())


def pmMoose():
    return PM(581269,587893)


def Moose():
    return QtGui.QIcon(pmMoose())


def pmMouse():
    return PM(384443,390147)


def Mouse():
    return QtGui.QIcon(pmMouse())


def pmOwl():
    return PM(587893,594599)


def Owl():
    return QtGui.QIcon(pmOwl())


def pmPanda():
    return PM(594599,598633)


def Panda():
    return QtGui.QIcon(pmPanda())


def pmPenguin():
    return PM(598633,604182)


def Penguin():
    return QtGui.QIcon(pmPenguin())


def pmPig():
    return PM(604182,612222)


def Pig():
    return QtGui.QIcon(pmPig())


def pmRabbit():
    return PM(612222,619523)


def Rabbit():
    return QtGui.QIcon(pmRabbit())


def pmRhino():
    return PM(619523,625910)


def Rhino():
    return QtGui.QIcon(pmRhino())


def pmRooster():
    return PM(625910,631173)


def Rooster():
    return QtGui.QIcon(pmRooster())


def pmShark():
    return PM(631173,636943)


def Shark():
    return QtGui.QIcon(pmShark())


def pmSheep():
    return PM(636943,640774)


def Sheep():
    return QtGui.QIcon(pmSheep())


def pmSnake():
    return PM(640774,646799)


def Snake():
    return QtGui.QIcon(pmSnake())


def pmTiger():
    return PM(646799,654836)


def Tiger():
    return QtGui.QIcon(pmTiger())


def pmTurkey():
    return PM(654836,662250)


def Turkey():
    return QtGui.QIcon(pmTurkey())


def pmTurtle():
    return PM(662250,668971)


def Turtle():
    return QtGui.QIcon(pmTurtle())


def pmWolf():
    return PM(668971,672066)


def Wolf():
    return QtGui.QIcon(pmWolf())


def pmSteven():
    return PM(672066,679218)


def Steven():
    return QtGui.QIcon(pmSteven())


def pmWheel():
    return PM(679218,687283)


def Wheel():
    return QtGui.QIcon(pmWheel())


def pmWheelchair():
    return PM(687283,696087)


def Wheelchair():
    return QtGui.QIcon(pmWheelchair())


def pmTouringMotorcycle():
    return PM(696087,702399)


def TouringMotorcycle():
    return QtGui.QIcon(pmTouringMotorcycle())


def pmContainer():
    return PM(702399,707734)


def Container():
    return QtGui.QIcon(pmContainer())


def pmBoatEquipment():
    return PM(707734,713257)


def BoatEquipment():
    return QtGui.QIcon(pmBoatEquipment())


def pmCar():
    return PM(713257,717903)


def Car():
    return QtGui.QIcon(pmCar())


def pmLorry():
    return PM(717903,723939)


def Lorry():
    return QtGui.QIcon(pmLorry())


def pmCarTrailer():
    return PM(723939,728036)


def CarTrailer():
    return QtGui.QIcon(pmCarTrailer())


def pmTowTruck():
    return PM(728036,732794)


def TowTruck():
    return QtGui.QIcon(pmTowTruck())


def pmQuadBike():
    return PM(732794,738763)


def QuadBike():
    return QtGui.QIcon(pmQuadBike())


def pmRecoveryTruck():
    return PM(738763,743760)


def RecoveryTruck():
    return QtGui.QIcon(pmRecoveryTruck())


def pmContainerLoader():
    return PM(743760,748902)


def ContainerLoader():
    return QtGui.QIcon(pmContainerLoader())


def pmPoliceCar():
    return PM(748902,753734)


def PoliceCar():
    return QtGui.QIcon(pmPoliceCar())


def pmExecutiveCar():
    return PM(753734,758412)


def ExecutiveCar():
    return QtGui.QIcon(pmExecutiveCar())


def pmTruck():
    return PM(758412,763875)


def Truck():
    return QtGui.QIcon(pmTruck())


def pmExcavator():
    return PM(763875,768766)


def Excavator():
    return QtGui.QIcon(pmExcavator())


def pmCabriolet():
    return PM(768766,773604)


def Cabriolet():
    return QtGui.QIcon(pmCabriolet())


def pmMixerTruck():
    return PM(773604,779914)


def MixerTruck():
    return QtGui.QIcon(pmMixerTruck())


def pmForkliftTruckLoaded():
    return PM(779914,786062)


def ForkliftTruckLoaded():
    return QtGui.QIcon(pmForkliftTruckLoaded())


def pmAmbulance():
    return PM(786062,792112)


def Ambulance():
    return QtGui.QIcon(pmAmbulance())


def pmDieselLocomotiveBoxcar():
    return PM(792112,796118)


def DieselLocomotiveBoxcar():
    return QtGui.QIcon(pmDieselLocomotiveBoxcar())


def pmTractorUnit():
    return PM(796118,801585)


def TractorUnit():
    return QtGui.QIcon(pmTractorUnit())


def pmFireTruck():
    return PM(801585,807924)


def FireTruck():
    return QtGui.QIcon(pmFireTruck())


def pmCargoShip():
    return PM(807924,812265)


def CargoShip():
    return QtGui.QIcon(pmCargoShip())


def pmSubwayTrain():
    return PM(812265,817155)


def SubwayTrain():
    return QtGui.QIcon(pmSubwayTrain())


def pmTruckMountedCrane():
    return PM(817155,822896)


def TruckMountedCrane():
    return QtGui.QIcon(pmTruckMountedCrane())


def pmAirAmbulance():
    return PM(822896,828009)


def AirAmbulance():
    return QtGui.QIcon(pmAirAmbulance())


def pmAirplane():
    return PM(828009,832897)


def Airplane():
    return QtGui.QIcon(pmAirplane())


def pmCaracol():
    return PM(832897,834713)


def Caracol():
    return QtGui.QIcon(pmCaracol())


def pmDownloads():
    return PM(834713,836555)


def Downloads():
    return QtGui.QIcon(pmDownloads())


def pmUno():
    return PM(836555,839017)


def Uno():
    return QtGui.QIcon(pmUno())


def pmMotoresExternos():
    return PM(839017,840919)


def MotoresExternos():
    return QtGui.QIcon(pmMotoresExternos())


def pmDatabase():
    return PM(840919,841462)


def Database():
    return QtGui.QIcon(pmDatabase())


def pmDatabaseC():
    return PM(841462,841887)


def DatabaseC():
    return QtGui.QIcon(pmDatabaseC())


def pmDatabaseF():
    return PM(841887,842351)


def DatabaseF():
    return QtGui.QIcon(pmDatabaseF())


def pmDatabaseCNew():
    return PM(842351,843206)


def DatabaseCNew():
    return QtGui.QIcon(pmDatabaseCNew())


def pmDatabaseMas():
    return PM(843206,844829)


def DatabaseMas():
    return QtGui.QIcon(pmDatabaseMas())


def pmAtacante():
    return PM(844829,845434)


def Atacante():
    return QtGui.QIcon(pmAtacante())


def pmAtacada():
    return PM(845434,846000)


def Atacada():
    return QtGui.QIcon(pmAtacada())


def pmGoToNext():
    return PM(846000,846412)


def GoToNext():
    return QtGui.QIcon(pmGoToNext())


def pmBlancas():
    return PM(846412,846763)


def Blancas():
    return QtGui.QIcon(pmBlancas())


def pmNegras():
    return PM(846763,847009)


def Negras():
    return QtGui.QIcon(pmNegras())


def pmFolderChange():
    return PM(73457,76215)


def FolderChange():
    return QtGui.QIcon(pmFolderChange())


def pmMarkers():
    return PM(847009,848704)


def Markers():
    return QtGui.QIcon(pmMarkers())


def pmTop():
    return PM(848704,849288)


def Top():
    return QtGui.QIcon(pmTop())


def pmBottom():
    return PM(849288,849877)


def Bottom():
    return QtGui.QIcon(pmBottom())


def pmSTS():
    return PM(849877,852068)


def STS():
    return QtGui.QIcon(pmSTS())


def pmRun():
    return PM(852068,853792)


def Run():
    return QtGui.QIcon(pmRun())


def pmWorldMap():
    return PM(853792,856533)


def WorldMap():
    return QtGui.QIcon(pmWorldMap())


def pmAfrica():
    return PM(856533,859019)


def Africa():
    return QtGui.QIcon(pmAfrica())


def pmMaps():
    return PM(859019,859963)


def Maps():
    return QtGui.QIcon(pmMaps())


def pmSol():
    return PM(859963,866315)


def Sol():
    return QtGui.QIcon(pmSol())


def pmSolNubes():
    return PM(866315,872172)


def SolNubes():
    return QtGui.QIcon(pmSolNubes())


def pmNubes():
    return PM(872172,875304)


def Nubes():
    return QtGui.QIcon(pmNubes())


def pmTormenta():
    return PM(875304,879955)


def Tormenta():
    return QtGui.QIcon(pmTormenta())


def pmWords():
    return PM(879955,883740)


def Words():
    return QtGui.QIcon(pmWords())


def pmAdaptVoice():
    return PM(369179,372153)


def AdaptVoice():
    return QtGui.QIcon(pmAdaptVoice())


def pmFixedElo():
    return PM(173836,175099)


def FixedElo():
    return QtGui.QIcon(pmFixedElo())


def pmX_Microfono():
    return PM(883740,886193)


def X_Microfono():
    return QtGui.QIcon(pmX_Microfono())


def pmSoundTool():
    return PM(886193,888652)


def SoundTool():
    return QtGui.QIcon(pmSoundTool())


def pmImportar():
    return PM(888652,891320)


def Importar():
    return QtGui.QIcon(pmImportar())


def pmVoyager1():
    return PM(891320,893770)


def Voyager1():
    return QtGui.QIcon(pmVoyager1())


def pmTrain():
    return PM(893770,895140)


def Train():
    return QtGui.QIcon(pmTrain())


def pmPlay():
    return PM(248962,251051)


def Play():
    return QtGui.QIcon(pmPlay())


def pmMeasure():
    return PM(132405,134028)


def Measure():
    return QtGui.QIcon(pmMeasure())


def pmPlayGame():
    return PM(895140,899498)


def PlayGame():
    return QtGui.QIcon(pmPlayGame())


def pmScanner():
    return PM(899498,899839)


def Scanner():
    return QtGui.QIcon(pmScanner())


def pmMenos():
    return PM(899839,900364)


def Menos():
    return QtGui.QIcon(pmMenos())


def pmSchool():
    return PM(900364,900905)


def School():
    return QtGui.QIcon(pmSchool())


def pmLaw():
    return PM(900905,901521)


def Law():
    return QtGui.QIcon(pmLaw())


def pmLearnGame():
    return PM(901521,901954)


def LearnGame():
    return QtGui.QIcon(pmLearnGame())


def pmUniversity():
    return PM(901954,902374)


def University():
    return QtGui.QIcon(pmUniversity())


def pmLonghaul():
    return PM(902374,903300)


def Longhaul():
    return QtGui.QIcon(pmLonghaul())


def pmTrekking():
    return PM(903300,903994)


def Trekking():
    return QtGui.QIcon(pmTrekking())


def pmPassword():
    return PM(903994,904447)


def Password():
    return QtGui.QIcon(pmPassword())


def pmSQL_RAW():
    return PM(895140,899498)


def SQL_RAW():
    return QtGui.QIcon(pmSQL_RAW())


def pmSun():
    return PM(339223,340101)


def Sun():
    return QtGui.QIcon(pmSun())


def pmLight():
    return PM(343101,344157)


def Light():
    return QtGui.QIcon(pmLight())


def pmLight32():
    return PM(904447,906147)


def Light32():
    return QtGui.QIcon(pmLight32())


def pmTOL():
    return PM(906147,906856)


def TOL():
    return QtGui.QIcon(pmTOL())


def pmUned():
    return PM(901954,902374)


def Uned():
    return QtGui.QIcon(pmUned())


def pmUwe():
    return PM(906856,907825)


def Uwe():
    return QtGui.QIcon(pmUwe())


def pmThinking():
    return PM(907825,908197)


def Thinking():
    return QtGui.QIcon(pmThinking())


def pmWashingMachine():
    return PM(908197,908860)


def WashingMachine():
    return QtGui.QIcon(pmWashingMachine())


def pmTerminal():
    return PM(908860,912404)


def Terminal():
    return QtGui.QIcon(pmTerminal())


def pmManualSave():
    return PM(912404,912987)


def ManualSave():
    return QtGui.QIcon(pmManualSave())


def pmSettings():
    return PM(912987,913425)


def Settings():
    return QtGui.QIcon(pmSettings())


def pmStrength():
    return PM(913425,914096)


def Strength():
    return QtGui.QIcon(pmStrength())


def pmSingular():
    return PM(914096,914951)


def Singular():
    return QtGui.QIcon(pmSingular())


def pmScript():
    return PM(914951,915520)


def Script():
    return QtGui.QIcon(pmScript())


def pmScriptFree():
    return PM(915520,916080)


def ScriptFree():
    return QtGui.QIcon(pmScriptFree())


def pmTexto():
    return PM(916080,918925)


def Texto():
    return QtGui.QIcon(pmTexto())


def pmLampara():
    return PM(918925,919634)


def Lampara():
    return QtGui.QIcon(pmLampara())


def pmFile():
    return PM(919634,921934)


def File():
    return QtGui.QIcon(pmFile())


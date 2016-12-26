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
    return PM(229544,229992)

def Kibitzer():
    return QtGui.QIcon(pmKibitzer())

def pmKibitzer_Pausa():
    return PM(229992,230856)

def Kibitzer_Pausa():
    return QtGui.QIcon(pmKibitzer_Pausa())

def pmKibitzer_Continuar():
    return PM(230856,231687)

def Kibitzer_Continuar():
    return QtGui.QIcon(pmKibitzer_Continuar())

def pmKibitzer_Terminar():
    return PM(231687,232611)

def Kibitzer_Terminar():
    return QtGui.QIcon(pmKibitzer_Terminar())

def pmDelete():
    return PM(231687,232611)

def Delete():
    return QtGui.QIcon(pmDelete())

def pmModificarP():
    return PM(232611,233677)

def ModificarP():
    return QtGui.QIcon(pmModificarP())

def pmGrupo_Si():
    return PM(233677,234139)

def Grupo_Si():
    return QtGui.QIcon(pmGrupo_Si())

def pmGrupo_No():
    return PM(234139,234462)

def Grupo_No():
    return QtGui.QIcon(pmGrupo_No())

def pmMotor_Si():
    return PM(234462,234924)

def Motor_Si():
    return QtGui.QIcon(pmMotor_Si())

def pmMotor_No():
    return PM(231687,232611)

def Motor_No():
    return QtGui.QIcon(pmMotor_No())

def pmMotor_Actual():
    return PM(234924,235941)

def Motor_Actual():
    return QtGui.QIcon(pmMotor_Actual())

def pmMotor():
    return PM(235941,236568)

def Motor():
    return QtGui.QIcon(pmMotor())

def pmMoverInicio():
    return PM(236568,237421)

def MoverInicio():
    return QtGui.QIcon(pmMoverInicio())

def pmMoverFinal():
    return PM(237421,238297)

def MoverFinal():
    return QtGui.QIcon(pmMoverFinal())

def pmMoverAdelante():
    return PM(238297,239158)

def MoverAdelante():
    return QtGui.QIcon(pmMoverAdelante())

def pmMoverAtras():
    return PM(239158,240026)

def MoverAtras():
    return QtGui.QIcon(pmMoverAtras())

def pmMoverLibre():
    return PM(240026,240846)

def MoverLibre():
    return QtGui.QIcon(pmMoverLibre())

def pmMoverTiempo():
    return PM(240846,242039)

def MoverTiempo():
    return QtGui.QIcon(pmMoverTiempo())

def pmMoverMas():
    return PM(242039,243078)

def MoverMas():
    return QtGui.QIcon(pmMoverMas())

def pmMoverGrabar():
    return PM(243078,243934)

def MoverGrabar():
    return QtGui.QIcon(pmMoverGrabar())

def pmMoverGrabarTodos():
    return PM(243934,244978)

def MoverGrabarTodos():
    return QtGui.QIcon(pmMoverGrabarTodos())

def pmMoverJugar():
    return PM(230856,231687)

def MoverJugar():
    return QtGui.QIcon(pmMoverJugar())

def pmPelicula():
    return PM(244978,247112)

def Pelicula():
    return QtGui.QIcon(pmPelicula())

def pmPelicula_Pausa():
    return PM(247112,248871)

def Pelicula_Pausa():
    return QtGui.QIcon(pmPelicula_Pausa())

def pmPelicula_Seguir():
    return PM(248871,250960)

def Pelicula_Seguir():
    return QtGui.QIcon(pmPelicula_Seguir())

def pmPelicula_Rapido():
    return PM(250960,253019)

def Pelicula_Rapido():
    return QtGui.QIcon(pmPelicula_Rapido())

def pmPelicula_Lento():
    return PM(253019,254894)

def Pelicula_Lento():
    return QtGui.QIcon(pmPelicula_Lento())

def pmPelicula_Repetir():
    return PM(39848,42142)

def Pelicula_Repetir():
    return QtGui.QIcon(pmPelicula_Repetir())

def pmPelicula_PGN():
    return PM(254894,255802)

def Pelicula_PGN():
    return QtGui.QIcon(pmPelicula_PGN())

def pmMemoria():
    return PM(255802,257743)

def Memoria():
    return QtGui.QIcon(pmMemoria())

def pmEntrenar():
    return PM(257743,259282)

def Entrenar():
    return QtGui.QIcon(pmEntrenar())

def pmEnviar():
    return PM(257743,259282)

def Enviar():
    return QtGui.QIcon(pmEnviar())

def pmTrasteros():
    return PM(259282,264085)

def Trasteros():
    return QtGui.QIcon(pmTrasteros())

def pmTrastero():
    return PM(264085,264547)

def Trastero():
    return QtGui.QIcon(pmTrastero())

def pmTrastero_Quitar():
    return PM(231687,232611)

def Trastero_Quitar():
    return QtGui.QIcon(pmTrastero_Quitar())

def pmTrastero_Nuevo():
    return PM(264547,266055)

def Trastero_Nuevo():
    return QtGui.QIcon(pmTrastero_Nuevo())

def pmNuevoMas():
    return PM(264547,266055)

def NuevoMas():
    return QtGui.QIcon(pmNuevoMas())

def pmTemas():
    return PM(266055,268278)

def Temas():
    return QtGui.QIcon(pmTemas())

def pmTutorialesCrear():
    return PM(268278,274547)

def TutorialesCrear():
    return QtGui.QIcon(pmTutorialesCrear())

def pmMover():
    return PM(274547,275129)

def Mover():
    return QtGui.QIcon(pmMover())

def pmSeleccionado():
    return PM(274547,275129)

def Seleccionado():
    return QtGui.QIcon(pmSeleccionado())

def pmSeleccionar():
    return PM(275129,280833)

def Seleccionar():
    return QtGui.QIcon(pmSeleccionar())

def pmVista():
    return PM(280833,282757)

def Vista():
    return QtGui.QIcon(pmVista())

def pmInformacionPGNUno():
    return PM(282757,284135)

def InformacionPGNUno():
    return QtGui.QIcon(pmInformacionPGNUno())

def pmDailyTest():
    return PM(284135,286475)

def DailyTest():
    return QtGui.QIcon(pmDailyTest())

def pmJuegaPorMi():
    return PM(286475,288195)

def JuegaPorMi():
    return QtGui.QIcon(pmJuegaPorMi())

def pmArbol():
    return PM(288195,289880)

def Arbol():
    return QtGui.QIcon(pmArbol())

def pmGrabarFichero():
    return PM(69942,71405)

def GrabarFichero():
    return QtGui.QIcon(pmGrabarFichero())

def pmClip():
    return PM(289880,292026)

def Clip():
    return QtGui.QIcon(pmClip())

def pmFics():
    return PM(292026,292443)

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
    return PM(292443,295795)

def Flechas():
    return QtGui.QIcon(pmFlechas())

def pmMarcos():
    return PM(295795,297242)

def Marcos():
    return QtGui.QIcon(pmMarcos())

def pmSVGs():
    return PM(297242,300811)

def SVGs():
    return QtGui.QIcon(pmSVGs())

def pmAmarillo():
    return PM(300811,302063)

def Amarillo():
    return QtGui.QIcon(pmAmarillo())

def pmNaranja():
    return PM(302063,303295)

def Naranja():
    return QtGui.QIcon(pmNaranja())

def pmVerde():
    return PM(303295,304571)

def Verde():
    return QtGui.QIcon(pmVerde())

def pmAzul():
    return PM(304571,305659)

def Azul():
    return QtGui.QIcon(pmAzul())

def pmMagenta():
    return PM(305659,306947)

def Magenta():
    return QtGui.QIcon(pmMagenta())

def pmRojo():
    return PM(306947,308166)

def Rojo():
    return QtGui.QIcon(pmRojo())

def pmGris():
    return PM(308166,309124)

def Gris():
    return QtGui.QIcon(pmGris())

def pmEstrella():
    return PM(198150,199078)

def Estrella():
    return QtGui.QIcon(pmEstrella())

def pmAmarillo32():
    return PM(309124,311104)

def Amarillo32():
    return QtGui.QIcon(pmAmarillo32())

def pmNaranja32():
    return PM(311104,313228)

def Naranja32():
    return QtGui.QIcon(pmNaranja32())

def pmVerde32():
    return PM(313228,315349)

def Verde32():
    return QtGui.QIcon(pmVerde32())

def pmAzul32():
    return PM(315349,317728)

def Azul32():
    return QtGui.QIcon(pmAzul32())

def pmMagenta32():
    return PM(317728,320179)

def Magenta32():
    return QtGui.QIcon(pmMagenta32())

def pmRojo32():
    return PM(320179,321994)

def Rojo32():
    return QtGui.QIcon(pmRojo32())

def pmGris32():
    return PM(321994,323908)

def Gris32():
    return QtGui.QIcon(pmGris32())

def pmPuntoBlanco():
    return PM(323908,324257)

def PuntoBlanco():
    return QtGui.QIcon(pmPuntoBlanco())

def pmPuntoAmarillo():
    return PM(233677,234139)

def PuntoAmarillo():
    return QtGui.QIcon(pmPuntoAmarillo())

def pmPuntoNaranja():
    return PM(324257,324719)

def PuntoNaranja():
    return QtGui.QIcon(pmPuntoNaranja())

def pmPuntoVerde():
    return PM(234462,234924)

def PuntoVerde():
    return QtGui.QIcon(pmPuntoVerde())

def pmPuntoAzul():
    return PM(264085,264547)

def PuntoAzul():
    return QtGui.QIcon(pmPuntoAzul())

def pmPuntoMagenta():
    return PM(324719,325218)

def PuntoMagenta():
    return QtGui.QIcon(pmPuntoMagenta())

def pmPuntoRojo():
    return PM(325218,325717)

def PuntoRojo():
    return QtGui.QIcon(pmPuntoRojo())

def pmPuntoNegro():
    return PM(234139,234462)

def PuntoNegro():
    return QtGui.QIcon(pmPuntoNegro())

def pmPuntoEstrella():
    return PM(325717,326144)

def PuntoEstrella():
    return QtGui.QIcon(pmPuntoEstrella())

def pmComentario():
    return PM(326144,326781)

def Comentario():
    return QtGui.QIcon(pmComentario())

def pmComentarioMas():
    return PM(326781,327720)

def ComentarioMas():
    return QtGui.QIcon(pmComentarioMas())

def pmComentarioEditar():
    return PM(243078,243934)

def ComentarioEditar():
    return QtGui.QIcon(pmComentarioEditar())

def pmApertura():
    return PM(327720,328686)

def Apertura():
    return QtGui.QIcon(pmApertura())

def pmAperturaComentario():
    return PM(328686,329682)

def AperturaComentario():
    return QtGui.QIcon(pmAperturaComentario())

def pmBookGuide():
    return PM(329682,330559)

def BookGuide():
    return QtGui.QIcon(pmBookGuide())

def pmMas():
    return PM(330559,331068)

def Mas():
    return QtGui.QIcon(pmMas())

def pmMasR():
    return PM(331068,331556)

def MasR():
    return QtGui.QIcon(pmMasR())

def pmMasDoc():
    return PM(331556,332357)

def MasDoc():
    return QtGui.QIcon(pmMasDoc())

def pmNuevaDB():
    return PM(332357,336991)

def NuevaDB():
    return QtGui.QIcon(pmNuevaDB())

def pmPotencia():
    return PM(199078,199944)

def Potencia():
    return QtGui.QIcon(pmPotencia())

def pmSorpresa():
    return PM(336991,338050)

def Sorpresa():
    return QtGui.QIcon(pmSorpresa())

def pmSonrisa():
    return PM(338050,339132)

def Sonrisa():
    return QtGui.QIcon(pmSonrisa())

def pmBMT():
    return PM(339132,340010)

def BMT():
    return QtGui.QIcon(pmBMT())

def pmCorazon():
    return PM(340010,341888)

def Corazon():
    return QtGui.QIcon(pmCorazon())

def pmOjo():
    return PM(341888,343010)

def Ojo():
    return QtGui.QIcon(pmOjo())

def pmOcultar():
    return PM(341888,343010)

def Ocultar():
    return QtGui.QIcon(pmOcultar())

def pmMostrar():
    return PM(343010,344066)

def Mostrar():
    return QtGui.QIcon(pmMostrar())

def pmBlog():
    return PM(344066,344588)

def Blog():
    return QtGui.QIcon(pmBlog())

def pmVariantes():
    return PM(344588,345495)

def Variantes():
    return QtGui.QIcon(pmVariantes())

def pmVariantesG():
    return PM(345495,347922)

def VariantesG():
    return QtGui.QIcon(pmVariantesG())

def pmCambiar():
    return PM(347922,349636)

def Cambiar():
    return QtGui.QIcon(pmCambiar())

def pmAnterior():
    return PM(349636,351690)

def Anterior():
    return QtGui.QIcon(pmAnterior())

def pmSiguiente():
    return PM(351690,353760)

def Siguiente():
    return QtGui.QIcon(pmSiguiente())

def pmSiguienteF():
    return PM(353760,355935)

def SiguienteF():
    return QtGui.QIcon(pmSiguienteF())

def pmAnteriorF():
    return PM(355935,358129)

def AnteriorF():
    return QtGui.QIcon(pmAnteriorF())

def pmX():
    return PM(358129,359411)

def X():
    return QtGui.QIcon(pmX())

def pmTools():
    return PM(359411,362012)

def Tools():
    return QtGui.QIcon(pmTools())

def pmTacticas():
    return PM(362012,364585)

def Tacticas():
    return QtGui.QIcon(pmTacticas())

def pmCancelarPeque():
    return PM(364585,365446)

def CancelarPeque():
    return QtGui.QIcon(pmCancelarPeque())

def pmAceptarPeque():
    return PM(234924,235941)

def AceptarPeque():
    return QtGui.QIcon(pmAceptarPeque())

def pmP_16c():
    return PM(365446,365970)

def P_16c():
    return QtGui.QIcon(pmP_16c())

def pmLibre():
    return PM(365970,368362)

def Libre():
    return QtGui.QIcon(pmLibre())

def pmEnBlanco():
    return PM(368362,369088)

def EnBlanco():
    return QtGui.QIcon(pmEnBlanco())

def pmDirector():
    return PM(369088,372062)

def Director():
    return QtGui.QIcon(pmDirector())

def pmTorneos():
    return PM(372062,373800)

def Torneos():
    return QtGui.QIcon(pmTorneos())

def pmAperturas():
    return PM(373800,374725)

def Aperturas():
    return QtGui.QIcon(pmAperturas())

def pmV_Blancas():
    return PM(374725,375005)

def V_Blancas():
    return QtGui.QIcon(pmV_Blancas())

def pmV_Blancas_Mas():
    return PM(375005,375285)

def V_Blancas_Mas():
    return QtGui.QIcon(pmV_Blancas_Mas())

def pmV_Blancas_Mas_Mas():
    return PM(375285,375557)

def V_Blancas_Mas_Mas():
    return QtGui.QIcon(pmV_Blancas_Mas_Mas())

def pmV_Negras():
    return PM(375557,375832)

def V_Negras():
    return QtGui.QIcon(pmV_Negras())

def pmV_Negras_Mas():
    return PM(375832,376107)

def V_Negras_Mas():
    return QtGui.QIcon(pmV_Negras_Mas())

def pmV_Negras_Mas_Mas():
    return PM(376107,376376)

def V_Negras_Mas_Mas():
    return QtGui.QIcon(pmV_Negras_Mas_Mas())

def pmV_Blancas_Igual_Negras():
    return PM(376376,376678)

def V_Blancas_Igual_Negras():
    return QtGui.QIcon(pmV_Blancas_Igual_Negras())

def pmMezclar():
    return PM(151525,153921)

def Mezclar():
    return QtGui.QIcon(pmMezclar())

def pmVoyager():
    return PM(376678,378640)

def Voyager():
    return QtGui.QIcon(pmVoyager())

def pmReindexar():
    return PM(378640,380457)

def Reindexar():
    return QtGui.QIcon(pmReindexar())

def pmRename():
    return PM(380457,381441)

def Rename():
    return QtGui.QIcon(pmRename())

def pmAdd():
    return PM(381441,382394)

def Add():
    return QtGui.QIcon(pmAdd())

def pmMas22():
    return PM(382394,383058)

def Mas22():
    return QtGui.QIcon(pmMas22())

def pmMenos22():
    return PM(383058,383502)

def Menos22():
    return QtGui.QIcon(pmMenos22())

def pmTransposition():
    return PM(383502,384352)

def Transposition():
    return QtGui.QIcon(pmTransposition())

def pmRat():
    return PM(384352,390056)

def Rat():
    return QtGui.QIcon(pmRat())

def pmAlligator():
    return PM(390056,395048)

def Alligator():
    return QtGui.QIcon(pmAlligator())

def pmAnt():
    return PM(395048,401746)

def Ant():
    return QtGui.QIcon(pmAnt())

def pmBat():
    return PM(401746,404700)

def Bat():
    return QtGui.QIcon(pmBat())

def pmBear():
    return PM(404700,411979)

def Bear():
    return QtGui.QIcon(pmBear())

def pmBee():
    return PM(411979,416981)

def Bee():
    return QtGui.QIcon(pmBee())

def pmBird():
    return PM(416981,423040)

def Bird():
    return QtGui.QIcon(pmBird())

def pmBull():
    return PM(423040,430009)

def Bull():
    return QtGui.QIcon(pmBull())

def pmBulldog():
    return PM(430009,436900)

def Bulldog():
    return QtGui.QIcon(pmBulldog())

def pmButterfly():
    return PM(436900,444274)

def Butterfly():
    return QtGui.QIcon(pmButterfly())

def pmCat():
    return PM(444274,450546)

def Cat():
    return QtGui.QIcon(pmCat())

def pmChicken():
    return PM(450546,456357)

def Chicken():
    return QtGui.QIcon(pmChicken())

def pmCow():
    return PM(456357,463100)

def Cow():
    return QtGui.QIcon(pmCow())

def pmCrab():
    return PM(463100,468689)

def Crab():
    return QtGui.QIcon(pmCrab())

def pmCrocodile():
    return PM(468689,474830)

def Crocodile():
    return QtGui.QIcon(pmCrocodile())

def pmDeer():
    return PM(474830,481137)

def Deer():
    return QtGui.QIcon(pmDeer())

def pmDog():
    return PM(481137,487740)

def Dog():
    return QtGui.QIcon(pmDog())

def pmDonkey():
    return PM(487740,493387)

def Donkey():
    return QtGui.QIcon(pmDonkey())

def pmDuck():
    return PM(493387,499930)

def Duck():
    return QtGui.QIcon(pmDuck())

def pmEagle():
    return PM(499930,504748)

def Eagle():
    return QtGui.QIcon(pmEagle())

def pmElephant():
    return PM(504748,511229)

def Elephant():
    return QtGui.QIcon(pmElephant())

def pmFish():
    return PM(511229,518070)

def Fish():
    return QtGui.QIcon(pmFish())

def pmFox():
    return PM(518070,524853)

def Fox():
    return QtGui.QIcon(pmFox())

def pmFrog():
    return PM(524853,531269)

def Frog():
    return QtGui.QIcon(pmFrog())

def pmGiraffe():
    return PM(531269,538447)

def Giraffe():
    return QtGui.QIcon(pmGiraffe())

def pmGorilla():
    return PM(538447,544986)

def Gorilla():
    return QtGui.QIcon(pmGorilla())

def pmHippo():
    return PM(544986,552107)

def Hippo():
    return QtGui.QIcon(pmHippo())

def pmHorse():
    return PM(552107,558654)

def Horse():
    return QtGui.QIcon(pmHorse())

def pmInsect():
    return PM(558654,564589)

def Insect():
    return QtGui.QIcon(pmInsect())

def pmLion():
    return PM(564589,573499)

def Lion():
    return QtGui.QIcon(pmLion())

def pmMonkey():
    return PM(573499,581178)

def Monkey():
    return QtGui.QIcon(pmMonkey())

def pmMoose():
    return PM(581178,587802)

def Moose():
    return QtGui.QIcon(pmMoose())

def pmMouse():
    return PM(384352,390056)

def Mouse():
    return QtGui.QIcon(pmMouse())

def pmOwl():
    return PM(587802,594508)

def Owl():
    return QtGui.QIcon(pmOwl())

def pmPanda():
    return PM(594508,598542)

def Panda():
    return QtGui.QIcon(pmPanda())

def pmPenguin():
    return PM(598542,604091)

def Penguin():
    return QtGui.QIcon(pmPenguin())

def pmPig():
    return PM(604091,612131)

def Pig():
    return QtGui.QIcon(pmPig())

def pmRabbit():
    return PM(612131,619432)

def Rabbit():
    return QtGui.QIcon(pmRabbit())

def pmRhino():
    return PM(619432,625819)

def Rhino():
    return QtGui.QIcon(pmRhino())

def pmRooster():
    return PM(625819,631082)

def Rooster():
    return QtGui.QIcon(pmRooster())

def pmShark():
    return PM(631082,636852)

def Shark():
    return QtGui.QIcon(pmShark())

def pmSheep():
    return PM(636852,640683)

def Sheep():
    return QtGui.QIcon(pmSheep())

def pmSnake():
    return PM(640683,646708)

def Snake():
    return QtGui.QIcon(pmSnake())

def pmTiger():
    return PM(646708,654745)

def Tiger():
    return QtGui.QIcon(pmTiger())

def pmTurkey():
    return PM(654745,662159)

def Turkey():
    return QtGui.QIcon(pmTurkey())

def pmTurtle():
    return PM(662159,668880)

def Turtle():
    return QtGui.QIcon(pmTurtle())

def pmWolf():
    return PM(668880,671975)

def Wolf():
    return QtGui.QIcon(pmWolf())

def pmSteven():
    return PM(671975,679127)

def Steven():
    return QtGui.QIcon(pmSteven())

def pmWheel():
    return PM(679127,687192)

def Wheel():
    return QtGui.QIcon(pmWheel())

def pmWheelchair():
    return PM(687192,695996)

def Wheelchair():
    return QtGui.QIcon(pmWheelchair())

def pmTouringMotorcycle():
    return PM(695996,702308)

def TouringMotorcycle():
    return QtGui.QIcon(pmTouringMotorcycle())

def pmContainer():
    return PM(702308,707643)

def Container():
    return QtGui.QIcon(pmContainer())

def pmBoatEquipment():
    return PM(707643,713166)

def BoatEquipment():
    return QtGui.QIcon(pmBoatEquipment())

def pmCar():
    return PM(713166,717812)

def Car():
    return QtGui.QIcon(pmCar())

def pmLorry():
    return PM(717812,723848)

def Lorry():
    return QtGui.QIcon(pmLorry())

def pmCarTrailer():
    return PM(723848,727945)

def CarTrailer():
    return QtGui.QIcon(pmCarTrailer())

def pmTowTruck():
    return PM(727945,732703)

def TowTruck():
    return QtGui.QIcon(pmTowTruck())

def pmQuadBike():
    return PM(732703,738672)

def QuadBike():
    return QtGui.QIcon(pmQuadBike())

def pmRecoveryTruck():
    return PM(738672,743669)

def RecoveryTruck():
    return QtGui.QIcon(pmRecoveryTruck())

def pmContainerLoader():
    return PM(743669,748811)

def ContainerLoader():
    return QtGui.QIcon(pmContainerLoader())

def pmPoliceCar():
    return PM(748811,753643)

def PoliceCar():
    return QtGui.QIcon(pmPoliceCar())

def pmExecutiveCar():
    return PM(753643,758321)

def ExecutiveCar():
    return QtGui.QIcon(pmExecutiveCar())

def pmTruck():
    return PM(758321,763784)

def Truck():
    return QtGui.QIcon(pmTruck())

def pmExcavator():
    return PM(763784,768675)

def Excavator():
    return QtGui.QIcon(pmExcavator())

def pmCabriolet():
    return PM(768675,773513)

def Cabriolet():
    return QtGui.QIcon(pmCabriolet())

def pmMixerTruck():
    return PM(773513,779823)

def MixerTruck():
    return QtGui.QIcon(pmMixerTruck())

def pmForkliftTruckLoaded():
    return PM(779823,785971)

def ForkliftTruckLoaded():
    return QtGui.QIcon(pmForkliftTruckLoaded())

def pmAmbulance():
    return PM(785971,792021)

def Ambulance():
    return QtGui.QIcon(pmAmbulance())

def pmDieselLocomotiveBoxcar():
    return PM(792021,796027)

def DieselLocomotiveBoxcar():
    return QtGui.QIcon(pmDieselLocomotiveBoxcar())

def pmTractorUnit():
    return PM(796027,801494)

def TractorUnit():
    return QtGui.QIcon(pmTractorUnit())

def pmFireTruck():
    return PM(801494,807833)

def FireTruck():
    return QtGui.QIcon(pmFireTruck())

def pmCargoShip():
    return PM(807833,812174)

def CargoShip():
    return QtGui.QIcon(pmCargoShip())

def pmSubwayTrain():
    return PM(812174,817064)

def SubwayTrain():
    return QtGui.QIcon(pmSubwayTrain())

def pmTruckMountedCrane():
    return PM(817064,822805)

def TruckMountedCrane():
    return QtGui.QIcon(pmTruckMountedCrane())

def pmAirAmbulance():
    return PM(822805,827918)

def AirAmbulance():
    return QtGui.QIcon(pmAirAmbulance())

def pmAirplane():
    return PM(827918,832806)

def Airplane():
    return QtGui.QIcon(pmAirplane())

def pmCaracol():
    return PM(832806,834622)

def Caracol():
    return QtGui.QIcon(pmCaracol())

def pmDownloads():
    return PM(834622,836464)

def Downloads():
    return QtGui.QIcon(pmDownloads())

def pmUno():
    return PM(836464,838926)

def Uno():
    return QtGui.QIcon(pmUno())

def pmMotoresExternos():
    return PM(838926,840828)

def MotoresExternos():
    return QtGui.QIcon(pmMotoresExternos())

def pmDatabase():
    return PM(840828,841371)

def Database():
    return QtGui.QIcon(pmDatabase())

def pmDatabaseC():
    return PM(841371,841796)

def DatabaseC():
    return QtGui.QIcon(pmDatabaseC())

def pmDatabaseF():
    return PM(841796,842260)

def DatabaseF():
    return QtGui.QIcon(pmDatabaseF())

def pmDatabaseCNew():
    return PM(842260,843115)

def DatabaseCNew():
    return QtGui.QIcon(pmDatabaseCNew())

def pmDatabaseMas():
    return PM(843115,844738)

def DatabaseMas():
    return QtGui.QIcon(pmDatabaseMas())

def pmAtacante():
    return PM(844738,845343)

def Atacante():
    return QtGui.QIcon(pmAtacante())

def pmAtacada():
    return PM(845343,845909)

def Atacada():
    return QtGui.QIcon(pmAtacada())

def pmGoToNext():
    return PM(845909,846321)

def GoToNext():
    return QtGui.QIcon(pmGoToNext())

def pmBlancas():
    return PM(846321,846672)

def Blancas():
    return QtGui.QIcon(pmBlancas())

def pmNegras():
    return PM(846672,846918)

def Negras():
    return QtGui.QIcon(pmNegras())

def pmFolderChange():
    return PM(73457,76215)

def FolderChange():
    return QtGui.QIcon(pmFolderChange())

def pmMarkers():
    return PM(846918,848613)

def Markers():
    return QtGui.QIcon(pmMarkers())

def pmTop():
    return PM(848613,849197)

def Top():
    return QtGui.QIcon(pmTop())

def pmBottom():
    return PM(849197,849786)

def Bottom():
    return QtGui.QIcon(pmBottom())

def pmSTS():
    return PM(849786,851977)

def STS():
    return QtGui.QIcon(pmSTS())

def pmRun():
    return PM(851977,853701)

def Run():
    return QtGui.QIcon(pmRun())

def pmWorldMap():
    return PM(853701,856442)

def WorldMap():
    return QtGui.QIcon(pmWorldMap())

def pmAfrica():
    return PM(856442,858928)

def Africa():
    return QtGui.QIcon(pmAfrica())

def pmMaps():
    return PM(858928,859872)

def Maps():
    return QtGui.QIcon(pmMaps())

def pmSol():
    return PM(859872,866224)

def Sol():
    return QtGui.QIcon(pmSol())

def pmSolNubes():
    return PM(866224,872081)

def SolNubes():
    return QtGui.QIcon(pmSolNubes())

def pmNubes():
    return PM(872081,875213)

def Nubes():
    return QtGui.QIcon(pmNubes())

def pmTormenta():
    return PM(875213,879864)

def Tormenta():
    return QtGui.QIcon(pmTormenta())

def pmWords():
    return PM(879864,883649)

def Words():
    return QtGui.QIcon(pmWords())

def pmAdaptVoice():
    return PM(369088,372062)

def AdaptVoice():
    return QtGui.QIcon(pmAdaptVoice())

def pmFixedElo():
    return PM(173836,175099)

def FixedElo():
    return QtGui.QIcon(pmFixedElo())

def pmX_Microfono():
    return PM(883649,886102)

def X_Microfono():
    return QtGui.QIcon(pmX_Microfono())

def pmSoundTool():
    return PM(886102,888561)

def SoundTool():
    return QtGui.QIcon(pmSoundTool())

def pmImportar():
    return PM(888561,891229)

def Importar():
    return QtGui.QIcon(pmImportar())

def pmVoyager1():
    return PM(891229,893679)

def Voyager1():
    return QtGui.QIcon(pmVoyager1())

def pmTrain():
    return PM(893679,895049)

def Train():
    return QtGui.QIcon(pmTrain())

def pmPlay():
    return PM(248871,250960)

def Play():
    return QtGui.QIcon(pmPlay())

def pmMeasure():
    return PM(132405,134028)

def Measure():
    return QtGui.QIcon(pmMeasure())

def pmPlayGame():
    return PM(895049,899407)

def PlayGame():
    return QtGui.QIcon(pmPlayGame())

def pmScanner():
    return PM(899407,899748)

def Scanner():
    return QtGui.QIcon(pmScanner())

def pmMenos():
    return PM(899748,900273)

def Menos():
    return QtGui.QIcon(pmMenos())

def pmSchool():
    return PM(900273,900814)

def School():
    return QtGui.QIcon(pmSchool())

def pmLaw():
    return PM(900814,901430)

def Law():
    return QtGui.QIcon(pmLaw())

def pmLearnGame():
    return PM(901430,901863)

def LearnGame():
    return QtGui.QIcon(pmLearnGame())

def pmUniversity():
    return PM(901863,902283)

def University():
    return QtGui.QIcon(pmUniversity())

def pmLonghaul():
    return PM(902283,903209)

def Longhaul():
    return QtGui.QIcon(pmLonghaul())

def pmTrekking():
    return PM(903209,903903)

def Trekking():
    return QtGui.QIcon(pmTrekking())

def pmPassword():
    return PM(903903,904356)

def Password():
    return QtGui.QIcon(pmPassword())

def pmSQL_RAW():
    return PM(895049,899407)

def SQL_RAW():
    return QtGui.QIcon(pmSQL_RAW())

def pmSun():
    return PM(339132,340010)

def Sun():
    return QtGui.QIcon(pmSun())

def pmLight():
    return PM(343010,344066)

def Light():
    return QtGui.QIcon(pmLight())

def pmLight32():
    return PM(904356,906056)

def Light32():
    return QtGui.QIcon(pmLight32())

def pmTOL():
    return PM(906056,906765)

def TOL():
    return QtGui.QIcon(pmTOL())

def pmUned():
    return PM(901863,902283)

def Uned():
    return QtGui.QIcon(pmUned())

def pmUwe():
    return PM(906765,907734)

def Uwe():
    return QtGui.QIcon(pmUwe())

def pmThinking():
    return PM(907734,908106)

def Thinking():
    return QtGui.QIcon(pmThinking())

def pmWashingMachine():
    return PM(908106,908769)

def WashingMachine():
    return QtGui.QIcon(pmWashingMachine())

def pmTerminal():
    return PM(908769,912313)

def Terminal():
    return QtGui.QIcon(pmTerminal())

def pmManualSave():
    return PM(912313,912896)

def ManualSave():
    return QtGui.QIcon(pmManualSave())


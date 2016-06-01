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
    return PM(16111,23157)

def MensEspera():
    return QtGui.QIcon(pmMensEspera())

def pmUtilidades():
    return PM(23157,29586)

def Utilidades():
    return QtGui.QIcon(pmUtilidades())

def pmTerminar():
    return PM(29586,31336)

def Terminar():
    return QtGui.QIcon(pmTerminar())

def pmNuevaPartida():
    return PM(31336,33084)

def NuevaPartida():
    return QtGui.QIcon(pmNuevaPartida())

def pmOpciones():
    return PM(33084,34812)

def Opciones():
    return QtGui.QIcon(pmOpciones())

def pmEntrenamiento():
    return PM(7481,9510)

def Entrenamiento():
    return QtGui.QIcon(pmEntrenamiento())

def pmAplazar():
    return PM(34812,37879)

def Aplazar():
    return QtGui.QIcon(pmAplazar())

def pmCapturas():
    return PM(37879,39920)

def Capturas():
    return QtGui.QIcon(pmCapturas())

def pmReiniciar():
    return PM(39920,42214)

def Reiniciar():
    return QtGui.QIcon(pmReiniciar())

def pmMotores():
    return PM(42214,48113)

def Motores():
    return QtGui.QIcon(pmMotores())

def pmImportarGM():
    return PM(48113,50713)

def ImportarGM():
    return QtGui.QIcon(pmImportarGM())

def pmAbandonar():
    return PM(50713,54713)

def Abandonar():
    return QtGui.QIcon(pmAbandonar())

def pmEmpezar():
    return PM(54713,56749)

def Empezar():
    return QtGui.QIcon(pmEmpezar())

def pmOtros():
    return PM(56749,61219)

def Otros():
    return QtGui.QIcon(pmOtros())

def pmAnalizar():
    return PM(61219,62756)

def Analizar():
    return QtGui.QIcon(pmAnalizar())

def pmMainMenu():
    return PM(62756,67066)

def MainMenu():
    return QtGui.QIcon(pmMainMenu())

def pmFinPartida():
    return PM(67066,70014)

def FinPartida():
    return QtGui.QIcon(pmFinPartida())

def pmGrabar():
    return PM(70014,71477)

def Grabar():
    return QtGui.QIcon(pmGrabar())

def pmGrabarComo():
    return PM(71477,73529)

def GrabarComo():
    return QtGui.QIcon(pmGrabarComo())

def pmRecuperar():
    return PM(73529,76287)

def Recuperar():
    return QtGui.QIcon(pmRecuperar())

def pmInformacion():
    return PM(76287,78246)

def Informacion():
    return QtGui.QIcon(pmInformacion())

def pmNuevo():
    return PM(78246,79000)

def Nuevo():
    return QtGui.QIcon(pmNuevo())

def pmCopiar():
    return PM(79000,80181)

def Copiar():
    return QtGui.QIcon(pmCopiar())

def pmModificar():
    return PM(80181,84578)

def Modificar():
    return QtGui.QIcon(pmModificar())

def pmBorrar():
    return PM(84578,89569)

def Borrar():
    return QtGui.QIcon(pmBorrar())

def pmMarcar():
    return PM(89569,94498)

def Marcar():
    return QtGui.QIcon(pmMarcar())

def pmPegar():
    return PM(94498,96809)

def Pegar():
    return QtGui.QIcon(pmPegar())

def pmFichero():
    return PM(96809,101494)

def Fichero():
    return QtGui.QIcon(pmFichero())

def pmNuestroFichero():
    return PM(101494,104541)

def NuestroFichero():
    return QtGui.QIcon(pmNuestroFichero())

def pmFicheroRepite():
    return PM(104541,106037)

def FicheroRepite():
    return QtGui.QIcon(pmFicheroRepite())

def pmInformacionPGN():
    return PM(106037,107055)

def InformacionPGN():
    return QtGui.QIcon(pmInformacionPGN())

def pmVer():
    return PM(107055,108509)

def Ver():
    return QtGui.QIcon(pmVer())

def pmInicio():
    return PM(108509,110523)

def Inicio():
    return QtGui.QIcon(pmInicio())

def pmFinal():
    return PM(110523,112517)

def Final():
    return QtGui.QIcon(pmFinal())

def pmFiltrar():
    return PM(112517,119007)

def Filtrar():
    return QtGui.QIcon(pmFiltrar())

def pmArriba():
    return PM(119007,121160)

def Arriba():
    return QtGui.QIcon(pmArriba())

def pmAbajo():
    return PM(121160,123268)

def Abajo():
    return QtGui.QIcon(pmAbajo())

def pmEstadisticas():
    return PM(123268,125407)

def Estadisticas():
    return QtGui.QIcon(pmEstadisticas())

def pmRendirse():
    return PM(125407,129253)

def Rendirse():
    return QtGui.QIcon(pmRendirse())

def pmCheck():
    return PM(129253,132477)

def Check():
    return QtGui.QIcon(pmCheck())

def pmTablas():
    return PM(132477,134100)

def Tablas():
    return QtGui.QIcon(pmTablas())

def pmAtras():
    return PM(134100,135619)

def Atras():
    return QtGui.QIcon(pmAtras())

def pmBuscar():
    return PM(135619,137604)

def Buscar():
    return QtGui.QIcon(pmBuscar())

def pmLibros():
    return PM(137604,139732)

def Libros():
    return QtGui.QIcon(pmLibros())

def pmAceptar():
    return PM(139732,143079)

def Aceptar():
    return QtGui.QIcon(pmAceptar())

def pmCancelar():
    return PM(143079,145062)

def Cancelar():
    return QtGui.QIcon(pmCancelar())

def pmDefecto():
    return PM(145062,148381)

def Defecto():
    return QtGui.QIcon(pmDefecto())

def pmGenerar():
    return PM(148381,151597)

def Generar():
    return QtGui.QIcon(pmGenerar())

def pmInsertar():
    return PM(151597,153993)

def Insertar():
    return QtGui.QIcon(pmInsertar())

def pmJugar():
    return PM(153993,156202)

def Jugar():
    return QtGui.QIcon(pmJugar())

def pmConfigurar():
    return PM(156202,159286)

def Configurar():
    return QtGui.QIcon(pmConfigurar())

def pmS_Aceptar():
    return PM(139732,143079)

def S_Aceptar():
    return QtGui.QIcon(pmS_Aceptar())

def pmS_Cancelar():
    return PM(143079,145062)

def S_Cancelar():
    return QtGui.QIcon(pmS_Cancelar())

def pmS_Microfono():
    return PM(159286,164727)

def S_Microfono():
    return QtGui.QIcon(pmS_Microfono())

def pmS_LeerWav():
    return PM(48113,50713)

def S_LeerWav():
    return QtGui.QIcon(pmS_LeerWav())

def pmS_Play():
    return PM(164727,170065)

def S_Play():
    return QtGui.QIcon(pmS_Play())

def pmS_StopPlay():
    return PM(170065,170675)

def S_StopPlay():
    return QtGui.QIcon(pmS_StopPlay())

def pmS_StopMicrofono():
    return PM(170065,170675)

def S_StopMicrofono():
    return QtGui.QIcon(pmS_StopMicrofono())

def pmS_Record():
    return PM(170675,173908)

def S_Record():
    return QtGui.QIcon(pmS_Record())

def pmS_Limpiar():
    return PM(84578,89569)

def S_Limpiar():
    return QtGui.QIcon(pmS_Limpiar())

def pmHistorial():
    return PM(173908,175171)

def Historial():
    return QtGui.QIcon(pmHistorial())

def pmPegar16():
    return PM(175171,176165)

def Pegar16():
    return QtGui.QIcon(pmPegar16())

def pmRivalesMP():
    return PM(176165,177291)

def RivalesMP():
    return QtGui.QIcon(pmRivalesMP())

def pmCamara():
    return PM(177291,178813)

def Camara():
    return QtGui.QIcon(pmCamara())

def pmUsuarios():
    return PM(178813,180053)

def Usuarios():
    return QtGui.QIcon(pmUsuarios())

def pmResistencia():
    return PM(180053,183115)

def Resistencia():
    return QtGui.QIcon(pmResistencia())

def pmRemoto():
    return PM(183115,187237)

def Remoto():
    return QtGui.QIcon(pmRemoto())

def pmRemotoServidor():
    return PM(187237,188261)

def RemotoServidor():
    return QtGui.QIcon(pmRemotoServidor())

def pmRemotoCliente():
    return PM(188261,189505)

def RemotoCliente():
    return QtGui.QIcon(pmRemotoCliente())

def pmCebra():
    return PM(189505,191958)

def Cebra():
    return QtGui.QIcon(pmCebra())

def pmGafas():
    return PM(191958,192942)

def Gafas():
    return QtGui.QIcon(pmGafas())

def pmPuente():
    return PM(192942,193578)

def Puente():
    return QtGui.QIcon(pmPuente())

def pmWeb():
    return PM(193578,194760)

def Web():
    return QtGui.QIcon(pmWeb())

def pmMail():
    return PM(194760,195720)

def Mail():
    return QtGui.QIcon(pmMail())

def pmAyuda():
    return PM(195720,196901)

def Ayuda():
    return QtGui.QIcon(pmAyuda())

def pmFAQ():
    return PM(196901,198222)

def FAQ():
    return QtGui.QIcon(pmFAQ())

def pmPuntuacion():
    return PM(198222,199150)

def Puntuacion():
    return QtGui.QIcon(pmPuntuacion())

def pmActualiza():
    return PM(199150,200016)

def Actualiza():
    return QtGui.QIcon(pmActualiza())

def pmRefresh():
    return PM(200016,202408)

def Refresh():
    return QtGui.QIcon(pmRefresh())

def pmJuegaSolo():
    return PM(202408,203590)

def JuegaSolo():
    return QtGui.QIcon(pmJuegaSolo())

def pmPlayer():
    return PM(202408,203590)

def Player():
    return QtGui.QIcon(pmPlayer())

def pmJS_Rotacion():
    return PM(203590,205500)

def JS_Rotacion():
    return QtGui.QIcon(pmJS_Rotacion())

def pmCoordina():
    return PM(203590,205500)

def Coordina():
    return QtGui.QIcon(pmCoordina())

def pmEstrellaAzul():
    return PM(205500,207006)

def EstrellaAzul():
    return QtGui.QIcon(pmEstrellaAzul())

def pmElo():
    return PM(205500,207006)

def Elo():
    return QtGui.QIcon(pmElo())

def pmMate():
    return PM(207006,207567)

def Mate():
    return QtGui.QIcon(pmMate())

def pmEloTimed():
    return PM(207567,209051)

def EloTimed():
    return QtGui.QIcon(pmEloTimed())

def pmPGN():
    return PM(209051,211049)

def PGN():
    return QtGui.QIcon(pmPGN())

def pmPGN_Importar():
    return PM(211049,211753)

def PGN_Importar():
    return QtGui.QIcon(pmPGN_Importar())

def pmAyudaGR():
    return PM(211753,217631)

def AyudaGR():
    return QtGui.QIcon(pmAyudaGR())

def pmBotonAyuda():
    return PM(217631,220091)

def BotonAyuda():
    return QtGui.QIcon(pmBotonAyuda())

def pmColores():
    return PM(220091,221322)

def Colores():
    return QtGui.QIcon(pmColores())

def pmEditarColores():
    return PM(221322,223625)

def EditarColores():
    return QtGui.QIcon(pmEditarColores())

def pmGranMaestro():
    return PM(223625,224481)

def GranMaestro():
    return QtGui.QIcon(pmGranMaestro())

def pmFavoritos():
    return PM(224481,226247)

def Favoritos():
    return QtGui.QIcon(pmFavoritos())

def pmCarpeta():
    return PM(211049,211753)

def Carpeta():
    return QtGui.QIcon(pmCarpeta())

def pmDivision():
    return PM(226247,226912)

def Division():
    return QtGui.QIcon(pmDivision())

def pmDivisionF():
    return PM(226912,228026)

def DivisionF():
    return QtGui.QIcon(pmDivisionF())

def pmKibitzer():
    return PM(228026,228474)

def Kibitzer():
    return QtGui.QIcon(pmKibitzer())

def pmKibitzer_Pausa():
    return PM(228474,229338)

def Kibitzer_Pausa():
    return QtGui.QIcon(pmKibitzer_Pausa())

def pmKibitzer_Continuar():
    return PM(229338,230169)

def Kibitzer_Continuar():
    return QtGui.QIcon(pmKibitzer_Continuar())

def pmKibitzer_Terminar():
    return PM(230169,231093)

def Kibitzer_Terminar():
    return QtGui.QIcon(pmKibitzer_Terminar())

def pmDelete():
    return PM(230169,231093)

def Delete():
    return QtGui.QIcon(pmDelete())

def pmModificarP():
    return PM(231093,232159)

def ModificarP():
    return QtGui.QIcon(pmModificarP())

def pmGrupo_Si():
    return PM(232159,232621)

def Grupo_Si():
    return QtGui.QIcon(pmGrupo_Si())

def pmGrupo_No():
    return PM(232621,232944)

def Grupo_No():
    return QtGui.QIcon(pmGrupo_No())

def pmMotor_Si():
    return PM(232944,233406)

def Motor_Si():
    return QtGui.QIcon(pmMotor_Si())

def pmMotor_No():
    return PM(230169,231093)

def Motor_No():
    return QtGui.QIcon(pmMotor_No())

def pmMotor_Actual():
    return PM(233406,234423)

def Motor_Actual():
    return QtGui.QIcon(pmMotor_Actual())

def pmMotor():
    return PM(234423,235050)

def Motor():
    return QtGui.QIcon(pmMotor())

def pmMoverInicio():
    return PM(235050,235903)

def MoverInicio():
    return QtGui.QIcon(pmMoverInicio())

def pmMoverFinal():
    return PM(235903,236779)

def MoverFinal():
    return QtGui.QIcon(pmMoverFinal())

def pmMoverAdelante():
    return PM(236779,237640)

def MoverAdelante():
    return QtGui.QIcon(pmMoverAdelante())

def pmMoverAtras():
    return PM(237640,238508)

def MoverAtras():
    return QtGui.QIcon(pmMoverAtras())

def pmMoverLibre():
    return PM(238508,239328)

def MoverLibre():
    return QtGui.QIcon(pmMoverLibre())

def pmMoverTiempo():
    return PM(239328,240521)

def MoverTiempo():
    return QtGui.QIcon(pmMoverTiempo())

def pmMoverMas():
    return PM(240521,241560)

def MoverMas():
    return QtGui.QIcon(pmMoverMas())

def pmMoverGrabar():
    return PM(241560,242416)

def MoverGrabar():
    return QtGui.QIcon(pmMoverGrabar())

def pmMoverGrabarTodos():
    return PM(242416,243460)

def MoverGrabarTodos():
    return QtGui.QIcon(pmMoverGrabarTodos())

def pmMoverJugar():
    return PM(229338,230169)

def MoverJugar():
    return QtGui.QIcon(pmMoverJugar())

def pmPelicula():
    return PM(243460,245594)

def Pelicula():
    return QtGui.QIcon(pmPelicula())

def pmPelicula_Pausa():
    return PM(245594,247353)

def Pelicula_Pausa():
    return QtGui.QIcon(pmPelicula_Pausa())

def pmPelicula_Seguir():
    return PM(247353,249442)

def Pelicula_Seguir():
    return QtGui.QIcon(pmPelicula_Seguir())

def pmPelicula_Rapido():
    return PM(249442,251501)

def Pelicula_Rapido():
    return QtGui.QIcon(pmPelicula_Rapido())

def pmPelicula_Lento():
    return PM(251501,253376)

def Pelicula_Lento():
    return QtGui.QIcon(pmPelicula_Lento())

def pmPelicula_Repetir():
    return PM(39920,42214)

def Pelicula_Repetir():
    return QtGui.QIcon(pmPelicula_Repetir())

def pmMemoria():
    return PM(253376,255317)

def Memoria():
    return QtGui.QIcon(pmMemoria())

def pmEntrenar():
    return PM(255317,256856)

def Entrenar():
    return QtGui.QIcon(pmEntrenar())

def pmEnviar():
    return PM(255317,256856)

def Enviar():
    return QtGui.QIcon(pmEnviar())

def pmTrasteros():
    return PM(256856,261659)

def Trasteros():
    return QtGui.QIcon(pmTrasteros())

def pmTrastero():
    return PM(261659,262121)

def Trastero():
    return QtGui.QIcon(pmTrastero())

def pmTrastero_Quitar():
    return PM(230169,231093)

def Trastero_Quitar():
    return QtGui.QIcon(pmTrastero_Quitar())

def pmTrastero_Nuevo():
    return PM(262121,263629)

def Trastero_Nuevo():
    return QtGui.QIcon(pmTrastero_Nuevo())

def pmNuevoMas():
    return PM(262121,263629)

def NuevoMas():
    return QtGui.QIcon(pmNuevoMas())

def pmTemas():
    return PM(263629,265852)

def Temas():
    return QtGui.QIcon(pmTemas())

def pmTutorialesCrear():
    return PM(265852,272121)

def TutorialesCrear():
    return QtGui.QIcon(pmTutorialesCrear())

def pmMover():
    return PM(272121,272703)

def Mover():
    return QtGui.QIcon(pmMover())

def pmSeleccionado():
    return PM(272121,272703)

def Seleccionado():
    return QtGui.QIcon(pmSeleccionado())

def pmSeleccionar():
    return PM(272703,278407)

def Seleccionar():
    return QtGui.QIcon(pmSeleccionar())

def pmVista():
    return PM(278407,280331)

def Vista():
    return QtGui.QIcon(pmVista())

def pmInformacionPGNUno():
    return PM(280331,281709)

def InformacionPGNUno():
    return QtGui.QIcon(pmInformacionPGNUno())

def pmDailyTest():
    return PM(281709,284049)

def DailyTest():
    return QtGui.QIcon(pmDailyTest())

def pmJuegaPorMi():
    return PM(284049,285769)

def JuegaPorMi():
    return QtGui.QIcon(pmJuegaPorMi())

def pmArbol():
    return PM(285769,287454)

def Arbol():
    return QtGui.QIcon(pmArbol())

def pmGrabarFichero():
    return PM(70014,71477)

def GrabarFichero():
    return QtGui.QIcon(pmGrabarFichero())

def pmClip():
    return PM(287454,289600)

def Clip():
    return QtGui.QIcon(pmClip())

def pmFics():
    return PM(289600,290017)

def Fics():
    return QtGui.QIcon(pmFics())

def pmFide():
    return PM(9510,11487)

def Fide():
    return QtGui.QIcon(pmFide())

def pmFlechas():
    return PM(290017,293369)

def Flechas():
    return QtGui.QIcon(pmFlechas())

def pmMarcos():
    return PM(293369,294816)

def Marcos():
    return QtGui.QIcon(pmMarcos())

def pmSVGs():
    return PM(294816,298385)

def SVGs():
    return QtGui.QIcon(pmSVGs())

def pmAmarillo():
    return PM(298385,299637)

def Amarillo():
    return QtGui.QIcon(pmAmarillo())

def pmNaranja():
    return PM(299637,300869)

def Naranja():
    return QtGui.QIcon(pmNaranja())

def pmVerde():
    return PM(300869,302145)

def Verde():
    return QtGui.QIcon(pmVerde())

def pmAzul():
    return PM(302145,303233)

def Azul():
    return QtGui.QIcon(pmAzul())

def pmMagenta():
    return PM(303233,304521)

def Magenta():
    return QtGui.QIcon(pmMagenta())

def pmRojo():
    return PM(304521,305740)

def Rojo():
    return QtGui.QIcon(pmRojo())

def pmGris():
    return PM(305740,306698)

def Gris():
    return QtGui.QIcon(pmGris())

def pmEstrella():
    return PM(198222,199150)

def Estrella():
    return QtGui.QIcon(pmEstrella())

def pmVerde32():
    return PM(306698,308819)

def Verde32():
    return QtGui.QIcon(pmVerde32())

def pmRojo32():
    return PM(308819,310634)

def Rojo32():
    return QtGui.QIcon(pmRojo32())

def pmPuntoBlanco():
    return PM(310634,310983)

def PuntoBlanco():
    return QtGui.QIcon(pmPuntoBlanco())

def pmPuntoAmarillo():
    return PM(232159,232621)

def PuntoAmarillo():
    return QtGui.QIcon(pmPuntoAmarillo())

def pmPuntoNaranja():
    return PM(310983,311445)

def PuntoNaranja():
    return QtGui.QIcon(pmPuntoNaranja())

def pmPuntoVerde():
    return PM(232944,233406)

def PuntoVerde():
    return QtGui.QIcon(pmPuntoVerde())

def pmPuntoAzul():
    return PM(261659,262121)

def PuntoAzul():
    return QtGui.QIcon(pmPuntoAzul())

def pmPuntoMagenta():
    return PM(311445,311944)

def PuntoMagenta():
    return QtGui.QIcon(pmPuntoMagenta())

def pmPuntoRojo():
    return PM(311944,312443)

def PuntoRojo():
    return QtGui.QIcon(pmPuntoRojo())

def pmPuntoNegro():
    return PM(232621,232944)

def PuntoNegro():
    return QtGui.QIcon(pmPuntoNegro())

def pmPuntoEstrella():
    return PM(312443,312870)

def PuntoEstrella():
    return QtGui.QIcon(pmPuntoEstrella())

def pmComentario():
    return PM(312870,313507)

def Comentario():
    return QtGui.QIcon(pmComentario())

def pmComentarioMas():
    return PM(313507,314446)

def ComentarioMas():
    return QtGui.QIcon(pmComentarioMas())

def pmComentarioEditar():
    return PM(241560,242416)

def ComentarioEditar():
    return QtGui.QIcon(pmComentarioEditar())

def pmApertura():
    return PM(314446,315412)

def Apertura():
    return QtGui.QIcon(pmApertura())

def pmAperturaComentario():
    return PM(315412,316408)

def AperturaComentario():
    return QtGui.QIcon(pmAperturaComentario())

def pmBookGuide():
    return PM(316408,317285)

def BookGuide():
    return QtGui.QIcon(pmBookGuide())

def pmMas():
    return PM(317285,317794)

def Mas():
    return QtGui.QIcon(pmMas())

def pmMasR():
    return PM(317794,318282)

def MasR():
    return QtGui.QIcon(pmMasR())

def pmMasDoc():
    return PM(318282,319083)

def MasDoc():
    return QtGui.QIcon(pmMasDoc())

def pmPotencia():
    return PM(199150,200016)

def Potencia():
    return QtGui.QIcon(pmPotencia())

def pmSorpresa():
    return PM(319083,320142)

def Sorpresa():
    return QtGui.QIcon(pmSorpresa())

def pmSonrisa():
    return PM(320142,321224)

def Sonrisa():
    return QtGui.QIcon(pmSonrisa())

def pmBMT():
    return PM(321224,322102)

def BMT():
    return QtGui.QIcon(pmBMT())

def pmCorazon():
    return PM(322102,323980)

def Corazon():
    return QtGui.QIcon(pmCorazon())

def pmOjo():
    return PM(323980,325102)

def Ojo():
    return QtGui.QIcon(pmOjo())

def pmOcultar():
    return PM(323980,325102)

def Ocultar():
    return QtGui.QIcon(pmOcultar())

def pmMostrar():
    return PM(325102,326158)

def Mostrar():
    return QtGui.QIcon(pmMostrar())

def pmBlog():
    return PM(326158,326680)

def Blog():
    return QtGui.QIcon(pmBlog())

def pmVariantes():
    return PM(326680,327587)

def Variantes():
    return QtGui.QIcon(pmVariantes())

def pmVariantesG():
    return PM(327587,330014)

def VariantesG():
    return QtGui.QIcon(pmVariantesG())

def pmCambiar():
    return PM(330014,331728)

def Cambiar():
    return QtGui.QIcon(pmCambiar())

def pmAnterior():
    return PM(331728,333782)

def Anterior():
    return QtGui.QIcon(pmAnterior())

def pmSiguiente():
    return PM(333782,335852)

def Siguiente():
    return QtGui.QIcon(pmSiguiente())

def pmSiguienteF():
    return PM(335852,338027)

def SiguienteF():
    return QtGui.QIcon(pmSiguienteF())

def pmAnteriorF():
    return PM(338027,340221)

def AnteriorF():
    return QtGui.QIcon(pmAnteriorF())

def pmX():
    return PM(340221,341503)

def X():
    return QtGui.QIcon(pmX())

def pmTools():
    return PM(341503,344104)

def Tools():
    return QtGui.QIcon(pmTools())

def pmTacticas():
    return PM(344104,346677)

def Tacticas():
    return QtGui.QIcon(pmTacticas())

def pmCancelarPeque():
    return PM(346677,347538)

def CancelarPeque():
    return QtGui.QIcon(pmCancelarPeque())

def pmAceptarPeque():
    return PM(233406,234423)

def AceptarPeque():
    return QtGui.QIcon(pmAceptarPeque())

def pmP_16c():
    return PM(347538,348062)

def P_16c():
    return QtGui.QIcon(pmP_16c())

def pmLibre():
    return PM(348062,350454)

def Libre():
    return QtGui.QIcon(pmLibre())

def pmEnBlanco():
    return PM(350454,351180)

def EnBlanco():
    return QtGui.QIcon(pmEnBlanco())

def pmDirector():
    return PM(351180,354154)

def Director():
    return QtGui.QIcon(pmDirector())

def pmTorneos():
    return PM(354154,355892)

def Torneos():
    return QtGui.QIcon(pmTorneos())

def pmAperturas():
    return PM(355892,356817)

def Aperturas():
    return QtGui.QIcon(pmAperturas())

def pmV_Blancas():
    return PM(356817,357097)

def V_Blancas():
    return QtGui.QIcon(pmV_Blancas())

def pmV_Blancas_Mas():
    return PM(357097,357377)

def V_Blancas_Mas():
    return QtGui.QIcon(pmV_Blancas_Mas())

def pmV_Blancas_Mas_Mas():
    return PM(357377,357649)

def V_Blancas_Mas_Mas():
    return QtGui.QIcon(pmV_Blancas_Mas_Mas())

def pmV_Negras():
    return PM(357649,357924)

def V_Negras():
    return QtGui.QIcon(pmV_Negras())

def pmV_Negras_Mas():
    return PM(357924,358199)

def V_Negras_Mas():
    return QtGui.QIcon(pmV_Negras_Mas())

def pmV_Negras_Mas_Mas():
    return PM(358199,358468)

def V_Negras_Mas_Mas():
    return QtGui.QIcon(pmV_Negras_Mas_Mas())

def pmV_Blancas_Igual_Negras():
    return PM(358468,358770)

def V_Blancas_Igual_Negras():
    return QtGui.QIcon(pmV_Blancas_Igual_Negras())

def pmMezclar():
    return PM(151597,153993)

def Mezclar():
    return QtGui.QIcon(pmMezclar())

def pmVoyager():
    return PM(358770,360732)

def Voyager():
    return QtGui.QIcon(pmVoyager())

def pmReindexar():
    return PM(360732,362549)

def Reindexar():
    return QtGui.QIcon(pmReindexar())

def pmRename():
    return PM(362549,363533)

def Rename():
    return QtGui.QIcon(pmRename())

def pmAdd():
    return PM(363533,364486)

def Add():
    return QtGui.QIcon(pmAdd())

def pmMas22():
    return PM(364486,365150)

def Mas22():
    return QtGui.QIcon(pmMas22())

def pmMenos22():
    return PM(365150,365594)

def Menos22():
    return QtGui.QIcon(pmMenos22())

def pmTransposition():
    return PM(365594,366444)

def Transposition():
    return QtGui.QIcon(pmTransposition())

def pmRat():
    return PM(366444,372148)

def Rat():
    return QtGui.QIcon(pmRat())

def pmAlligator():
    return PM(372148,377140)

def Alligator():
    return QtGui.QIcon(pmAlligator())

def pmAnt():
    return PM(377140,383838)

def Ant():
    return QtGui.QIcon(pmAnt())

def pmBat():
    return PM(383838,386792)

def Bat():
    return QtGui.QIcon(pmBat())

def pmBear():
    return PM(386792,394071)

def Bear():
    return QtGui.QIcon(pmBear())

def pmBee():
    return PM(394071,399073)

def Bee():
    return QtGui.QIcon(pmBee())

def pmBird():
    return PM(399073,405132)

def Bird():
    return QtGui.QIcon(pmBird())

def pmBull():
    return PM(405132,412101)

def Bull():
    return QtGui.QIcon(pmBull())

def pmBulldog():
    return PM(412101,418992)

def Bulldog():
    return QtGui.QIcon(pmBulldog())

def pmButterfly():
    return PM(418992,426366)

def Butterfly():
    return QtGui.QIcon(pmButterfly())

def pmCat():
    return PM(426366,432638)

def Cat():
    return QtGui.QIcon(pmCat())

def pmChicken():
    return PM(432638,438449)

def Chicken():
    return QtGui.QIcon(pmChicken())

def pmCow():
    return PM(438449,445192)

def Cow():
    return QtGui.QIcon(pmCow())

def pmCrab():
    return PM(445192,450781)

def Crab():
    return QtGui.QIcon(pmCrab())

def pmCrocodile():
    return PM(450781,456922)

def Crocodile():
    return QtGui.QIcon(pmCrocodile())

def pmDeer():
    return PM(456922,463229)

def Deer():
    return QtGui.QIcon(pmDeer())

def pmDog():
    return PM(463229,469832)

def Dog():
    return QtGui.QIcon(pmDog())

def pmDonkey():
    return PM(469832,475479)

def Donkey():
    return QtGui.QIcon(pmDonkey())

def pmDuck():
    return PM(475479,482022)

def Duck():
    return QtGui.QIcon(pmDuck())

def pmEagle():
    return PM(482022,486840)

def Eagle():
    return QtGui.QIcon(pmEagle())

def pmElephant():
    return PM(486840,493321)

def Elephant():
    return QtGui.QIcon(pmElephant())

def pmFish():
    return PM(493321,500162)

def Fish():
    return QtGui.QIcon(pmFish())

def pmFox():
    return PM(500162,506945)

def Fox():
    return QtGui.QIcon(pmFox())

def pmFrog():
    return PM(506945,513361)

def Frog():
    return QtGui.QIcon(pmFrog())

def pmGiraffe():
    return PM(513361,520539)

def Giraffe():
    return QtGui.QIcon(pmGiraffe())

def pmGorilla():
    return PM(520539,527078)

def Gorilla():
    return QtGui.QIcon(pmGorilla())

def pmHippo():
    return PM(527078,534199)

def Hippo():
    return QtGui.QIcon(pmHippo())

def pmHorse():
    return PM(534199,540746)

def Horse():
    return QtGui.QIcon(pmHorse())

def pmInsect():
    return PM(540746,546681)

def Insect():
    return QtGui.QIcon(pmInsect())

def pmLion():
    return PM(546681,555591)

def Lion():
    return QtGui.QIcon(pmLion())

def pmMonkey():
    return PM(555591,563270)

def Monkey():
    return QtGui.QIcon(pmMonkey())

def pmMoose():
    return PM(563270,569894)

def Moose():
    return QtGui.QIcon(pmMoose())

def pmMouse():
    return PM(366444,372148)

def Mouse():
    return QtGui.QIcon(pmMouse())

def pmOwl():
    return PM(569894,576600)

def Owl():
    return QtGui.QIcon(pmOwl())

def pmPanda():
    return PM(576600,580634)

def Panda():
    return QtGui.QIcon(pmPanda())

def pmPenguin():
    return PM(580634,586183)

def Penguin():
    return QtGui.QIcon(pmPenguin())

def pmPig():
    return PM(586183,594223)

def Pig():
    return QtGui.QIcon(pmPig())

def pmRabbit():
    return PM(594223,601524)

def Rabbit():
    return QtGui.QIcon(pmRabbit())

def pmRhino():
    return PM(601524,607911)

def Rhino():
    return QtGui.QIcon(pmRhino())

def pmRooster():
    return PM(607911,613174)

def Rooster():
    return QtGui.QIcon(pmRooster())

def pmShark():
    return PM(613174,618944)

def Shark():
    return QtGui.QIcon(pmShark())

def pmSheep():
    return PM(618944,622775)

def Sheep():
    return QtGui.QIcon(pmSheep())

def pmSnake():
    return PM(622775,628800)

def Snake():
    return QtGui.QIcon(pmSnake())

def pmTiger():
    return PM(628800,636837)

def Tiger():
    return QtGui.QIcon(pmTiger())

def pmTurkey():
    return PM(636837,644251)

def Turkey():
    return QtGui.QIcon(pmTurkey())

def pmTurtle():
    return PM(644251,650972)

def Turtle():
    return QtGui.QIcon(pmTurtle())

def pmWolf():
    return PM(650972,654067)

def Wolf():
    return QtGui.QIcon(pmWolf())

def pmWheel():
    return PM(654067,662132)

def Wheel():
    return QtGui.QIcon(pmWheel())

def pmWheelchair():
    return PM(662132,670936)

def Wheelchair():
    return QtGui.QIcon(pmWheelchair())

def pmTouringMotorcycle():
    return PM(670936,677248)

def TouringMotorcycle():
    return QtGui.QIcon(pmTouringMotorcycle())

def pmContainer():
    return PM(677248,682583)

def Container():
    return QtGui.QIcon(pmContainer())

def pmBoatEquipment():
    return PM(682583,688106)

def BoatEquipment():
    return QtGui.QIcon(pmBoatEquipment())

def pmCar():
    return PM(688106,692752)

def Car():
    return QtGui.QIcon(pmCar())

def pmLorry():
    return PM(692752,698788)

def Lorry():
    return QtGui.QIcon(pmLorry())

def pmCarTrailer():
    return PM(698788,702885)

def CarTrailer():
    return QtGui.QIcon(pmCarTrailer())

def pmTowTruck():
    return PM(702885,707643)

def TowTruck():
    return QtGui.QIcon(pmTowTruck())

def pmQuadBike():
    return PM(707643,713612)

def QuadBike():
    return QtGui.QIcon(pmQuadBike())

def pmRecoveryTruck():
    return PM(713612,718609)

def RecoveryTruck():
    return QtGui.QIcon(pmRecoveryTruck())

def pmContainerLoader():
    return PM(718609,723751)

def ContainerLoader():
    return QtGui.QIcon(pmContainerLoader())

def pmPoliceCar():
    return PM(723751,728583)

def PoliceCar():
    return QtGui.QIcon(pmPoliceCar())

def pmExecutiveCar():
    return PM(728583,733261)

def ExecutiveCar():
    return QtGui.QIcon(pmExecutiveCar())

def pmTruck():
    return PM(733261,738724)

def Truck():
    return QtGui.QIcon(pmTruck())

def pmExcavator():
    return PM(738724,743615)

def Excavator():
    return QtGui.QIcon(pmExcavator())

def pmCabriolet():
    return PM(743615,748453)

def Cabriolet():
    return QtGui.QIcon(pmCabriolet())

def pmMixerTruck():
    return PM(748453,754763)

def MixerTruck():
    return QtGui.QIcon(pmMixerTruck())

def pmForkliftTruckLoaded():
    return PM(754763,760911)

def ForkliftTruckLoaded():
    return QtGui.QIcon(pmForkliftTruckLoaded())

def pmAmbulance():
    return PM(760911,766961)

def Ambulance():
    return QtGui.QIcon(pmAmbulance())

def pmDieselLocomotiveBoxcar():
    return PM(766961,770967)

def DieselLocomotiveBoxcar():
    return QtGui.QIcon(pmDieselLocomotiveBoxcar())

def pmTractorUnit():
    return PM(770967,776434)

def TractorUnit():
    return QtGui.QIcon(pmTractorUnit())

def pmFireTruck():
    return PM(776434,782773)

def FireTruck():
    return QtGui.QIcon(pmFireTruck())

def pmCargoShip():
    return PM(782773,787114)

def CargoShip():
    return QtGui.QIcon(pmCargoShip())

def pmSubwayTrain():
    return PM(787114,792004)

def SubwayTrain():
    return QtGui.QIcon(pmSubwayTrain())

def pmTruckMountedCrane():
    return PM(792004,797745)

def TruckMountedCrane():
    return QtGui.QIcon(pmTruckMountedCrane())

def pmAirAmbulance():
    return PM(797745,802858)

def AirAmbulance():
    return QtGui.QIcon(pmAirAmbulance())

def pmAirplane():
    return PM(802858,807746)

def Airplane():
    return QtGui.QIcon(pmAirplane())

def pmCaracol():
    return PM(807746,809562)

def Caracol():
    return QtGui.QIcon(pmCaracol())

def pmDownloads():
    return PM(809562,811404)

def Downloads():
    return QtGui.QIcon(pmDownloads())

def pmXFCC():
    return PM(811404,812564)

def XFCC():
    return QtGui.QIcon(pmXFCC())

def pmLastXFCC():
    return PM(812564,813386)

def LastXFCC():
    return QtGui.QIcon(pmLastXFCC())

def pmUno():
    return PM(813386,815848)

def Uno():
    return QtGui.QIcon(pmUno())

def pmMotoresExternos():
    return PM(815848,817750)

def MotoresExternos():
    return QtGui.QIcon(pmMotoresExternos())

def pmDatabase():
    return PM(817750,818293)

def Database():
    return QtGui.QIcon(pmDatabase())

def pmDatabaseC():
    return PM(818293,818718)

def DatabaseC():
    return QtGui.QIcon(pmDatabaseC())

def pmDatabaseF():
    return PM(818718,819182)

def DatabaseF():
    return QtGui.QIcon(pmDatabaseF())

def pmAtacante():
    return PM(819182,819787)

def Atacante():
    return QtGui.QIcon(pmAtacante())

def pmAtacada():
    return PM(819787,820353)

def Atacada():
    return QtGui.QIcon(pmAtacada())

def pmGoToNext():
    return PM(820353,820765)

def GoToNext():
    return QtGui.QIcon(pmGoToNext())

def pmBlancas():
    return PM(820765,821116)

def Blancas():
    return QtGui.QIcon(pmBlancas())

def pmNegras():
    return PM(821116,821362)

def Negras():
    return QtGui.QIcon(pmNegras())

def pmFolderChange():
    return PM(73529,76287)

def FolderChange():
    return QtGui.QIcon(pmFolderChange())

def pmMarkers():
    return PM(821362,823057)

def Markers():
    return QtGui.QIcon(pmMarkers())

def pmTop():
    return PM(823057,823641)

def Top():
    return QtGui.QIcon(pmTop())

def pmBottom():
    return PM(823641,824230)

def Bottom():
    return QtGui.QIcon(pmBottom())

def pmSTS():
    return PM(824230,826421)

def STS():
    return QtGui.QIcon(pmSTS())

def pmRun():
    return PM(826421,828145)

def Run():
    return QtGui.QIcon(pmRun())

def pmWorldMap():
    return PM(828145,830886)

def WorldMap():
    return QtGui.QIcon(pmWorldMap())

def pmAfrica():
    return PM(830886,833372)

def Africa():
    return QtGui.QIcon(pmAfrica())

def pmMaps():
    return PM(833372,834316)

def Maps():
    return QtGui.QIcon(pmMaps())

def pmSol():
    return PM(834316,840668)

def Sol():
    return QtGui.QIcon(pmSol())

def pmSolNubes():
    return PM(840668,846525)

def SolNubes():
    return QtGui.QIcon(pmSolNubes())

def pmNubes():
    return PM(846525,849657)

def Nubes():
    return QtGui.QIcon(pmNubes())

def pmTormenta():
    return PM(849657,854308)

def Tormenta():
    return QtGui.QIcon(pmTormenta())

def pmWords():
    return PM(854308,858093)

def Words():
    return QtGui.QIcon(pmWords())

def pmAdaptVoice():
    return PM(351180,354154)

def AdaptVoice():
    return QtGui.QIcon(pmAdaptVoice())

def pmFixedElo():
    return PM(173908,175171)

def FixedElo():
    return QtGui.QIcon(pmFixedElo())

def pmX_Microfono():
    return PM(858093,860546)

def X_Microfono():
    return QtGui.QIcon(pmX_Microfono())

def pmSoundTool():
    return PM(860546,863005)

def SoundTool():
    return QtGui.QIcon(pmSoundTool())

def pmImportar():
    return PM(863005,865673)

def Importar():
    return QtGui.QIcon(pmImportar())

def pmVoyager1():
    return PM(865673,868123)

def Voyager1():
    return QtGui.QIcon(pmVoyager1())


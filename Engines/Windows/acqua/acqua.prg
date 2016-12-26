// Compilare con: hbmk2 -w3 -gtcgi acqua.prg hbct.hbc hbmemio.hbc



#require "hbmemio"
REQUEST HB_MEMIO

//-----------------------------------------------------
     #define VERSIONE   __DATE__
//    #define VERSIONE "20160618"
//-----------------------------------------------------
#define CRLF Chr( 13 ) + Chr( 10 )
#define LF Chr( 10 )
#define MACRO_VALORE_RE        10000
#define MACRO_VALORE_DONNA        40
#define MACRO_VALORE_TORRE        20
#define MACRO_VALORE_ALFIERE      10
#define MACRO_VALORE_CAVALLO       6
#define MACRO_VALORE_PEDONE        3
//-----------------------------------------------------
#define LUNGHEZZA_COMANDO 8192
//-----------------------------------------------------
#define BONUS_SCACCO_A_RE            2
#define BONUS_SCACCO_A_DONNA         1
#define BONUS_SCACCO_A_TORRE         0.4
#define BONUS_SCACCO_A_ALFIERE       0.3
#define BONUS_SCACCO_A_CAVALLO       0.2
#define BONUS_SCACCO_A_PEDONE        0.1
//-----------------------------------------------------


STATIC SCACCHIERA[ 8, 8 ]
STATIC SCACCHIERA2[ 8, 8 ]
STATIC acPOSIZIONI_SCACCHIERA, anPOSIZIONI_SCACCHIERA_QT
STATIC TRATTO := 0
STATIC lLOGFILE := .F.



// ---------------------------------------------------------------------------
FUNCTION Main()

   // ----------------------Dichiarazioni------------------
   LOCAL cStringa        // -------Input STDIN da UCI-------
   LOCAL aChiTocca := "bianco"    // -------"bianco" o "nero"------
   local nTempoDiPensamento
   LOCAL cMossa
   LOCAL acToken, acMoves
   LOCAL nNumeroMosse
   LOCAL k, xTemp
   LOCAL nTempoBianco, nTempoNero
   // --------------------------Settaggi--------------
   SET CENTURY ON
   SET DATE italian

   cls


   acPOSIZIONI_SCACCHIERA := {}
   anPOSIZIONI_SCACCHIERA_QT := {}

   reset_scacchiera()

   DO WHILE .T.

      // ---------------Legge da UCI e salva su Log---------------
      cStringa := Space( LUNGHEZZA_COMANDO )
      FRead( 0, @cStringa, LUNGHEZZA_COMANDO )
      cStringa := CharRem( Chr( 13 ) + Chr( 10 ), cStringa )  // Toglie CR e LF
      cStringa :=   AllTrim( cStringa )
      SalvaSuLog( DToC( Date() ) + Space( 1 ) + Time() + " GUI: >>> " + cStringa + CRLF )

      // -----------Se il messaggio dalla Gui e' "uci"--------------
      IF Left( cStringa, 3 ) == "uci" .AND. Len( cStringa ) == 3
         RispondiAllaGui( "id name Acqua (ver. " + VERSIONE + ")" )
         RispondiAllaGui( "id author Giovanni Di Maria" )
         RispondiAllaGui( "option name LogFile type check default false" )
         RispondiAllaGui( "uciok" )
      ENDIF

      // -----------Se il messaggio dalla Gui e' "ucinewgame"--------------
      IF Left( cStringa, 10 ) == "ucinewgame"
         reset_scacchiera()
      ENDIF

      // -----------Se il messaggio dalla Gui e' "isready"--------------
      IF Left( cStringa, 7 ) == "isready"
         RispondiAllaGui( "readyok" )
      ENDIF

      // -----------Se il messaggio dalla Gui e' SOLO "position startpos"--------------
      IF Left( cStringa, 17 ) == "position startpos" .AND. Len( cStringa ) == 17
         reset_scacchiera()
         aChiTocca = "bianco"
      ENDIF

      // -----------Se il messaggio dalla Gui e' "position startpos moves"--------------
      IF Left( cStringa, 23 ) == "position startpos moves"
         // ----Tokenizza le mosse mandate dalla GUI e le mette in array (le mosse iniziano dall'elemento 4)-----
         acToken := hb_ATokens( cStringa )
         acMoves := {}
         FOR k = 4 TO Len( acToken )
            AAdd( acMoves, acToken[ k ] )
         NEXT k
         nNumeroMosse := Len( acMoves )
         // -------Stabilisce di chi e' il TRATTO-----
         if( nNumeroMosse  ) % 2 == 1
            aChiTocca := "nero"
         ELSE
            aChiTocca := "bianco"
         ENDIF
         // ------Dispone i pezzi sulla scacchiera secondo l'elenco delle mosse dopo "position startpos moves"------
         reset_scacchiera()
         aggiorna_scacchiera( SCACCHIERA, acMoves )
      ENDIF


      // -----------Se il messaggio dalla Gui e' SOLO "position fen xxxxxxxxx" ma NON ci sono mosse seguenti--------------
      IF Left( cStringa, 12 ) == "position fen" .AND. At( "moves", cStringa ) == 0
         reset_scacchiera()
         aChiTocca := ScacchieraDaFen( cStringa )       // Aggiorna Scacchiera
         // -----Prende il numero di mossa a cui si e' arrivati-----
         TRATTO := Val( SubStr( cStringa, RAt( Space( 1 ), cStringa ) ) )

      ENDIF



      // -----------Se il messaggio dalla Gui e' "position fen" e ci sono MOSSE SEGUENTI: es: position fen 8/2k5/pp6/8/4k3/8/8/8 w - - 0 1 moves a6a7 e4d5 b6b7 d5c5 b7b8q--------------
      IF Left( cStringa, 12 ) == "position fen" .AND. At( "moves", cStringa ) > 0
         aChiTocca := ScacchieraDaFen( cStringa )       // Aggiorna Scacchiera
         acMoves := hb_ATokens( AllTrim( SubStr( cStringa, At( "moves", cStringa ) + 5 ) ) )    // Prende solo le mosse vere
         aggiorna_scacchiera( SCACCHIERA, acMoves )
         // ---------Dopo aver messo i pezzi sulla scacchiera dal FEN, determina a chi tocca dall'elenco mosse-------
         IF aChiTocca == "bianco" .AND. Len( acMoves ) % 2 == 1   // Se dal FEN toccava al Bianco e le mosse successive sono DISPARI
            aChiTocca := "nero"
         ELSEIF aChiTocca == "bianco" .AND. Len( acMoves ) % 2 == 0   // Se dal FEN toccava al Bianco e le mosse successive sono PARI
            aChiTocca := "bianco"
         ELSEIF aChiTocca == "nero" .AND. Len( acMoves ) % 2 == 1     // Se dal FEN toccava al Nero le mosse successive sono DISPARI
            aChiTocca := "bianco"
         ELSEIF aChiTocca == "nero" .AND. Len( acMoves ) % 2 == 0     // Se dal FEN toccava al Nero le mosse successive sono PARI
            aChiTocca := "nero"
         ENDIF
         // -----Prende il numero di mossa a cui si e' arrivati-----
         xTemp := Left( cStringa, At( "moves", cStringa ) -1 ) // Prende la stringa FEN senza le MOVES
         xTemp := AllTrim( xTemp )

         xTemp :=  SubStr( xTemp,      RAt( Space( 1 ), xTemp )  )
         TRATTO := Val( xTemp ) + Len( acMoves )

      ENDIF



      // -----------Se il messaggio dalla Gui e' "go..."--------------
      IF Left( cStringa, 2 ) == "go" .OR. Left( cStringa, 4 ) == "stop"
         // -----Calcola il tempo di pensamento--------
         nTempoBianco := Val( SubStr( cStringa, At( "wtime", cStringa ) + 6 ) )
         nTempoNero := Val( SubStr( cStringa, At( "btime", cStringa ) + 6 ) )
         IF aChiTocca == "bianco"
            nTempoDiPensamento := nTempoBianco / 40
         ENDIF
         IF aChiTocca == "nero"
            nTempoDiPensamento := nTempoNero / 40
         ENDIF
         IF nTempoDiPensamento < 1000
            nTempoDiPensamento := 1000
         ENDIF

         Millisec( nTempoDiPensamento )
         // -----------------Processa mosse Candidate per il NERO------------
         IF aChiTocca == "nero"
            GeneraMosseDelNero( "mem:candidate.dbf" )
            USE mem:candidate
            DO WHILE ! Eof()
               cMossa := AllTrim( field->mossa )
               simula_mossa( cMossa )
               CaseControllateDalBianco( "k", -MACRO_VALORE_RE )
               CaseControllateDalBianco( "q", -MACRO_VALORE_DONNA )
               CaseControllateDalBianco( "r", -MACRO_VALORE_TORRE )
               CaseControllateDalBianco( "b", -MACRO_VALORE_ALFIERE )
               CaseControllateDalBianco( "n", -MACRO_VALORE_CAVALLO )
               CaseControllateDalBianco( "p", -MACRO_VALORE_PEDONE )
               BonusPezzoCheCattura( cMossa )
               BonusPerStradaPercorsaDallaMossa( cMossa )
               BonusValorizzaMossaPezziPiuDistantiDalReAvversario( cMossa, "K" )
               BonusValorizzaMosseCheSiAvvicinanoAlReAvversario( cMossa, "K" )
               CaseControllateDalNero( "K", BONUS_SCACCO_A_RE )                // Minaccia
               CaseControllateDalNero( "Q", BONUS_SCACCO_A_DONNA )             // Minaccia
               CaseControllateDalNero( "R", BONUS_SCACCO_A_TORRE )             // Minaccia
               CaseControllateDalNero( "B", BONUS_SCACCO_A_ALFIERE )           // Minaccia
               CaseControllateDalNero( "N", BONUS_SCACCO_A_CAVALLO )           // Minaccia
               CaseControllateDalNero( "P", BONUS_SCACCO_A_PEDONE )            // Minaccia
               CaseControllateDalNero( "matto", 1 )
               BonusPerChiPromuove( "q", MACRO_VALORE_DONNA )
               BonusAvanzamentoPedone( cMossa )
               // -----------------------------------
               BonusSeMuoveUnPezzo( cMossa, "p", 0, 15, 0.2 )
               BonusSeMuoveUnPezzo( cMossa, "n", 0, 15, 0.4 )
               BonusSeMuoveUnPezzo( cMossa, "b", 0, 15, 0.6 )
               BonusSeMuoveUnPezzo( cMossa, "q", 0, 15, -1 )
               BonusSeMuoveUnPezzo( cMossa, "r", 0, 15, -2 )
               BonusSeMuoveUnPezzo( cMossa, "k", 0, 15, -2 )
               BonusSeMuoveUnPezzo( cMossa, "q", 15, 1000, 0.4 )
               BonusSeMuoveUnPezzo( cMossa, "b", 15, 1000, 0.5 )
               BonusSeMuoveUnPezzo( cMossa, "n", 15, 1000, 0.5 )
               BonusSeMuoveUnPezzo( cMossa, "r", 15, 1000, 0.5 )
               BonusSeMuoveUnPezzo( cMossa, "k", 30, 1000, 0.6 )
               MalusEvitaPattaTripliceRipetizione()
               // -----------------------------------
               BonusMossa( "d7d5", cMossa, "p", 0, 10, 0.2 )          // Agevola apertura al centro
               BonusMossa( "e7e5", cMossa, "p", 0, 10, 0.3 )          // Agevola apertura al centro
               BonusMossa( "c7c5", cMossa, "p", 0, 10, 0.2 )          // Agevola apertura al centro
               BonusMossa( "f7f5", cMossa, "p", 0, 10, -1 )
               BonusMossa( "f7f6", cMossa, "p", 0, 10, -1 )
               BonusMossa( "g8f6", cMossa, "n", 0, 15, 0.3 )          // Agevola sgombro per l'arrocco
               BonusMossa( "f8e7", cMossa, "b", 0, 15, 0.3 )          // Agevola sgombro per l'arrocco
               BonusMossa( "f8d6", cMossa, "b", 0, 15, 0.3 )          // Agevola sgombro per l'arrocco
               BonusMossa( "f8c5", cMossa, "b", 0, 15, 0.3 )          // Agevola sgombro per l'arrocco
               BonusMossa( "f8b4", cMossa, "b", 0, 15, 0.2 )          // Agevola sgombro per l'arrocco
               BonusMossa( "f8a3", cMossa, "b", 0, 15, 0.2 )          // Agevola sgombro per l'arrocco
               TorreConquistaColonnaAperta( "r", 8, 1 )
               TorreConquistaColonnaAperta( "r", 7, 1 )
               RaddoppioDiTorre( "r", 1 )
               DoppioPedone( "p", -0.5 )
               TriploPedone( "p", -1 )
               BonusTorreInUnaRiga( cMossa, "r", 2, 0.3 )
               BonusTorreInUnaRiga( cMossa, "r", 1, 0.3 )
*BonusOpposizione("k","K",1)
               SKIP
            ENDDO
            USE
         ENDIF
         // -----------------Processa mosse Candidate per il BIANCO------------
         IF aChiTocca == "bianco"
            GeneraMosseDelBianco( "mem:candidate.dbf" )
            USE mem:candidate
            DO WHILE ! Eof()
               cMossa := AllTrim( field->mossa )
               simula_mossa( cMossa )
               CaseControllateDalNero( "K", -MACRO_VALORE_RE )
               CaseControllateDalNero( "Q", -MACRO_VALORE_DONNA )
               CaseControllateDalNero( "R", -MACRO_VALORE_TORRE )
               CaseControllateDalNero( "B", -MACRO_VALORE_ALFIERE )
               CaseControllateDalNero( "N", -MACRO_VALORE_CAVALLO )
               CaseControllateDalNero( "P", -MACRO_VALORE_PEDONE )
               BonusPezzoCheCattura( cMossa )
               BonusPerStradaPercorsaDallaMossa( cMossa )
               BonusValorizzaMossaPezziPiuDistantiDalReAvversario( cMossa, "k" )
               BonusValorizzaMosseCheSiAvvicinanoAlReAvversario( cMossa, "k" )
               CaseControllateDalBianco( "k", BONUS_SCACCO_A_RE )                // Minaccia
               CaseControllateDalBianco( "q", BONUS_SCACCO_A_DONNA )             // Minaccia
               CaseControllateDalBianco( "r", BONUS_SCACCO_A_TORRE )             // Minaccia
               CaseControllateDalBianco( "b", BONUS_SCACCO_A_ALFIERE )           // Minaccia
               CaseControllateDalBianco( "n", BONUS_SCACCO_A_CAVALLO )           // Minaccia
               CaseControllateDalBianco( "p", BONUS_SCACCO_A_PEDONE )            // Minaccia
               CaseControllateDalBianco( "matto", 1 )
               BonusPerChiPromuove( "Q", MACRO_VALORE_DONNA )
               BonusAvanzamentoPedone( cMossa )
               // -----------------------------------
               BonusSeMuoveUnPezzo( cMossa, "P", 0, 15, 0.2 )
               BonusSeMuoveUnPezzo( cMossa, "N", 0, 15, 0.4 )
               BonusSeMuoveUnPezzo( cMossa, "B", 0, 15, 0.6 )
               BonusSeMuoveUnPezzo( cMossa, "Q", 0, 15, -1 )
               BonusSeMuoveUnPezzo( cMossa, "R", 0, 15, -2 )
               BonusSeMuoveUnPezzo( cMossa, "K", 0, 15, -2 )
               BonusSeMuoveUnPezzo( cMossa, "Q", 15, 1000, 0.4 )
               BonusSeMuoveUnPezzo( cMossa, "B", 15, 1000, 0.5 )
               BonusSeMuoveUnPezzo( cMossa, "N", 15, 1000, 0.5 )
               BonusSeMuoveUnPezzo( cMossa, "R", 15, 1000, 0.5 )
               BonusSeMuoveUnPezzo( cMossa, "K", 30, 1000, 0.6 )
               MalusEvitaPattaTripliceRipetizione()
               // -----------------------------------
               BonusMossa( "d2d4", cMossa, "P", 0, 10, 0.3 )          // Agevola apertura al centro
               BonusMossa( "e2e4", cMossa, "P", 0, 10, 0.3 )          // Agevola apertura al centro
               BonusMossa( "c2c4", cMossa, "P", 0, 10, 0.2 )          // Agevola apertura al centro
               BonusMossa( "f2f4", cMossa, "P", 0, 10, -1 )
               BonusMossa( "f2f3", cMossa, "P", 0, 10, -1 )
               BonusMossa( "g1f3", cMossa, "N", 0, 15, 0.3 )          // Agevola sgombro per l'arrocco
               BonusMossa( "f1e2", cMossa, "B", 0, 15, 0.3 )          // Agevola sgombro per l'arrocco
               BonusMossa( "f1d3", cMossa, "B", 0, 15, 0.3 )          // Agevola sgombro per l'arrocco
               BonusMossa( "f1c4", cMossa, "B", 0, 15, 0.3 )          // Agevola sgombro per l'arrocco
               BonusMossa( "f1b5", cMossa, "B", 0, 15, 0.2 )          // Agevola sgombro per l'arrocco
               BonusMossa( "f1a6", cMossa, "B", 0, 15, 0.2 )          // Agevola sgombro per l'arrocco
               TorreConquistaColonnaAperta( "R", 1, 1 )
               TorreConquistaColonnaAperta( "R", 2, 1 )
               RaddoppioDiTorre( "R", 1 )
               DoppioPedone( "P", -0.5 )
               TriploPedone( "P", -1 )
               BonusTorreInUnaRiga( cMossa, "R", 7, 0.3 )
               BonusTorreInUnaRiga( cMossa, "R", 8, 0.3 )
*BonusOpposizione("K","k",1)
               SKIP
            ENDDO
            USE
         ENDIF
         // ---------------Acqua risponde con la mossa migliore------------
         RispondiAllaGui( "bestmove " + SceglieMiglioreMossa( "mem:candidate.dbf" ) )

      ENDIF


      // -----------Se il messaggio dalla Gui e' "show"--------------
      IF Left( cStringa, 4 ) == "show"
         show()
      ENDIF

      // -----------Se il messaggio dalla Gui e' "candidate"--------------
      IF Left( cStringa, 9 ) == "candidate"
         lista_candidate( "mem:candidate.dbf" )
      ENDIF

      // -----------Se il messaggio dalla Gui e' "posizioni"--------------
      IF Left( cStringa, 9 ) == "posizioni"
         lista_posizioni()
      ENDIF


      // -----------Se il messaggio dalla Gui e' "LOGFILE TRUE"--------------
      IF lower(Left( cStringa, 33)) == lower("setoption name LogFile value true")
         lLOGFILE := .T.
      ENDIF

      // -----------Se il messaggio dalla Gui e' "LOGFILE FALSE"--------------
      IF lower(Left( cStringa, 34)) == lower("setoption name LogFile value false")
         lLOGFILE := .F.
      ENDIF


      // -----------Se il messaggio dalla Gui e' "quit"--------------
      IF Left( cStringa, 4 ) == "quit"
         SalvaSuLog( Replicate( "=", 60 ) + CRLF )
         RETURN NIL
      ENDIF

   ENDDO

   RETURN NIL

















// ---------------------------------------------------------
FUNCTION reset_scacchiera()

   LOCAL x, y
   LOCAL cStringa

   // -------Svuota scacchiera---------
   FOR x := 1 TO 8
      FOR y := 1 TO 8
         SCACCHIERA[ x, y ] := '-'
      NEXT y
   NEXT x
   // ----Pedoni-------
   FOR x = 1 TO 8
      SCACCHIERA[ x, 2 ] := "P"
      SCACCHIERA[ x, 7 ] := "p"
   NEXT x
   // ------Cavalli------
   SCACCHIERA[ 2, 1 ] := "N"
   SCACCHIERA[ 7, 1 ] := "N"
   SCACCHIERA[ 2, 8 ] := "n"
   SCACCHIERA[ 7, 8 ] := "n"
   // ------Alfieri--------
   SCACCHIERA[ 3, 1 ] := "B"
   SCACCHIERA[ 6, 1 ] := "B"
   SCACCHIERA[ 3, 8 ] := "b"
   SCACCHIERA[ 6, 8 ] := "b"
   // ----Torri---------
   SCACCHIERA[ 1, 1 ] := "R"
   SCACCHIERA[ 8, 1 ] := "R"
   SCACCHIERA[ 1, 8 ] := "r"
   SCACCHIERA[ 8, 8 ] := "r"
   // -----Donne-----
   SCACCHIERA[ 4, 1 ] := "Q"
   SCACCHIERA[ 4, 8 ] := "q"
   // -----Re-------
   SCACCHIERA[ 5, 1 ] := "K"
   SCACCHIERA[ 5, 8 ] := "k"

   // ------------------------Memorizza posizione iniziale in DBF---------
   cStringa := ""
   FOR y := 8 TO 1 STEP -1
      FOR x := 1 TO 8
         cStringa += SCACCHIERA[ x, y ]
      NEXT x
   NEXT y

   acPOSIZIONI_SCACCHIERA := {}
   anPOSIZIONI_SCACCHIERA_QT := {}
   AAdd( acPOSIZIONI_SCACCHIERA, cStringa )
   AAdd( anPOSIZIONI_SCACCHIERA_QT, 1 )

   RETURN NIL















// ---------------------------------------------------------
FUNCTION show()

   LOCAL x, y

   // -------------Visualizza scacchiera-------------------
   ?
   FOR y := 8 TO 1 STEP -1
      FOR x := 1 TO 8
         ?? SCACCHIERA[ x, y ] + Space( 1 )
      NEXT x
      ?
   NEXT y
   ?

   RETURN NIL


















// ---------------------------------------------------------------------------
FUNCTION lista_candidate( cDbfName )

   USE ( cDbfName )
   INDEX ON field->voto TO mem:candidate DESCEND
   GO TOP
   LIST field->mossa, field->voto OFF
   ?
   ? "================================================" + CRLF
   USE

   RETURN NIL























// ---------------------------------------------------------------------------
FUNCTION lista_posizioni()

   LOCAL k

   FOR k = 1 TO Len( acPOSIZIONI_SCACCHIERA )

      ? acPOSIZIONI_SCACCHIERA[ k ], " ", Str( anPOSIZIONI_SCACCHIERA_QT[ k ], 3 )
   NEXT k
   ? "===============================================================" + CRLF

   RETURN NIL






















// ---------------------------------------------------------
FUNCTION aggiorna_scacchiera( SCACCHIERA, acMoves )

   LOCAL nPartenza_x, nPartenza_y, nArrivo_x, nArrivo_y
   LOCAL x, y, k, cMossa, cStringa
   LOCAL nIndicePosizioni

   TRATTO :=     Int(    ( Len( acMoves ) ) * 0.5 + 0.5  )                 // y = floor(0.5 + 0.5*x)

   FOR k := 1 TO Len( acMoves )

      cMossa := AllTrim( acMoves[ k ] )

      nPartenza_x := Asc( SubStr( cMossa, 1, 1 ) ) -96
      nPartenza_y := Val( SubStr( cMossa, 2, 1 ) )
      nArrivo_x := Asc( SubStr( cMossa, 3, 1 ) ) -96
      nArrivo_y := Val( SubStr( cMossa, 4, 1 ) )
      // ----------Arrocco corto bianco-------------
      IF cMossa == "e1g1" .AND. SCACCHIERA[ 5, 1 ] == "K"
         SCACCHIERA[ 5, 1 ] := "-"
         SCACCHIERA[ 8, 1 ] := "-"
         SCACCHIERA[ 7, 1 ] := "K"
         SCACCHIERA[ 6, 1 ] := "R"
         // ----------Arrocco lungo bianco-------------
      ELSEIF cMossa == "e1c1" .AND. SCACCHIERA[ 5, 1 ] == "K"
         SCACCHIERA[ 5, 1 ] := "-"
         SCACCHIERA[ 1, 1 ] := "-"
         SCACCHIERA[ 3, 1 ] := "K"
         SCACCHIERA[ 4, 1 ] := "R"
         // ----------Arrocco corto nero-------------
      ELSEIF cMossa == "e8g8" .AND. SCACCHIERA[ 5, 8 ] == "k"
         SCACCHIERA[ 5, 8 ] := "-"
         SCACCHIERA[ 8, 8 ] := "-"
         SCACCHIERA[ 7, 8 ] := "k"
         SCACCHIERA[ 6, 8 ] := "r"
         // ----------Arrocco lungo nero-------------
      ELSEIF cMossa == "e8c8" .AND. SCACCHIERA[ 5, 8 ] == "k"
         SCACCHIERA[ 5, 8 ] := "-"
         SCACCHIERA[ 1, 8 ] := "-"
         SCACCHIERA[ 3, 8 ] := "k"
         SCACCHIERA[ 4, 8 ] := "r"
         // ----------Pedone cattura EnPassant-------------
      ELSEIF SCACCHIERA[ nPartenza_x, nPartenza_y ] $ "pP" .AND. SCACCHIERA[ nArrivo_x, nArrivo_y ] == "-" .AND. Abs( nPartenza_x - nArrivo_x ) == 1 .AND. Abs( nPartenza_y - nArrivo_y ) == 1
         SCACCHIERA[ nArrivo_x, nArrivo_y ] := SCACCHIERA[ nPartenza_x, nPartenza_y ]
         SCACCHIERA[ nPartenza_x, nPartenza_y ] := "-"
         SCACCHIERA[ narrivo_x, nPartenza_y ] := "-"
         // ----------Sposta pezzo-------------
      ELSE
         SCACCHIERA[ nArrivo_x, nArrivo_y ] := SCACCHIERA[ nPartenza_x, nPartenza_y ]
         SCACCHIERA[ nPartenza_x, nPartenza_y ] := "-"
         // ----------Promozione-------------
         IF Len( cMossa ) == 5 .AND. nArrivo_y > nPartenza_y        // Sta promuovendo il Bianco: a7a8
            SCACCHIERA[ nArrivo_x, nArrivo_y ] := Upper( Right( cMossa, 1 ) )
         ENDIF
         IF Len( cMossa ) == 5 .AND. nArrivo_y < nPartenza_y        // Sta promuovendo il Nero: c2c1
            SCACCHIERA[ nArrivo_x, nArrivo_y ] := Lower( Right( cMossa, 1 ) )
         ENDIF
      ENDIF


      // --------Aggiorna posizioni della scacchiera nell'Array-----
      cStringa := ""
      FOR y := 8 TO 1 STEP -1
         FOR x := 1 TO 8
            cStringa += SCACCHIERA[ x, y ]
         NEXT x
      NEXT y


      nIndicePosizioni := AScan( acPOSIZIONI_SCACCHIERA, cStringa )

      IF nIndicePosizioni == 0
         AAdd( acPOSIZIONI_SCACCHIERA, cStringa )
         AAdd( anPOSIZIONI_SCACCHIERA_QT, 1 )
      ELSE
         anPOSIZIONI_SCACCHIERA_QT[ nIndicePosizioni ]++
      ENDIF

   NEXT k

   RETURN NIL




























// ---------------------------------------------------------
//
// 77777777
// 66666667
// 55555567
// 44444567
// 33334567
// 22234567
// 11234567
// 01234567



FUNCTION fDistanzaTraDueCase( x1, y1, x2, y2 )

   LOCAL nDistx, nDisty, nDistTot

   nDistx :=  Abs( x1 - x2 )
   nDisty :=   Abs( y1 - y2 )
   nDistTot := Max( nDistx, nDisty )

   RETURN nDistTot





















// ---------------------------------------------------------
FUNCTION GeneraMosseDelNero( cDbfName )

   LOCAL x, y, k
   LOCAL xDest, yDest

   dbCreate( cDbfName, {    { "mossa", "C", 5, 0 },   { "voto", "N", 11, 3 }     }     )
   USE ( cDbfName )

   // --------------------Pedoni----------------------
   FOR y := 8 TO 1 STEP -1
      FOR x := 1 TO 8
         // -----------------Pedone cattura a sinistra------------
         IF y >= 2 .AND. y <= 7 .AND.  x > 1 .AND. SCACCHIERA[ x, y ] == "p" .AND. SCACCHIERA[ x -1, y -1 ] $ "PNBRQK"
            APPEND BLANK
            IF y == 2
               REPLACE field->mossa WITH Chr( 96 + x ) + AllTrim( Str( y ) ) + Chr( 96 + x -1 ) + AllTrim( Str( y -1 ) ) + "q"
            ELSE
               REPLACE field->mossa WITH Chr( 96 + x ) + AllTrim( Str( y ) ) + Chr( 96 + x -1 ) + AllTrim( Str( y -1 ) )
            ENDIF
         ENDIF
         // -----------------Pedone cattura a destra------------
         IF y >= 2 .AND. y <= 7 .AND. x < 8 .AND. SCACCHIERA[ x, y ] == "p" .AND. SCACCHIERA[ x + 1, y -1 ] $ "PNBRQK"
            APPEND BLANK
            IF y == 2
               REPLACE field->mossa WITH Chr( 96 + x ) + AllTrim( Str( y ) ) + Chr( 96 + x + 1 ) + AllTrim( Str( y -1 ) ) + "q"
            ELSE
               REPLACE field->mossa WITH Chr( 96 + x ) + AllTrim( Str( y ) ) + Chr( 96 + x + 1 ) + AllTrim( Str( y -1 ) )
            ENDIF
         ENDIF
         // -----------------Pedone di due passi------------
         IF y == 7 .AND. SCACCHIERA[ x, y ] == "p" .AND. SCACCHIERA[ x, y -1 ] == "-" .AND. SCACCHIERA[ x, y -2 ] == "-"
            APPEND BLANK
            REPLACE field->mossa WITH Chr( 96 + x ) + AllTrim( Str( y ) ) + Chr( 96 + x ) + AllTrim( Str( y -2 ) )
         ENDIF
         // -----------------Pedone di un passo------------
         IF y >= 2 .AND. y <= 7 .AND. SCACCHIERA[ x, y ] == "p" .AND. SCACCHIERA[ x, y -1 ] == "-"
            APPEND BLANK
            IF y == 2
               REPLACE field->mossa WITH Chr( 96 + x ) + AllTrim( Str( y ) ) + Chr( 96 + x ) + AllTrim( Str( y -1 ) ) + "q"
            ELSE
               REPLACE field->mossa WITH Chr( 96 + x ) + AllTrim( Str( y ) ) + Chr( 96 + x ) + AllTrim( Str( y -1 ) )
            ENDIF
         ENDIF
      NEXT x
   NEXT y
   // --------------------Cavalli---------------------
   FOR y := 8 TO 1 STEP -1
      FOR x := 1 TO 8
         // --------------Cavallo a L diritta in alto a destra-------
         xDest := x + 1
         yDest := y + 2
         IF xDest <= 8 .AND. yDest <= 8 .AND. SCACCHIERA[ x, y ] == "n" .AND. SCACCHIERA[ xDest, yDest ] $ "-PNBRQK"
            APPEND BLANK
            REPLACE field->mossa WITH Chr( 96 + x ) + AllTrim( Str( y ) ) + Chr( 96 + xDest ) + AllTrim( Str( yDest ) )
         ENDIF
         // --------------Cavallo a L rovesciata in alto a destra-------
         xDest := x + 2
         yDest := y + 1
         IF xDest <= 8 .AND. yDest <= 8 .AND. SCACCHIERA[ x, y ] == "n" .AND. SCACCHIERA[ xDest, yDest ] $ "-PNBRQK"
            APPEND BLANK
            REPLACE field->mossa WITH Chr( 96 + x ) + AllTrim( Str( y ) ) + Chr( 96 + xDest ) + AllTrim( Str( yDest ) )
         ENDIF
         // --------------Cavallo a L rovesciata in basso a destra-------
         xDest := x + 2
         yDest := y - 1
         IF xDest <= 8 .AND. yDest >= 1 .AND. SCACCHIERA[ x, y ] == "n" .AND. SCACCHIERA[ xDest, yDest ] $ "-PNBRQK"
            APPEND BLANK
            REPLACE field->mossa WITH Chr( 96 + x ) + AllTrim( Str( y ) ) + Chr( 96 + xDest ) + AllTrim( Str( yDest ) )
         ENDIF
         // --------------Cavallo a L diritta in basso a destra-------
         xDest := x + 1
         yDest := y - 2
         IF xDest <= 8 .AND. yDest >= 1 .AND. SCACCHIERA[ x, y ] == "n" .AND. SCACCHIERA[ xDest, yDest ] $ "-PNBRQK"
            APPEND BLANK
            REPLACE field->mossa WITH Chr( 96 + x ) + AllTrim( Str( y ) ) + Chr( 96 + xDest ) + AllTrim( Str( yDest ) )
         ENDIF
         // --------------Cavallo a L diritta in basso a sinistra-------
         xDest := x - 1
         yDest := y - 2
         IF xDest >= 1 .AND. yDest >= 1 .AND. SCACCHIERA[ x, y ] == "n" .AND. SCACCHIERA[ xDest, yDest ] $ "-PNBRQK"
            APPEND BLANK
            REPLACE field->mossa WITH Chr( 96 + x ) + AllTrim( Str( y ) ) + Chr( 96 + xDest ) + AllTrim( Str( yDest ) )
         ENDIF
         // --------------Cavallo a L rovesciata in basso a sinistra-------
         xDest := x - 2
         yDest := y - 1
         IF xDest >= 1 .AND. yDest >= 1 .AND. SCACCHIERA[ x, y ] == "n" .AND. SCACCHIERA[ xDest, yDest ] $ "-PNBRQK"
            APPEND BLANK
            REPLACE field->mossa WITH Chr( 96 + x ) + AllTrim( Str( y ) ) + Chr( 96 + xDest ) + AllTrim( Str( yDest ) )
         ENDIF
         // --------------Cavallo a L rovesciata in alto a sinistra-------
         xDest := x - 2
         yDest := y + 1
         IF xDest >= 1 .AND. yDest <= 8 .AND. SCACCHIERA[ x, y ] == "n" .AND. SCACCHIERA[ xDest, yDest ] $ "-PNBRQK"
            APPEND BLANK
            REPLACE field->mossa WITH Chr( 96 + x ) + AllTrim( Str( y ) ) + Chr( 96 + xDest ) + AllTrim( Str( yDest ) )
         ENDIF
         // --------------Cavallo a L dritta in alto a sinistra-------
         xDest := x - 1
         yDest := y + 2
         IF xDest >= 1 .AND. yDest <= 8 .AND. SCACCHIERA[ x, y ] == "n" .AND. SCACCHIERA[ xDest, yDest ] $ "-PNBRQK"
            APPEND BLANK
            REPLACE field->mossa WITH Chr( 96 + x ) + AllTrim( Str( y ) ) + Chr( 96 + xDest ) + AllTrim( Str( yDest ) )
         ENDIF
      NEXT x
   NEXT y

   // --------------------Alfieri----------------------
   FOR y := 8 TO 1 STEP -1
      FOR x := 1 TO 8
         IF SCACCHIERA[ x, y ] == "b"
            // -----------------Alfiere in alto a destra------------
            FOR k = 1 TO 7
               IF ( x + k ) > 8 .OR. ( y + k ) > 8
                  EXIT
               ENDIF
               IF SCACCHIERA[ x + k, y + k ] $ "PNBRQK"    // --------Se c'e' un pezzo bianco poi esce
                  APPEND BLANK
                  REPLACE field->mossa WITH Chr( 96 + x ) + AllTrim( Str( y ) ) + Chr( 96 + x + k ) + AllTrim( Str( y + k ) )
                  EXIT
               ENDIF
               IF SCACCHIERA[ x + k, y + k ] $ "-"    // --------Se la casa e' vuota, continua la ricerca
                  APPEND BLANK
                  REPLACE field->mossa WITH Chr( 96 + x ) + AllTrim( Str( y ) ) + Chr( 96 + x + k ) + AllTrim( Str( y + k ) )
               ENDIF
               IF SCACCHIERA[ x + k, y + k ] $ "pnbrqk"    // --------Se c'e' un pezzo nero, esce
                  EXIT
               ENDIF
            NEXT k
            // -----------------Alfiere in basso a destra------------
            FOR k = 1 TO 7
               IF ( x + k ) > 8 .OR. ( y - k ) < 1
                  EXIT
               ENDIF
               IF SCACCHIERA[ x + k, y - k ] $ "PNBRQK"    // --------Se c'e' un pezzo bianco poi esce
                  APPEND BLANK
                  REPLACE field->mossa WITH Chr( 96 + x ) + AllTrim( Str( y ) ) + Chr( 96 + x + k ) + AllTrim( Str( y - k ) )
                  EXIT
               ENDIF
               IF SCACCHIERA[ x + k, y - k ] $ "-"    // --------Se la casa e' vuota, continua la ricerca
                  APPEND BLANK
                  REPLACE field->mossa WITH Chr( 96 + x ) + AllTrim( Str( y ) ) + Chr( 96 + x + k ) + AllTrim( Str( y - k ) )
               ENDIF
               IF SCACCHIERA[ x + k, y - k ] $ "pnbrqk"    // --------Se c'e' un pezzo nero, esce
                  EXIT
               ENDIF
            NEXT k
            // -----------------Alfiere in basso a sinistra------------
            FOR k = 1 TO 7
               IF ( x - k ) < 1 .OR. ( y - k ) < 1
                  EXIT
               ENDIF
               IF SCACCHIERA[ x - k, y - k ] $ "PNBRQK"    // --------Se c'e' un pezzo bianco poi esce
                  APPEND BLANK
                  REPLACE field->mossa WITH Chr( 96 + x ) + AllTrim( Str( y ) ) + Chr( 96 + x - k ) + AllTrim( Str( y - k ) )
                  EXIT
               ENDIF
               IF SCACCHIERA[ x - k, y - k ] $ "-"    // --------Se la casa e' vuota, continua la ricerca
                  APPEND BLANK
                  REPLACE field->mossa WITH Chr( 96 + x ) + AllTrim( Str( y ) ) + Chr( 96 + x - k ) + AllTrim( Str( y - k ) )
               ENDIF
               IF SCACCHIERA[ x - k, y - k ] $ "pnbrqk"    // --------Se c'e' un pezzo nero, esce
                  EXIT
               ENDIF
            NEXT k
            // -----------------Alfiere in alto a sinistra------------
            FOR k = 1 TO 7
               IF ( x - k ) < 1 .OR. ( y + k ) > 8
                  EXIT
               ENDIF
               IF SCACCHIERA[ x - k, y + k ] $ "PNBRQK"    // --------Se c'e' un pezzo bianco poi esce
                  APPEND BLANK
                  REPLACE field->mossa WITH Chr( 96 + x ) + AllTrim( Str( y ) ) + Chr( 96 + x - k ) + AllTrim( Str( y + k ) )
                  EXIT
               ENDIF
               IF SCACCHIERA[ x - k, y + k ] $ "-"    // --------Se la casa e' vuota, continua la ricerca
                  APPEND BLANK
                  REPLACE field->mossa WITH Chr( 96 + x ) + AllTrim( Str( y ) ) + Chr( 96 + x - k ) + AllTrim( Str( y + k ) )
               ENDIF
               IF SCACCHIERA[ x - k, y + k ] $ "pnbrqk"    // --------Se c'e' un pezzo nero, esce
                  EXIT
               ENDIF
            NEXT k
         ENDIF
      NEXT x
   NEXT y
   // --------------------Torri--------------------
   FOR y := 8 TO 1 STEP -1
      FOR x := 1 TO 8
         IF SCACCHIERA[ x, y ] == "r"
            // -----------------Torre a destra------------
            FOR k = 1 TO 7
               IF ( x + k ) > 8
                  EXIT
               ENDIF
               IF SCACCHIERA[ x + k, y ] $ "PNBRQK"    // --------Se c'e' un pezzo bianco poi esce
                  APPEND BLANK
                  REPLACE field->mossa WITH Chr( 96 + x ) + AllTrim( Str( y ) ) + Chr( 96 + x + k ) + AllTrim( Str( y ) )
                  EXIT
               ENDIF
               IF SCACCHIERA[ x + k, y ] $ "-"    // --------Se la casa e' vuota, continua la ricerca
                  APPEND BLANK
                  REPLACE field->mossa WITH Chr( 96 + x ) + AllTrim( Str( y ) ) + Chr( 96 + x + k ) + AllTrim( Str( y ) )
               ENDIF
               IF SCACCHIERA[ x + k, y ] $ "pnbrqk"    // --------Se c'e' un pezzo nero, esce
                  EXIT
               ENDIF
            NEXT k
            // -----------------Torre in basso------------
            FOR k = 1 TO 7
               IF ( y - k ) < 1
                  EXIT
               ENDIF
               IF SCACCHIERA[ x, y - k ] $ "PNBRQK"    // --------Se c'e' un pezzo bianco poi esce
                  APPEND BLANK
                  REPLACE field->mossa WITH Chr( 96 + x ) + AllTrim( Str( y ) ) + Chr( 96 + x ) + AllTrim( Str( y - k ) )
                  EXIT
               ENDIF
               IF SCACCHIERA[ x, y - k ] $ "-"    // --------Se la casa e' vuota, continua la ricerca
                  APPEND BLANK
                  REPLACE field->mossa WITH Chr( 96 + x ) + AllTrim( Str( y ) ) + Chr( 96 + x ) + AllTrim( Str( y - k ) )
               ENDIF
               IF SCACCHIERA[ x, y - k ] $ "pnbrqk"    // --------Se c'e' un pezzo nero, esce
                  EXIT
               ENDIF
            NEXT k
            // -----------------Torre a sinistra------------
            FOR k = 1 TO 7
               IF ( x - k ) < 1
                  EXIT
               ENDIF
               IF SCACCHIERA[ x - k, y ] $ "PNBRQK"    // --------Se c'e' un pezzo bianco poi esce
                  APPEND BLANK
                  REPLACE field->mossa WITH Chr( 96 + x ) + AllTrim( Str( y ) ) + Chr( 96 + x - k ) + AllTrim( Str( y ) )
                  EXIT
               ENDIF
               IF SCACCHIERA[ x - k, y ] $ "-"    // --------Se la casa e' vuota, continua la ricerca
                  APPEND BLANK
                  REPLACE field->mossa WITH Chr( 96 + x ) + AllTrim( Str( y ) ) + Chr( 96 + x - k ) + AllTrim( Str( y ) )
               ENDIF
               IF SCACCHIERA[ x - k, y ] $ "pnbrqk"    // --------Se c'e' un pezzo nero, esce
                  EXIT
               ENDIF
            NEXT k
            // -----------------Torre in alto------------
            FOR k = 1 TO 7
               IF ( y + k ) > 8
                  EXIT
               ENDIF
               IF SCACCHIERA[ x, y + k ] $ "PNBRQK"    // --------Se c'e' un pezzo bianco poi esce
                  APPEND BLANK
                  REPLACE field->mossa WITH Chr( 96 + x ) + AllTrim( Str( y ) ) + Chr( 96 + x ) + AllTrim( Str( y + k ) )
                  EXIT
               ENDIF
               IF SCACCHIERA[ x, y + k ] $ "-"    // --------Se la casa e' vuota, continua la ricerca
                  APPEND BLANK
                  REPLACE field->mossa WITH Chr( 96 + x ) + AllTrim( Str( y ) ) + Chr( 96 + x ) + AllTrim( Str( y + k ) )
               ENDIF
               IF SCACCHIERA[ x, y + k ] $ "pnbrqk"    // --------Se c'e' un pezzo nero, esce
                  EXIT
               ENDIF
            NEXT k
         ENDIF
      NEXT x
   NEXT y

   // --------------------Donne----------------------
   FOR y := 8 TO 1 STEP -1
      FOR x := 1 TO 8
         IF SCACCHIERA[ x, y ] == "q"
            // -----------------Donna a destra------------
            FOR k = 1 TO 7
               IF ( x + k ) > 8
                  EXIT
               ENDIF
               IF SCACCHIERA[ x + k, y ] $ "PNBRQK"    // --------Se c'e' un pezzo bianco poi esce
                  APPEND BLANK
                  REPLACE field->mossa WITH Chr( 96 + x ) + AllTrim( Str( y ) ) + Chr( 96 + x + k ) + AllTrim( Str( y ) )
                  EXIT
               ENDIF
               IF SCACCHIERA[ x + k, y ] $ "-"    // --------Se la casa e' vuota, continua la ricerca
                  APPEND BLANK
                  REPLACE field->mossa WITH Chr( 96 + x ) + AllTrim( Str( y ) ) + Chr( 96 + x + k ) + AllTrim( Str( y ) )
               ENDIF
               IF SCACCHIERA[ x + k, y ] $ "pnbrqk"    // --------Se c'e' un pezzo nero, esce
                  EXIT
               ENDIF
            NEXT k
            // -----------------Donna in basso------------
            FOR k = 1 TO 7
               IF ( y - k ) < 1
                  EXIT
               ENDIF
               IF SCACCHIERA[ x, y - k ] $ "PNBRQK"    // --------Se c'e' un pezzo bianco poi esce
                  APPEND BLANK
                  REPLACE field->mossa WITH Chr( 96 + x ) + AllTrim( Str( y ) ) + Chr( 96 + x ) + AllTrim( Str( y - k ) )
                  EXIT
               ENDIF
               IF SCACCHIERA[ x, y - k ] $ "-"    // --------Se la casa e' vuota, continua la ricerca
                  APPEND BLANK
                  REPLACE field->mossa WITH Chr( 96 + x ) + AllTrim( Str( y ) ) + Chr( 96 + x ) + AllTrim( Str( y - k ) )
               ENDIF
               IF SCACCHIERA[ x, y - k ] $ "pnbrqk"    // --------Se c'e' un pezzo nero, esce
                  EXIT
               ENDIF
            NEXT k
            // -----------------Donna a sinistra------------
            FOR k = 1 TO 7
               IF ( x - k ) < 1
                  EXIT
               ENDIF
               IF SCACCHIERA[ x - k, y ] $ "PNBRQK"    // --------Se c'e' un pezzo bianco poi esce
                  APPEND BLANK
                  REPLACE field->mossa WITH Chr( 96 + x ) + AllTrim( Str( y ) ) + Chr( 96 + x - k ) + AllTrim( Str( y ) )
                  EXIT
               ENDIF
               IF SCACCHIERA[ x - k, y ] $ "-"    // --------Se la casa e' vuota, continua la ricerca
                  APPEND BLANK
                  REPLACE field->mossa WITH Chr( 96 + x ) + AllTrim( Str( y ) ) + Chr( 96 + x - k ) + AllTrim( Str( y ) )
               ENDIF
               IF SCACCHIERA[ x - k, y ] $ "pnbrqk"    // --------Se c'e' un pezzo nero, esce
                  EXIT
               ENDIF
            NEXT k
            // -----------------Donna in alto------------
            FOR k = 1 TO 7
               IF ( y + k ) > 8
                  EXIT
               ENDIF
               IF SCACCHIERA[ x, y + k ] $ "PNBRQK"    // --------Se c'e' un pezzo bianco poi esce
                  APPEND BLANK
                  REPLACE field->mossa WITH Chr( 96 + x ) + AllTrim( Str( y ) ) + Chr( 96 + x ) + AllTrim( Str( y + k ) )
                  EXIT
               ENDIF
               IF SCACCHIERA[ x, y + k ] $ "-"    // --------Se la casa e' vuota, continua la ricerca
                  APPEND BLANK
                  REPLACE field->mossa WITH Chr( 96 + x ) + AllTrim( Str( y ) ) + Chr( 96 + x ) + AllTrim( Str( y + k ) )
               ENDIF
               IF SCACCHIERA[ x, y + k ] $ "pnbrqk"    // --------Se c'e' un pezzo nero, esce
                  EXIT
               ENDIF
            NEXT k
            // -----------------Donna in alto a destra------------
            FOR k = 1 TO 7
               IF ( x + k ) > 8 .OR. ( y + k ) > 8
                  EXIT
               ENDIF
               IF SCACCHIERA[ x + k, y + k ] $ "PNBRQK"    // --------Se c'e' un pezzo bianco poi esce
                  APPEND BLANK
                  REPLACE field->mossa WITH Chr( 96 + x ) + AllTrim( Str( y ) ) + Chr( 96 + x + k ) + AllTrim( Str( y + k ) )
                  EXIT
               ENDIF
               IF SCACCHIERA[ x + k, y + k ] $ "-"    // --------Se la casa e' vuota, continua la ricerca
                  APPEND BLANK
                  REPLACE field->mossa WITH Chr( 96 + x ) + AllTrim( Str( y ) ) + Chr( 96 + x + k ) + AllTrim( Str( y + k ) )
               ENDIF
               IF SCACCHIERA[ x + k, y + k ] $ "pnbrqk"    // --------Se c'e' un pezzo nero, esce
                  EXIT
               ENDIF
            NEXT k
            // -----------------Donna in basso a destra------------
            FOR k = 1 TO 7
               IF ( x + k ) > 8 .OR. ( y - k ) < 1
                  EXIT
               ENDIF
               IF SCACCHIERA[ x + k, y - k ] $ "PNBRQK"    // --------Se c'e' un pezzo bianco poi esce
                  APPEND BLANK
                  REPLACE field->mossa WITH Chr( 96 + x ) + AllTrim( Str( y ) ) + Chr( 96 + x + k ) + AllTrim( Str( y - k ) )
                  EXIT
               ENDIF
               IF SCACCHIERA[ x + k, y - k ] $ "-"    // --------Se la casa e' vuota, continua la ricerca
                  APPEND BLANK
                  REPLACE field->mossa WITH Chr( 96 + x ) + AllTrim( Str( y ) ) + Chr( 96 + x + k ) + AllTrim( Str( y - k ) )
               ENDIF
               IF SCACCHIERA[ x + k, y - k ] $ "pnbrqk"    // --------Se c'e' un pezzo nero, esce
                  EXIT
               ENDIF
            NEXT k
            // -----------------Donna in basso a sinistra------------
            FOR k = 1 TO 7
               IF ( x - k ) < 1 .OR. ( y - k ) < 1
                  EXIT
               ENDIF
               IF SCACCHIERA[ x - k, y - k ] $ "PNBRQK"    // --------Se c'e' un pezzo bianco poi esce
                  APPEND BLANK
                  REPLACE field->mossa WITH Chr( 96 + x ) + AllTrim( Str( y ) ) + Chr( 96 + x - k ) + AllTrim( Str( y - k ) )
                  EXIT
               ENDIF
               IF SCACCHIERA[ x - k, y - k ] $ "-"    // --------Se la casa e' vuota, continua la ricerca
                  APPEND BLANK
                  REPLACE field->mossa WITH Chr( 96 + x ) + AllTrim( Str( y ) ) + Chr( 96 + x - k ) + AllTrim( Str( y - k ) )
               ENDIF
               IF SCACCHIERA[ x - k, y - k ] $ "pnbrqk"    // --------Se c'e' un pezzo nero, esce
                  EXIT
               ENDIF
            NEXT k
            // -----------------Donna in alto a sinistra------------
            FOR k = 1 TO 7
               IF ( x - k ) < 1 .OR. ( y + k ) > 8
                  EXIT
               ENDIF
               IF SCACCHIERA[ x - k, y + k ] $ "PNBRQK"    // --------Se c'e' un pezzo bianco poi esce
                  APPEND BLANK
                  REPLACE field->mossa WITH Chr( 96 + x ) + AllTrim( Str( y ) ) + Chr( 96 + x - k ) + AllTrim( Str( y + k ) )
                  EXIT
               ENDIF
               IF SCACCHIERA[ x - k, y + k ] $ "-"    // --------Se la casa e' vuota, continua la ricerca
                  APPEND BLANK
                  REPLACE field->mossa WITH Chr( 96 + x ) + AllTrim( Str( y ) ) + Chr( 96 + x - k ) + AllTrim( Str( y + k ) )
               ENDIF
               IF SCACCHIERA[ x - k, y + k ] $ "pnbrqk"    // --------Se c'e' un pezzo nero, esce
                  EXIT
               ENDIF
            NEXT k
         ENDIF
      NEXT x
   NEXT y
   // --------------------Re----------------------
   FOR y := 8 TO 1 STEP -1
      FOR x := 1 TO 8
         IF SCACCHIERA[ x, y ] == "k"
            // -----------------Re a destra------------
            FOR k = 1 TO 1
               IF ( x + k ) > 8
                  EXIT
               ENDIF
               IF SCACCHIERA[ x + k, y ] $ "PNBRQK"    // --------Se c'e' un pezzo bianco poi esce
                  APPEND BLANK
                  REPLACE field->mossa WITH Chr( 96 + x ) + AllTrim( Str( y ) ) + Chr( 96 + x + k ) + AllTrim( Str( y ) )
                  EXIT
               ENDIF
               IF SCACCHIERA[ x + k, y ] $ "-"    // --------Se la casa e' vuota, continua la ricerca
                  APPEND BLANK
                  REPLACE field->mossa WITH Chr( 96 + x ) + AllTrim( Str( y ) ) + Chr( 96 + x + k ) + AllTrim( Str( y ) )
               ENDIF
               IF SCACCHIERA[ x + k, y ] $ "pnbrqk"    // --------Se c'e' un pezzo nero, esce
                  EXIT
               ENDIF
            NEXT k
            // -----------------Re in basso------------
            FOR k = 1 TO 1
               IF ( y - k ) < 1
                  EXIT
               ENDIF
               IF SCACCHIERA[ x, y - k ] $ "PNBRQK"    // --------Se c'e' un pezzo bianco poi esce
                  APPEND BLANK
                  REPLACE field->mossa WITH Chr( 96 + x ) + AllTrim( Str( y ) ) + Chr( 96 + x ) + AllTrim( Str( y - k ) )
                  EXIT
               ENDIF
               IF SCACCHIERA[ x, y - k ] $ "-"    // --------Se la casa e' vuota, continua la ricerca
                  APPEND BLANK
                  REPLACE field->mossa WITH Chr( 96 + x ) + AllTrim( Str( y ) ) + Chr( 96 + x ) + AllTrim( Str( y - k ) )
               ENDIF
               IF SCACCHIERA[ x, y - k ] $ "pnbrqk"    // --------Se c'e' un pezzo nero, esce
                  EXIT
               ENDIF
            NEXT k
            // -----------------Re a sinistra------------
            FOR k = 1 TO 1
               IF ( x - k ) < 1
                  EXIT
               ENDIF
               IF SCACCHIERA[ x - k, y ] $ "PNBRQK"    // --------Se c'e' un pezzo bianco poi esce
                  APPEND BLANK
                  REPLACE field->mossa WITH Chr( 96 + x ) + AllTrim( Str( y ) ) + Chr( 96 + x - k ) + AllTrim( Str( y ) )
                  EXIT
               ENDIF
               IF SCACCHIERA[ x - k, y ] $ "-"    // --------Se la casa e' vuota, continua la ricerca
                  APPEND BLANK
                  REPLACE field->mossa WITH Chr( 96 + x ) + AllTrim( Str( y ) ) + Chr( 96 + x - k ) + AllTrim( Str( y ) )
               ENDIF
               IF SCACCHIERA[ x - k, y ] $ "pnbrqk"    // --------Se c'e' un pezzo nero, esce
                  EXIT
               ENDIF
            NEXT k
            // -----------------Re in alto------------
            FOR k = 1 TO 1
               IF ( y + k ) > 8
                  EXIT
               ENDIF
               IF SCACCHIERA[ x, y + k ] $ "PNBRQK"    // --------Se c'e' un pezzo bianco poi esce
                  APPEND BLANK
                  REPLACE field->mossa WITH Chr( 96 + x ) + AllTrim( Str( y ) ) + Chr( 96 + x ) + AllTrim( Str( y + k ) )
                  EXIT
               ENDIF
               IF SCACCHIERA[ x, y + k ] $ "-"    // --------Se la casa e' vuota, continua la ricerca
                  APPEND BLANK
                  REPLACE field->mossa WITH Chr( 96 + x ) + AllTrim( Str( y ) ) + Chr( 96 + x ) + AllTrim( Str( y + k ) )
               ENDIF
               IF SCACCHIERA[ x, y + k ] $ "pnbrqk"    // --------Se c'e' un pezzo nero, esce
                  EXIT
               ENDIF
            NEXT k
            // -----------------Re in alto a destra------------
            FOR k = 1 TO 1
               IF ( x + k ) > 8 .OR. ( y + k ) > 8
                  EXIT
               ENDIF
               IF SCACCHIERA[ x + k, y + k ] $ "PNBRQK"    // --------Se c'e' un pezzo bianco poi esce
                  APPEND BLANK
                  REPLACE field->mossa WITH Chr( 96 + x ) + AllTrim( Str( y ) ) + Chr( 96 + x + k ) + AllTrim( Str( y + k ) )
                  EXIT
               ENDIF
               IF SCACCHIERA[ x + k, y + k ] $ "-"    // --------Se la casa e' vuota, continua la ricerca
                  APPEND BLANK
                  REPLACE field->mossa WITH Chr( 96 + x ) + AllTrim( Str( y ) ) + Chr( 96 + x + k ) + AllTrim( Str( y + k ) )
               ENDIF
               IF SCACCHIERA[ x + k, y + k ] $ "pnbrqk"    // --------Se c'e' un pezzo nero, esce
                  EXIT
               ENDIF
            NEXT k
            // -----------------Re in basso a destra------------
            FOR k = 1 TO 1
               IF ( x + k ) > 8 .OR. ( y - k ) < 1
                  EXIT
               ENDIF
               IF SCACCHIERA[ x + k, y - k ] $ "PNBRQK"    // --------Se c'e' un pezzo bianco poi esce
                  APPEND BLANK
                  REPLACE field->mossa WITH Chr( 96 + x ) + AllTrim( Str( y ) ) + Chr( 96 + x + k ) + AllTrim( Str( y - k ) )
                  EXIT
               ENDIF
               IF SCACCHIERA[ x + k, y - k ] $ "-"    // --------Se la casa e' vuota, continua la ricerca
                  APPEND BLANK
                  REPLACE field->mossa WITH Chr( 96 + x ) + AllTrim( Str( y ) ) + Chr( 96 + x + k ) + AllTrim( Str( y - k ) )
               ENDIF
               IF SCACCHIERA[ x + k, y - k ] $ "pnbrqk"    // --------Se c'e' un pezzo nero, esce
                  EXIT
               ENDIF
            NEXT k
            // -----------------Re in basso a sinistra------------
            FOR k = 1 TO 1
               IF ( x - k ) < 1 .OR. ( y - k ) < 1
                  EXIT
               ENDIF
               IF SCACCHIERA[ x - k, y - k ] $ "PNBRQK"    // --------Se c'e' un pezzo bianco poi esce
                  APPEND BLANK
                  REPLACE field->mossa WITH Chr( 96 + x ) + AllTrim( Str( y ) ) + Chr( 96 + x - k ) + AllTrim( Str( y - k ) )
                  EXIT
               ENDIF
               IF SCACCHIERA[ x - k, y - k ] $ "-"    // --------Se la casa e' vuota, continua la ricerca
                  APPEND BLANK
                  REPLACE field->mossa WITH Chr( 96 + x ) + AllTrim( Str( y ) ) + Chr( 96 + x - k ) + AllTrim( Str( y - k ) )
               ENDIF
               IF SCACCHIERA[ x - k, y - k ] $ "pnbrqk"    // --------Se c'e' un pezzo nero, esce
                  EXIT
               ENDIF
            NEXT k
            // -----------------Re in alto a sinistra------------
            FOR k = 1 TO 1
               IF ( x - k ) < 1 .OR. ( y + k ) > 8
                  EXIT
               ENDIF
               IF SCACCHIERA[ x - k, y + k ] $ "PNBRQK"    // --------Se c'e' un pezzo bianco poi esce
                  APPEND BLANK
                  REPLACE field->mossa WITH Chr( 96 + x ) + AllTrim( Str( y ) ) + Chr( 96 + x - k ) + AllTrim( Str( y + k ) )
                  EXIT
               ENDIF
               IF SCACCHIERA[ x - k, y + k ] $ "-"    // --------Se la casa e' vuota, continua la ricerca
                  APPEND BLANK
                  REPLACE field->mossa WITH Chr( 96 + x ) + AllTrim( Str( y ) ) + Chr( 96 + x - k ) + AllTrim( Str( y + k ) )
               ENDIF
               IF SCACCHIERA[ x - k, y + k ] $ "pnbrqk"    // --------Se c'e' un pezzo nero, esce
                  EXIT
               ENDIF
            NEXT k
         ENDIF
      NEXT x
   NEXT y

   USE

   RETURN NIL



















// ---------------------------------------------------------
FUNCTION GeneraMosseDelBianco( cDbfName )

   LOCAL x, y, k
   LOCAL xDest, yDest

   dbCreate( cDbfName, {    { "mossa", "C", 5, 0 },   { "voto", "N", 11, 3 }     }     )
   USE ( cDbfName )

   // ----------------------Pedoni-------------------------
   FOR y := 1 TO 8
      FOR x := 1 TO 8
         // -----------------Pedone cattura a sinistra------------
         IF y <= 7 .AND. y >= 2 .AND.  x > 1 .AND. SCACCHIERA[ x, y ] == "P" .AND. SCACCHIERA[ x -1, y + 1 ] $ Lower( "PNBRQK" )
            APPEND BLANK
            IF y == 7
               REPLACE field->mossa WITH Chr( 96 + x ) + AllTrim( Str( y ) ) + Chr( 96 + x -1 ) + AllTrim( Str( y + 1 ) ) + "Q"
            ELSE
               REPLACE field->mossa WITH Chr( 96 + x ) + AllTrim( Str( y ) ) + Chr( 96 + x -1 ) + AllTrim( Str( y + 1 ) )
            ENDIF
         ENDIF
         // -----------------Pedone cattura a destra------------
         IF y <= 7 .AND. y >= 2 .AND. x < 8 .AND. SCACCHIERA[ x, y ] == "P" .AND. SCACCHIERA[ x + 1, y + 1 ] $ Lower( "PNBRQK" )
            APPEND BLANK
            IF y == 7
               REPLACE field->mossa WITH Chr( 96 + x ) + AllTrim( Str( y ) ) + Chr( 96 + x + 1 ) + AllTrim( Str( y + 1 ) ) + "Q"
            ELSE
               REPLACE field->mossa WITH Chr( 96 + x ) + AllTrim( Str( y ) ) + Chr( 96 + x + 1 ) + AllTrim( Str( y + 1 ) )
            ENDIF
         ENDIF
         // -----------------Pedone di due passi------------
         IF y == 2 .AND. SCACCHIERA[ x, y ] == "P" .AND. SCACCHIERA[ x, y + 1 ] == "-" .AND. SCACCHIERA[ x, y + 2 ] == "-"
            APPEND BLANK
            REPLACE field->mossa WITH Chr( 96 + x ) + AllTrim( Str( y ) ) + Chr( 96 + x ) + AllTrim( Str( y + 2 ) )
         ENDIF
         // -----------------Pedone di un passo------------
         IF y <= 7 .AND. y >= 2 .AND. SCACCHIERA[ x, y ] == "P" .AND. SCACCHIERA[ x, y + 1 ] == "-"
            APPEND BLANK
            IF y == 7
               REPLACE field->mossa WITH Chr( 96 + x ) + AllTrim( Str( y ) ) + Chr( 96 + x ) + AllTrim( Str( y + 1 ) ) + "Q"
            ELSE
               REPLACE field->mossa WITH Chr( 96 + x ) + AllTrim( Str( y ) ) + Chr( 96 + x ) + AllTrim( Str( y + 1 ) )
            ENDIF
         ENDIF
      NEXT x
   NEXT y

   // -------------------------Cavalli--------------------------------
   FOR y := 8 TO 1 STEP -1
      FOR x := 1 TO 8
         // --------------Cavallo a L diritta in alto a destra-------
         xDest := x + 1
         yDest := y + 2
         IF xDest <= 8 .AND. yDest <= 8 .AND. SCACCHIERA[ x, y ] == "N" .AND. SCACCHIERA[ xDest, yDest ] $ Lower( "-PNBRQK" )
            APPEND BLANK
            REPLACE field->mossa WITH Chr( 96 + x ) + AllTrim( Str( y ) ) + Chr( 96 + xDest ) + AllTrim( Str( yDest ) )
         ENDIF
         // --------------Cavallo a L rovesciata in alto a destra-------
         xDest := x + 2
         yDest := y + 1
         IF xDest <= 8 .AND. yDest <= 8 .AND. SCACCHIERA[ x, y ] == "N" .AND. SCACCHIERA[ xDest, yDest ] $ Lower( "-PNBRQK" )
            APPEND BLANK
            REPLACE field->mossa WITH Chr( 96 + x ) + AllTrim( Str( y ) ) + Chr( 96 + xDest ) + AllTrim( Str( yDest ) )
         ENDIF
         // --------------Cavallo a L rovesciata in basso a destra-------
         xDest := x + 2
         yDest := y - 1
         IF xDest <= 8 .AND. yDest >= 1 .AND. SCACCHIERA[ x, y ] == "N" .AND. SCACCHIERA[ xDest, yDest ] $ Lower( "-PNBRQK" )
            APPEND BLANK
            REPLACE field->mossa WITH Chr( 96 + x ) + AllTrim( Str( y ) ) + Chr( 96 + xDest ) + AllTrim( Str( yDest ) )
         ENDIF
         // --------------Cavallo a L diritta in basso a destra-------
         xDest := x + 1
         yDest := y - 2
         IF xDest <= 8 .AND. yDest >= 1 .AND. SCACCHIERA[ x, y ] == "N" .AND. SCACCHIERA[ xDest, yDest ] $ Lower( "-PNBRQK" )
            APPEND BLANK
            REPLACE field->mossa WITH Chr( 96 + x ) + AllTrim( Str( y ) ) + Chr( 96 + xDest ) + AllTrim( Str( yDest ) )
         ENDIF
         // --------------Cavallo a L diritta in basso a sinistra-------
         xDest := x - 1
         yDest := y - 2
         IF xDest >= 1 .AND. yDest >= 1 .AND. SCACCHIERA[ x, y ] == "N" .AND. SCACCHIERA[ xDest, yDest ] $ Lower( "-PNBRQK" )
            APPEND BLANK
            REPLACE field->mossa WITH Chr( 96 + x ) + AllTrim( Str( y ) ) + Chr( 96 + xDest ) + AllTrim( Str( yDest ) )
         ENDIF
         // --------------Cavallo a L rovesciata in basso a sinistra-------
         xDest := x - 2
         yDest := y - 1
         IF xDest >= 1 .AND. yDest >= 1 .AND. SCACCHIERA[ x, y ] == "N" .AND. SCACCHIERA[ xDest, yDest ] $ Lower( "-PNBRQK" )
            APPEND BLANK
            REPLACE field->mossa WITH Chr( 96 + x ) + AllTrim( Str( y ) ) + Chr( 96 + xDest ) + AllTrim( Str( yDest ) )
         ENDIF
         // --------------Cavallo a L rovesciata in alto a sinistra-------
         xDest := x - 2
         yDest := y + 1
         IF xDest >= 1 .AND. yDest <= 8 .AND. SCACCHIERA[ x, y ] == "N" .AND. SCACCHIERA[ xDest, yDest ] $ Lower( "-PNBRQK" )
            APPEND BLANK
            REPLACE field->mossa WITH Chr( 96 + x ) + AllTrim( Str( y ) ) + Chr( 96 + xDest ) + AllTrim( Str( yDest ) )
         ENDIF
         // --------------Cavallo a L dritta in alto a sinistra-------
         xDest := x - 1
         yDest := y + 2
         IF xDest >= 1 .AND. yDest <= 8 .AND. SCACCHIERA[ x, y ] == "N" .AND. SCACCHIERA[ xDest, yDest ] $ Lower( "-PNBRQK" )
            APPEND BLANK
            REPLACE field->mossa WITH Chr( 96 + x ) + AllTrim( Str( y ) ) + Chr( 96 + xDest ) + AllTrim( Str( yDest ) )
         ENDIF
      NEXT x
   NEXT y

   // -----------------------Alfieri-----------------------------
   FOR y := 8 TO 1 STEP -1
      FOR x := 1 TO 8
         IF SCACCHIERA[ x, y ] == "B"
            // -----------------Alfiere in alto a destra------------
            FOR k = 1 TO 7
               IF ( x + k ) > 8 .OR. ( y + k ) > 8
                  EXIT
               ENDIF
               IF SCACCHIERA[ x + k, y + k ] $ Lower( "PNBRQK" )    // --------Se c'e' un pezzo nero poi esce
                  APPEND BLANK
                  REPLACE field->mossa WITH Chr( 96 + x ) + AllTrim( Str( y ) ) + Chr( 96 + x + k ) + AllTrim( Str( y + k ) )
                  EXIT
               ENDIF
               IF SCACCHIERA[ x + k, y + k ] $ "-"    // --------Se la casa e' vuota, continua la ricerca
                  APPEND BLANK
                  REPLACE field->mossa WITH Chr( 96 + x ) + AllTrim( Str( y ) ) + Chr( 96 + x + k ) + AllTrim( Str( y + k ) )
               ENDIF
               IF SCACCHIERA[ x + k, y + k ] $ Upper( "pnbrqk" )   // --------Se c'e' un pezzo bianco, esce
                  EXIT
               ENDIF
            NEXT k
            // -----------------Alfiere in basso a destra------------
            FOR k = 1 TO 7
               IF ( x + k ) > 8 .OR. ( y - k ) < 1
                  EXIT
               ENDIF
               IF SCACCHIERA[ x + k, y - k ] $ Lower( "PNBRQK" )   // --------Se c'e' un pezzo nero poi esce
                  APPEND BLANK
                  REPLACE field->mossa WITH Chr( 96 + x ) + AllTrim( Str( y ) ) + Chr( 96 + x + k ) + AllTrim( Str( y - k ) )
                  EXIT
               ENDIF
               IF SCACCHIERA[ x + k, y - k ] $ "-"    // --------Se la casa e' vuota, continua la ricerca
                  APPEND BLANK
                  REPLACE field->mossa WITH Chr( 96 + x ) + AllTrim( Str( y ) ) + Chr( 96 + x + k ) + AllTrim( Str( y - k ) )
               ENDIF
               IF SCACCHIERA[ x + k, y - k ] $ Upper( "pnbrqk" )    // --------Se c'e' un pezzo bianco, esce
                  EXIT
               ENDIF
            NEXT k
            // -----------------Alfiere in basso a sinistra------------
            FOR k = 1 TO 7
               IF ( x - k ) < 1 .OR. ( y - k ) < 1
                  EXIT
               ENDIF
               IF SCACCHIERA[ x - k, y - k ] $ Lower( "PNBRQK" )    // --------Se c'e' un pezzo nero poi esce
                  APPEND BLANK
                  REPLACE field->mossa WITH Chr( 96 + x ) + AllTrim( Str( y ) ) + Chr( 96 + x - k ) + AllTrim( Str( y - k ) )
                  EXIT
               ENDIF
               IF SCACCHIERA[ x - k, y - k ] $ "-"    // --------Se la casa e' vuota, continua la ricerca
                  APPEND BLANK
                  REPLACE field->mossa WITH Chr( 96 + x ) + AllTrim( Str( y ) ) + Chr( 96 + x - k ) + AllTrim( Str( y - k ) )
               ENDIF
               IF SCACCHIERA[ x - k, y - k ] $ Upper( "pnbrqk" )    // --------Se c'e' un pezzo bianco, esce
                  EXIT
               ENDIF
            NEXT k
            // -----------------Alfiere in alto a sinistra------------
            FOR k = 1 TO 7
               IF ( x - k ) < 1 .OR. ( y + k ) > 8
                  EXIT
               ENDIF
               IF SCACCHIERA[ x - k, y + k ] $ Lower( "PNBRQK" )   // --------Se c'e' un pezzo nero poi esce
                  APPEND BLANK
                  REPLACE field->mossa WITH Chr( 96 + x ) + AllTrim( Str( y ) ) + Chr( 96 + x - k ) + AllTrim( Str( y + k ) )
                  EXIT
               ENDIF
               IF SCACCHIERA[ x - k, y + k ] $ "-"    // --------Se la casa e' vuota, continua la ricerca
                  APPEND BLANK
                  REPLACE field->mossa WITH Chr( 96 + x ) + AllTrim( Str( y ) ) + Chr( 96 + x - k ) + AllTrim( Str( y + k ) )
               ENDIF
               IF SCACCHIERA[ x - k, y + k ] $ Upper( "pnbrqk" )    // --------Se c'e' un pezzo bianco, esce
                  EXIT
               ENDIF
            NEXT k
         ENDIF
      NEXT x
   NEXT y

   // ------------------------Torri--------------------------
   FOR y := 8 TO 1 STEP -1
      FOR x := 1 TO 8
         IF SCACCHIERA[ x, y ] == "R"
            // -----------------Torre a destra------------
            FOR k = 1 TO 7
               IF ( x + k ) > 8
                  EXIT
               ENDIF
               IF SCACCHIERA[ x + k, y ] $ Lower( "PNBRQK" )   // --------Se c'e' un pezzo nero poi esce
                  APPEND BLANK
                  REPLACE field->mossa WITH Chr( 96 + x ) + AllTrim( Str( y ) ) + Chr( 96 + x + k ) + AllTrim( Str( y ) )
                  EXIT
               ENDIF
               IF SCACCHIERA[ x + k, y ] $ "-"    // --------Se la casa e' vuota, continua la ricerca
                  APPEND BLANK
                  REPLACE field->mossa WITH Chr( 96 + x ) + AllTrim( Str( y ) ) + Chr( 96 + x + k ) + AllTrim( Str( y ) )
               ENDIF
               IF SCACCHIERA[ x + k, y ] $ Upper( "pnbrqk" )    // --------Se c'e' un pezzo bianco, esce
                  EXIT
               ENDIF
            NEXT k
            // -----------------Torre in basso------------
            FOR k = 1 TO 7
               IF ( y - k ) < 1
                  EXIT
               ENDIF
               IF SCACCHIERA[ x, y - k ] $ Lower( "PNBRQK" )    // --------Se c'e' un pezzo nero poi esce
                  APPEND BLANK
                  REPLACE field->mossa WITH Chr( 96 + x ) + AllTrim( Str( y ) ) + Chr( 96 + x ) + AllTrim( Str( y - k ) )
                  EXIT
               ENDIF
               IF SCACCHIERA[ x, y - k ] $ "-"    // --------Se la casa e' vuota, continua la ricerca
                  APPEND BLANK
                  REPLACE field->mossa WITH Chr( 96 + x ) + AllTrim( Str( y ) ) + Chr( 96 + x ) + AllTrim( Str( y - k ) )
               ENDIF
               IF SCACCHIERA[ x, y - k ] $ Upper( "pnbrqk" )    // --------Se c'e' un pezzo bianco, esce
                  EXIT
               ENDIF
            NEXT k
            // -----------------Torre a sinistra------------
            FOR k = 1 TO 7
               IF ( x - k ) < 1
                  EXIT
               ENDIF
               IF SCACCHIERA[ x - k, y ] $ Lower( "PNBRQK" )    // --------Se c'e' un pezzo nero poi esce
                  APPEND BLANK
                  REPLACE field->mossa WITH Chr( 96 + x ) + AllTrim( Str( y ) ) + Chr( 96 + x - k ) + AllTrim( Str( y ) )
                  EXIT
               ENDIF
               IF SCACCHIERA[ x - k, y ] $ "-"    // --------Se la casa e' vuota, continua la ricerca
                  APPEND BLANK
                  REPLACE field->mossa WITH Chr( 96 + x ) + AllTrim( Str( y ) ) + Chr( 96 + x - k ) + AllTrim( Str( y ) )
               ENDIF
               IF SCACCHIERA[ x - k, y ] $ Upper( "pnbrqk" )    // --------Se c'e' un pezzo bianco, esce
                  EXIT
               ENDIF
            NEXT k
            // -----------------Torre in alto------------
            FOR k = 1 TO 7
               IF ( y + k ) > 8
                  EXIT
               ENDIF
               IF SCACCHIERA[ x, y + k ] $ Lower( "PNBRQK" )    // --------Se c'e' un pezzo nero poi esce
                  APPEND BLANK
                  REPLACE field->mossa WITH Chr( 96 + x ) + AllTrim( Str( y ) ) + Chr( 96 + x ) + AllTrim( Str( y + k ) )
                  EXIT
               ENDIF
               IF SCACCHIERA[ x, y + k ] $ "-"    // --------Se la casa e' vuota, continua la ricerca
                  APPEND BLANK
                  REPLACE field->mossa WITH Chr( 96 + x ) + AllTrim( Str( y ) ) + Chr( 96 + x ) + AllTrim( Str( y + k ) )
               ENDIF
               IF SCACCHIERA[ x, y + k ] $ Upper( "pnbrqk" )    // --------Se c'e' un pezzo bianco, esce
                  EXIT
               ENDIF
            NEXT k
         ENDIF
      NEXT x
   NEXT y


   // -------------------Donne-------------------------
   FOR y := 8 TO 1 STEP -1
      FOR x := 1 TO 8
         IF SCACCHIERA[ x, y ] == "Q"
            // -----------------Donna a destra------------
            FOR k = 1 TO 7
               IF ( x + k ) > 8
                  EXIT
               ENDIF
               IF SCACCHIERA[ x + k, y ] $ Lower( "PNBRQK" )    // --------Se c'e' un pezzo nero poi esce
                  APPEND BLANK
                  REPLACE field->mossa WITH Chr( 96 + x ) + AllTrim( Str( y ) ) + Chr( 96 + x + k ) + AllTrim( Str( y ) )
                  EXIT
               ENDIF
               IF SCACCHIERA[ x + k, y ] $ "-"    // --------Se la casa e' vuota, continua la ricerca
                  APPEND BLANK
                  REPLACE field->mossa WITH Chr( 96 + x ) + AllTrim( Str( y ) ) + Chr( 96 + x + k ) + AllTrim( Str( y ) )
               ENDIF
               IF SCACCHIERA[ x + k, y ] $ Upper( "pnbrqk" )    // --------Se c'e' un pezzo bianco, esce
                  EXIT
               ENDIF
            NEXT k
            // -----------------Donna in basso------------
            FOR k = 1 TO 7
               IF ( y - k ) < 1
                  EXIT
               ENDIF
               IF SCACCHIERA[ x, y - k ] $ Lower( "PNBRQK" )    // --------Se c'e' un pezzo nero poi esce
                  APPEND BLANK
                  REPLACE field->mossa WITH Chr( 96 + x ) + AllTrim( Str( y ) ) + Chr( 96 + x ) + AllTrim( Str( y - k ) )
                  EXIT
               ENDIF
               IF SCACCHIERA[ x, y - k ] $ "-"    // --------Se la casa e' vuota, continua la ricerca
                  APPEND BLANK
                  REPLACE field->mossa WITH Chr( 96 + x ) + AllTrim( Str( y ) ) + Chr( 96 + x ) + AllTrim( Str( y - k ) )
               ENDIF
               IF SCACCHIERA[ x, y - k ] $ Upper( "pnbrqk" )    // --------Se c'e' un pezzo bianco, esce
                  EXIT
               ENDIF
            NEXT k
            // -----------------Donna a sinistra------------
            FOR k = 1 TO 7
               IF ( x - k ) < 1
                  EXIT
               ENDIF
               IF SCACCHIERA[ x - k, y ] $ Lower( "PNBRQK" )    // --------Se c'e' un pezzo nero poi esce
                  APPEND BLANK
                  REPLACE field->mossa WITH Chr( 96 + x ) + AllTrim( Str( y ) ) + Chr( 96 + x - k ) + AllTrim( Str( y ) )
                  EXIT
               ENDIF
               IF SCACCHIERA[ x - k, y ] $ "-"    // --------Se la casa e' vuota, continua la ricerca
                  APPEND BLANK
                  REPLACE field->mossa WITH Chr( 96 + x ) + AllTrim( Str( y ) ) + Chr( 96 + x - k ) + AllTrim( Str( y ) )
               ENDIF
               IF SCACCHIERA[ x - k, y ] $ Upper( "pnbrqk" )    // --------Se c'e' un pezzo bianco, esce
                  EXIT
               ENDIF
            NEXT k
            // -----------------Donna in alto------------
            FOR k = 1 TO 7
               IF ( y + k ) > 8
                  EXIT
               ENDIF
               IF SCACCHIERA[ x, y + k ] $ Lower( "PNBRQK" )    // --------Se c'e' un pezzo nero poi esce
                  APPEND BLANK
                  REPLACE field->mossa WITH Chr( 96 + x ) + AllTrim( Str( y ) ) + Chr( 96 + x ) + AllTrim( Str( y + k ) )
                  EXIT
               ENDIF
               IF SCACCHIERA[ x, y + k ] $ "-"    // --------Se la casa e' vuota, continua la ricerca
                  APPEND BLANK
                  REPLACE field->mossa WITH Chr( 96 + x ) + AllTrim( Str( y ) ) + Chr( 96 + x ) + AllTrim( Str( y + k ) )
               ENDIF
               IF SCACCHIERA[ x, y + k ] $ Upper( "pnbrqk" )    // --------Se c'e' un pezzo bianco, esce
                  EXIT
               ENDIF
            NEXT k
            // -----------------Donna in alto a destra------------
            FOR k = 1 TO 7
               IF ( x + k ) > 8 .OR. ( y + k ) > 8
                  EXIT
               ENDIF
               IF SCACCHIERA[ x + k, y + k ] $ Lower( "PNBRQK" )    // --------Se c'e' un pezzo nero poi esce
                  APPEND BLANK
                  REPLACE field->mossa WITH Chr( 96 + x ) + AllTrim( Str( y ) ) + Chr( 96 + x + k ) + AllTrim( Str( y + k ) )
                  EXIT
               ENDIF
               IF SCACCHIERA[ x + k, y + k ] $ "-"    // --------Se la casa e' vuota, continua la ricerca
                  APPEND BLANK
                  REPLACE field->mossa WITH Chr( 96 + x ) + AllTrim( Str( y ) ) + Chr( 96 + x + k ) + AllTrim( Str( y + k ) )
               ENDIF
               IF SCACCHIERA[ x + k, y + k ] $ Upper( "pnbrqk" )    // --------Se c'e' un pezzo bianco, esce
                  EXIT
               ENDIF
            NEXT k
            // -----------------Donna in basso a destra------------
            FOR k = 1 TO 7
               IF ( x + k ) > 8 .OR. ( y - k ) < 1
                  EXIT
               ENDIF
               IF SCACCHIERA[ x + k, y - k ] $ Lower( "PNBRQK" )    // --------Se c'e' un pezzo nero poi esce
                  APPEND BLANK
                  REPLACE field->mossa WITH Chr( 96 + x ) + AllTrim( Str( y ) ) + Chr( 96 + x + k ) + AllTrim( Str( y - k ) )
                  EXIT
               ENDIF
               IF SCACCHIERA[ x + k, y - k ] $ "-"    // --------Se la casa e' vuota, continua la ricerca
                  APPEND BLANK
                  REPLACE field->mossa WITH Chr( 96 + x ) + AllTrim( Str( y ) ) + Chr( 96 + x + k ) + AllTrim( Str( y - k ) )
               ENDIF
               IF SCACCHIERA[ x + k, y - k ] $ Upper( "pnbrqk" )    // --------Se c'e' un pezzo bianco, esce
                  EXIT
               ENDIF
            NEXT k
            // -----------------Donna in basso a sinistra------------
            FOR k = 1 TO 7
               IF ( x - k ) < 1 .OR. ( y - k ) < 1
                  EXIT
               ENDIF
               IF SCACCHIERA[ x - k, y - k ] $ Lower( "PNBRQK" )    // --------Se c'e' un pezzo nero poi esce
                  APPEND BLANK
                  REPLACE field->mossa WITH Chr( 96 + x ) + AllTrim( Str( y ) ) + Chr( 96 + x - k ) + AllTrim( Str( y - k ) )
                  EXIT
               ENDIF
               IF SCACCHIERA[ x - k, y - k ] $ "-"    // --------Se la casa e' vuota, continua la ricerca
                  APPEND BLANK
                  REPLACE field->mossa WITH Chr( 96 + x ) + AllTrim( Str( y ) ) + Chr( 96 + x - k ) + AllTrim( Str( y - k ) )
               ENDIF
               IF SCACCHIERA[ x - k, y - k ] $ Upper( "pnbrqk" )    // --------Se c'e' un pezzo bianco, esce
                  EXIT
               ENDIF
            NEXT k
            // -----------------Donna in alto a sinistra------------
            FOR k = 1 TO 7
               IF ( x - k ) < 1 .OR. ( y + k ) > 8
                  EXIT
               ENDIF
               IF SCACCHIERA[ x - k, y + k ] $ Lower( "PNBRQK" )    // --------Se c'e' un pezzo nero poi esce
                  APPEND BLANK
                  REPLACE field->mossa WITH Chr( 96 + x ) + AllTrim( Str( y ) ) + Chr( 96 + x - k ) + AllTrim( Str( y + k ) )
                  EXIT
               ENDIF
               IF SCACCHIERA[ x - k, y + k ] $ "-"    // --------Se la casa e' vuota, continua la ricerca
                  APPEND BLANK
                  REPLACE field->mossa WITH Chr( 96 + x ) + AllTrim( Str( y ) ) + Chr( 96 + x - k ) + AllTrim( Str( y + k ) )
               ENDIF
               IF SCACCHIERA[ x - k, y + k ] $ Upper( "pnbrqk" )    // --------Se c'e' un pezzo bianco, esce
                  EXIT
               ENDIF
            NEXT k
         ENDIF
      NEXT x
   NEXT y

   // ------------------------Re-----------------------------
   FOR y := 8 TO 1 STEP -1
      FOR x := 1 TO 8
         IF SCACCHIERA[ x, y ] == "K"
            // -----------------Re a destra------------
            FOR k = 1 TO 1
               IF ( x + k ) > 8
                  EXIT
               ENDIF
               IF SCACCHIERA[ x + k, y ] $ Lower( "PNBRQK" )    // --------Se c'e' un pezzo nero poi esce
                  APPEND BLANK
                  REPLACE field->mossa WITH Chr( 96 + x ) + AllTrim( Str( y ) ) + Chr( 96 + x + k ) + AllTrim( Str( y ) )
                  EXIT
               ENDIF
               IF SCACCHIERA[ x + k, y ] $ "-"    // --------Se la casa e' vuota, continua la ricerca
                  APPEND BLANK
                  REPLACE field->mossa WITH Chr( 96 + x ) + AllTrim( Str( y ) ) + Chr( 96 + x + k ) + AllTrim( Str( y ) )
               ENDIF
               IF SCACCHIERA[ x + k, y ] $ Upper( "pnbrqk" )    // --------Se c'e' un pezzo bianco, esce
                  EXIT
               ENDIF
            NEXT k
            // -----------------Re in basso------------
            FOR k = 1 TO 1
               IF ( y - k ) < 1
                  EXIT
               ENDIF
               IF SCACCHIERA[ x, y - k ] $ Lower( "PNBRQK" )    // --------Se c'e' un pezzo nero poi esce
                  APPEND BLANK
                  REPLACE field->mossa WITH Chr( 96 + x ) + AllTrim( Str( y ) ) + Chr( 96 + x ) + AllTrim( Str( y - k ) )
                  EXIT
               ENDIF
               IF SCACCHIERA[ x, y - k ] $ "-"    // --------Se la casa e' vuota, continua la ricerca
                  APPEND BLANK
                  REPLACE field->mossa WITH Chr( 96 + x ) + AllTrim( Str( y ) ) + Chr( 96 + x ) + AllTrim( Str( y - k ) )
               ENDIF
               IF SCACCHIERA[ x, y - k ] $ Upper( "pnbrqk" )    // --------Se c'e' un pezzo bianco, esce
                  EXIT
               ENDIF
            NEXT k
            // -----------------Re a sinistra------------
            FOR k = 1 TO 1
               IF ( x - k ) < 1
                  EXIT
               ENDIF
               IF SCACCHIERA[ x - k, y ] $ Lower( "PNBRQK" )    // --------Se c'e' un pezzo nero poi esce
                  APPEND BLANK
                  REPLACE field->mossa WITH Chr( 96 + x ) + AllTrim( Str( y ) ) + Chr( 96 + x - k ) + AllTrim( Str( y ) )
                  EXIT
               ENDIF
               IF SCACCHIERA[ x - k, y ] $ "-"    // --------Se la casa e' vuota, continua la ricerca
                  APPEND BLANK
                  REPLACE field->mossa WITH Chr( 96 + x ) + AllTrim( Str( y ) ) + Chr( 96 + x - k ) + AllTrim( Str( y ) )
               ENDIF
               IF SCACCHIERA[ x - k, y ] $ Upper( "pnbrqk" )    // --------Se c'e' un pezzo bianco, esce
                  EXIT
               ENDIF
            NEXT k
            // -----------------Re in alto------------
            FOR k = 1 TO 1
               IF ( y + k ) > 8
                  EXIT
               ENDIF
               IF SCACCHIERA[ x, y + k ] $ Lower( "PNBRQK" )    // --------Se c'e' un pezzo nero poi esce
                  APPEND BLANK
                  REPLACE field->mossa WITH Chr( 96 + x ) + AllTrim( Str( y ) ) + Chr( 96 + x ) + AllTrim( Str( y + k ) )
                  EXIT
               ENDIF
               IF SCACCHIERA[ x, y + k ] $ "-"    // --------Se la casa e' vuota, continua la ricerca
                  APPEND BLANK
                  REPLACE field->mossa WITH Chr( 96 + x ) + AllTrim( Str( y ) ) + Chr( 96 + x ) + AllTrim( Str( y + k ) )
               ENDIF
               IF SCACCHIERA[ x, y + k ] $ Upper( "pnbrqk" )    // --------Se c'e' un pezzo bianco, esce
                  EXIT
               ENDIF
            NEXT k
            // -----------------Re in alto a destra------------
            FOR k = 1 TO 1
               IF ( x + k ) > 8 .OR. ( y + k ) > 8
                  EXIT
               ENDIF
               IF SCACCHIERA[ x + k, y + k ] $ Lower( "PNBRQK" )    // --------Se c'e' un pezzo nero poi esce
                  APPEND BLANK
                  REPLACE field->mossa WITH Chr( 96 + x ) + AllTrim( Str( y ) ) + Chr( 96 + x + k ) + AllTrim( Str( y + k ) )
                  EXIT
               ENDIF
               IF SCACCHIERA[ x + k, y + k ] $ "-"    // --------Se la casa e' vuota, continua la ricerca
                  APPEND BLANK
                  REPLACE field->mossa WITH Chr( 96 + x ) + AllTrim( Str( y ) ) + Chr( 96 + x + k ) + AllTrim( Str( y + k ) )
               ENDIF
               IF SCACCHIERA[ x + k, y + k ] $ Upper( "pnbrqk" )    // --------Se c'e' un pezzo nero, esce
                  EXIT
               ENDIF
            NEXT k
            // -----------------Re in basso a destra------------
            FOR k = 1 TO 1
               IF ( x + k ) > 8 .OR. ( y - k ) < 1
                  EXIT
               ENDIF
               IF SCACCHIERA[ x + k, y - k ] $ Lower( "PNBRQK" )    // --------Se c'e' un pezzo nero poi esce
                  APPEND BLANK
                  REPLACE field->mossa WITH Chr( 96 + x ) + AllTrim( Str( y ) ) + Chr( 96 + x + k ) + AllTrim( Str( y - k ) )
                  EXIT
               ENDIF
               IF SCACCHIERA[ x + k, y - k ] $ "-"    // --------Se la casa e' vuota, continua la ricerca
                  APPEND BLANK
                  REPLACE field->mossa WITH Chr( 96 + x ) + AllTrim( Str( y ) ) + Chr( 96 + x + k ) + AllTrim( Str( y - k ) )
               ENDIF
               IF SCACCHIERA[ x + k, y - k ] $ Upper( "pnbrqk" )    // --------Se c'e' un pezzo bianco, esce
                  EXIT
               ENDIF
            NEXT k
            // -----------------Re in basso a sinistra------------
            FOR k = 1 TO 1
               IF ( x - k ) < 1 .OR. ( y - k ) < 1
                  EXIT
               ENDIF
               IF SCACCHIERA[ x - k, y - k ] $ Lower( "PNBRQK" )    // --------Se c'e' un pezzo nero poi esce
                  APPEND BLANK
                  REPLACE field->mossa WITH Chr( 96 + x ) + AllTrim( Str( y ) ) + Chr( 96 + x - k ) + AllTrim( Str( y - k ) )
                  EXIT
               ENDIF
               IF SCACCHIERA[ x - k, y - k ] $ "-"    // --------Se la casa e' vuota, continua la ricerca
                  APPEND BLANK
                  REPLACE field->mossa WITH Chr( 96 + x ) + AllTrim( Str( y ) ) + Chr( 96 + x - k ) + AllTrim( Str( y - k ) )
               ENDIF
               IF SCACCHIERA[ x - k, y - k ] $ Upper( "pnbrqk" )    // --------Se c'e' un pezzo bianco, esce
                  EXIT
               ENDIF
            NEXT k
            // -----------------Re in alto a sinistra------------
            FOR k = 1 TO 1
               IF ( x - k ) < 1 .OR. ( y + k ) > 8
                  EXIT
               ENDIF
               IF SCACCHIERA[ x - k, y + k ] $ Lower( "PNBRQK" )    // --------Se c'e' un pezzo nero poi esce
                  APPEND BLANK
                  REPLACE field->mossa WITH Chr( 96 + x ) + AllTrim( Str( y ) ) + Chr( 96 + x - k ) + AllTrim( Str( y + k ) )
                  EXIT
               ENDIF
               IF SCACCHIERA[ x - k, y + k ] $ "-"    // --------Se la casa e' vuota, continua la ricerca
                  APPEND BLANK
                  REPLACE field->mossa WITH Chr( 96 + x ) + AllTrim( Str( y ) ) + Chr( 96 + x - k ) + AllTrim( Str( y + k ) )
               ENDIF
               IF SCACCHIERA[ x - k, y + k ] $ Upper( "pnbrqk" )    // --------Se c'e' un pezzo bianco, esce
                  EXIT
               ENDIF
            NEXT k
         ENDIF
      NEXT x
   NEXT y


   USE

   RETURN NIL

















// ---------------------------------------------------------------------------
FUNCTION simula_mossa( cMossa )

   LOCAL x, y
   LOCAL nPartenza_x, nPartenza_y, nArrivo_x, nArrivo_y

   // -------------------Clona la scacchiera---------
   cMossa := AllTrim( cMossa )
   FOR y := 8 TO 1 STEP -1
      FOR x := 1 TO 8
         SCACCHIERA2[ x, y ] := SCACCHIERA[ x, y ]
      NEXT x
   NEXT y
   // ----------Simula mosse candidate-----------
   nPartenza_x := Asc( SubStr( cMossa, 1, 1 ) ) -96
   nPartenza_y := Val( SubStr( cMossa, 2, 1 ) )
   nArrivo_x := Asc( SubStr( cMossa, 3, 1 ) ) -96
   nArrivo_y := Val( SubStr( cMossa, 4, 1 ) )
   SCACCHIERA2[ nArrivo_x, nArrivo_y ] := SCACCHIERA2[ nPartenza_x, nPartenza_y ]
   SCACCHIERA2[ nPartenza_x, nPartenza_y ] := "-"
   // ----------Simula Promozione-------------
   IF Len( cMossa ) == 5 .AND. nArrivo_y > nPartenza_y        // Sta promuovendo il Bianco: a7a8
      SCACCHIERA2[ nArrivo_x, nArrivo_y ] := Upper( Right( cMossa, 1 ) )
   ENDIF
   IF Len( cMossa ) == 5 .AND. nArrivo_y < nPartenza_y        // Sta promuovendo il Nero: c2c1
      SCACCHIERA2[ nArrivo_x, nArrivo_y ] := Lower( Right( cMossa, 1 ) )
   ENDIF

   RETURN NIL


















// ---------------------------------------------------------------------------
FUNCTION SalvaSuLog( cStringa )

   LOCAL nHandle
   LOCAL cNomeFile := "acqua.log"

if lLOGFILE == .T.

   IF ! File( cNomeFile )
      nHandle := FCreate( cNomeFile )
      FClose( nHandle )
   ENDIF
   nHandle := FOpen( cNomeFile, 1 )
   FSeek( nHandle, 0, 2 )  // It go to the bottom of the file
   FWrite( nHandle, cStringa )
   FClose( nHandle )
endif
   RETURN NIL

















// ---------------------------------------------------------------------------
FUNCTION RispondiAllaGui( cStringa )

   OutStd( cStringa + LF )
   SalvaSuLog( Space( 20 ) + DToC( Date() ) + Space( 1 ) + Time() + " Engine: >>> " + cStringa + CRLF )

   RETURN NIL








// ---------------------------------------------------------------------------
FUNCTION SceglieMiglioreMossa( cDbfName )

   LOCAL cMossaScelta

   USE ( cDbfName )
   INDEX ON field->voto TO mem:candidate DESCEND
   GO TOP
   cMossaScelta := field->mossa
   USE

   RETURN cMossaScelta

















// ---------------------------------------------------------------------------
FUNCTION CaseControllateDalBianco(  cPezzoMinacciato, nBonus )

   LOCAL x, y, k
   LOCAL CASE_CONTROLLATE[ 8, 8 ]
   LOCAL fSottoScacco
   LOCAL nNumeroPezziNeri, nReX, nReY, nCaseLegaliAttornoAlReNero

   // ------------Azzera matrice---------------
   FOR y = 8 TO 1 STEP -1
      FOR x = 1 TO 8
         CASE_CONTROLLATE[ x, y ] := ""
      NEXT x
   NEXT y

   // --------------SEGNA le case CONTROLLATE-----
   FOR y := 8 TO 1 STEP -1
      FOR x := 1 TO 8
         // -----------------Alfiere o Donna in alto a destra------------
         IF SCACCHIERA2[ x, y ] $ "BQ"
            FOR k = 1 TO 7
               IF ( x + k ) > 8 .OR. ( y + k ) > 8
                  EXIT
               ENDIF
               CASE_CONTROLLATE[ x + k, y + k  ] += SCACCHIERA2[ x, y ]
               IF SCACCHIERA2[ x + k, y + k ] <> "-"
                  EXIT
               ENDIF
            NEXT k
         ENDIF
         // -----------------Alfiere o Donna in basso a destra------------
         IF SCACCHIERA2[ x, y ] $ "BQ"
            FOR k = 1 TO 7
               IF ( x + k ) > 8 .OR. ( y - k ) < 1
                  EXIT
               ENDIF
               CASE_CONTROLLATE[ x + k, y - k ] += SCACCHIERA2[ x, y ]
               IF SCACCHIERA2[ x + k, y - k ] <> "-"
                  EXIT
               ENDIF
            NEXT k
         ENDIF
         // -----------------Alfiere o Donna in alto a sinistra------------
         IF SCACCHIERA2[ x, y ] $ "BQ"
            FOR k = 1 TO 7
               IF ( x - k ) < 1 .OR. ( y + k ) > 8
                  EXIT
               ENDIF
               CASE_CONTROLLATE[  x - k, y + k ] += SCACCHIERA2[ x, y ]
               IF SCACCHIERA2[ x - k, y + k ] <> "-"
                  EXIT
               ENDIF
            NEXT k
         ENDIF
         // -----------------Alfiere o Donna in basso a sinistra------------
         IF SCACCHIERA2[ x, y ] $ "BQ"
            FOR k = 1 TO 7
               IF ( x - k ) < 1 .OR. ( y - k ) < 1
                  EXIT
               ENDIF
               CASE_CONTROLLATE[ x - k, y - k ] += SCACCHIERA2[ x, y ]
               IF SCACCHIERA2[ x - k, y - k ] <> "-"
                  EXIT
               ENDIF
            NEXT k
         ENDIF
         // -----------------Torre o Donna a destra------------
         IF SCACCHIERA2[ x, y ] $ "RQ"
            FOR k = 1 TO 7
               IF ( x + k ) > 8
                  EXIT
               ENDIF
               CASE_CONTROLLATE[ x + k, y  ] += SCACCHIERA2[ x, y ]
               IF SCACCHIERA2[ x + k, y  ] <> "-"
                  EXIT
               ENDIF
            NEXT k
         ENDIF
         // -----------------Torre o Donna a sinistra------------
         IF SCACCHIERA2[ x, y ] $ "RQ"
            FOR k = 1 TO 7
               IF ( x - k ) < 1
                  EXIT
               ENDIF
               CASE_CONTROLLATE[ x - k, y ] += SCACCHIERA2[ x, y ]
               IF SCACCHIERA2[ x - k, y  ] <> "-"
                  EXIT
               ENDIF
            NEXT k
         ENDIF
         // -----------------Torre o Donna in alto------------
         IF SCACCHIERA2[ x, y ] $ "RQ"
            FOR k = 1 TO 7
               IF ( y + k ) > 8
                  EXIT
               ENDIF
               CASE_CONTROLLATE[  x, y + k   ] += SCACCHIERA2[ x, y ]
               IF SCACCHIERA2[ x, y + k  ] <> "-"
                  EXIT
               ENDIF
            NEXT k
         ENDIF
         // -----------------Torre o Donna in basso------------
         IF SCACCHIERA2[ x, y ] $ "RQ"
            FOR k = 1 TO 7
               IF ( y - k ) < 1
                  EXIT
               ENDIF
               CASE_CONTROLLATE[ x, y - k ] += SCACCHIERA2[ x, y ]
               IF SCACCHIERA2[ x, y - k  ] <> "-"
                  EXIT
               ENDIF
            NEXT k
         ENDIF
         // -----------------Cavallo------------
         IF SCACCHIERA2[ x, y ] $ "N"
            IF ( x + 1 ) <= 8 .AND. ( y + 2 ) <= 8
               CASE_CONTROLLATE[ x + 1, y + 2  ] += "N"
            ENDIF
            IF ( x + 2 ) <= 8 .AND. ( y + 1 ) <= 8
               CASE_CONTROLLATE[ x + 2, y + 1  ] += "N"
            ENDIF
            IF ( x + 2 ) <= 8 .AND. ( y - 1 ) >= 1
               CASE_CONTROLLATE[ x + 2, y - 1  ] += "N"
            ENDIF
            IF ( x + 1 ) <= 8 .AND. ( y - 2 ) >= 1
               CASE_CONTROLLATE[ x + 1, y - 2  ] += "N"
            ENDIF
            IF ( x - 1 ) >= 1 .AND. ( y - 2 ) >= 1
               CASE_CONTROLLATE[ x - 1, y - 2  ] += "N"
            ENDIF
            IF ( x - 2 ) >= 1 .AND. ( y - 1 ) >= 1
               CASE_CONTROLLATE[ x - 2, y - 1  ] += "N"
            ENDIF
            IF ( x - 2 ) >= 1 .AND. ( y + 1 ) <= 8
               CASE_CONTROLLATE[ x - 2, y + 1  ] += "N"
            ENDIF
            IF ( x - 1 ) >= 1 .AND. ( y + 2 ) <= 8
               CASE_CONTROLLATE[ x - 1, y + 2  ] += "N"
            ENDIF
         ENDIF
         // -----------------Pedone------------
         IF SCACCHIERA2[ x, y ] $ "P"
            IF ( x - 1 ) >= 1 .AND. ( y + 1 ) <= 8
               CASE_CONTROLLATE[ x - 1, y + 1  ] += "P"
            ENDIF
            IF ( x + 1 ) <= 8 .AND. ( y + 1 ) <= 8
               CASE_CONTROLLATE[ x + 1, y + 1  ] += "P"
            ENDIF
         ENDIF
         // -----------------RE-----------
         IF SCACCHIERA2[ x, y ] $ "K"
            IF ( x + 1 ) <= 8 .AND. ( y + 1 ) <= 8
               CASE_CONTROLLATE[ x + 1, y + 1  ] += "K"
            ENDIF
            IF ( x + 1 ) <= 8
               CASE_CONTROLLATE[ x + 1, y  ] += "K"
            ENDIF
            IF ( x + 1 ) <= 8 .AND. ( y - 1 ) >= 1
               CASE_CONTROLLATE[ x + 1, y - 1  ] += "K"
            ENDIF
            IF ( y - 1 ) >= 1
               CASE_CONTROLLATE[ x, y - 1  ] += "K"
            ENDIF
            IF ( x - 1 ) >= 1 .AND. ( y - 1 ) >= 1
               CASE_CONTROLLATE[ x - 1, y - 1  ] += "K"
            ENDIF
            IF ( x - 1 ) >= 1
               CASE_CONTROLLATE[ x - 1, y  ] += "K"
            ENDIF
            IF ( x - 1 ) >= 1 .AND. ( y + 1 ) <= 8
               CASE_CONTROLLATE[ x - 1, y + 1  ] += "K"
            ENDIF
            IF ( y + 1 ) <= 8
               CASE_CONTROLLATE[ x, y + 1  ] += "K"
            ENDIF
         ENDIF
      NEXT x
   NEXT y

   // ----------------------Se il pezzo NERO e' sotto scacco----------------
   fSottoScacco := .F.
   FOR y := 8 TO 1 STEP -1
      FOR x := 1 TO 8
         IF SCACCHIERA2[ x, y ] == cPezzoMinacciato .AND. Len( CASE_CONTROLLATE[ x, y ] ) > 0
            fSottoScacco := .T.
         ENDIF
      NEXT x
   NEXT y
   IF fSottoScacco
      REPLACE field->voto WITH field->voto + nBonus
   ENDIF





   // ----------------------Cerca MATTO del Bianco in 1 mossa----------------
   IF cPezzoMinacciato == "matto"
      // ----Conta pezzi Neri------
      nNumeroPezziNeri := 0
      FOR y := 8 TO 1 STEP -1
         FOR x := 1 TO 8
            IF SCACCHIERA2[ x, y ] $ "pnbrqk"
               nNumeroPezziNeri++
            ENDIF
         NEXT x
      NEXT y

      // ----- Cerca il RE Nero -------
      FOR y := 8 TO 1 STEP -1
         FOR x := 1 TO 8
            IF SCACCHIERA2[ x, y ] == "k"
               nReX := x
               nReY := y
            ENDIF
         NEXT x
      NEXT y
      // ----- Controlla TUTTE le 8 case LEGALMENTE accessibili intorno al Re Nero -------
      nCaseLegaliAttornoAlReNero := 0
      FOR y := 1 TO -1 STEP -1
         FOR x := -1 TO 1
            // ------ Conta le case LEGALMENTE ACCESSIBILI attorno al Re Nero (senza uscire dai bordi)
            IF AllTrim( Str( y ) ) + AllTrim( Str( x ) ) <> "00" .AND.  nReY + y >= 1 .AND.  nReY + y <= 8 .AND. nReX + x <= 8 .AND. nReX + x >= 1   // Se non sborda fuori
               IF SCACCHIERA2[ nReX + x, nReY + y ] $ "-PNBRQK" .AND. Len( CASE_CONTROLLATE[ nReX + x, nReY + y ] ) == 0
                  nCaseLegaliAttornoAlReNero++
               ENDIF
            ENDIF
         NEXT x
      NEXT y

      // ----------------------Se il Re Nero e' sotto scacco----------------
      fSottoScacco := .F.
      FOR y := 8 TO 1 STEP -1
         FOR x := 1 TO 8
            IF SCACCHIERA2[ x, y ] == "k" .AND. Len( CASE_CONTROLLATE[ x, y ] ) > 0
               fSottoScacco := .T.
            ENDIF
         NEXT x
      NEXT y


      // --------Se il pezzo avversario e' UNO SOLO, le case legali del Re sono 0 e sottoscacco .T.  = SCACCOMATTO
      IF nNumeroPezziNeri == 1 .AND. nCaseLegaliAttornoAlReNero == 0 .AND. fSottoScacco == .T.
         REPLACE field->voto WITH field->voto + 3333
      ENDIF
      // --------Se il pezzo avversario e' UNO SOLO, le case legali del Re sono 0 e sottoscacco .F.  = STALLO
      IF nNumeroPezziNeri == 1 .AND. nCaseLegaliAttornoAlReNero == 0 .AND. fSottoScacco == .F.
         REPLACE field->voto WITH field->voto -10
      ENDIF





   ENDIF

   RETURN NIL












// ---------------------------------------------------------------------------
FUNCTION CaseControllateDalNero( cPezzoMinacciato, nBonus )

   LOCAL x, y, k
   LOCAL CASE_CONTROLLATE[ 8, 8 ]
   LOCAL fSottoScacco
   LOCAL nNumeroPezziBianchi, nReX, nReY, nCaseLegaliAttornoAlReBianco

   // ------------Azzera matrice---------------
   FOR y = 8 TO 1 STEP -1
      FOR x = 1 TO 8
         CASE_CONTROLLATE[ x, y ] := ""
      NEXT x
   NEXT y

   // --------------SEGNA le case CONTROLLATE-----
   FOR y := 8 TO 1 STEP -1
      FOR x := 1 TO 8
         // -----------------Alfiere o Donna in alto a destra------------
         IF SCACCHIERA2[ x, y ] $ "bq"
            FOR k = 1 TO 7
               IF ( x + k ) > 8 .OR. ( y + k ) > 8
                  EXIT
               ENDIF
               CASE_CONTROLLATE[ x + k, y + k  ] += SCACCHIERA2[ x, y ]
               IF SCACCHIERA2[ x + k, y + k ] <> "-"
                  EXIT
               ENDIF
            NEXT k
         ENDIF
         // -----------------Alfiere o Donna in basso a destra------------
         IF SCACCHIERA2[ x, y ] $ "bq"
            FOR k = 1 TO 7
               IF ( x + k ) > 8 .OR. ( y - k ) < 1
                  EXIT
               ENDIF
               CASE_CONTROLLATE[ x + k, y - k ] += SCACCHIERA2[ x, y ]
               IF SCACCHIERA2[ x + k, y - k ] <> "-"
                  EXIT
               ENDIF
            NEXT k
         ENDIF
         // -----------------Alfiere o Donna in alto a sinistra------------
         IF SCACCHIERA2[ x, y ] $ "bq"
            FOR k = 1 TO 7
               IF ( x - k ) < 1 .OR. ( y + k ) > 8
                  EXIT
               ENDIF
               CASE_CONTROLLATE[  x - k, y + k ] += SCACCHIERA2[ x, y ]
               IF SCACCHIERA2[ x - k, y + k ] <> "-"
                  EXIT
               ENDIF
            NEXT k
         ENDIF
         // -----------------Alfiere o Donna in basso a sinistra------------
         IF SCACCHIERA2[ x, y ] $ "bq"
            FOR k = 1 TO 7
               IF ( x - k ) < 1 .OR. ( y - k ) < 1
                  EXIT
               ENDIF
               CASE_CONTROLLATE[ x - k, y - k ] += SCACCHIERA2[ x, y ]
               IF SCACCHIERA2[ x - k, y - k ] <> "-"
                  EXIT
               ENDIF
            NEXT k
         ENDIF
         // -----------------Torre o Donna a destra------------
         IF SCACCHIERA2[ x, y ] $ "rq"
            FOR k = 1 TO 7
               IF ( x + k ) > 8
                  EXIT
               ENDIF
               CASE_CONTROLLATE[ x + k, y  ] += SCACCHIERA2[ x, y ]
               IF SCACCHIERA2[ x + k, y  ] <> "-"
                  EXIT
               ENDIF
            NEXT k
         ENDIF
         // -----------------Torre o Donna a sinistra------------
         IF SCACCHIERA2[ x, y ] $ "rq"
            FOR k = 1 TO 7
               IF ( x - k ) < 1
                  EXIT
               ENDIF
               CASE_CONTROLLATE[ x - k, y ] += SCACCHIERA2[ x, y ]
               IF SCACCHIERA2[ x - k, y  ] <> "-"
                  EXIT
               ENDIF
            NEXT k
         ENDIF
         // -----------------Torre o Donna in alto------------
         IF SCACCHIERA2[ x, y ] $ "rq"
            FOR k = 1 TO 7
               IF ( y + k ) > 8
                  EXIT
               ENDIF
               CASE_CONTROLLATE[  x, y + k   ] += SCACCHIERA2[ x, y ]
               IF SCACCHIERA2[ x, y + k  ] <> "-"
                  EXIT
               ENDIF
            NEXT k
         ENDIF
         // -----------------Torre o Donna in basso------------
         IF SCACCHIERA2[ x, y ] $ "rq"
            FOR k = 1 TO 7
               IF ( y - k ) < 1
                  EXIT
               ENDIF
               CASE_CONTROLLATE[ x, y - k ] += SCACCHIERA2[ x, y ]
               IF SCACCHIERA2[ x, y - k  ] <> "-"
                  EXIT
               ENDIF
            NEXT k
         ENDIF
         // -----------------Cavallo------------
         IF SCACCHIERA2[ x, y ] $ "n"
            IF ( x + 1 ) <= 8 .AND. ( y + 2 ) <= 8
               CASE_CONTROLLATE[ x + 1, y + 2  ] += "n"
            ENDIF
            IF ( x + 2 ) <= 8 .AND. ( y + 1 ) <= 8
               CASE_CONTROLLATE[ x + 2, y + 1  ] += "n"
            ENDIF
            IF ( x + 2 ) <= 8 .AND. ( y - 1 ) >= 1
               CASE_CONTROLLATE[ x + 2, y - 1  ] += "n"
            ENDIF
            IF ( x + 1 ) <= 8 .AND. ( y - 2 ) >= 1
               CASE_CONTROLLATE[ x + 1, y - 2  ] += "n"
            ENDIF
            IF ( x - 1 ) >= 1 .AND. ( y - 2 ) >= 1
               CASE_CONTROLLATE[ x - 1, y - 2  ] += "n"
            ENDIF
            IF ( x - 2 ) >= 1 .AND. ( y - 1 ) >= 1
               CASE_CONTROLLATE[ x - 2, y - 1  ] += "n"
            ENDIF
            IF ( x - 2 ) >= 1 .AND. ( y + 1 ) <= 8
               CASE_CONTROLLATE[ x - 2, y + 1  ] += "n"
            ENDIF
            IF ( x - 1 ) >= 1 .AND. ( y + 2 ) <= 8
               CASE_CONTROLLATE[ x - 1, y + 2  ] += "n"
            ENDIF
         ENDIF
         // -----------------Pedone------------
         IF SCACCHIERA2[ x, y ] $ "p"
            IF ( x - 1 ) >= 1 .AND. ( y - 1 ) >= 1
               CASE_CONTROLLATE[ x - 1, y - 1  ] += "p"
            ENDIF
            IF ( x + 1 ) <= 8 .AND. ( y - 1 ) >= 1
               CASE_CONTROLLATE[ x + 1, y - 1  ] += "p"
            ENDIF
         ENDIF
         // -----------------RE-----------
         IF SCACCHIERA2[ x, y ] $ "k"
            IF ( x + 1 ) <= 8 .AND. ( y + 1 ) <= 8
               CASE_CONTROLLATE[ x + 1, y + 1  ] += "k"
            ENDIF
            IF ( x + 1 ) <= 8
               CASE_CONTROLLATE[ x + 1, y  ] += "k"
            ENDIF
            IF ( x + 1 ) <= 8 .AND. ( y - 1 ) >= 1
               CASE_CONTROLLATE[ x + 1, y - 1  ] += "k"
            ENDIF
            IF ( y - 1 ) >= 1
               CASE_CONTROLLATE[ x, y - 1  ] += "k"
            ENDIF
            IF ( x - 1 ) >= 1 .AND. ( y - 1 ) >= 1
               CASE_CONTROLLATE[ x - 1, y - 1  ] += "k"
            ENDIF
            IF ( x - 1 ) >= 1
               CASE_CONTROLLATE[ x - 1, y  ] += "k"
            ENDIF
            IF ( x - 1 ) >= 1 .AND. ( y + 1 ) <= 8
               CASE_CONTROLLATE[ x - 1, y + 1  ] += "k"
            ENDIF
            IF ( y + 1 ) <= 8
               CASE_CONTROLLATE[ x, y + 1  ] += "k"
            ENDIF
         ENDIF
      NEXT x
   NEXT y

   // ----------------------Se il pezzo Bianco e' sotto scacco----------------
   fSottoScacco := .F.
   FOR y := 8 TO 1 STEP -1
      FOR x := 1 TO 8
         IF SCACCHIERA2[ x, y ] == cPezzoMinacciato .AND. Len( CASE_CONTROLLATE[ x, y ] ) > 0
            fSottoScacco := .T.
         ENDIF
      NEXT x
   NEXT y

   IF fSottoScacco
      REPLACE field->voto WITH field->voto + nBonus
   ENDIF

   // ----------------------Cerca MATTO del Nero in 1 mossa----------------
   IF cPezzoMinacciato == "matto"
      // ----Conta pezzi Bianchi------
      nNumeroPezziBianchi := 0
      FOR y := 8 TO 1 STEP -1
         FOR x := 1 TO 8
            IF SCACCHIERA2[ x, y ] $ "PNBRQK"
               nNumeroPezziBianchi++
            ENDIF
         NEXT x
      NEXT y

      // ----- Cerca il RE Bianco -------
      FOR y := 8 TO 1 STEP -1
         FOR x := 1 TO 8
            IF SCACCHIERA2[ x, y ] == "K"
               nReX := x
               nReY := y
            ENDIF
         NEXT x
      NEXT y
      // ----- Controlla TUTTE le 8 case LEGALMENTE accessibili intorno al Re Bianco -------
      nCaseLegaliAttornoAlReBianco := 0
      FOR y := 1 TO -1 STEP -1
         FOR x := -1 TO 1
            // ------ Conta le case LEGALMENTE ACCESSIBILI attorno al Re Bianco (senza uscire dai bordi)
            IF AllTrim( Str( y ) ) + AllTrim( Str( x ) ) <> "00" .AND.  nReY + y >= 1 .AND.  nReY + y <= 8 .AND. nReX + x <= 8 .AND. nReX + x >= 1   // Se non sborda fuori
               IF SCACCHIERA2[ nReX + x, nReY + y ] $ "-pnbrqk" .AND. Len( CASE_CONTROLLATE[ nReX + x, nReY + y ] ) == 0
                  nCaseLegaliAttornoAlReBianco++
               ENDIF
            ENDIF
         NEXT x
      NEXT y
      // ----------------------Se il Re Bianco e' sotto scacco----------------
      fSottoScacco := .F.
      FOR y := 8 TO 1 STEP -1
         FOR x := 1 TO 8
            IF SCACCHIERA2[ x, y ] == "K" .AND. Len( CASE_CONTROLLATE[ x, y ] ) > 0
               fSottoScacco := .T.
            ENDIF
         NEXT x
      NEXT y
      // --------Se il pezzo avversario e' UNO SOLO, le case legali del Re sono 0 e sottoscacco .T.  = SCACCOMATTO
      IF nNumeroPezziBianchi == 1 .AND. nCaseLegaliAttornoAlReBianco == 0 .AND. fSottoScacco == .T.
         REPLACE field->voto WITH field->voto + 3333
      ENDIF
      // --------Se il pezzo avversario e' UNO SOLO, le case legali del Re sono 0 e sottoscacco .F.  = STALLO
      IF nNumeroPezziBianchi == 1 .AND. nCaseLegaliAttornoAlReBianco == 0 .AND. fSottoScacco == .F.
         REPLACE field->voto WITH field->voto -10
      ENDIF





   ENDIF

   RETURN NIL













// ---------------------------------------------------------------------------
FUNCTION BonusPezzoCheCattura( cMossa )

   LOCAL nArrivo_x, nArrivo_y

   nArrivo_x := Asc( SubStr( cMossa, 3, 1 ) ) -96
   nArrivo_y := Val( SubStr( cMossa, 4, 1 ) )

   IF SCACCHIERA[ nArrivo_x, nArrivo_y ] $ "qQ"
      REPLACE field->voto WITH field->voto + MACRO_VALORE_DONNA
   ENDIF
   IF SCACCHIERA[ nArrivo_x, nArrivo_y ] $ "rR"
      REPLACE field->voto WITH field->voto + MACRO_VALORE_TORRE
   ENDIF
   IF SCACCHIERA[ nArrivo_x, nArrivo_y ] $ "bB"
      REPLACE field->voto WITH field->voto + MACRO_VALORE_ALFIERE
   ENDIF
   IF SCACCHIERA[ nArrivo_x, nArrivo_y ] $ "nN"
      REPLACE field->voto WITH field->voto + MACRO_VALORE_CAVALLO
   ENDIF
   IF SCACCHIERA[ nArrivo_x, nArrivo_y ] $ "pP"
      REPLACE field->voto WITH field->voto + MACRO_VALORE_PEDONE
   ENDIF

   RETURN NIL















// ---------------------------------------------------------------------------
FUNCTION BonusPerStradaPercorsaDallaMossa( cMossa )

   LOCAL nPartenza_x, nPartenza_y, nArrivo_x, nArrivo_y

   nPartenza_x := Asc( SubStr( cMossa, 1, 1 ) ) -96
   nPartenza_y := Val( SubStr( cMossa, 2, 1 ) )
   nArrivo_x := Asc( SubStr( cMossa, 3, 1 ) ) -96
   nArrivo_y := Val( SubStr( cMossa, 4, 1 ) )

   REPLACE field->voto WITH field->voto + fDistanzaTraDueCase( nPartenza_x, nPartenza_y, nArrivo_x, nArrivo_y ) / 28

   RETURN NIL











// ---------------------------------------------------------------------------
FUNCTION BonusValorizzaMossaPezziPiuDistantiDalReAvversario( cMossa, cReAvversario )

   LOCAL nPartenza_x, nPartenza_y
   LOCAL x, y
   LOCAL nReX, nReY

   nPartenza_x := Asc( SubStr( cMossa, 1, 1 ) ) -96
   nPartenza_y := Val( SubStr( cMossa, 2, 1 ) )
   // --------------Cerca RE avversario-------
   FOR y := 8 TO 1 STEP -1
      FOR x := 1 TO 8
         IF SCACCHIERA2[ x, y ] == cReAvversario
            nReX := x
            nReY := y
         ENDIF
      NEXT x
   NEXT y

   REPLACE field->voto WITH field->voto + fDistanzaTraDueCase( nPartenza_x, nPartenza_y, nReX, nReY ) / 28

   RETURN NIL










// ---------------------------------------------------------------------------
FUNCTION BonusValorizzaMosseCheSiAvvicinanoAlReAvversario( cMossa, cReAvversario )

   LOCAL nArrivo_x, nArrivo_y
   LOCAL x, y
   LOCAL nReX, nReY

   nArrivo_x := Asc( SubStr( cMossa, 3, 1 ) ) -96
   nArrivo_y := Val( SubStr( cMossa, 4, 1 ) )
   // --------------Cerca RE avversario-------
   FOR y := 8 TO 1 STEP -1
      FOR x := 1 TO 8
         IF SCACCHIERA2[ x, y ] == cReAvversario
            nReX := x
            nReY := y
         ENDIF
      NEXT x
   NEXT y
   REPLACE field->voto WITH field->voto - fDistanzaTraDueCase( nArrivo_x, nArrivo_y, nReX, nReY ) / 28

   RETURN NIL









// ---------------------------------------------------------------------------
FUNCTION BonusPerChiPromuove( cMossa, cPezzoPromosso, nBonus )

   IF Len( cMossa ) == 5 .AND. Right( cMossa, 1 ) == cPezzoPromosso
      REPLACE field->voto WITH field->voto + nBonus
   ENDIF

   RETURN NIL












// ---------------------------------------------------------------------------
FUNCTION BonusAvanzamentoPedone( cMossa )

   LOCAL nPartenza_x, nPartenza_y
   LOCAL anValoriDiAvanzamento := { 0.8, 0.9, 1.0, 1.1, 1.2, 1.3 }

   nPartenza_x := Asc( SubStr( cMossa, 1, 1 ) ) -96
   nPartenza_y := Val( SubStr( cMossa, 2, 1 ) )
   // --------------Pedoni NERI------------
   IF SCACCHIERA[ nPartenza_x, nPartenza_y ] == "p" .AND. nPartenza_y == 7
      REPLACE field->voto WITH field->voto + anValoriDiAvanzamento[ 1 ]
   ENDIF
   IF SCACCHIERA[ nPartenza_x, nPartenza_y ] == "p" .AND. nPartenza_y == 6
      REPLACE field->voto WITH field->voto + anValoriDiAvanzamento[ 2 ]
   ENDIF
   IF SCACCHIERA[ nPartenza_x, nPartenza_y ] == "p" .AND. nPartenza_y == 5
      REPLACE field->voto WITH field->voto + anValoriDiAvanzamento[ 3 ]
   ENDIF
   IF SCACCHIERA[ nPartenza_x, nPartenza_y ] == "p" .AND. nPartenza_y == 4
      REPLACE field->voto WITH field->voto + anValoriDiAvanzamento[ 4 ]
   ENDIF
   IF SCACCHIERA[ nPartenza_x, nPartenza_y ] == "p" .AND. nPartenza_y == 3
      REPLACE field->voto WITH field->voto + anValoriDiAvanzamento[ 5 ]
   ENDIF
   IF SCACCHIERA[ nPartenza_x, nPartenza_y ] == "p" .AND. nPartenza_y == 2
      REPLACE field->voto WITH field->voto + anValoriDiAvanzamento[ 6 ]
   ENDIF
   // --------------Pedoni BIANCHI------------
   IF SCACCHIERA[ nPartenza_x, nPartenza_y ] == "P" .AND. nPartenza_y == 2
      REPLACE field->voto WITH field->voto + anValoriDiAvanzamento[ 1 ]
   ENDIF
   IF SCACCHIERA[ nPartenza_x, nPartenza_y ] == "P" .AND. nPartenza_y == 3
      REPLACE field->voto WITH field->voto + anValoriDiAvanzamento[ 2 ]
   ENDIF
   IF SCACCHIERA[ nPartenza_x, nPartenza_y ] == "P" .AND. nPartenza_y == 4
      REPLACE field->voto WITH field->voto + anValoriDiAvanzamento[ 3 ]
   ENDIF
   IF SCACCHIERA[ nPartenza_x, nPartenza_y ] == "P" .AND. nPartenza_y == 5
      REPLACE field->voto WITH field->voto + anValoriDiAvanzamento[ 4 ]
   ENDIF
   IF SCACCHIERA[ nPartenza_x, nPartenza_y ] == "P" .AND. nPartenza_y == 6
      REPLACE field->voto WITH field->voto + anValoriDiAvanzamento[ 5 ]
   ENDIF
   IF SCACCHIERA[ nPartenza_x, nPartenza_y ] == "P" .AND. nPartenza_y == 7
      REPLACE field->voto WITH field->voto + anValoriDiAvanzamento[ 6 ]
   ENDIF

   RETURN NIL










// ---------------------------------------------------------------------------
FUNCTION BonusSeMuoveUnPezzo( cMossa, cPezzo, nDalTratto, nAlTratto, nBonus )

   LOCAL nPartenza_x, nPartenza_y

   nPartenza_x := Asc( SubStr( cMossa, 1, 1 ) ) -96
   nPartenza_y := Val( SubStr( cMossa, 2, 1 ) )

   IF SCACCHIERA[ nPartenza_x, nPartenza_y ] == cPezzo .AND. TRATTO >= nDalTratto .AND. TRATTO <= nAlTratto
      REPLACE field->voto WITH field->voto + nBonus
   ENDIF

   RETURN NIL












// ---------------------------------------------------------------------------
FUNCTION MalusEvitaPattaTripliceRipetizione()

   LOCAL x, y
   LOCAL cStringa, nMalus
   LOCAL nIndicePosizioni

   cStringa := ""
   FOR y := 8 TO 1 STEP -1
      FOR x := 1 TO 8
         cStringa += SCACCHIERA2[ x, y ]
      NEXT x
   NEXT y

   nIndicePosizioni := AScan( acPOSIZIONI_SCACCHIERA, cStringa )

   IF nIndicePosizioni > 0 .AND. anPOSIZIONI_SCACCHIERA_QT[ nIndicePosizioni ] == 1 // Con "2" l'avversario causava la triplice ripetizione
      nMalus := -10
   ELSE
      nMalus := 0
   ENDIF

   REPLACE field->voto WITH field->voto + nMalus

   RETURN NIL











// ---------------------------------------------------------------------------
FUNCTION BonusMossa( cMossa, cMossaDaProcessare, cPezzo, nDalTratto, nAlTratto, nBonus )

   LOCAL nPartenza_x, nPartenza_y

   nPartenza_x := Asc( SubStr( cMossa, 1, 1 ) ) -96
   nPartenza_y := Val( SubStr( cMossa, 2, 1 ) )

   IF  cMossa == cMossaDaProcessare .AND. SCACCHIERA[ nPartenza_x, nPartenza_y ] == cPezzo  .AND. TRATTO >= nDalTratto .AND. TRATTO <= nAlTratto
      REPLACE field->voto WITH field->voto + nBonus
   ENDIF

   RETURN NIL










// ---------------------------------------------------------------------------
FUNCTION TorreConquistaColonnaAperta( cQualeTorre, nRiga, nBonus )

   LOCAL x,  t
   LOCAL fColonnaAperta

   // --------------Cerca Torre-------
   FOR x := 1 TO 8
      IF SCACCHIERA2[ x, nRiga ] == cQualeTorre
         fColonnaAperta := .T.
         FOR t := 8 TO 1 STEP -1
            IF t <> nRiga .AND. SCACCHIERA2[ x, t ] <> "-"   // Ovviamente NON considera la torre stessa
               fColonnaAperta := .F.
            ENDIF
         NEXT t
         IF fColonnaAperta
            REPLACE field->voto WITH field->voto + nBonus
         ENDIF
      ENDIF
   NEXT x

   RETURN NIL







// ---------------------------------------------------------------------------
FUNCTION RaddoppioDiTorre( cQualeTorre, nBonus )

   LOCAL x, y
   LOCAL  nQtTorri, nQtVuote, lFlag

   // --------------Cerca Torre-------
   lFlag := .F.
   FOR x := 1 TO 8
      nQtTorri := 0
      nQtVuote := 0
      FOR y := 8 TO 1 STEP -1
         IF SCACCHIERA2[ x, y ] == cQualeTorre
            nQtTorri++
         ENDIF
         IF SCACCHIERA2[ x, y ] == "-"
            nQtVuote++
         ENDIF
      NEXT y
      IF nQtTorri == 2 .AND. nQtVuote == 6
         lFlag := .T.
      ENDIF
   NEXT x
   // ----Se e' stata trovato un raddoppio di torri su colonna vuote, da' il bonus----
   IF lFlag
      REPLACE field->voto WITH field->voto + nBonus
   ENDIF

   RETURN NIL






// ---------------------------------------------------------------------------
FUNCTION DoppioPedone( cQualePedone, nBonus )

   LOCAL x, y
   LOCAL nQtPedoni, nColonneConDoppiPedoni

   // --------------Cerca Doppi pedoni-------
   nColonneConDoppiPedoni := 0
   FOR x := 1 TO 8
      nQtPedoni := 0
      FOR y := 8 TO 1 STEP -1
         IF SCACCHIERA2[ x, y ] == cQualePedone
            nQtPedoni++
         ENDIF
      NEXT y
      IF nQtPedoni == 2
         nColonneConDoppiPedoni++
      ENDIF
   NEXT x
   REPLACE field->voto WITH field->voto + nBonus * nColonneConDoppiPedoni

   RETURN NIL







// ---------------------------------------------------------------------------
FUNCTION TriploPedone( cQualePedone, nBonus )

   LOCAL x, y
   LOCAL nQtPedoni, nColonneConDoppiPedoni

   // --------------Cerca tripli pedoni-------
   nColonneConDoppiPedoni := 0
   FOR x := 1 TO 8
      nQtPedoni := 0
      FOR y := 8 TO 1 STEP -1
         IF SCACCHIERA2[ x, y ] == cQualePedone
            nQtPedoni++
         ENDIF
      NEXT y
      IF nQtPedoni == 3
         nColonneConDoppiPedoni++
      ENDIF
   NEXT x
   REPLACE field->voto WITH field->voto + nBonus * nColonneConDoppiPedoni

   RETURN NIL









// ---------------------------------------------------------------------------
FUNCTION BonusTorreInUnaRiga( cMossa, cQualeTorre, nRiga, nBonus )

   LOCAL nArrivo_x, nArrivo_y

   nArrivo_x := Asc( SubStr( cMossa, 3, 1 ) ) -96
   nArrivo_y := Val( SubStr( cMossa, 4, 1 ) )

   IF SCACCHIERA2[ nArrivo_x, nArrivo_y ] == cQualeTorre .AND. nArrivo_y == nRiga
      REPLACE field->voto WITH field->voto + nBonus
   ENDIF

   RETURN NIL




















// ---------------------------------------------------------------------------
FUNCTION ScacchieraDaFen( cStringa )

   LOCAL cFen, cFenScacchiera, cCarattere
   LOCAL k, x, y
   LOCAL nPosizioneSpazio, aChiTocca
   LOCAL nIndicePosizioni

   cFen := AllTrim( SubStr( cStringa, At( "fen", cStringa ) + 3 ) )
   cFenScacchiera := ""

   // ------Costruisce scacchiera LINEARE secondo la stringa FEN------
   FOR k = 1 TO Len( cFen )
      cCarattere := SubStr( cFen, k, 1 )
      IF cCarattere >= "1" .AND. cCarattere <= "8"
         cFenScacchiera += Replicate( "-", Val( cCarattere ) )
      ENDIF
      IF cCarattere >= "a" .AND. cCarattere <= "z"
         cFenScacchiera += cCarattere
      ENDIF
      IF cCarattere >= "A" .AND. cCarattere <= "Z"
         cFenScacchiera += cCarattere
      ENDIF
      IF cCarattere == Space( 1 )
         EXIT
      ENDIF
   NEXT k

   // ------Dispone i pezzi sulla scacchiera secondo la stringa LINEARE------
   k := 0
   FOR y := 8 TO 1 STEP -1
      FOR x := 1 TO 8
         k++
         SCACCHIERA[ x, y ] := SubStr( cFenScacchiera, k, 1 )
      NEXT x
   NEXT y

   // ------Cerca lo spazio per determinare A CHI TOCCA------
   nPosizioneSpazio := At( Space( 1 ), cFen )
   // -------Stabilisce di chi e' il TRATTO --- ("w" = al Bianco, "b" = al Nero)---
   if( SubStr( cFen, nPosizioneSpazio + 1, 1 )  )  == "b"
      aChiTocca = "nero"
   ENDIF
   if( SubStr( cFen, nPosizioneSpazio + 1, 1 )  )  == "w"
      aChiTocca = "bianco"
   ENDIF

   // --------Aggiorna posizioni della scacchiera nell'Array-----
   cStringa := ""
   FOR y := 8 TO 1 STEP -1
      FOR x := 1 TO 8
         cStringa += SCACCHIERA[ x, y ]
      NEXT x
   NEXT y
   nIndicePosizioni := AScan( acPOSIZIONI_SCACCHIERA, cStringa )
   IF nIndicePosizioni == 0
      AAdd( acPOSIZIONI_SCACCHIERA, cStringa )
      AAdd( anPOSIZIONI_SCACCHIERA_QT, 1 )
   ELSE
      anPOSIZIONI_SCACCHIERA_QT[ nIndicePosizioni ]++
   ENDIF

   RETURN aChiTocca



















// ---------------------------------------------------------------------------
FUNCTION BonusOpposizione(cRe1,cRe2,nBonus)
   local x,y
   LOCAL nRe1X, nRe1Y
   LOCAL nRe2X, nRe2Y

   // --------------Cerca i Re------------
   FOR y := 8 TO 1 STEP -1
      FOR x := 1 TO 8
         IF SCACCHIERA2[ x, y ] == cRe1
            nRe1X:=x
            nRe1Y:=y
         ENDIF
         IF SCACCHIERA2[ x, y ] == cRe2
            nRe2X:=x
            nRe2Y:=y
         ENDIF
      NEXT x
   NEXT y

   if abs(nRe1x-nRe2x)==0 .and. abs(nRe1y-nRe2y)==2
      REPLACE field->voto WITH field->voto + nBonus
   endif
   if abs(nRe1x-nRe2x)==2 .and. abs(nRe1y-nRe2y)==0
      REPLACE field->voto WITH field->voto + nBonus
   endif

   RETURN nil





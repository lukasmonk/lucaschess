# -*- coding: latin-1 -*-

import collections
import random

import Code.Util as Util

class ThanksTo():
    def __init__(self):
        d = self.dic = collections.OrderedDict()

        d["Contributors"] = _("Contributors")
        d["Translators"] = _("Translators")
        d["Images"] = _("Images")
        d["Themes"] = _("Themes")
        d["Pieces"] = _("Pieces")
        d["Training"] = _("Training")
        d["Engines-1"] = "%s/1" % _("Engines")
        d["Engines-2"] = "%s/2" % _("Engines")
        d["Games"] = _("Games")
        d["Programming"] = _("Programming")
        d["Dedicated"] = _("Dedicated to")

    def listaMotores(self, bloque):
        li= [
            ["Tarrasch ToyEngine Beta V0.905", "Bill Forster", "http://www.triplehappy.com/"],
            ["Rocinante 2.0", "Antonio Torrecillas", "http://sites.google.com/site/barajandotrebejos/"],
            ["Bikjump 2.01 (32-bit)", "Aart J.C. Bik", "http://www.aartbik.com/"],
            ["Clarabit 1.00", "Salvador Pallares Bejarano", "http://sapabe.googlepages.com"],
            ["Lime v 66", "Richard Allbert", "http://www.geocities.com/taciturn_lemon"],
            ["Chispa 4.0.3", "Federico Corigliano", "http://chispachess.blogspot.com/"],
            ["Gaia 3.5", "Jean-Francois Romang, David Rabel", "http://gaiachess.free.fr"],
            ["Greko 12, 9.8", "Vladimir Medvedev", "http://sourceforge.net/projects/greko/"],
            ["Pawny 0.3.1", "Mincho Georgiev", "http://pawny.netii.net/"],
            ["Umko 0.7", "Borko Boskovic", "http://umko.sourceforge.net/"],
            ["Garbochess 2.20", "Gary Linscott", "http://forwardcoding.com/projects/chess/chess.html"],
            ["Ufim 8.02", "Niyas Khasanov", "http://wbec-ridderkerk.nl/html/details1/Ufim.html"],
            ["Alaric 707", "Peter Fendrich", "http://alaric.fendrich.se/index.html"],
            ["Cyrano 06B17", "Harald Johnsen", "http://sites.estvideo.net/tipunch/cyrano/"],
            ["Daydreamer 1.75 JA", "Aaron Becker", "http://github.com/AaronBecker/daydreamer/downloads"],
            ["Glaurung 2.2 JA", "Tord RomsTad", "http://www.glaurungchess.com/"],
            ["Toga deepTogaNPS 1.9.6", "WHMoweryJr,Thomas Gaksch,Fabien Letouzey", "http://www.computerchess.info/tdbb/phpBB3/viewtopic.php?f=9&t=357"],
            ["Komodo 5.1r2 32-bit", "Don Dailey, Larry Kaufman", "http://komodochess.com/"],
            ["Rybka 2.3.2a 32-bit", "Vasik Rajlich", "http://rybkachess.com/"],
            ["Critter 1.6a 32bits", "Richard Vida", "http://www.vlasak.biz/critter/"],
            ["Stockfish 6", "Tord Romstad, Marco Costalba, Joona Kiiski", "http://stockfishchess.org/"],
            ["Amyan 1.62", "Antonio Dieguez R.", "http://www.pincha.cl/amyan/amyane.html"],
            ["Hamsters 0.5", "Alessandro Scotti", "https://chessprogramming.wikispaces.com/Alessandro+Scotti"],
            ["Cheng 4 ver 0.38", "Martin Sedlák", "http://www.vlasak.biz/cheng"],
            ["Cinnamon 1.2b", "Giuseppe Cannella", "http://cinnamonchess.altervista.org/"],
            ["Fruit 2.3.1", "Fabien Letouzey", "http://www.fruitchess.com/"],
            ["Rhetoric 1.4.1", "Alberto Sanjuan", "http://www.chessrhetoric.com"],
            ["Discocheck 5.2", "Lucas Braesch", "https://github.com/lucasart/"],
            ["Gaviota 1.0", "Miguel A. Ballicora", "https://sites.google.com/site/gaviotachessengine"],
            ["Godel 3.4.9", "Juan Manuel Vazquez", "https://sites.google.com/site/godelchessengine"],
            ["Simplex 0.98", "Antonio Torrecillas", "http://sites.google.com/site/barajandotrebejos/"],
            ["Roce 0.0390", "Roman Hartmann", "http://www.rocechess.ch/rocee.html"],
            ["Texel 0.98", "Peter Österlund", "http://web.comhem.se/petero2home/javachess/index.html#texel"],
            ["Rodent 1.6", "Pawel Koziol", "http://www.pkoziol.cal24.pl/rodent/rodent.htm"],
        ]
        li.sort(key=lambda x: x[0])
        # for n, a, u in li:
            # k = n.split(" ")[0].lower()
            # p rint '                                    ( "%s", "%s", "%s", "%s" ),'%(k,a,n,u)
        if bloque == "1":
            return li[:len(li)/2]
        else:
            return li[len(li)/2:]

    def texto(self, clave):
        if ( "-" in clave ):
            clave, arg = clave.split("-")
            return getattr(self, clave)(arg)
        return getattr(self, clave)()

    def tableIni(self):
        return '<center><table border="1" cellpadding="3" cellspacing="0">'

    def tableEnd(self):
        return '</table>'

    def th(self, txt, mas=""):
        return "<th %s>%s</th>"%(mas,txt)

    def Contributors(self):
        txt = "<br><center><big>%s: <b>Michele Tumbarello</b></big><br>"%_("Chief engineer officer")

        txt += self.tableIni()

        txt += "<tr>"
        txt += self.th(_("Version %s") % "9")
        txt += self.th(_("Version %s") % "8")
        txt += self.th(_("Other versions"))
        txt += "</tr>"

        liV9 = []

        li = [ "Indianajones", "Pavel Rehulka", "ADT", "Adrijan", "Nils Andersson", "Urban Gustavsson",
            "Johannes Reimers", "Red Hood", "Robert Anderson", "Laudecir Daniel", "Reinhard",
            "Di Maria Giovanni", "Filomeno Marmalé", "Max Aloyau",
            # "Chris McFarland",
            # "Myles Turple",
            # "thetasquared",

        ]
        random.shuffle(li)
        li1 = [
            "Chessindia forum",
            "Immortalchess forum",
        ]
        liV9.extend(li)
        liV9.extend(li1)

        liV8 = []

        li = [
            "Indianajones", "James", "Uli", "Pavel Rehulka", "Laudecir Daniel", "Xavier Jimenez",
            "Rajkrishna", "ADT", "Vishy", "thetasquared", "Mike Eddies", "jayriginal", "baddadza",
            "bbbaro25us", "Victor Perez", "M.Larson", "Filomeno Marmalé", "Shahin Jafarli (shahinjy)",
            "Heikki Junes", "Toan Luong", "R. Sehgal", "WyoCas", "J.Reimers", "Dariusz Popadowski",
            "Ken Brown", "Dieter Heinrich", "Nils Andersson", "Chris K.", "Philou",
        ]
        random.shuffle(li)
        li1 = [
            "Chessindia forum",
            "Immortalchess forum",
        ]
        liV8.extend(li)
        liV8.extend(li1)

        liOthers = [
            # "Ezequiel Canario"
        ]
        li2 = [
            "Felicia", "Shahin Jafarli (shahinjy)", "Alfons", "Raúl Giorgi", "Red Hood", "Filomeno Marmalé",
            "Roberto Mizzoni", "bolokay", "Istolacio", "Mohammed Abdalazez", "Rui Grafino", "Georg Pfefferle",
            "Lolo S.", "Joaquín Alvarez", "Ransith Fernando",
        ]
        random.shuffle(li2)
        li3 = [
            "Gianfranco Cutipa", "Daniel Trebejo", "Jose Luis García", "Carmen Martínez", "..."
        ]
        liOthers.extend(li2)
        liOthers.extend(li3)

        txt += "<tr>"

        tam = len(liOthers)

        txt += "<td>"
        txt += '<table border="0" cellpadding="6" cellspacing="0"><tr>'
        for x in range(len(liV9) / tam + 1):
            txt += "<td>"
            for uno in liV9[x * tam:x * tam + tam]:
                txt += "<b>%s</b><br>" % uno
            txt += "</td>"
        txt += '</tr></table>'
        txt += "</td>"

        txt += "<td>"
        txt += '<table border="0" cellpadding="6" cellspacing="0"><tr>'
        for x in range(len(liV8) / tam + 1):
            txt += "<td>"
            for uno in liV8[x * tam:x * tam + tam]:
                txt += "<b>%s</b><br>" % uno
            txt += "</td>"
        txt += '</tr></table>'
        txt += "</td>"

        txt += "<td>"
        txt += '<table border="0" cellpadding="6" cellspacing="0">'
        txt += '<tr>'
        for uno in liOthers:
            txt += "<b>%s</b><br>" % uno
        txt += '</tr></table>'
        txt += "</td>"

        txt += "</tr>"
        txt += self.tableEnd()
        return txt

    def Translators(self):
        dic = {
            _("Portuguese"): ("Rui Grafino", "" ),
            _("French"): ("Max Aloyau", "Lolo S."),
            _("Russian"): ("Reinhard, Vladimir", "Slavik Pavlov"),
            _("German"): ("Alfons", "Georg Pfefferle"),
            _("Italiano"): ("Maurizio Peroni,Michele Tumbarello", ""),
            _("Azeri"): ("Shahin Jafarli (shahinjy)", ""),
            _("Catalan"): ("Salvador Morral i Esteve", ""),
            _("Polish"): ("Miroslaw Kaminski,Dariusz Popadowski", ""),
            _("Spanish"): ("Lucas", ""),
            _("Czech"): ("Jindrich Skula", ""),
            _("Vietnamese"): ("Toan Luong", ""),
            _("Swedish"): ("Nils Andersson", ""),
            _("Finnish"): ("Heikki J.", ""),
            _("Indonesian"): ("Heri Darmanto", ""),
            (_("Portuguese") + " (BR)"): ("Laudecir Daniel",""),
            _("Arabic"): ("Mohamad Alhedhed",""),
            _("Dutch"): ("Willy Lefebvre",""),
            _("Chinese simplified"): ("Kevin Sicong Jiang",""),
            _("Turkish"): ("Mustafa Nazmi ÖZDEMIR",""),
        }
        def r(lng):
            return Util.iniBase8dic("Locale/%s/lang.ini"%lng)["AUTHOR"]
        dic[_("Slovenian")] = (r("si"), "")
        # _("English"):"Lucas, Georg Pfefferle, Lolo S., bolokay",
        li = dic.keys()
        li.sort()
        # random.shuffle(li)
        txt = self.tableIni()

        txt += "<tr>"
        txt += self.th(_("Language"))
        txt += self.th(_("Translator"))
        txt += self.th(_("Previous"))
        txt += "</tr>"
        for uno in li:
            current, previous = dic[uno]
            txt += "<tr>"
            txt += "<th>%s</th><td><b>%s</b></td><td>%s</td>" % ( uno, current, previous )
            txt += "</tr>"
        txt += self.tableEnd()
        return txt

    def Images(self):
        txt = self.tableIni()

        txt += "<tr>"
        txt += "<td></td>"
        txt += self.th(_("Author"))
        txt += self.th(_("Web"))
        txt += self.th(_("License"))
        txt += "</tr>"

        li = [
            ("Nuvola", '<a href="http://icon-king.com/?p=15">David Vignoni</a>',
             'http://www.icon-king.com/projects/nuvola/', "LGPL" ),
            ("Gnome", 'Gnome', "http://svn.gnome.org/viewvc/gnome-icon-theme/", "GPL" ),
            ("Silk icon set 1.3", 'Mark James', 'http://www.famfamfam.com/lab/icons/silk/',
             "Creative Commons Attribution 2.5 License" ),
            ("Wooicons1", 'Janik Baumgartner', 'http://www.woothemes.com/2010/08/woocons1/', "GPL" ),
            ("Ultimate Gnome 0.5.1", 'Marco Tessarotto',
             'http://gnome-look.org/content/show.php/Ultimate+Gnome?content=75000', "GPL" ),
            ("SnowIsh SVG", "Saki", 'http://gnome-look.org/content/show.php/SnowIsh+SVG+%26+PNG?content=32599', "GPL" ),
            ("Cartoon animal icons", "Martin Bérubé", 'http://www.how-to-draw-funny-cartoons.com/',
             "Free for personal non-commercial use" ),
            ("Album of vehicles", "Icons-Land", "http://www.icons-land.com/vista-icons-transport-icon-set.php",
             "Icons-Land Demo License Agreement" ),
            ("Figurines", 'Armando H. Marroquín', 'http://www.enpassant.dk/chess/fonteng.htm', _("Freeware") ),
            ("Icons for Windows8", 'VisualPharm', 'http://www.visualpharm.com/',
             "Creative Commons Attribution-NoDerivs 3.0 Unported" ),
        ]
        for tipo, autor, web, licencia in li:
            txt += "<tr>"
            txt += "<td align=right><b>%s</b></td>" % tipo
            txt += "<td>%s</td>" % autor
            web = '<a href="%s">%s</a>' % (web, web)
            txt += "<td>%s</td>" % web
            txt += "<td>%s</td>" % licencia
            txt += "</tr>"

        txt += "</table></center>"
        return txt

    def Themes(self):
        txt = self.tableIni()

        txt += "<tr>"
        txt += self.th(_("Author"))
        txt += self.th(_("License"))
        txt += "</tr>"

        li = [
            ( "Michele Tumbarello", _("Permission of author") ),
            ( "Felicia", _("Permission of author") ),
            ( "Mohammed Abdalazez", _("Permission of author") ),
            ( "Red Hood", _("Permission of author") ),
        ]
        for autor, licencia in li:
            txt += "<tr>"
            txt += "<td align=center>%s</td>" % autor
            txt += "<td>%s</td>" % licencia
            txt += "</tr>"
        txt += "</table>"

        txt += '<br><table border="1" cellpadding="5" cellspacing="0" >'
        txt += "<tr>"
        txt += self.th(_("Colors"))
        txt += self.th(_("Author"))
        txt += "<th>%s: <a href=\"%s\">%s</a></th>" % (
            _("License"), "http://creativecommons.org/licenses/by-nc-sa/3.0/",
            "Attribution-NonCommercial-ShareAlike 3.0 Unported" )
        txt += "</tr>"

        li = [
            ( "Armani Closet", "mimi22", "http://www.colourlovers.com/palette/117475/armani_closet" ),
            ( "Chocolate Creams", "Skyblue2u", "http://www.colourlovers.com/palette/582195/Chocolate_Creams" ),
            ( "Good Friends", "Yasmino", "http://www.colourlovers.com/palette/77121/Good_Friends" ),
            ( "Nectarius", "note", "http://www.colourlovers.com/palette/1897208/nectarius" ),
            ( "Trajan", "Jaime Guadagni aka The Cooler", "http://www.colourlovers.com/palette/67170/Trajan" )
        ]

        for tipo, autor, web in li:
            txt += "<tr>"
            txt += "<td align=right>%s</td>" % tipo
            txt += "<td align=center>%s</td>" % autor
            txt += '<td><a href="%s">%s</a></td>' % (web, web)
            txt += "</tr>"

        txt += "</table><center>"
        return txt

    def Pieces(self):
        txt = self.tableIni()

        txt += "<tr>"
        txt += "<td></td>"
        txt += self.th(_("Author"))
        txt += self.th(_("Web"))
        txt += self.th(_("License"))
        txt += "</tr>"

        li = [
            ("ChessiconsV3.5", '<a href=mailto:peterwong@virtualpieces.net>Peter Wong</a>',
             '<a href="http://www.virtualpieces.net">http://www.virtualpieces.net</a>', _("Permission of author") ),
            ("Merida-Internet", 'Felix Kling', '<a href="http://www.rybkachess.com">http://www.rybkachess.com</a>',
             _("Permission of author") ),
            ("Spatial-Fantasy\nSKulls-Freak-Prmi", 'Maurizio Monge',
             '<a href="http://poisson.phc.unipi.it/~monge/chess_art.php">Maurizio Monge\'s homepage</a>', "GPL" ),
            ("Cburnett", 'Cburnett',
             '<a href="http://en.wikipedia.org/wiki/User:Cburnett/GFDL_images/Chess">Wikimedia Commons</a>',
             "Creative Commons Attribution 2.5 License" ),
            ("Chess Alpha", 'Eric Bentzen',
             '<a href="http://www.enpassant.dk/chess/fonteng.htm">http://www.enpassant.dk/chess/fonteng.htm</a>',
             _("Permission of author") ),
            ("Montreal", 'Gary Katch',
             '<a href="http://alcor.concordia.ca/~gpkatch/montreal_font.html">http://alcor.concordia.ca/~gpkatch/montreal_font.html</a>',
             _("Permission of author") ),
            ("Magnetic-Leipzig<br>AlfonsoX-Maya<br>Condal", 'Armando H. Marroquín',
             '<a href="http://www.enpassant.dk/chess/fonteng.htm">http://www.enpassant.dk/chess/fonteng.htm</a>',
             _("Freeware") ),
            ("Chess Pirat", 'Klaus Wolf',
             '<a href="http://www.enpassant.dk/chess/fonteng.htm">http://www.enpassant.dk/chess/fonteng.htm</a>',
             _("Freeware") ),
            ("Chess Regular", 'Alastair Scott',
             '<a href="http://www.enpassant.dk/chess/fonteng.htm">http://www.enpassant.dk/chess/fonteng.htm</a>',
             _("Freeware") ),
            ("Kilfiger", 'James Kilfiger',
             '<a href="https://sites.google.com/site/jameskilfiger/">https://sites.google.com/site/jameskilfiger/</a>',
             "SIL open font licence" ),
            ("Cartoon", "Based on work by Martin Bérubé",
             '<a href="http://www.how-to-draw-funny-cartoons.com">http://www.how-to-draw-funny-cartoons.com</a>',
             _("Free for personal non-commercial use") ),
            ("Qwertyxp2000", "Qwertyxp2000",
             '<a href="http://commons.wikimedia.org/wiki/File%3AChess_pieces_(qwertyxp2000).svg">http://commons.wikimedia.org/wiki/File%3AChess_pieces_(qwertyxp2000).svg</a>',
             "Creative Commons Attribution 2.5 License" ),
            ("Jin Alpha", "Eric De Mund",
             '<a href="http://ixian.com/chess/jin-piece-sets/">http://ixian.com/chess/jin-piece-sets/</a>',
             "Creative Commons Attribution-Share Alike 3.0 Unported" ),
            ("Etruscan<br>Etruscan clear", "Fabrice",
             '<a href="http://zipanatura.fr/">http://zipanatura.fr/</a>',
             "CC BY-NC-ND 4.0" ),
             ("Stauton 3D", "Marc Graziani", '<a href="<https://plus.google.com/101635611158475796811/about">Marc Graziani</a>', _("Permission of author") ),
        ]

        for tipo, autor, web, licencia in li:
            txt += "<tr>"
            txt += "<td align=right><b>%s</b></td>" % tipo
            txt += "<td>%s</td>" % autor
            txt += "<td>%s</td>" % web
            txt += "<td>%s</td>" % licencia
            txt += "</tr>"

        txt += "</table><center>"
        return txt

    def Training(self):
        txt = self.tableIni()

        txt += "<tr>"
        txt += self.th(_("Training"))
        txt += self.th(_("Web"))
        txt += self.th(_("License"))
        txt += "</tr>"

        li = (
            (_("Checkmates by Eduardo Sadier"),
             '<a href="http://edusadier.googlepages.com/">http://edusadier.googlepages.com</a>',
             _("Permission of author") ),
            (_("From Pascal Georges, SCID"), '<a href="http://scid.sourceforge.net">http://scid.sourceforge.net</a>',
             _("Permission of author") ),
            (_("Endgames by Rui Grafino"), '', _("Permission of author") ),
            (_("Varied positions by Joaquin Alvarez"), '', _("Permission of author") ),
            (_("Tactics by Uwe Auerswald"), '', _("Freeware") ),
            (_("Endgames by Victor Perez"), '', _("Permission of author") ),
        )
        for autor, web, licencia in li:
            txt += "<tr>"
            txt += "<td align=center><b>%s</b></td>" % autor
            txt += "<td>%s</td>" % web
            txt += "<td>%s</td>" % licencia
            txt += "</tr>"

        txt += "</table><center>"
        return txt

    def Engines(self, orden):
        txt = self.tableIni()
        txt += "<tr>"
        txt += self.th(_("Engine"))
        txt += self.th(_("Author"))
        txt += self.th(_("Web"))
        txt += "</tr>"
        for nombre, autor, url in self.listaMotores(orden):
            txt += "<tr>"
            txt += "<th>%s</th>" % nombre
            txt += "<th>%s</th>" % autor
            txt += "<td><a href=\"%s\">%s</a></td>" % (url, url)
            txt += "</tr>"
        txt += self.tableEnd()
        return txt

    def Games(self):
        li = (
            ( "Jordi Gonzalez Boada", "http://www.jordigonzalezboada.com/ajedrez/aperturas.php" ),
            ( "PGN Mentor", "http://www.pgnmentor.com/files.html" ),
            ( "Dan Corbit", "http://cap.connx.com/" ),
            ( "The Week in Chess", "http://theweekinchess.com/" ),
            ( "Wikipedia", "http://en.wikipedia.org/wiki/List_of_chess_games" ),
            ( "fics", "http://ficsgames.org/cgi-bin/download.cgi" ),
            ( "Norman Pollock", "http://www.hoflink.com/~npollock/chess.html" ),
        )
        txt = '<center><table border="1" cellpadding="5" cellspacing="0" >'
        for nom, web in li:
            txt += "<tr>"
            txt += "<th>%s</th>" % nom
            txt += "<td><a href=\"%s\">%s</a></td>" % (web, web)
            txt += "</tr>"
        txt += "</table><center>"
        return txt

    def Programming(self):
        li = (
            ( _("Programming language"), "Python 2.7", "http://www.python.org/" ),
            ( _("GUI"), "PyQt4 - GPL", "http://www.riverbankcomputing.co.uk" ),
            ( _("Audio"), "PyAudio v0.2.4 - MIT License", "http://people.csail.mit.edu/hubert/pyaudio/" ),
            # ( "suds", _X(_("Created by %1"), "Jeff Ortel"), "https://fedorahosted.org/suds/"),
            ( "psutil", _X(_("Created by %1"), "Giampaolo Rodola"), "http://code.google.com/p/psutil/"),
            ( "Python for Windows extensions", _X(_("Created by %1"), "Mark Hammond"), "http://sourceforge.net/projects/pywin32" ),
            ( "pygal", _X(_("Created by %1"), "Kozea"), "http://pygal.org"),
            ( "chardet", _X(_("Created by %1"), "Ian Cordasco"), "https://github.com/chardet/chardet"),
            (_("Polyglot books"), _X(_("Based on work by %1"), "Michel Van den Bergh"), "http://alpha.uhasselt.be/Research/Algebra/Toga/book_format.html"),
            ( "Polyglot1.4w",_X(_("Created by %1"), "Fabien Letouzy") + ". " + _X(_("Modified by %1"), "Fonzy Bluemers"), "http://www.geenvis.net/" ),
            ( "winglet", _X(_("Created by %1"), "Stef Luijten"), "http://aghaznawi.comuf.com/computer%20chess/winglet/index.htm"),
            # ( "XFCC protocol", _X(_("Created by %1"), "Martin Bennedik"), "http://www.xfcc.org/"),
            ( "STS", _X(_("Created by %1"), "Dan Corbit,Swaminathan"), "https://sites.google.com/site/strategictestsuite/about-1"),
            ( "CMU Sphinx", "Open Source Toolkit For Speech Recognition by Carnegie Mellon University", "http://cmusphinx.sourceforge.net/"),
            ( "PyPocketSphinx", _X(_("Created by %1"), "Dmitry Prazdnichnov"), "https://github.com/bambocher/PyPocketSphinx"),
        )
        txt = self.tableIni()

        txt += "<tr>"
        txt += self.th(_("Utility"))
        txt += self.th(_("Author"))
        txt += self.th(_("Web"))
        txt += "</tr>"

        for tipo, nom, web in li:
            txt += "<tr>"
            txt += "<th>%s</th>" % tipo
            txt += "<td>%s</td>" % nom
            txt += "<td><a href=\"%s\">%s</a></td>" % (web, web)
            txt += "</tr>"

        txt += self.tableEnd()
        return txt

    def Dedicated(self):
        txt = "<center><big style=\"color:teal;\"><b>Lucas & Luisa</b></big></center>"
        return txt

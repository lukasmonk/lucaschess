# -*- coding: latin-1 -*-

import collections
import random

from Code import Util


class ThanksTo:
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
        d["Engines-3"] = "%s/3" % _("Engines")
        d["Games"] = _("Games")
        d["Programming"] = _("Programming")
        d["Dedicated"] = _("Dedicated to")

    def listaMotores(self, bloque):
        li = [
            ["Acqua 2.0", "Giovanni Di Maria", "http://www.elektrosoft.it/scacchi/acqua/acqua.asp"],
            ["Tarrasch ToyEngine Beta V0.905", "Bill Forster", "http://www.triplehappy.com/"],
            ["Rocinante 2.0", "Antonio Torrecillas", "http://sites.google.com/site/barajandotrebejos/"],
            ["Roce 0.0390", "Roman Hartmann", "http://www.rocechess.ch/rocee.html"],
            ["Cinnamon 1.2b", "Giuseppe Cannella", "http://cinnamonchess.altervista.org/"],
            ["Bikjump 2.01 (32-bit)", "Aart J.C. Bik", "http://www.aartbik.com/"],
            ["Clarabit 1.00", "Salvador Pallares Bejarano", "http://sapabe.googlepages.com"],
            ["Lime v 66", "Richard Allbert", "http://www.geocities.com/taciturn_lemon"],
            ["Chispa 4.0.3", "Federico Corigliano", "http://chispachess.blogspot.com/"],
            ["Gaïa 3.5", "Jean-Francois Romang, David Rabel", "http://gaiachess.free.fr"],
            ["Simplex 0.9.8", "Antonio Torrecillas", "http://sites.google.com/site/barajandotrebejos/"],
            ["Greko 12.9", "Vladimir Medvedev", "http://greko.su/index_en.html"],
            ["Greko 7.1", "Vladimir Medvedev", "http://greko.su/index_en.html"],
            ["Pawny 0.3.1", "Mincho Georgiev", "http://pawny.netii.net/"],
            ["Hamsters 0.5", "Alessandro Scotti", "https://chessprogramming.wikispaces.com/Alessandro+Scotti"],
            ["Umko 0.7", "Borko Boskovic", "http://umko.sourceforge.net/"],
            ["Garbochess 2.20", "Gary Linscott", "http://forwardcoding.com/projects/chess/chess.html"],
            ["Ufim 8.02", "Niyas Khasanov", "http://wbec-ridderkerk.nl/html/details1/Ufim.html"],
            ["Amyan 1.62", "Antonio Dieguez R.", "http://www.pincha.cl/amyan/amyane.html"],
            ["Alaric 707", "Peter Fendrich", "http://alaric.fendrich.se/index.html"],
            ["Cyrano 06B17", "Harald Johnsen", "http://sites.estvideo.net/tipunch/cyrano/"],
            ["Daydreamer 1.75 JA", "Aaron Becker", "http://github.com/AaronBecker/daydreamer/downloads"],
            ["Gödel 4.4.5", "Juan Manuel Vazquez", "https://sites.google.com/site/godelchessengine"],
            ["Rhetoric 1.4.3", "Alberto Sanjuan", "http://www.chessrhetoric.com/"],
            ["Cheng 4 0.39", "Martin Sedlák", "http://www.vlasak.biz/cheng"],
            ["Glaurung 2.2 JA", "Tord RomsTad", "http://www.glaurungchess.com/"],
            ["Fruit 2.3.1", "Fabien Letouzey", "http://www.fruitchess.com/"],
            ["Rodent 1.6", "Pawel Koziol", "http://www.pkoziol.cal24.pl/rodent/rodent.htm"],
            ["RodentII 0.9", "Pawel Koziol", "http://www.pkoziol.cal24.pl/rodent/rodent.htm"],
            ["Discocheck 5.2.1", "Lucas Braesch", "https://github.com/lucasart"],
            ["Gaviota 1.0", "Miguel A. Ballicora", "https://sites.google.com/site/gaviotachessengine"],
            ["Toga deepTogaNPS 1.9.6", "WHMoweryJr,Thomas Gaksch,Fabien Letouzey", "http://www.computerchess.info/tdbb/phpBB3/viewtopic.php?f=9&t=357"],
            ["Komodo 10", "Don Dailey, Larry Kaufman, Mark Lefler", "http://komodochess.com/"],
            ["Rybka 2.3.2a 32-bit", "Vasik Rajlich", "http://rybkachess.com/"],
            ["Critter 1.6a 32bits", "Richard Vida", "http://www.vlasak.biz/critter/"],
            ["Texel 1.07", "Peter Österlund", "http://hem.bredband.net/petero2b/javachess/index.html#texel"],
            ["Stockfish 10", "Tord Romstad, Marco Costalba, Joona Kiiski", "http://stockfishchess.org/"],
            ["McCain X3", "Michael Byrne", "https://github.com/MichaelB7/Stockfish/releases"],
            ["Gull 3", "Vadim Demichev", "https://sourceforge.net/projects/gullchess/"],
            ["Delfi 5.4", "Fabio Cavicchio", "http://www.msbsoftware.it/delfi/"],
            # ["SmartThink 1.97", "Sergei S. Markoff", "http://genes1s.net/smarthink.php"],
            ["Hannibal 1.4b", "Samuel N. Hamilton and Edsel G. Apostol", "http://sites.google.com/site/edapostol/hannibal"],
            ["Monarch 1.7", "Steve Maughan", "http://www.monarchchess.com/"],
            ["Andscacs 0.9432n", "Daniel José Queraltó", "http://www.andscacs.com/"],
            ["Arminius 2017-01-01", "Volker Annus", "http://www.nnuss.de/Hermann/Arminius2017-01-01.zip"],
            ["WildCat", "Igor Korshunov", "http://www.igorkorshunov.narod.ru/WildCat"],
            ["Demolito", "Lucas Braesch", "https://github.com/lucasart/Demolito"],
            ["Spike 1.4", "Volker Böhm and Ralf Schäfer", "http://spike.lazypics.de/index_en.html"],
            ["Zappa 1.1", "Anthony Cozzie", "http://www.acoz.net/zappa/"],
            ["Irina 0.15", "Lucas Monge", "https://github.com/lukasmonk/irina"],
            ["Houdini 1.5a", "Robert Houdart", "http://www.cruxis.com/chess/houdini.htm"],
            ["Paladin 0.1", "Ankan Banerjee", "https://github.com/ankan-ban/chess_cpu"],
            ["Cdrill 1800", "Ferdinand Mosca", "https://sites.google.com/view/cdrill"]
        ]
        li.sort(key=lambda x: x[0])
        n = len(li)
        x = n*1.0/3
        bl = [0, int(x), int(2*x), len(li)]
        nbl = int(bloque)
        return li[bl[nbl-1]:bl[nbl]]

    def texto(self, clave):
        if "-" in clave:
            clave, arg = clave.split("-")
            return getattr(self, clave)(arg)
        return getattr(self, clave)()

    def tableIni(self):
        return '<center><table border="1" cellpadding="3" cellspacing="0">'

    def tableEnd(self):
        return '</table>'

    def th(self, txt, mas=""):
        return "<th %s>%s</th>" % (mas, txt)

    def Contributors(self):
        txt = "<br><center><big>%s: <b>Michele Tumbarello</b></big><br>" % _("Chief engineer officer")
        txt += self.tableIni()

        def version(num, liBase, liResto):
            random.shuffle(liBase)
            liBase.extend(liResto)
            txt = "<tr>"
            txt += self.th(_("Version %s") % num)
            txt += "<td><b>"
            n = 0
            for uno in liBase:
                if n > 80:
                    txt += "<br>"
                    n = 0
                txt += '<span style="color:dimgray">' + uno + "</span>, "
                n += len(uno) + 2
            txt = txt[:-2]
            txt += "</b></td>"
            txt += "</tr>"
            return txt

        # Version 11
        liBase = ["Alfonso Solbes", "Max Aloyau", "tico-tico", "Nils Andersson", "Bernhard", "Ed Smith", "Rob",
                  "Giovanni di Maria", "vga", "Remes", "Péter Rabi", "Iñaki Rodriguez"]
        liResto = ["Immortalchess forum",]
        txt += version(11, liBase, liResto)

        # Version 10
        liBase = ["Remes", "Max Aloyau", "Alfonso Solbes", "tico-tico", "Nils Andersson", "Bernhard", "Ed Smith"]
        liResto = ["Immortalchess forum", ]
        txt += version(10, liBase, liResto)

        # Version 9
        liBase = ["Indianajones", "Pavel Rehulka", "ADT", "Adrijan", "Nils Andersson", "Urban Gustavsson",
                  "Johannes Reimers", "Red Hood", "Robert Anderson", "Laudecir Daniel", "Reinhard",
                  "Di Maria Giovanni", "Filomeno Marmalé", "Max Aloyau",
                  ]
        liResto = ["Chessindia forum", "Immortalchess forum", ]
        txt += version(9, liBase, liResto)

        # Version 8
        liBase = ["Indianajones", "James", "Uli", "Pavel Rehulka", "Laudecir Daniel", "Xavier Jimenez",
                  "Rajkrishna", "ADT", "Vishy", "thetasquared", "Mike Eddies", "jayriginal", "baddadza",
                  "bbbaro25us", "Victor Perez", "M.Larson", "Filomeno Marmalé", "Shahin Jafarli (shahinjy)",
                  "Heikki Junes", "Toan Luong", "R. Sehgal", "WyoCas", "J.Reimers", "Dariusz Popadowski",
                  "Ken Brown", "Dieter Heinrich", "Nils Andersson", "Chris K.", "Philou",
                  ]
        liResto = ["Chessindia forum", "Immortalchess forum", ]
        txt += version(8, liBase, liResto)

        # Version 1..7
        liBase = ["Felicia", "Shahin Jafarli (shahinjy)", "Alfons", "Raúl Giorgi", "Red Hood", "Filomeno Marmalé",
                  "Roberto Mizzoni", "bolokay", "Istolacio", "Mohammed Abdalazez", "Rui Grafino", "Georg Pfefferle",
                  "Lolo S.", "Joaquín Alvarez", "Ransith Fernando",
                  ]
        liResto = ["Gianfranco Cutipa", "Daniel Trebejo", "Jose Luis García", "Carmen Martínez", ]  # "Ezequiel Canario"
        txt += version("1..7", liBase, liResto)

        txt += self.tableEnd()
        return txt

    def Translators(self):
        dic = {
            _("Arabic"): ("Mohamad Alhedhed", ""),
            _("Azeri"): ("Shahin Jafarli (shahinjy)", ""),
            _("Catalan"): ("Salvador Morral i Esteve", ""),
            _("Chinese simplified"): ("Kevin Sicong Jiang,Stephen Yang", ""),
            _("Czech"): ("Jindrich Skula", ""),
            _("Dutch"): ("Willy Lefebvre", ""),
            _("Finnish"): ("Heikki J.", ""),
            _("French"): ("Max Aloyau", "Lolo S."),
            _("German"): ("Tobias, Frank Stender, Andreas","Alfons, Georg Pfefferle"),
            _("Greek"): ("Nick Delta", ""),
            _("Indonesian"): ("Heri Darmanto", ""),
            _("Italiano"): ("Michele Tumbarello", "Maurizio Peroni"),
            _("Polish"): ("Dariusz Popadowski", "Miroslaw Kaminski"),
            _("Portuguese"): ("Rui Grafino", ""),
            (_("Portuguese") + " (BR)"): ("Laudecir Daniel", ""),
            _("Romanian"):("Dan-Alexandru Raportaru, Tipter Napoleon",""),
            _("Russian"): ("Vladimir", "Slavik Pavlov,Nils Andersson, Reinhard, "),
            _("Spanish"): ("Lucas", ""),
            _("Swedish"): ("Nils Andersson", ""),
            _("Ukrainian"): ("Volodymyr Soltys", "Maxym Makarchuk"),
            _("Vietnamese"): ("Toan Luong", ""),
        }

        def r(lng):
            return Util.iniBase8dic("Locale/%s/lang.ini" % lng)["AUTHOR"]

        dic[_("Slovenian")] = (r("si"), "")
        dic[_("Turkish")] = (r("tr"), "")
        # _("English"):"Lucas, Georg Pfefferle, Lolo S., bolokay",
        li = dic.keys()
        li.sort()
        # random.shuffle(li)
        txt = self.tableIni()

        txt += "<tr>"
        for x in range(2):
            txt += self.th(_("Language"))
            txt += self.th(_("Translator"))
            txt += self.th(_("Previous"))
            if x%2 == 0:
                txt += self.th("")

        txt += "</tr>"
        for n, uno in enumerate(li):
            current, previous = dic[uno]
            if n%2 == 0:
                txt += "<tr>"
            txt += "<th>%s</th><td><b>%s</b></td><td>%s</td>" % (uno, current, previous)
            if n%2 == 1:
                txt += "</tr>"
            else:
                txt += "<td></td>"
        if len(li)%2 == 1:
            txt += "<th></th><td></td><td></td></tr>"

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
            ("Nuvola", '<a href="http://icon-king.com/?p=15">David Vignoni</a>', 'http://www.icon-king.com/projects/nuvola/', "LGPL"),
            ("Gnome", 'Gnome', "https://github.com/GNOME/gnome-icon-theme", "GPL"),
            ("Silk icon set 1.3", 'Mark James', 'http://www.famfamfam.com/lab/icons/silk/', "Creative Commons Attribution 2.5 License"),
            ("Wooicons1", 'Janik Baumgartner', 'http://www.woothemes.com/2010/08/woocons1/', "GPL"),
            ("Ultimate Gnome 0.5.1", 'Marco Tessarotto', 'http://gnome-look.org/content/show.php/Ultimate+Gnome?content=75000', "GPL"),
            ("SnowIsh SVG", "Saki", 'http://gnome-look.org/content/show.php/SnowIsh+SVG+%26+PNG?content=32599', "GPL"),
            ("Cartoon animal icons", "Martin Bérubé", 'http://www.how-to-draw-funny-cartoons.com/', "Free for personal non-commercial use"),
            ("Album of vehicles", "Icons-Land", "http://www.icons-land.com/vista-icons-transport-icon-set.php", "Icons-Land Demo License Agreement"),
            ("Figurines", 'Armando H. Marroquín', 'http://www.enpassant.dk/chess/fonteng.htm', _("Freeware")),
            ("Icons for Windows8", 'Icons8', 'https://icons8.com', "Creative Commons Attribution-NoDerivs 3.0 Unported"),
            ("Transsiberian map", "Stefan Ertmann & Lokal Profil",
             "https://commons.wikimedia.org/wiki/File:Trans-Siberian_railway_map.svg", "CC BY-SA 2.5 via Wikimedia Commons"),
            ("Washing machine", "Shinnoske", "https://openclipart.org/detail/218905/simple-washing-machine", "Public domain"),
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
            ("Michele Tumbarello", _("Permission of author")),
            ("Felicia", _("Permission of author")),
            ("Mohammed Abdalazez", _("Permission of author")),
            ("Red Hood", _("Permission of author")),
            ("Michael Byrne", _("Permission of author")),
            ("Ben Citak", _("Permission of author")),
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
            "Attribution-NonCommercial-ShareAlike 3.0 Unported")
        txt += "</tr>"

        li = [
            ("Armani Closet", "mimi22", "http://www.colourlovers.com/palette/117475/armani_closet"),
            ("Chocolate Creams", "Skyblue2u", "http://www.colourlovers.com/palette/582195/Chocolate_Creams"),
            ("Good Friends", "Yasmino", "http://www.colourlovers.com/palette/77121/Good_Friends"),
            ("Nectarius", "note", "http://www.colourlovers.com/palette/1897208/nectarius"),
            ("Trajan", "Jaime Guadagni aka The Cooler", "http://www.colourlovers.com/palette/67170/Trajan")
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
            ("ChessiconsV3.5", '<a href=mailto:peterwong@virtualpieces.net>Peter Wong</a>', '<a href="http://www.virtualpieces.net">http://www.virtualpieces.net</a>', _("Permission of author")),
            ("Merida-Internet", 'Felix Kling', '<a href="http://www.rybkachess.com">http://www.rybkachess.com</a>', _("Permission of author")),
            ("Spatial-Fantasy<br>SKulls-Freak-Prmi", 'Maurizio Monge', '<a href="http://poisson.phc.unipi.it/~monge/chess_art.php">Maurizio Monge\'s homepage</a>', "GPL"),
            ("Cburnett", 'Cburnett', '<a href="http://en.wikipedia.org/wiki/User:Cburnett/GFDL_images/Chess">Wikimedia Commons</a>', "Creative Commons Attribution 2.5 License"),
            ("Chess Alpha", 'Eric Bentzen', '<a href="http://www.enpassant.dk/chess/fonteng.htm">http://www.enpassant.dk/chess/fonteng.htm</a>', _("Permission of author")),
            ("Montreal", 'Gary Katch', '<a href="http://alcor.concordia.ca/~gpkatch/montreal_font.html">http://alcor.concordia.ca/~gpkatch/montreal_font.html</a>', _("Permission of author")),
            ("Magnetic-Leipzig<br>AlfonsoX-Maya<br>Condal", 'Armando H. Marroquín', '<a href="http://www.enpassant.dk/chess/fonteng.htm">http://www.enpassant.dk/chess/fonteng.htm</a>', _("Freeware")),
            ("Chess Pirat", 'Klaus Wolf', '<a href="http://www.enpassant.dk/chess/fonteng.htm">http://www.enpassant.dk/chess/fonteng.htm</a>', _("Freeware")),
            ("Chess Regular", 'Alastair Scott', '<a href="http://www.enpassant.dk/chess/fonteng.htm">http://www.enpassant.dk/chess/fonteng.htm</a>', _("Freeware")),
            ("Kilfiger", 'James Kilfiger', '<a href="https://sites.google.com/site/jameskilfiger/">https://sites.google.com/site/jameskilfiger/</a>', "SIL open font licence"),
            ("Cartoon", "Based on work by Martin Bérubé", '<a href="http://www.how-to-draw-funny-cartoons.com">http://www.how-to-draw-funny-cartoons.com</a>', _("Free for personal non-commercial use")),
            ("Qwertyxp2000", "Qwertyxp2000", '<a href="http://commons.wikimedia.org/wiki/File%3AChess_pieces_(qwertyxp2000).svg">http://commons.wikimedia.org/wiki/File%3AChess_pieces_(qwertyxp2000).svg</a>', "Creative Commons Attribution 2.5 License"),
            ("Jin Alpha", "Eric De Mund", '<a href="http://ixian.com/chess/jin-piece-sets/">http://ixian.com/chess/jin-piece-sets/</a>', "Creative Commons Attribution-Share Alike 3.0 Unported"),
            ("Etruscan<br>Etruscan clear", "Fabrice", '<a href="http://zipanatura.fr/">http://zipanatura.fr/</a>', "CC BY-NC-ND 4.0"),
            ("Stauton 3D<br>Kidsdraw", "Marc Graziani", '<a href="https://plus.google.com/101635611158475796811/about">https://plus.google.com/101635611158475796811/about</a>', _("Permission of author")),
            ("Book Diagram<br>Book Engraved<br>Book Good Companions<br>Book Leipzig", "Ben Citak", '<a href="https://www.linkedin.com/in/benjamin-citak-04982714">LinkedIn</a>', _("Permission of author")),
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
             _("Permission of author")),
            (_("From Pascal Georges, SCID"), '<a href="http://scid.sourceforge.net">http://scid.sourceforge.net</a>',
             _("Permission of author")),
            (_("Endgames by Rui Grafino"), '', _("Permission of author")),
            (_("Varied positions by Joaquin Alvarez"), '', _("Permission of author")),
            (_("Tactics by Uwe Auerswald"), '', _("Freeware")),
            (_("Endgames by Victor Perez"), '', _("Permission of author")),
            (_("Tactics by UNED chess school"), '<a href="http://portal.uned.es/portal/page?_pageid=93,2564320&_dad=portal&_schema=PORTAL">http://portal.uned.es/portal/page?_pageid=93,2564320&_dad=portal&_schema=PORTAL</a>', _("Permission of author")),
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
            if "honey" in nombre:
                txt += '<th><font color="darkred">%s (%s)</font></th>' % (nombre, _("default"))
                txt += '<td><font color="darkred">%s</font></td>' % autor
            else:
                txt += "<td>%s</td>" % nombre
                txt += "<td>%s</td>" % autor
            txt += "<td><a href=\"%s\">%s</a></td>" % (url, url)
            txt += "</tr>"
        txt += self.tableEnd()
        return txt

    def Games(self):
        li = (
            ("Jordi Gonzalez Boada", "http://www.jordigonzalezboada.com/ajedrez/aperturas.php"),
            ("PGN Mentor", "http://www.pgnmentor.com/files.html"),
            ("Dan Corbit", "http://cap.connx.com/"),
            ("The Week in Chess", "http://theweekinchess.com/"),
            ("Wikipedia", "http://en.wikipedia.org/wiki/List_of_chess_games"),
            ("fics", "http://ficsgames.org/cgi-bin/download.cgi"),
            ("Norman Pollock", "http://40h.000webhostapp.com/"),
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
            (_("Programming language"), "Python 2.7", "http://www.python.org/"),
            (_("GUI"), "PyQt4 - GPL", "http://www.riverbankcomputing.co.uk"),
            (_("Audio"), "PyAudio v0.2.4 - MIT License", "http://people.csail.mit.edu/hubert/pyaudio/"),
            ("psutil", _X(_("Created by %1"), "Giampaolo Rodola"), "http://code.google.com/p/psutil/"),
            ("chardet", _X(_("Created by %1"), "Ian Cordasco"), "https://github.com/chardet/chardet"),
            (_("Polyglot books"), _X(_("Based on work by %1"), "Michel Van den Bergh"), "https://hardy.uhasselt.be/personal/vdbergh/Members/michel_id.html"),
            ("Polyglot1.4w", _X(_("Created by %1"), "Fabien Letouzy") + ". " + _X(_("Modified by %1"), "Fonzy Bluemers"), "http://www.geenvis.net/"),
            ("STS", _X(_("Created by %1"), "Dan Corbit,Swaminathan"), "https://sites.google.com/site/strategictestsuite/about-1"),
            ("python-chess", _X(_("Created by %1"), "Niklas Fiekas"), "https://github.com/niklasf/python-chess"),
            ("pillow", "Copyright 1995-2015, Fredrik Lundh and Contributors,<br>Alex Clark and Contributors", "https://github.com/python-pillow/Pillow"),
            ("photohash", "Chris Pickett and others", "https://github.com/bunchesofdonald/photohash"),
            ("cython", "Cython 0.24", "http://cython.org/"),
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

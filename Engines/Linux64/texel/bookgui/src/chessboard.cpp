/*
    Texel - A UCI chess engine.
    Copyright (C) 2016  Peter Österlund, peterosterlund2@gmail.com

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/*
 * chessboard.cpp
 *
 *  Created on: Apr 10, 2016
 *      Author: petero
 */

#include "chessboard.hpp"
#include "textio.hpp"

ChessBoard::ChessBoard(const Position& pos0, Gtk::DrawingArea* area)
    : pos(pos0), drawArea(area) {
    drawArea->signal_draw().connect(sigc::mem_fun(*this, &ChessBoard::draw));
    drawArea->signal_button_press_event().connect(sigc::mem_fun(*this, &ChessBoard::mouseDown), false);
    drawArea->signal_button_release_event().connect(sigc::mem_fun(*this, &ChessBoard::mouseUp), false);
    drawArea->signal_motion_notify_event().connect(sigc::mem_fun(*this, &ChessBoard::mouseMove), false);
    drawArea->add_events(Gdk::BUTTON_PRESS_MASK | Gdk::BUTTON_RELEASE_MASK | Gdk::BUTTON1_MOTION_MASK);

    FT_Init_FreeType(&ftLib);
    GBytes *fontBytes = g_resources_lookup_data("/font/ChessCases.ttf",
                                                G_RESOURCE_LOOKUP_FLAGS_NONE, NULL);
    gsize size;
    gconstpointer fontData = g_bytes_get_data(fontBytes, &size);
    FT_New_Memory_Face(ftLib, (const FT_Byte*)fontData, size, 0, &ftFace);
    fontFace = Cairo::FtFontFace::create(ftFace, 0);
}

ChessBoard::~ChessBoard() {
    FT_Done_Face(ftFace);
    FT_Done_FreeType(ftLib);
}

void
ChessBoard::queueDraw() {
    drawArea->queue_draw();
}

double
ChessBoard::getSquareSize() const {
    int w = drawArea->get_allocated_width();
    int h = drawArea->get_allocated_height();
    return std::min(w, h) / 8.0;
}

int
ChessBoard::getSquare(double xCrd, double yCrd) const {
    double sqSize = getSquareSize();
    int x = (int)floor(xCrd / sqSize);
    int y = 7 - (int)floor(yCrd / sqSize);
    if ((x < 0) || (x > 7) || (y < 0) || (y > 7))
        return -1;
    return Position::getSquare(x, y);
}

bool
ChessBoard::draw(const Cairo::RefPtr<Cairo::Context>& ctx) {
    double sqSize = getSquareSize();
    ctx->set_line_width(1.0);
    ctx->set_font_face(fontFace);
    ctx->set_font_size(sqSize);

    for (int x = 0; x < 8; x++) {
        for (int y = 0; y < 8; y++) {
            if (Position::darkSquare(x, y))
                ctx->set_source_rgb(0.514, 0.647, 0.824);
            else
                ctx->set_source_rgb(1.0, 1.0, 1.0);
            double xCrd = x * sqSize;
            double yCrd = (7-y) * sqSize;
            ctx->rectangle(xCrd, yCrd, sqSize, sqSize);
            ctx->fill();
            int sq = Position::getSquare(x, y);
            int piece = pos.getPiece(sq);
            if (sq != dragSquare)
                drawPiece(ctx, xCrd, yCrd, sqSize, piece);
        }
    }
    if (dragSquare >= 0)
        drawPiece(ctx, dragX - sqSize / 2, dragY - sqSize / 2, sqSize,
                  pos.getPiece(dragSquare));

    return false;
}

void
ChessBoard::drawPiece(const Cairo::RefPtr<Cairo::Context>& ctx,
                      double xCrd, double yCrd, double sqSize,
                      int piece) {
    const char* wChar = nullptr;
    const char* bChar = nullptr;
    switch (piece) {
    case Piece::WKING:   wChar = "k"; bChar = "H"; break;
    case Piece::WQUEEN:  wChar = "l"; bChar = "I"; break;
    case Piece::WROOK:   wChar = "m"; bChar = "J"; break;
    case Piece::WBISHOP: wChar = "n"; bChar = "K"; break;
    case Piece::WKNIGHT: wChar = "o"; bChar = "L"; break;
    case Piece::WPAWN:   wChar = "p"; bChar = "M"; break;
    case Piece::BKING:   wChar = "q"; bChar = "N"; break;
    case Piece::BQUEEN:  wChar = "r"; bChar = "O"; break;
    case Piece::BROOK:   wChar = "s"; bChar = "P"; break;
    case Piece::BBISHOP: wChar = "t"; bChar = "Q"; break;
    case Piece::BKNIGHT: wChar = "u"; bChar = "R"; break;
    case Piece::BPAWN:   wChar = "v"; bChar = "S"; break;
    default: break;
    }
    if (wChar) {
        ctx->set_source_rgb(1.0, 1.0, 1.0);
        ctx->move_to(xCrd, yCrd + sqSize);
        ctx->show_text(wChar);

        ctx->set_source_rgb(0.0, 0.0, 0.0);
        ctx->move_to(xCrd, yCrd + sqSize);
        ctx->show_text(bChar);
    }
}

bool
ChessBoard::mouseDown(GdkEventButton* event) {
    if (event->button == 1) {
        dragSquare = getSquare(event->x, event->y);
        if (dragSquare >= 0) {
            int piece = pos.getPiece(dragSquare);
            if (Piece::isWhite(piece) == pos.isWhiteMove()) {
                dragX = event->x;
                dragY = event->y;
                queueDraw();
            } else {
                dragSquare = -1;
            }
        }
    }
    return true;
}

bool
ChessBoard::mouseUp(GdkEventButton* event) {
    if (event->button == 1 && dragSquare >= 0) {
        int fromSq = dragSquare;
        int toSq = getSquare(event->x, event->y);
        dragSquare = -1;
        queueDraw();
        if (toSq >= 0)
            signal_move_made.emit(Move(fromSq, toSq, Piece::EMPTY));
    }
    return true;
}

bool
ChessBoard::mouseMove(GdkEventMotion* event) {
    dragX = event->x;
    dragY = event->y;
    queueDraw();
    return true;
}

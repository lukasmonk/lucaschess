/*
    Texel - A UCI chess engine.
    Copyright (C) 2012-2014  Peter Österlund, peterosterlund2@gmail.com

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
 * chessParseError.hpp
 *
 *  Created on: Feb 25, 2012
 *      Author: petero
 */

#ifndef CHESSPARSEERROR_HPP_
#define CHESSPARSEERROR_HPP_

#include <exception>
#include <string>

/**
 * Exception class to represent parse errors in FEN or algebraic notation.
 */
class ChessParseError : public std::exception {
public:
    ChessParseError();
    explicit ChessParseError(const std::string& msg);

    virtual const char* what() const noexcept override;

private:
    std::string msg_;
};

inline
ChessParseError::ChessParseError(const std::string& msg)
    : msg_(msg) {
}

inline const char*
ChessParseError::what() const throw() {
    return msg_.c_str();
}

#endif /* CHESSPARSEERROR_HPP_ */

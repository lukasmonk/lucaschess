/*
    Texel - A UCI chess engine.
    Copyright (C) 2012-2015  Peter Österlund, peterosterlund2@gmail.com

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

#include "cute.h"
#include "ide_listener.h"
#include "cute_runner.h"

#include "bitBoardTest.hpp"
#include "bookTest.hpp"
#include "computerPlayerTest.hpp"
#include "evaluateTest.hpp"
#include "gameTest.hpp"
#include "historyTest.hpp"
#include "killerTableTest.hpp"
#include "moveGenTest.hpp"
#include "moveTest.hpp"
#include "pieceTest.hpp"
#include "positionTest.hpp"
#include "searchTest.hpp"
#include "textioTest.hpp"
#include "transpositionTableTest.hpp"
#include "parallelTest.hpp"
#include "treeLoggerTest.hpp"
#include "utilTest.hpp"
#include "tbTest.hpp"
#include "tbgenTest.hpp"
#include "polyglotTest.hpp"

#include "computerPlayer.hpp"
#include "tbprobe.hpp"
#include "parameters.hpp"

void
runSuite(const SuiteBase& suite, const std::vector<std::string>& suiteNames) {
    if (suiteNames.empty() || contains(suiteNames, suite.getName())) {
        cute::ide_listener lis;
        cute::makeRunner(lis)(suite.getSuite(), suite.getName().c_str());
    }
}

int main(int argc, char* argv[]) {
    std::vector<std::string> suiteNames;
    for (int i = 1; i < argc; i++)
        suiteNames.emplace_back(argv[i]);

    UciParams::gtbPath->set(gtbDefaultPath);
    UciParams::rtbPath->set(rtbDefaultPath);
    UciParams::gtbCache->set("128");
    ComputerPlayer::initEngine();
    runSuite(BitBoardTest(), suiteNames);
    runSuite(BookTest(), suiteNames);
    runSuite(ComputerPlayerTest(), suiteNames);
    runSuite(EvaluateTest(), suiteNames);
    runSuite(GameTest(), suiteNames);
    runSuite(HistoryTest(), suiteNames);
    runSuite(KillerTableTest(), suiteNames);
    runSuite(MoveGenTest(), suiteNames);
    runSuite(MoveTest(), suiteNames);
    runSuite(PieceTest(), suiteNames);
    runSuite(PositionTest(), suiteNames);
    runSuite(SearchTest(), suiteNames);
    runSuite(TextIOTest(), suiteNames);
    runSuite(TranspositionTableTest(), suiteNames);
    runSuite(ParallelTest(), suiteNames);
    runSuite(TreeLoggerTest(), suiteNames);
    runSuite(UtilTest(), suiteNames);
    runSuite(TBTest(), suiteNames);
    runSuite(TBGenTest(), suiteNames);
    runSuite(PolyglotTest(), suiteNames);
    return 0;
}

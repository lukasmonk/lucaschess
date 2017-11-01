rm *.o
g++ -o rocinante Board.cpp BStar.cpp C3FoldRep.cpp CDebug.cpp DumpTree.cpp epd.cpp Evalu8.cpp HashJugadas.cpp JobWorker.cpp main.cpp MCTS_AB.cpp MoveList.cpp Parameters.cpp Search.cpp See.cpp SmpManager.cpp system.cpp TreeNode.cpp Uci.cpp ufo.cpp UndoData.cpp WeightMoves.cpp zobrist.cpp -Wfatal-errors -w -s -pipe -m64 -march=nocona -Ofast -std=c99 -DNDEBUG -flto -fwhole-program -lpthread -lm








 




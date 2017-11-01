/*
Rodent, a UCI chess playing engine derived from Sungorus 1.4
Copyright (C) 2009-2011 Pablo Vazquez (Sungorus author)
Copyright (C) 2011-2016 Pawel Koziol

Rodent is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published
by the Free Software Foundation, either version 3 of the License,
or (at your option) any later version.

Rodent is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty
of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

enum eTimeData { W_TIME, B_TIME, W_INC, B_INC, TIME, INC, MOVES_TO_GO, MOVE_TIME, 
               MAX_NODES, MAX_DEPTH, FLAG_INFINITE, SIZE_OF_DATA };

struct sTimer {
private:
  int data[SIZE_OF_DATA]; // various data used to set actual time per move (see eTimeData)
  int start_time;         // when we have begun searching
  int iteration_time;     // when we are allowed to start new iteration
  int allocated_time;     // basic time allocated for a move
  int adjustement;
  int smart_management;
  int BulletCorrection(int time);
public:
  int nps_limit;
  int special_mode;
  void Clear(void);
  void OnOldRootMove(void);
  void OnNewRootMove(void);
  void OnFailLow(void);
  void SetStartTime();
  void SetMoveTiming(void);
  void SetIterationTiming(void);
  int FinishIteration(void);
  int GetMS(void);
  int GetElapsedTime(void);
  int IsInfiniteMode(void);
  int TimeHasElapsed(void);
  void Init(void);
  void WasteTime(int miliseconds);
  int GetData(int slot);
  void SetData(int slot, int val);
  void SetSideData(int side);
  void SetSpeed(int elo);
};

extern sTimer Timer;

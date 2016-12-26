#pragma once

class UndoData
{
public:
	UndoData(void);
	~UndoData(void);
public:
   bool capture;

   int capture_square;
   int capture_piece;
   int capture_pos;

   int wtm;
   int CastleFlags;
   int EnPassant;
   int PlyNumber;
   bool IsPromote;
};

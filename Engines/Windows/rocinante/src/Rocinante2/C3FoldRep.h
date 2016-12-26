#pragma once

class C3FoldRep
{
public:
	C3FoldRep(void);
	~C3FoldRep(void);
static	void Reset();
static	void Add(char *fen);
static	bool IsRep(char *fen);
};

extern C3FoldRep ThreeFold;

#pragma once
const int MaxMoves = 256;

class MoveList
{
public:
	MoveList(void);
	~MoveList(void);
// List with moves
private:
	int size;
	unsigned short move[MaxMoves];
	int value[MaxMoves];
public:
	// funciones
	inline bool IsEmpty() {return size==0;};
	inline int Length() { return size;};
	inline void Clear() { size = 0;};
	inline void Add(unsigned short OneMove) {move[size] = OneMove;value[size] = 0;size++;};
	inline unsigned short Move(int pos) { return move[pos];};
	inline int Value(int pos) { return value[pos];};
	inline bool IsOk() { return size > 0 && size < MaxMoves;};
	inline int Size() { return size;};
	void Remove(int pos);
	inline void SetScore(int pos,int score) { value[pos] = score;};
	// TODO todo lo relacionado con la ordenacion
	void Sort();
private:
};

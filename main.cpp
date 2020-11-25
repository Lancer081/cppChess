#include <iostream>

using namespace std;

enum Squares {
	a8, b8, c8, d8, e8, f8, g8, h8,
	a7, b7, c7, d7, e7, f7, g7, h7,
	a6, b6, c6, d6, e6, f6, g6, h6,
	a5, b5, c5, d5, e5, f5, g5, h5,
	a4, b4, c4, d4, e4, f4, g4, h4,
	a3, b3, c3, d3, e3, f3, g3, h3,
	a2, b2, c2, d2, e2, f2, g2, h2,
	a1, b1, c1, d1, e1, f1, g1, h1, no_sq
};

enum Side { white, black, both };

enum Pieces { P, N, B, R, Q, K, p, n, b, r, q, k };

uint64_t getBit(uint64_t bits, int index)
{
	return (bits >> index) & 1ULL;
}

void setBit(uint64_t* bits, int index)
{
	*bits |= 1ULL << index;
}

void zeroBit(uint64_t* bits, int index)
{
	*bits &= ~(1ULL << index);
}

class Piece {
public:
	int color;
};

class Pawn: public Piece {
public:
	Pawn(int color)
	{
		this->color = color;
	}
};

class Board {
public:
	uint64_t bits[12];
	uint64_t occupancy[3];

	void init()
	{
		for (int piece = P; piece <= k; piece++)
			bits[piece] = 0ULL;

		for (int i = 0; i < 3; i++)
			occupancy[i] = 0ULL;
	}

	void print()
	{
		for (int rank = 0; rank < 8; rank++)
		{
			for (int file = 0; file < 8; file++)
			{
				int square = rank * 8 + file;

				if (getBit(bits[P], square))
					cout << " P";
				else if (getBit(bits[N], square))
					cout << " N";
				else if (getBit(bits[B], square))
					cout << " B";
				else if (getBit(bits[R], square))
					cout << " R";
				else if (getBit(bits[Q], square))
					cout << " Q";
				else if (getBit(bits[K], square))
					cout << " K";
				else if (getBit(bits[p], square))
					cout << " p";
				else if (getBit(bits[n], square))
					cout << " n";
				else if (getBit(bits[b], square))
					cout << " b";
				else if (getBit(bits[r], square))
					cout << " r";
				else if (getBit(bits[q], square))
					cout << " q";
				else if (getBit(bits[k], square))
					cout << " k";
				else
					cout << " -";
			}

			cout << endl;
		}

		cout << endl;
	}
};

void printBitboard(uint64_t bits)
{
	for (int i = 0; i < 64; i++)
	{
		if (i > 0 && i % 8 == 0)
			cout << endl;

		cout << getBit(bits, i) << " ";
	}
}

int main()
{
	Board board;

	board.init();

	board.print();

	return 0;
}

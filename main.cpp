#include <stdio.h>
#include <iostream>
#include <cstring>
#include <map>

using namespace std;

#define startPosition "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1 "
#define trickyPosition "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1 "
#define killerPosition "rnbqkb1r/pp1p1pPp/8/2p1pP2/1P1P4/3P3P/P1P1P3/RNBQKBNR w KQkq e6 0 1"

enum Square {
    a8, b8, c8, d8, e8, f8, g8, h8,
    a7, b7, c7, d7, e7, f7, g7, h7,
    a6, b6, c6, d6, e6, f6, g6, h6,
    a5, b5, c5, d5, e5, f5, g5, h5,
    a4, b4, c4, d4, e4, f4, g4, h4,
    a3, b3, c3, d3, e3, f3, g3, h3,
    a2, b2, c2, d2, e2, f2, g2, h2,
    a1, b1, c1, d1, e1, f1, g1, h1, noSq
};

enum Side { WHITE, BLACK, BOTH };

enum Piece { P, N, B, R, Q, K, p, n, b, r, q, k };

enum CastlingRights { WK = 1, WQ = 2, BK = 4, BQ = 8 };

const char *asciiPieces = "PNBRQKpnbrqk";

const char* notation[] = {
	"a8", "b8", "c8", "d8", "e8", "f8", "g8", "h8",
	"a7", "b7", "c7", "d7", "e7", "f7", "g7", "h7",
	"a6", "b6", "c6", "d6", "e6", "f6", "g6", "h6",
	"a5", "b5", "c5", "d5", "e5", "f5", "g5", "h5",
	"a4", "b4", "c4", "d4", "e4", "f4", "g4", "h4",
	"a3", "b3", "c3", "d3", "e3", "f3", "g3", "h3",
	"a2", "b2", "c2", "d2", "e2", "f2", "g2", "h2",
	"a1", "b1", "c1", "d1", "e1", "f1", "g1", "h1"
};

map<char, int> charToPiece = {
    {'P', P},
    {'N', N},
    {'B', B},
    {'R', R},
    {'Q', Q},
    {'K', K},
    {'p', p},
    {'n', n},
    {'b', b},
    {'r', r},
    {'q', q},
    {'k', k},
};

class Bitboard {
private:
	uint64_t bits;
	
public:
	Bitboard() { bits = 0ULL; }
	
	int getBit(int bit) { return (bits >> bit) & 1ULL; }
	void setBit(int bit) { bits |= 1ULL << bit; }
	void popBit(int bit) { bits &= ~(1ULL << bit); }
	
	uint64_t getBits() { return bits; }
	void setBits(uint64_t value) { bits = value; }
	
	void ORBits(uint64_t value) { bits |= value; }
	void ANDBits(uint64_t value) { bits &= value; }
	
	inline int countBits()
	{
		int count = 0;
		
		uint64_t bitsSave = bits;
		
		while (bits)
		{
			count++;
			bits &= bits - 1;
		}
		
		bits = bitsSave;
		
		return count;
	}
	
	inline int getLSB()
	{
		int index = 0;

		uint64_t bitsSave = bits;
		
		while ((bits & 1) == 0)
		{
			index++;
			bits >>= 1;
		}
		
		bits = bitsSave;

		return index;
	}
	
	void print()
	{
		for (int r = 0; r < 8; r++)
		{
			printf("%i ", 8 - r);
		
			for (int f = 0; f < 8; f++)
				printf("%i ", getBit(r * 8 + f));
			
			printf("\n");
		}
		
		printf("  a b c d e f g h\n\n");
	}
};

class Board {
private:
    Bitboard bitboards[12];
    Bitboard occupancy[3];
    
    int side;
    int castling;
    int enpassant;
	int ply;
    
public:
	Board()
	{
		side = WHITE;
		castling = 0;
		enpassant = noSq;
		ply = 0;

		memset(bitboards, 0, sizeof(bitboards));
		memset(occupancy, 0, sizeof(occupancy));
	}
	
	void print()
	{	
		for (int sq = 0; sq < 64; sq++)
		{
			bool noPiece = true;
		
			if (sq % 8 == 0)
				cout << endl << 8 - (sq / 8) << " ";
			
			for (int piece = P; piece <= k; piece++)
			{
				if (bitboards[piece].getBit(sq))
				{
					cout << asciiPieces[piece] << " ";
					noPiece = false;
					break;
				}
			}
			
			if (noPiece)
				cout << ". ";
		}
		
		cout << endl << "  a b c d e f g h" << endl << endl;
	}

	void parseFen(char* fen)
	{
		side = 0;
		enpassant = noSq;
		castling = 0;
		ply = 0;

		memset(bitboards, 0, sizeof(bitboards));
		memset(occupancy, 0, sizeof(occupancy));

		for (int rank = 0; rank < 8; rank++)
		{
			for (int file = 0; file < 8; file++)
			{
				int square = rank * 8 + file;

				if ((*fen >= 'a' && *fen <= 'z') || (*fen >= 'A' && *fen <= 'Z'))
				{
					int piece = charToPiece[*fen];
					bitboards[piece].setBit(square);
					fen++;
				}

				if (*fen >= '0' && *fen <= '9')
				{
					int offset = *fen - '0';
					int target_piece = -1;

					for (int piece = P; piece <= k; piece++)
						if (bitboards[piece].getBit(square))
							target_piece = piece;

					if (target_piece == -1)
						file--;

					file += offset;

					fen++;
				}

				if (*fen == '/')
					*fen++;
			}
		}

		// go to side to move
		fen++;

		(*fen == 'w') ? (side = WHITE) : (side = BLACK);

		// go to castling rights
		fen += 2;

		while (*fen != ' ')
		{
			switch (*fen)
			{
			case 'K': castling |= WK; break;
			case 'Q': castling |= WQ; break;
			case 'k': castling |= BK; break;
			case 'q': castling |= BQ; break;
			case '-': break;
			}

			fen++;
		}

		// go to enpassant square
		fen++;

		if (*fen != '-')
		{
			int file = fen[0] - 'a';
			int rank = 8 - (fen[1] - '0');

			enpassant = rank * 8 + file;
		}
		else
			enpassant = noSq;

		for (int piece = P; piece <= K; piece++)
			occupancy[WHITE].ORBits(bitboards[piece].getBits());

		for (int piece = p; piece <= k; piece++)
			occupancy[BLACK].ORBits(bitboards[piece].getBits());

		occupancy[BOTH].setBits(occupancy[WHITE].getBits() | occupancy[BLACK].getBits());

		//hashKey = generate_hash_key();
	}

    Bitboard* getBitboard(int piece) { return &bitboards[piece]; }
    Bitboard* getOccupancy(int side) { return &occupancy[side]; }
    
    int getSide() { return side; }
	int getCastling() { return castling; }
	int getPly() { return ply; }
	int getEnpassant() { return enpassant; }
};

class MoveList {
private:
	int moves[256];
	int count;
	
public:
	inline int getMove(int index) { return moves[index]; }
	inline int getCount() { return count; }
	
	inline void addMove(int move) { moves[count++] = move; }
};

class MoveGenerator {
private:
	const uint64_t NOT_A_FILE = 18374403900871474942ULL;
	const uint64_t NOT_H_FILE = 9187201950435737471ULL;
	const uint64_t NOT_GH_FILE = 4557430888798830399ULL;
	const uint64_t NOT_AB_FILE = 18229723555195321596ULL;
	
	Bitboard pawnAttacks[2][64];
	Bitboard knightAttacks[64];
	
public:
	MoveGenerator()
	{
		for (int side = 0; side < 2; side++)
			for (int sq = 0; sq < 64; sq++)
				pawnAttacks[side][sq] = maskPawnAttacks(side, sq);
				
		for (int sq = 0; sq < 64; sq++)
			knightAttacks[sq] = maskKnightAttacks(sq);
	}

	inline Bitboard getPawnAttacks(int side, int sq) { return pawnAttacks[side][sq]; }
	inline Bitboard getKnightAttacks(int sq) { return knightAttacks[sq]; }

	inline void generateMoves(Board board, MoveList* moves)
	{
		Bitboard* bitboard;
		
		int sq = 0, tSq = 0;
	
		for (int piece = P; piece <= k; piece++)
		{
			if (board.getSide() == WHITE)
			{
				if (piece == P)
				{
					bitboard = board.getBitboard(P);
					
					while (bitboard->getBits())
					{
						sq = bitboard->getLSB();
						tSq = sq - 8;
						
						if (!board.getOccupancy(BOTH)->getBit(tSq))
						{
							if (sq >= a7 && sq <= h7)
							{
								cout << "Pawn Promotion" << endl;
							}
							else
							{
								cout << "Pawn move: " << notation[sq] << notation[tSq] << endl;
								
								if (!board.getOccupancy(BOTH)->getBit(tSq - 8))
									cout << "Double pawn move: " << notation[sq] << notation[tSq - 8] << endl;
							}
						}
												
						bitboard->popBit(sq);
					}
				}
			}
		}
	}

	Bitboard maskPawnAttacks(int side, int sq)
	{
		Bitboard attacks, bitboard;
		
		bitboard.setBit(sq);
		
		if (side == WHITE)
		{
			attacks.ORBits((bitboard.getBits() >> 7) & NOT_A_FILE);
			attacks.ORBits((bitboard.getBits() >> 9) & NOT_H_FILE);
		}
		else
		{
			attacks.ORBits((bitboard.getBits() << 7) & NOT_H_FILE);
			attacks.ORBits((bitboard.getBits() << 9) & NOT_A_FILE);
		}
		
		return attacks;
	}
	
	Bitboard maskKnightAttacks(int sq)
	{
		Bitboard attacks, bitboard;
		
		bitboard.setBit(sq);
		
		attacks.ORBits((bitboard.getBits() << 6) & NOT_GH_FILE);
		attacks.ORBits((bitboard.getBits() << 10) & NOT_AB_FILE);
		attacks.ORBits((bitboard.getBits() << 15) & NOT_H_FILE);
		attacks.ORBits((bitboard.getBits() << 17) & NOT_A_FILE);
		attacks.ORBits((bitboard.getBits() >> 6) & NOT_AB_FILE);
		attacks.ORBits((bitboard.getBits() >> 10) & NOT_GH_FILE);
		attacks.ORBits((bitboard.getBits() >> 15) & NOT_A_FILE);
		attacks.ORBits((bitboard.getBits() >> 17) & NOT_H_FILE);
		
		return attacks;
	}
};

int main()
{
	Board board;
	MoveGenerator movegen;

	MoveList moves[1];

	board.parseFen(startPosition);
	board.print();
	movegen.generateMoves(board, moves);

	return 0;
}

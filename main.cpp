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

static inline int countSetBits(uint64_t bits)
{
	int count = 0;

	while (bits)
	{
		count++;
		bits &= bits - 1;
	}

	return count;
}

static inline int getLSBIndex(uint64_t bits)
{
	if (bits)
		return countSetBits((bits & (0 - bits)) - 1);
	else
		return -1;
}

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
		cout << endl << endl;
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

	cout << endl << endl;
}

class MoveGenerator {
public:
	const uint64_t notAFile = 0xfefefefefefefefe;
	const uint64_t notHFile = 0x7f7f7f7f7f7f7f7f;
	const uint64_t notHGFile = 4557430888798830399ULL;
	const uint64_t notABFile = 18229723555195321596ULL;
	const uint64_t rank4 = 0x00000000FF000000;
	const uint64_t rank5 = 0x000000FF00000000;

	const int bishopRelevantBits[64] = {
		6, 5, 5, 5, 5, 5, 5, 6,
		5, 5, 5, 5, 5, 5, 5, 5,
		5, 5, 7, 7, 7, 7, 5, 5,
		5, 5, 7, 9, 9, 7, 5, 5,
		5, 5, 7, 9, 9, 7, 5, 5,
		5, 5, 7, 7, 7, 7, 5, 5,
		5, 5, 5, 5, 5, 5, 5, 5,
		6, 5, 5, 5, 5, 5, 5, 6
	};

	const int rookRelevantBits[64] = {
		12, 11, 11, 11, 11, 11, 11, 12,
		11, 10, 10, 10, 10, 10, 10, 11,
		11, 10, 10, 10, 10, 10, 10, 11,
		11, 10, 10, 10, 10, 10, 10, 11,
		11, 10, 10, 10, 10, 10, 10, 11,
		11, 10, 10, 10, 10, 10, 10, 11,
		11, 10, 10, 10, 10, 10, 10, 11,
		12, 11, 11, 11, 11, 11, 11, 12
	};

	// rook magic numbers
	uint64_t rookMagicNumbers[64] = {
		0x8a80104000800020ULL,
		0x140002000100040ULL,
		0x2801880a0017001ULL,
		0x100081001000420ULL,
		0x200020010080420ULL,
		0x3001c0002010008ULL,
		0x8480008002000100ULL,
		0x2080088004402900ULL,
		0x800098204000ULL,
		0x2024401000200040ULL,
		0x100802000801000ULL,
		0x120800800801000ULL,
		0x208808088000400ULL,
		0x2802200800400ULL,
		0x2200800100020080ULL,
		0x801000060821100ULL,
		0x80044006422000ULL,
		0x100808020004000ULL,
		0x12108a0010204200ULL,
		0x140848010000802ULL,
		0x481828014002800ULL,
		0x8094004002004100ULL,
		0x4010040010010802ULL,
		0x20008806104ULL,
		0x100400080208000ULL,
		0x2040002120081000ULL,
		0x21200680100081ULL,
		0x20100080080080ULL,
		0x2000a00200410ULL,
		0x20080800400ULL,
		0x80088400100102ULL,
		0x80004600042881ULL,
		0x4040008040800020ULL,
		0x440003000200801ULL,
		0x4200011004500ULL,
		0x188020010100100ULL,
		0x14800401802800ULL,
		0x2080040080800200ULL,
		0x124080204001001ULL,
		0x200046502000484ULL,
		0x480400080088020ULL,
		0x1000422010034000ULL,
		0x30200100110040ULL,
		0x100021010009ULL,
		0x2002080100110004ULL,
		0x202008004008002ULL,
		0x20020004010100ULL,
		0x2048440040820001ULL,
		0x101002200408200ULL,
		0x40802000401080ULL,
		0x4008142004410100ULL,
		0x2060820c0120200ULL,
		0x1001004080100ULL,
		0x20c020080040080ULL,
		0x2935610830022400ULL,
		0x44440041009200ULL,
		0x280001040802101ULL,
		0x2100190040002085ULL,
		0x80c0084100102001ULL,
		0x4024081001000421ULL,
		0x20030a0244872ULL,
		0x12001008414402ULL,
		0x2006104900a0804ULL,
		0x1004081002402ULL
	};

	uint64_t bishopMagicNumbers[64] = {
		0x40040844404084ULL,
		0x2004208a004208ULL,
		0x10190041080202ULL,
		0x108060845042010ULL,
		0x581104180800210ULL,
		0x2112080446200010ULL,
		0x1080820820060210ULL,
		0x3c0808410220200ULL,
		0x4050404440404ULL,
		0x21001420088ULL,
		0x24d0080801082102ULL,
		0x1020a0a020400ULL,
		0x40308200402ULL,
		0x4011002100800ULL,
		0x401484104104005ULL,
		0x801010402020200ULL,
		0x400210c3880100ULL,
		0x404022024108200ULL,
		0x810018200204102ULL,
		0x4002801a02003ULL,
		0x85040820080400ULL,
		0x810102c808880400ULL,
		0xe900410884800ULL,
		0x8002020480840102ULL,
		0x220200865090201ULL,
		0x2010100a02021202ULL,
		0x152048408022401ULL,
		0x20080002081110ULL,
		0x4001001021004000ULL,
		0x800040400a011002ULL,
		0xe4004081011002ULL,
		0x1c004001012080ULL,
		0x8004200962a00220ULL,
		0x8422100208500202ULL,
		0x2000402200300c08ULL,
		0x8646020080080080ULL,
		0x80020a0200100808ULL,
		0x2010004880111000ULL,
		0x623000a080011400ULL,
		0x42008c0340209202ULL,
		0x209188240001000ULL,
		0x400408a884001800ULL,
		0x110400a6080400ULL,
		0x1840060a44020800ULL,
		0x90080104000041ULL,
		0x201011000808101ULL,
		0x1a2208080504f080ULL,
		0x8012020600211212ULL,
		0x500861011240000ULL,
		0x180806108200800ULL,
		0x4000020e01040044ULL,
		0x300000261044000aULL,
		0x802241102020002ULL,
		0x20906061210001ULL,
		0x5a84841004010310ULL,
		0x4010801011c04ULL,
		0xa010109502200ULL,
		0x4a02012000ULL,
		0x500201010098b028ULL,
		0x8040002811040900ULL,
		0x28000010020204ULL,
		0x6000020202d0240ULL,
		0x8918844842082200ULL,
		0x4010011029020020ULL
	};

	uint64_t pawnMoves[2][64];
	uint64_t knightMoves[64];
	uint64_t kingMoves[64];
	uint64_t bishopMoves[64][512];
	uint64_t rookMoves[64][4096];
	uint64_t rookMasks[64];
	uint64_t bishopMasks[64];

	void initMasks()
	{
		for (int i = 0; i < 64; i++)
		{
			pawnMoves[white][i] = maskPawnMoves(i, white);
			pawnMoves[black][i] = maskPawnMoves(i, black);
			//knightMoves[i] = maskKnightAttacks(i);
			//kingMoves[i] = maskKingAttacks(i);
		}
	}

	void initSliderMoves(bool isBishop)
	{
		for (int i = 0; i < 64; i++)
		{
			bishopMasks[i] = maskBishopAttacks(i);
			rookMasks[i] = maskRookAttacks(i);

			uint64_t attackMask = isBishop ? bishopMasks[i] : rookMasks[i];

			int relevantBitsCount = countSetBits(attackMask);
			int occupancyIndices = 1 << relevantBitsCount;

			for (int index = 0; index < occupancyIndices; index++)
			{
				if (isBishop)
				{
					uint64_t occupancy = setOccupancy(index, relevantBitsCount, attackMask);
					int magicIndex = (occupancy * bishopMagicNumbers[i]) >> (64 - bishopRelevantBits[i]);

					bishopMoves[i][magicIndex] = bishopAttacksOTF(i, occupancy);
				}
				else
				{
					uint64_t occupancy = setOccupancy(index, relevantBitsCount, attackMask);
					int magicIndex = (occupancy * rookMagicNumbers[i]) >> (64 - rookRelevantBits[i]);

					rookMoves[i][magicIndex] = rookAttacksOTF(i, occupancy);
				}
			}
		}
	}

	inline uint64_t getBishopAttacks(int square, uint64_t occupancy)
	{
		occupancy &= bishopMasks[square];
		occupancy *= bishopMagicNumbers[square];
		occupancy >>= 64 - bishopRelevantBits[square];

		return bishopMoves[square][occupancy];
	}

	inline uint64_t getRookAttacks(int square, uint64_t occupancy)
	{
		occupancy &= rookMasks[square];
		occupancy *= rookMagicNumbers[square];
		occupancy >>= 64 - rookRelevantBits[square];

		return rookMoves[square][occupancy];
	}
	
	uint64_t maskPawnMoves(int square, int side)
	{
		uint64_t bits = 0ULL;
		uint64_t attacks = 0ULL;

		setBit(&bits, square);

		if (side == white)
		{
			attacks |= (bits >> 7) & notAFile;
			attacks |= (bits >> 9) & notHFile;
			attacks |= bits >> 8;
			attacks |= (bits >> 16) & rank5;
		}
		else
		{
			attacks |= (bits << 7) & notHFile;
			attacks |= (bits << 9) & notAFile;
			attacks |= bits << 8;
			attacks |= (bits << 16) & rank4;
		}

		return attacks;
	}

	uint64_t maskKnightAttacks(int square)
	{
		uint64_t bits = 0ULL;
		uint64_t attacks = 0ULL;

		setBit(&bits, square);

		attacks |= (bits >> 6) & notABFile;
		attacks |= (bits >> 10) & notHGFile;
		attacks |= (bits >> 15) & notAFile;
		attacks |= (bits >> 17) & notHFile;
		attacks |= (bits << 6) & notHGFile;
		attacks |= (bits << 10) & notABFile;
		attacks |= (bits << 15) & notHFile;
		attacks |= (bits << 17) & notAFile;

		return attacks;
	}

	uint64_t maskKingAttacks(int square)
	{
		uint64_t bits = 0ULL;
		uint64_t attacks = 0ULL;

		setBit(&bits, square);

		attacks |= (bits << 1) & notAFile;
		attacks |= (bits << 7) & notHFile;
		attacks |= (bits << 8);
		attacks |= (bits << 9) & notAFile;
		attacks |= (bits >> 1) & notHFile;
		attacks |= (bits >> 7) & notAFile;
		attacks |= (bits >> 8);
		attacks |= (bits >> 9) & notHFile;

		return attacks;
	}

	uint64_t maskBishopAttacks(int square)
	{
		uint64_t attacks = 0ULL;

		int r, f;

		int tr = square / 8;
		int tf = square % 8;

		for (r = tr + 1, f = tf + 1; r <= 6 && f <= 6; r++, f++) attacks |= (1ULL << (r * 8 + f));
		for (r = tr - 1, f = tf + 1; r >= 1 && f <= 6; r--, f++) attacks |= (1ULL << (r * 8 + f));
		for (r = tr + 1, f = tf - 1; r <= 6 && f >= 1; r++, f--) attacks |= (1ULL << (r * 8 + f));
		for (r = tr - 1, f = tf - 1; r >= 1 && f >= 1; r--, f--) attacks |= (1ULL << (r * 8 + f));

		return attacks;
	}

	uint64_t maskRookAttacks(int square)
	{
		uint64_t attacks = 0ULL;

		int r, f;

		int tr = square / 8;
		int tf = square % 8;

		for (r = tr + 1; r <= 6; r++) attacks |= (1ULL << (r * 8 + tf));
		for (r = tr - 1; r >= 1; r--) attacks |= (1ULL << (r * 8 + tf));
		for (f = tf + 1; f <= 6; f++) attacks |= (1ULL << (tr * 8 + f));
		for (f = tf - 1; f >= 1; f--) attacks |= (1ULL << (tr * 8 + f));

		return attacks;
	}

	uint64_t bishopAttacksOTF(int square, uint64_t blockers)
	{
		uint64_t attacks = 0ULL;

		int r, f;

		int tr = square / 8;
		int tf = square % 8;

		for (r = tr + 1, f = tf + 1; r <= 7 && f <= 7; r++, f++)
		{
			attacks |= (1ULL << (r * 8 + f));
			if ((1ULL << (r * 8 + f)) & blockers) break;
		}
		for (r = tr - 1, f = tf + 1; r >= 0 && f <= 7; r--, f++)
		{
			attacks |= (1ULL << (r * 8 + f));
			if ((1ULL << (r * 8 + f)) & blockers) break;
		}
		for (r = tr + 1, f = tf - 1; r <= 7 && f >= 0; r++, f--)
		{
			attacks |= (1ULL << (r * 8 + f));
			if ((1ULL << (r * 8 + f)) & blockers) break;
		}
		for (r = tr - 1, f = tf - 1; r >= 0 && f >= 0; r--, f--)
		{
			attacks |= (1ULL << (r * 8 + f));
			if ((1ULL << (r * 8 + f)) & blockers) break;
		}

		return attacks;
	}

	uint64_t rookAttacksOTF(int square, uint64_t blockers)
	{
		uint64_t attacks = 0ULL;

		int r, f;

		int tr = square / 8;
		int tf = square % 8;

		for (r = tr + 1; r <= 7; r++)
		{
			attacks |= (1ULL << (r * 8 + tf));
			if ((1ULL << (r * 8 + tf)) & blockers) break;
		}
		for (r = tr - 1; r >= 0; r--)
		{
			attacks |= (1ULL << (r * 8 + tf));
			if ((1ULL << (r * 8 + tf)) & blockers) break;
		}
		for (f = tf + 1; f <= 7; f++)
		{
			attacks |= (1ULL << (tr * 8 + f));
			if ((1ULL << (tr * 8 + f)) & blockers) break;
		}
		for (f = tf - 1; f >= 0; f--)
		{
			attacks |= (1ULL << (tr * 8 + f));
			if ((1ULL << (tr * 8 + f)) & blockers) break;
		}

		return attacks;
	}

	uint64_t setOccupancy(int index, int bitsInMask, uint64_t attackMask)
	{
		uint64_t occupancy = 0ULL;

		for (int i = 0; i < bitsInMask; i++)
		{
			int square = getLSBIndex(attackMask);

			zeroBit(&attackMask, square);

			if (index & (1 << i))
				occupancy |= (1ULL << square);
		}

		return occupancy;
	}
};

unsigned int seed = 1804289383;

unsigned int getRandomU32Number()
{
	unsigned int number = seed;

	number ^= number << 13;
	number ^= number >> 17;
	number ^= number << 5;

	seed = number;

	return number;
}

uint64_t getRandomU64Number()
{
	// define 4 random numbers
	uint64_t n1, n2, n3, n4;

	// init random numbers slicing 16 bits from MS1B side
	n1 = (uint64_t)(getRandomU32Number()) & 0xFFFF;
	n2 = (uint64_t)(getRandomU32Number()) & 0xFFFF;
	n3 = (uint64_t)(getRandomU32Number()) & 0xFFFF;
	n4 = (uint64_t)(getRandomU32Number()) & 0xFFFF;

	// return random number
	return n1 | (n2 << 16) | (n3 << 32) | (n4 << 48);
}

int main()
{
	Board board;
	MoveGenerator moveGen;

	board.init();
	moveGen.initMasks();
	//moveGen.initSliderMoves(true);
	//moveGen.initSliderMoves(false);

	//setBit(&board.bits[P], e2);

	//printBitboard(moveGen.getBishopAttacks(d4, 0ULL));

	board.print();

	return 0;
}

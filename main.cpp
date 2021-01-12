#include <iostream>
#include <cstring>
#include <map>
#include <sys/time.h>

using namespace std;

#define setBit(bb, bit) (bb |= (1ULL << bit))
#define popBit(bb, bit) (bb &= ~(1ULL << bit))
#define getBit(bb, bit) ((bb >> bit) & 1ULL)

#define Move(source, target, piece, promoted, capture, doublePawn, enpassant, castling) \
    (source) |          \
    (target << 6) |     \
    (piece << 12) |     \
    (promoted << 16) |  \
    (capture << 20) |   \
    (doublePawn << 21) | \
    (enpassant << 22) | \
    (castling << 23)    \

#define getSource(move) (move & 0x3f)
#define getTarget(move) ((move & 0xfc0) >> 6)
#define getPiece(move) ((move & 0xf000) >> 12)
#define getPromoted(move) ((move & 0xf0000) >> 16)
#define getCapture(move) (move & 0x100000)
#define getDouble(move) (move & 0x200000)
#define getEnpassant(move) (move & 0x400000)
#define getCastling(move) (move & 0x800000)

enum Side { WHITE, BLACK, BOTH };
enum Pieces { P, N, B, R, Q, K, p, n, b, r, q, k };
enum Castling { WK = 1, WQ = 2, BK = 4, BQ = 8 };

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

class Position {
public:
	int side;
	int ca;
	int ep;

	uint64_t hashKey;
	uint64_t bb[12];
	uint64_t occ[3];

	int ply;

	int pvTable[64][64];
	int pvLength[64];
	
	void reset()
	{
		side = 0;
		ca = 0;
		ep = noSq;
		hashKey = 0ULL;

		memset(bb, 0, sizeof(bb));
		memset(occ, 0, sizeof(occ));

		ply = 0;
	}
};

class SearchInfo {
public:
	long nodes;
	
	// UCI variables
	bool quit = 0;
	int movestogo = 30;
	int movetime = -1;
	int tTime = -1;
	int inc = 0;
	int starttime = 0;
	int stoptime = 0;
	int timeset = 0;
	bool stopped = 0;
};

class MoveList {
public:
	int moves[256];
	int count;
};

class Undo {
public:
	uint64_t bb[64][12];
	uint64_t occ[64][3];

	int ca[64];
	int ep[64];
	int side[64];
};

map<int, char> pieceToChar = {
    {P, 'P'},
    {N, 'N'},
    {B, 'B'},
    {R, 'R'},
    {Q, 'Q'},
    {K, 'K'},
    {p, 'p'},
    {n, 'n'},
    {b, 'b'},
    {r, 'r'},
    {q, 'q'},
    {k, 'k'},
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

string unicodePieces[12] = {"♙", "♘", "♗", "♖", "♕", "♔", "♟︎", "♞", "♝", "♜", "♛", "♚"};

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

// castling rights update constants
const int castlingRights[64] = {
     7, 15, 15, 15,  3, 15, 15, 11,
    15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15,
    13, 15, 15, 15, 12, 15, 15, 14
};

const uint64_t NOT_A_FILE = 18374403900871474942ULL;
const uint64_t NOT_H_FILE = 9187201950435737471ULL;
const uint64_t NOT_GH_FILE = 4557430888798830399ULL;
const uint64_t NOT_AB_FILE = 18229723555195321596ULL;

int materialScore[12] = {
    100,      // white pawn score
    300,      // white knight score
    325,      // white bishop score
    500,      // white rook score
    900,      // white queen score
  20000,      // white king score
   -100,      // black pawn score
   -300,      // black knight score
   -325,      // black bishop score
   -500,      // black rook score
   -900,      // black queen score
 -20000,      // black king score
};

const int pawnScore[64] = 
{
    0,  0,  0,  0,  0,  0,  0,  0,
	50, 50, 50, 50, 50, 50, 50, 50,
	10, 10, 20, 30, 30, 20, 10, 10,
 	5,  5, 10, 25, 25, 10,  5,  5,
 	0,  0,  0, 20, 20,  0,  0,  0,
 	5, -5,-10,  0,  0,-10, -5,  5,
 	5, 10, 10,-20,-20, 10, 10,  5,
 	0,  0,  0,  0,  0,  0,  0,  0
};

const int knightScore[64] = 
{
    -50,-40,-30,-30,-30,-30,-40,-50,
	-40,-20,  0,  0,  0,  0,-20,-40,
	-30,  0, 10, 15, 15, 10,  0,-30,
	-30,  5, 15, 20, 20, 15,  5,-30,
	-30,  0, 15, 20, 20, 15,  0,-30,
	-30,  5, 10, 15, 15, 10,  5,-30,
	-40,-20,  0,  5,  5,  0,-20,-40,
	-50,-40,-30,-30,-30,-30,-40,-50
};

const int bishopScore[64] = 
{
    -20,-10,-10,-10,-10,-10,-10,-20,
	-10,  0,  0,  0,  0,  0,  0,-10,
	-10,  0,  5, 10, 10,  5,  0,-10,
	-10,  5,  5, 10, 10,  5,  5,-10,
	-10,  0, 10, 10, 10, 10,  0,-10,
	-10, 10, 10, 10, 10, 10, 10,-10,
	-10,  5,  0,  0,  0,  0,  5,-10,
	-20,-10,-10,-10,-10,-10,-10,-20

};

const int rookScore[64] =
{
     0,  0,  0,  0,  0,  0,  0,  0,
 	 5, 10, 10, 10, 10, 10, 10,  5,
	-5,  0,  0,  0,  0,  0,  0, -5,
 	-5,  0,  0,  0,  0,  0,  0, -5,
 	-5,  0,  0,  0,  0,  0,  0, -5,
	-5,  0,  0,  0,  0,  0,  0, -5,
 	-5,  0,  0,  0,  0,  0,  0, -5,
 	 0,  0,  0,  5,  5,  0,  0,  0

};

const int queenScore[64] = 
{
	-20,-10,-10, -5, -5,-10,-10,-20,
	-10,  0,  0,  0,  0,  0,  0,-10,
	-10,  0,  5,  5,  5,  5,  0,-10,
 	-5,  0,  5,  5,  5,  5,  0, -5,
	 0,  0,  5,  5,  5,  5,  0, -5,
	-10,  5,  5,  5,  5,  5,  0,-10,
	-10,  0,  5,  0,  0,  0,  0,-10,
	-20,-10,-10, -5, -5,-10,-10,-20
};

const int kingScore[64] = 
{
    -30,-40,-40,-50,-50,-40,-40,-30,
	-30,-40,-40,-50,-50,-40,-40,-30,
	-30,-40,-40,-50,-50,-40,-40,-30,
	-30,-40,-40,-50,-50,-40,-40,-30,
	-20,-30,-30,-40,-40,-30,-30,-20,
	-10,-20,-20,-20,-20,-20,-20,-10,
	 20, 20,  0,  0,  0,  0, 20, 20,
	 20, 30, 10,  0,  0, 10, 30, 20
};

const int mirrorScore[64] =
{
	a1, b1, c1, d1, e1, f1, g1, h1,
	a2, b2, c2, d2, e2, f2, g2, h2,
	a3, b3, c3, d3, e3, f3, g3, h3,
	a4, b4, c4, d4, e4, f4, g4, h4,
	a5, b5, c5, d5, e5, f5, g5, h5,
	a6, b6, c6, d6, e6, f6, g6, h6,
	a7, b7, c7, d7, e7, f7, g7, h7,
	a8, b8, c8, d8, e8, f8, g8, h8
};

static int MVV_LVA[12][12] = {
 	105, 205, 305, 405, 505, 605,  105, 205, 305, 405, 505, 605,
	104, 204, 304, 404, 504, 604,  104, 204, 304, 404, 504, 604,
	103, 203, 303, 403, 503, 603,  103, 203, 303, 403, 503, 603,
	102, 202, 302, 402, 502, 602,  102, 202, 302, 402, 502, 602,
	101, 201, 301, 401, 501, 601,  101, 201, 301, 401, 501, 601,
	100, 200, 300, 400, 500, 600,  100, 200, 300, 400, 500, 600,

	105, 205, 305, 405, 505, 605,  105, 205, 305, 405, 505, 605,
	104, 204, 304, 404, 504, 604,  104, 204, 304, 404, 504, 604,
	103, 203, 303, 403, 503, 603,  103, 203, 303, 403, 503, 603,
	102, 202, 302, 402, 502, 602,  102, 202, 302, 402, 502, 602,
	101, 201, 301, 401, 501, 601,  101, 201, 301, 401, 501, 601,
	100, 200, 300, 400, 500, 600,  100, 200, 300, 400, 500, 600
};

const string notation[] = {
    "a8", "b8", "c8", "d8", "e8", "f8", "g8", "h8",
    "a7", "b7", "c7", "d7", "e7", "f7", "g7", "h7",
    "a6", "b6", "c6", "d6", "e6", "f6", "g6", "h6",
    "a5", "b5", "c5", "d5", "e5", "f5", "g5", "h5",
    "a4", "b4", "c4", "d4", "e4", "f4", "g4", "h4",
    "a3", "b3", "c3", "d3", "e3", "f3", "g3", "h3",
    "a2", "b2", "c2", "d2", "e2", "f2", "g2", "h2",
    "a1", "b1", "c1", "d1", "e1", "f1", "g1", "h1",
};

char *startPosition = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1 ";
char *trickyPosition = "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1 ";
char *killerPosition = "rnbqkb1r/pp1p1pPp/8/2p1pP2/1P1P4/3P3P/P1P1P3/RNBQKBNR w KQkq e6 0 1";

uint64_t zobrist[12][64];
uint64_t castleKeys[16];
uint64_t epKeys[64];
uint64_t sideKey;

uint64_t pawnAttacks[2][64];
uint64_t knightAttacks[64];
uint64_t kingAttacks[64];
uint64_t rookMasks[64];
uint64_t bishopMasks[64];
uint64_t rookAttacks[64][4096];
uint64_t bishopAttacks[64][512];

Undo undo[1];
Position pos[1];
SearchInfo sInfo[1];

int getTimeMS()
{
	struct timeval time_value;
    gettimeofday(&time_value, NULL);
    return time_value.tv_sec * 1000 + time_value.tv_usec / 1000;
}

void PrintBitboard(uint64_t bb)
{
	for (int sq = 0; sq < 64; sq++)
	{
		if (sq % 8 == 0) cout << endl;
		cout << (int)getBit(bb, sq);
	}
	
	cout << endl << endl;
}

void PrintBoard(Position *pos)
{
	for (int i = 0; i < 64; i++)
	{
		if (i % 8 == 0)
			cout << endl << 8 - (i / 8) << " ";
		
		if (getBit(pos->bb[P], i)) cout << unicodePieces[P] << " ";
		else if (getBit(pos->bb[p], i)) cout << unicodePieces[p] << " ";
		else if (getBit(pos->bb[N], i)) cout << unicodePieces[N] << " ";
		else if (getBit(pos->bb[n], i)) cout << unicodePieces[n] << " ";
		else if (getBit(pos->bb[B], i)) cout << unicodePieces[B] << " ";
		else if (getBit(pos->bb[b], i)) cout << unicodePieces[b] << " ";
		else if (getBit(pos->bb[R], i)) cout << unicodePieces[R] << " ";
		else if (getBit(pos->bb[r], i)) cout << unicodePieces[r] << " ";
		else if (getBit(pos->bb[Q], i)) cout << unicodePieces[Q] << " ";
		else if (getBit(pos->bb[q], i)) cout << unicodePieces[q] << " ";
		else if (getBit(pos->bb[K], i)) cout << unicodePieces[K] << " ";
		else if (getBit(pos->bb[k], i)) cout << unicodePieces[k] << " ";
		else cout << ". ";
	}

	cout << endl << "  a b c d e f g h" << endl;

	cout << endl << "Side: " << (pos->side == WHITE ? "white" : "black") << endl;
	cout << "Enpassant: " << ((pos->ep != noSq) ? notation[pos->ep] : "no") << endl;
	cout << "Castling: " << ((pos->ca & WK) ? 'K' : '-') << ((pos->ca & WQ) ? 'Q' : '-')
		 << ((pos->ca & BK) ? 'k' : '-') << ((pos->ca & BQ) ? 'q' : '-') << endl;
	cout << "Hash Key: " << (uint64_t)pos->hashKey << "ULL" << endl << endl;
}

static inline int CountBits(uint64_t bb)
{
	int count = 0;
	
	while (bb)
	{
		bb &= bb - 1;
		count++;
	}
	
	return count;
}

static inline int GetLSB(uint64_t bb)
{
	if (bb)
		return CountBits((bb & -bb) - 1);
	else
		return -1;
}

uint64_t SetOccupancy(int index, int bitsInMask, uint64_t attackMask)
{
	uint64_t occupancy = 0ULL;

	int square = 0;

	for (int count = 0; count < bitsInMask; count++)
	{
		square = GetLSB(attackMask);
		popBit(attackMask, square);

		if (index & (1 << count))
			occupancy |= (1ULL << square);
	}

	return occupancy;
}

uint64_t MaskPawnAttacks(int side, int square)
{
	uint64_t attacks = 0ULL;
	uint64_t bitboard = 0ULL;
	
	setBit(bitboard, square);
	
	if (side == WHITE)
	{
		attacks |= (bitboard >> 7) & NOT_A_FILE;
		attacks |= (bitboard >> 9) & NOT_H_FILE;
	}
	else
	{
		attacks |= (bitboard << 7) & NOT_H_FILE;
		attacks |= (bitboard << 9) & NOT_A_FILE;
	}
	
	return attacks;
}
uint64_t MaskKnightAttacks(int sqr)
{
	uint64_t bitboard = 0ULL;
	uint64_t attacks = 0ULL;

	setBit(bitboard, sqr);

	attacks |= (bitboard >> 15) & NOT_A_FILE;
	attacks |= (bitboard >> 17) & NOT_H_FILE;
	attacks |= (bitboard >> 10) & NOT_GH_FILE;
	attacks |= (bitboard >> 6) & NOT_AB_FILE;
	attacks |= (bitboard << 15) & NOT_H_FILE;
	attacks |= (bitboard << 17) & NOT_A_FILE;
	attacks |= (bitboard << 10) & NOT_AB_FILE;
	attacks |= (bitboard << 6) & NOT_GH_FILE;

	return attacks;
}

uint64_t MaskKingAttacks(int sqr)
{
	uint64_t bitboard = 0ULL;
	uint64_t attacks = 0ULL;

	setBit(bitboard, sqr);

	attacks |= (bitboard >> 7) & NOT_A_FILE;
	attacks |= bitboard >> 8;
	attacks |= (bitboard >> 9) & NOT_H_FILE;
	attacks |= (bitboard >> 1) & NOT_H_FILE;
	attacks |= (bitboard << 7) & NOT_H_FILE;
	attacks |= bitboard << 8;
	attacks |= (bitboard << 9) & NOT_A_FILE;
	attacks |= (bitboard << 1) & NOT_A_FILE;

	return attacks;
}

uint64_t MaskBishopAttacks(int sqr)
{
	uint64_t attacks = 0ULL;

	int r, f;

	int tr = sqr / 8;
	int tf = sqr % 8;

	for (r = tr + 1, f = tf + 1; r <= 6 && f <= 6; r++, f++) attacks |= (1ULL << (r * 8 + f));
	for (r = tr - 1, f = tf + 1; r >= 1 && f <= 6; r--, f++) attacks |= (1ULL << (r * 8 + f));
	for (r = tr + 1, f = tf - 1; r <= 6 && f >= 1; r++, f--) attacks |= (1ULL << (r * 8 + f));
	for (r = tr - 1, f = tf - 1; r >= 1 && f >= 1; r--, f--) attacks |= (1ULL << (r * 8 + f));

	return attacks;
}

uint64_t MaskRookAttacks(int sqr)
{
	uint64_t attacks = 0ULL;

	int r, f;

	int tr = sqr / 8;
	int tf = sqr % 8;

	for (r = tr + 1; r <= 6; r++) attacks |= (1ULL << (r * 8 + tf));
	for (r = tr - 1; r >= 1; r--) attacks |= (1ULL << (r * 8 + tf));
	for (f = tf + 1; f <= 6; f++) attacks |= (1ULL << (tr * 8 + f));
	for (f = tf - 1; f >= 1; f--) attacks |= (1ULL << (tr * 8 + f));

	return attacks;
}

uint64_t BishopAttacksOTF(int sqr, uint64_t blockers)
{
	uint64_t attacks = 0ULL;

	int r, f;

	int tr = sqr / 8;
	int tf = sqr % 8;

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

uint64_t RookAttacksOTF(int square, uint64_t blockers)
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

static inline uint64_t GetRookAttacks(int sqr, uint64_t occupancy)
{
	occupancy &= rookMasks[sqr];
	occupancy *= rookMagicNumbers[sqr];
	occupancy >>= 64 - rookRelevantBits[sqr];

	return rookAttacks[sqr][occupancy];
}

static inline uint64_t GetBishopAttacks(int sqr, uint64_t occupancy)
{
	occupancy &= bishopMasks[sqr];
	occupancy *= bishopMagicNumbers[sqr];
	occupancy >>= 64 - bishopRelevantBits[sqr];

	return bishopAttacks[sqr][occupancy];
}

static inline uint64_t GetQueenAttacks(int sqr, uint64_t occupancy) 
{ 
	return GetBishopAttacks(sqr, occupancy) | GetRookAttacks(sqr, occupancy); 
}

static inline void AddMove(MoveList* moves, int move)
{
	moves->moves[moves->count] = move;
	moves->count++;
}

static inline bool IsSqAttacked(int square, int side)
{
	if (side == WHITE && (pawnAttacks[BLACK][square] & pos->bb[P])) return true;
	if (side == BLACK && (pawnAttacks[WHITE][square] & pos->bb[p])) return true;
	if (knightAttacks[square] & (side == WHITE ? pos->bb[N] : pos->bb[n])) return true;
	if (kingAttacks[square] & (side == WHITE ? pos->bb[K] : pos->bb[k])) return true;
	if (GetBishopAttacks(square, pos->occ[BOTH]) & (side == WHITE ? pos->bb[B] : pos->bb[b])) return true;
	if (GetRookAttacks(square, pos->occ[BOTH]) & (side == WHITE ? pos->bb[R] : pos->bb[r])) return true;
	if (GetQueenAttacks(square, pos->occ[BOTH]) & ((side == WHITE ? pos->bb[Q] : pos->bb[q]))) return true;

	return false;
}

static inline void CopyBoard()
{
	undo->ep[pos->ply] = pos->ep;
	undo->ca[pos->ply] = pos->ca;
	undo->side[pos->ply] = pos->side;

	memcpy(undo->bb[pos->ply], pos->bb, sizeof(pos->bb));
	memcpy(undo->occ[pos->ply], pos->occ, sizeof(pos->occ));
}

static inline void TakeBack()
{
	pos->ep = undo->ep[pos->ply];
	pos->ca = undo->ca[pos->ply];

	if (pos->side < 0 || pos->side > 2) pos->side ^= 1;

	pos->side = undo->side[pos->ply];

	memcpy(pos->bb, undo->bb[pos->ply], sizeof(pos->bb));
	memcpy(pos->occ, undo->occ[pos->ply], sizeof(pos->occ));
}

static inline void MovePiece(int from, int to, int piece)
{
	popBit(pos->bb[piece], from);
	setBit(pos->bb[piece], to);
	
	pos->hashKey ^= zobrist[piece][from];
	pos->hashKey ^= zobrist[piece][to];
}

static inline int MakeMove(int move)
{
	CopyBoard();

	int fromSquare = getSource(move);
    int toSquare = getTarget(move);
    int piece = getPiece(move);
    int promotedPiece = getPromoted(move);
    int capture = getCapture(move);
    int doublePush = getDouble(move);
    int enpass = getEnpassant(move);
    int castle = getCastling(move);

	MovePiece(fromSquare, toSquare, piece);
	
	if (capture)
	{
		int startPiece, endPiece;

		if (pos->side == WHITE)
		{
			startPiece = p;
			endPiece = k;
		}
		else
		{
			startPiece = P;
			endPiece = K;
		}
			
		for (int bbPiece = startPiece; bbPiece <= endPiece; bbPiece++)
		{
			if (getBit(pos->bb[bbPiece], toSquare))
			{
				popBit(pos->bb[bbPiece], toSquare);
				pos->hashKey ^= zobrist[bbPiece][toSquare];
				break;
			}
		}
	}

	if (promotedPiece)
	{
		(pos->side == WHITE) ? popBit(pos->bb[P], toSquare) : popBit(pos->bb[p], toSquare);
		(pos->side == WHITE) ? pos->hashKey ^= zobrist[P][toSquare] : pos->hashKey ^= zobrist[p][toSquare];
		setBit(pos->bb[promotedPiece], toSquare);
		pos->hashKey ^= zobrist[promotedPiece][toSquare];
	}

	if (enpass)
	{
		pos->side == WHITE ? popBit(pos->bb[p], toSquare + 8) : popBit(pos->bb[P], toSquare - 8);
		pos->side == WHITE ? pos->hashKey ^= zobrist[p][toSquare + 8] : pos->hashKey ^= zobrist[P][toSquare - 8];
	}
	
	if (pos->ep != noSq)
		pos->hashKey ^= epKeys[pos->ep];

	pos->ep = noSq;

	if (doublePush)
	{
		if (pos->side == WHITE)
		{
			pos->ep = toSquare + 8;
			pos->hashKey ^= epKeys[toSquare + 8];
		}
		else
		{
			pos->ep = toSquare - 8;
			pos->hashKey ^= epKeys[toSquare - 8];
		}
	}

	if (castle)
    {
        switch (toSquare)
        {
            // white castles king side
            case g1: MovePiece(h1, f1, R); break;
            // white castles queen side
            case c1: MovePiece(a1, d1, R); break;        
            // black castles king side
            case g8: MovePiece(h8, f8, r); break;
            // black castles queen side
            case c8: MovePiece(a8, d8, r); break;
        }
    }

	pos->ca &= castlingRights[fromSquare];
	pos->ca &= castlingRights[toSquare];
	
	pos->hashKey ^= castleKeys[pos->ca];

    memset(pos->occ, 0, 24);

    for (int bb_piece = P; bb_piece <= K; bb_piece++)
        pos->occ[WHITE] |= pos->bb[bb_piece];

    for (int bb_piece = p; bb_piece <= k; bb_piece++)
        pos->occ[BLACK] |= pos->bb[bb_piece];

	pos->occ[BOTH] = pos->occ[WHITE] | pos->occ[BLACK];

    pos->side ^= 1;
    pos->hashKey ^= sideKey;

	if (IsSqAttacked(pos->side == WHITE ? GetLSB(pos->bb[k]) : GetLSB(pos->bb[K]), pos->side))
	{
		TakeBack();
		return 0;
	}
	else
		return 1;
}

static inline void GenerateMoves(MoveList* moves)
{
	int fromSquare, toSquare;

	uint64_t bitboard = 0ULL;
	uint64_t attacks = 0ULL;

	moves->count = 0;

	for (int piece = P; piece <= k; piece++)
	{
		bitboard = pos->bb[piece];

		if (pos->side == WHITE)
		{
			if (piece == P)
			{
				while (bitboard)
				{
					fromSquare = GetLSB(bitboard);
					toSquare = fromSquare - 8;

					// generate quiet pawn moves
					if (!getBit(pos->occ[BOTH], toSquare))
					{
						// promotion
						if (fromSquare >= a7 && fromSquare <= h7)
						{
							AddMove(moves, Move(fromSquare, toSquare, piece, Q, 0, 0, 0, 0));
							AddMove(moves, Move(fromSquare, toSquare, piece, R, 0, 0, 0, 0));
							AddMove(moves, Move(fromSquare, toSquare, piece, B, 0, 0, 0, 0));
							AddMove(moves, Move(fromSquare, toSquare, piece, N, 0, 0, 0, 0));
						}
						else
						{
							// single pawn push
							AddMove(moves, Move(fromSquare, toSquare, piece, 0, 0, 0, 0, 0));

							// double pawn push
							if ((fromSquare >= a2 && fromSquare <= h2) && !getBit(pos->occ[BOTH], toSquare - 8))
								AddMove(moves, Move(fromSquare, toSquare - 8, piece, 0, 0, 1, 0, 0));
						}
					}

					attacks = pawnAttacks[pos->side][fromSquare] & pos->occ[BLACK];

					// generate pawn captures
					while (attacks)
					{
						toSquare = GetLSB(attacks);

						if (fromSquare >= a7 && fromSquare <= h7)
						{
							AddMove(moves, Move(fromSquare, toSquare, piece, Q, 1, 0, 0, 0));
							AddMove(moves, Move(fromSquare, toSquare, piece, R, 1, 0, 0, 0));
							AddMove(moves, Move(fromSquare, toSquare, piece, B, 1, 0, 0, 0));
							AddMove(moves, Move(fromSquare, toSquare, piece, N, 1, 0, 0, 0));
						}
						else
							AddMove(moves, Move(fromSquare, toSquare, piece, 0, 1, 0, 0, 0));

						popBit(attacks, toSquare);
					}

					// generate enpassant captures
					if (pos->ep != noSq)
					{
						uint64_t enpassant_attacks = pawnAttacks[pos->side][fromSquare] & (1ULL << pos->ep);

						if (enpassant_attacks)
						{
							int target_enpassant = (enpassant_attacks);
							AddMove(moves, Move(fromSquare, target_enpassant, piece, 0, 1, 0, 1, 0));
						}
					}

					popBit(bitboard, fromSquare);
				}
			}
			else if (piece == K)
			{
				if (pos->ca & WK)
				{
					if (!getBit(pos->occ[BOTH], f1) && !getBit(pos->occ[BOTH], g1))
					{
						if (!IsSqAttacked(e1, BLACK) && !IsSqAttacked(f1, BLACK))
							AddMove(moves, Move(e1, g1, piece, 0, 0, 0, 0, 1));
					}
				}
				else if (pos->ca & WQ)
				{
					if (!getBit(pos->occ[BOTH], d1) && !getBit(pos->occ[BOTH], c1) && !getBit(pos->occ[BOTH], b1))
					{
						if (!IsSqAttacked(e1, BLACK) && !IsSqAttacked(d1, BLACK))
							AddMove(moves, Move(e1, c1, piece, 0, 0, 0, 0, 1));
					}
				}
			}
		}
		else
		{
			if (piece == p)
			{
				while (bitboard)
				{
					fromSquare = GetLSB(bitboard);
					toSquare = fromSquare + 8;

					// generate quiet pawn moves
					if (!getBit(pos->occ[BOTH], toSquare))
					{
						if (fromSquare >= a2 && fromSquare <= h2)
						{
							AddMove(moves, Move(fromSquare, toSquare, piece, q, 0, 0, 0, 0));
							AddMove(moves, Move(fromSquare, toSquare, piece, r, 0, 0, 0, 0));
							AddMove(moves, Move(fromSquare, toSquare, piece, b, 0, 0, 0, 0));
							AddMove(moves, Move(fromSquare, toSquare, piece, n, 0, 0, 0, 0));
						}
						else
						{
							// generate single pawn pushes
							AddMove(moves, Move(fromSquare, toSquare, piece, 0, 0, 0, 0, 0));

							// generate double pawn pushes
							if ((fromSquare >= a7 && fromSquare <= h7) && !getBit(pos->occ[BOTH], toSquare + 8))
								AddMove(moves, Move(fromSquare, toSquare + 8, piece, 0, 0, 1, 0, 0));
						}
					}

					attacks = pawnAttacks[pos->side][fromSquare] & pos->occ[WHITE];

					// generate pawn captures
					while (attacks)
					{
						toSquare = GetLSB(attacks);

						if (fromSquare >= a2 && fromSquare <= h2)
						{
							AddMove(moves, Move(fromSquare, toSquare, piece, q, 1, 0, 0, 0));
							AddMove(moves, Move(fromSquare, toSquare, piece, r, 1, 0, 0, 0));
							AddMove(moves, Move(fromSquare, toSquare, piece, b, 1, 0, 0, 0));
							AddMove(moves, Move(fromSquare, toSquare, piece, n, 1, 0, 0, 0));
						}
						else
							AddMove(moves, Move(fromSquare, toSquare, piece, 0, 1, 0, 0, 0));

						popBit(attacks, toSquare);
					}

					// generate enpassant captures
					if (pos->ep != noSq)
					{
						uint64_t enpassant_attacks = pawnAttacks[pos->side][fromSquare] & (1ULL << pos->ep);

						if (enpassant_attacks)
						{
							int target_enpassant = GetLSB(enpassant_attacks);
							AddMove(moves, Move(fromSquare, target_enpassant, piece, 0, 1, 0, 1, 0));
						}
					}

					popBit(bitboard, fromSquare);
				}
			}
			else if (piece == k)
			{
				if (pos->ca & BK)
				{
					if (!getBit(pos->occ[BOTH], f8) && !getBit(pos->occ[BOTH], g8))
					{
						if (!IsSqAttacked(e8, WHITE) && !IsSqAttacked(f8, WHITE))
							AddMove(moves, Move(e8, g8, piece, 0, 0, 0, 0, 1));
					}
				}
				else if (pos->ca & BQ)
				{
					if (!getBit(pos->occ[BOTH], d8) && !getBit(pos->occ[BOTH], c8) && !getBit(pos->occ[BOTH], b8))
					{
						if (!IsSqAttacked(e8, WHITE) && !IsSqAttacked(d8, WHITE))
						{
							AddMove(moves, Move(e8, c8, piece, 0, 0, 0, 0, 1));
						}
					}
				}
			}
		}

		// generate knight moves
		if ((pos->side == WHITE) ? piece == N : piece == n)
		{
			while (bitboard)
			{
				fromSquare = GetLSB(bitboard);

				attacks = knightAttacks[fromSquare] & ((pos->side == WHITE) ? ~pos->occ[WHITE] : ~pos->occ[BLACK]);

				while (attacks)
				{
					toSquare = GetLSB(attacks);

					// quiet moves
					if (!getBit(((pos->side == WHITE) ? pos->occ[BLACK] : pos->occ[WHITE]), toSquare))
						AddMove(moves, Move(fromSquare, toSquare, piece, 0, 0, 0, 0, 0));
					// capture moves
					else
						AddMove(moves, Move(fromSquare, toSquare, piece, 0, 1, 0, 0, 0));

					popBit(attacks, toSquare);
				}

				popBit(bitboard, fromSquare);
			}
		}

		// generate bishop moves
		if ((pos->side == WHITE) ? piece == B : piece == b)
		{
			while (bitboard)
			{
				fromSquare = GetLSB(bitboard);

				attacks = GetBishopAttacks(fromSquare, pos->occ[BOTH]) & ((pos->side == WHITE) ? ~pos->occ[WHITE] : ~pos->occ[BLACK]);

				while (attacks)
				{
					toSquare = GetLSB(attacks);

					// quiet moves
					if (!getBit(((pos->side == WHITE) ? pos->occ[BLACK] : pos->occ[WHITE]), toSquare))
						AddMove(moves, Move(fromSquare, toSquare, piece, 0, 0, 0, 0, 0));
					// capture moves
					else
						AddMove(moves, Move(fromSquare, toSquare, piece, 0, 1, 0, 0, 0));

					popBit(attacks, toSquare);
				}

				popBit(bitboard, fromSquare);
			}
		}

		// generate rook moves
		if ((pos->side == WHITE) ? piece == R : piece == r)
		{
			while (bitboard)
			{
				fromSquare = GetLSB(bitboard);

				attacks = GetRookAttacks(fromSquare, pos->occ[BOTH]) & ((pos->side == WHITE) ? ~pos->occ[WHITE] : ~pos->occ[BLACK]);

				while (attacks)
				{
					toSquare = GetLSB(attacks);

					// quiet moves
					if (!getBit(((pos->side == WHITE) ? pos->occ[BLACK] : pos->occ[WHITE]), toSquare))
						AddMove(moves, Move(fromSquare, toSquare, piece, 0, 0, 0, 0, 0));
					// capture moves
					else
						AddMove(moves, Move(fromSquare, toSquare, piece, 0, 1, 0, 0, 0));

					popBit(attacks, toSquare);
				}

				popBit(bitboard, fromSquare);
			}
		}

		// generate queen moves
		if ((pos->side == WHITE) ? piece == Q : piece == q)
		{
			while (bitboard)
			{
				fromSquare = GetLSB(bitboard);

				attacks = GetQueenAttacks(fromSquare, pos->occ[BOTH]) & ((pos->side == WHITE) ? ~pos->occ[WHITE] : ~pos->occ[BLACK]);

				while (attacks)
				{
					toSquare = GetLSB(attacks);

					// quiet moves
					if (!getBit(((pos->side == WHITE) ? pos->occ[BLACK] : pos->occ[WHITE]), toSquare))
						AddMove(moves, Move(fromSquare, toSquare, piece, 0, 0, 0, 0, 0));
					// capture moves
					else
						AddMove(moves, Move(fromSquare, toSquare, piece, 0, 1, 0, 0, 0));

					popBit(attacks, toSquare);
				}

				popBit(bitboard, fromSquare);
			}
		}

		// generate king moves
		if ((pos->side == WHITE) ? piece == K : piece == k)
		{
			while (bitboard)
			{
				fromSquare = GetLSB(bitboard);

				attacks = kingAttacks[fromSquare] & ((pos->side == WHITE) ? ~pos->occ[WHITE] : ~pos->occ[BLACK]);

				while (attacks)
				{
					toSquare = GetLSB(attacks);

					// quiet moves
					if (!getBit(((pos->side == WHITE) ? pos->occ[BLACK] : pos->occ[WHITE]), toSquare))
						AddMove(moves, Move(fromSquare, toSquare, piece, 0, 0, 0, 0, 0));
					// capture moves
					else
						AddMove(moves, Move(fromSquare, toSquare, piece, 0, 1, 0, 0, 0));

					popBit(attacks, toSquare);
				}

				popBit(bitboard, fromSquare);
			}
		}
	}
}

static inline int Evaluate()
{
    int score = 0;
    int square = 0;

    uint64_t bb = 0ULL;

    for (int piece = P; piece <= k; piece++)
    {
        bb = pos->bb[piece];

        while (bb)
        {
            square = GetLSB(bb);

            score += materialScore[piece];

            switch (piece)
            {
                case P: score += pawnScore[square]; break;
                case N: score += knightScore[square]; break;
                case B: 
                    score += bishopScore[square]; 
                    //score += CountBits(GetBishopAttacks(square, pos->occ[BOTH]));
                    break;
                case R: 
                    score += rookScore[square]; 
                    //score += countBits(GetRookAttacks(square, pos->occ[BOTH]));
                    break;
                case Q: 
                    score += queenScore[square]; 
                    //score += countBits(GetQueenAttacks(square, pos->occ[BOTH]));
                    break;
                case K: score += kingScore[square]; break;

                case p: score -= pawnScore[mirrorScore[square]]; break;
                case n: score -= knightScore[mirrorScore[square]]; break;
                case b: 
                    score -= bishopScore[mirrorScore[square]]; 
                    //score -= countBits(getBishopAttacks(square, pos->occ[both]));
                    break;
                case r: 
                    score -= rookScore[mirrorScore[square]]; 
                    //score -= countBits(getRookAttacks(square, pos->occ[both]));
                    break;
                case q: 
                    score -= queenScore[mirrorScore[square]]; 
                    //score -= countBits(getQueenAttacks(square, pos->occ[both]));
                    break;
                case k: score -= kingScore[mirrorScore[square]]; break;
            }

            popBit(bb, square);
        }
    }

    return pos->side == WHITE ? score : -score;
}

static inline int NegaMax(int depth, int alpha, int beta)
{
	if (depth == 0)
		return Evaluate();

	int score = 0;
	int legalMoves = 0;

	bool inCheck = IsSqAttacked(pos->side == WHITE ? pos->bb[k] : pos->bb[K], pos->side ^ 1);

	MoveList moves[1];
	GenerateMoves(moves);

	for (int i = 0; i < moves->count; i++)
	{
		CopyBoard();

		if (!MakeMove(moves->moves[i]))
			continue;

		legalMoves++;
		pos->ply++;
		score = -NegaMax(depth - 1, -beta, -alpha);
		pos->ply--;

		TakeBack();

		if (score >= beta)
			return beta;

		if (score > alpha)
		{
			alpha = score;

			pos->pvTable[pos->ply][pos->ply] = moves->moves[i];

            for (int nextPly = pos->ply + 1; nextPly < pos->pvLength[pos->ply + 1]; nextPly++)
				pos->pvTable[pos->ply][nextPly] = pos->pvTable[pos->ply + 1][nextPly];

			pos->pvLength[pos->ply] = pos->pvLength[pos->ply + 1];
		}
	}

	if (!legalMoves)
	{
		if (inCheck)
			return -49000 + pos->ply;
		else
			return 0;
	}

	return alpha;
}

static inline void Perft(int depth)
{
	if (depth == 0)
	{
		sInfo->nodes++;
		return;
	}

	MoveList moves[1];
	GenerateMoves(moves);

	for (int i = 0; i < moves->count; i++)
	{
		CopyBoard();

		if (!MakeMove(moves->moves[i]))
			continue;

		pos->ply++;
		Perft(depth - 1);
		pos->ply--;

		TakeBack();
	}
}

uint64_t rand64()
{
	return rand() ^ ((uint64_t)rand() << 15) ^ ((uint64_t)rand() << 30) ^ ((uint64_t)rand() << 45) ^ ((uint64_t)rand() << 60);
}

void initZobristKeys()
{
	for (int piece = P; piece <= k; piece++)
		for (int sq = 0; sq < 64; sq++)
			zobrist[piece][sq] = rand64();
			
	for (int i = 0; i < 16; i++)
		castleKeys[i] = rand64();
		
	for (int sq = 0; sq < 64; sq++)
		epKeys[sq] = rand64();
		
	sideKey = rand64();
}

uint64_t generateHashKey(Position *pos)
{
	uint64_t finalKey = 0ULL;
	uint64_t bb = 0ULL;
		
	int sq = 0;
		
	for (int piece = P; piece <= k; piece++)
	{
		bb = pos->bb[piece];
			
		while (bb)
		{
			sq = GetLSB(bb);
			finalKey ^= zobrist[piece][sq];
			popBit(bb, sq);
		}
	}
		
	if (pos->ep != noSq)
		finalKey ^= epKeys[pos->ep];
			
	finalKey ^= castleKeys[pos->ca];
		
	if(pos->side == BLACK)
		finalKey ^= sideKey;
		
	return finalKey;
}

void ParseFen(Position *pos, char* fen)
{
	pos->reset();

	for (int rank = 0; rank < 8; rank++)
	{
		for (int file = 0; file < 8; file++)
		{
			int square = rank * 8 + file;

			if ((*fen >= 'a' && *fen <= 'z') || (*fen >= 'A' && *fen <= 'Z'))
			{
				int piece = charToPiece[*fen];
				setBit(pos->bb[piece], square);
				fen++;
			}

			if (*fen >= '0' && *fen <= '9')
			{
				int offset = *fen - '0';
				int target_piece = -1;

				for (int piece = P; piece <= k; piece++)
				{
					if (getBit(pos->bb[piece], square))
						target_piece = piece;
				}

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

	(*fen == 'w') ? (pos->side = WHITE) : (pos->side = BLACK);

	// go to castling rights
	fen += 2;

	while (*fen != ' ')
	{
		switch (*fen)
		{
		case 'K': pos->ca |= WK; break;
		case 'Q': pos->ca |= WQ; break;
		case 'k': pos->ca |= BK; break;
		case 'q': pos->ca |= BQ; break;
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

		pos->ep = rank * 8 + file;
	}
	else
		pos->ep = noSq;

	for (int piece = P; piece <= K; piece++)
		pos->occ[WHITE] |= pos->bb[piece];

	for (int piece = p; piece <= k; piece++)
		pos->occ[BLACK] |= pos->bb[piece];

    pos->occ[BOTH] = pos->occ[BLACK] | pos->occ[WHITE];

	pos->hashKey = generateHashKey(pos);
}

void initAttackMasks()
{
	for (int square = 0; square < 64; square++)
	{
		pawnAttacks[WHITE][square] = MaskPawnAttacks(WHITE, square);
		pawnAttacks[BLACK][square] = MaskPawnAttacks(BLACK, square);
		knightAttacks[square] = MaskKnightAttacks(square);
		kingAttacks[square] = MaskKingAttacks(square);
		bishopMasks[square] = MaskBishopAttacks(square);
		rookMasks[square] = MaskRookAttacks(square);
	}
}

void initSliderAttacks(bool isBishop)
{
	for (int sqr = 0; sqr < 64; sqr++)
	{
		uint64_t attackMask = isBishop ? bishopMasks[sqr] : rookMasks[sqr];
		int relevantBitsCount = CountBits(attackMask);
		int occupancyIndices = (1 << relevantBitsCount);

		for (int index = 0; index < occupancyIndices; index++)
		{
			if (isBishop)
			{
				uint64_t occupancy = SetOccupancy(index, relevantBitsCount, attackMask);
				int magic_index = (occupancy * bishopMagicNumbers[sqr]) >> (64 - bishopRelevantBits[sqr]);
				bishopAttacks[sqr][magic_index] = BishopAttacksOTF(sqr, occupancy);
			}
			else
			{
				uint64_t occupancy = SetOccupancy(index, relevantBitsCount, attackMask);
				int magic_index = (occupancy * rookMagicNumbers[sqr]) >> (64 - rookRelevantBits[sqr]);
				rookAttacks[sqr][magic_index] = RookAttacksOTF(sqr, occupancy);
			}
		}
	}
}

void InitAll()
{
	initAttackMasks();
	initSliderAttacks(true);
	initSliderAttacks(false);
	initZobristKeys();
}

int main()
{
	InitAll();

	ParseFen(pos, startPosition);
	PrintBoard(pos);

	int score = -NegaMax(5, -50000, 50000);

	cout << "Score: " << score << endl;
	cout << "Best Move: " << notation[getSource(pos->pvTable[0][0])] << notation[getTarget(pos->pvTable[0][0])] << endl;
	
	return 0;	
}

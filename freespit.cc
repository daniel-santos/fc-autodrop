#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdint>
#include <cassert>
#include <unistd.h>

// All hot-path scalars in one cache line. The deck itself lives on the
// caller's stack (also 64-byte aligned) — this struct holds everything
// else the inner loop touches.
struct alignas(64) state {
    unsigned	seed;		// current deal number (hot)
    unsigned	loop_end;	// seed-loop termination value (hot)
    unsigned	solvable;	// count of fully-cleared deals (hot)
    unsigned	start;		// first seed (set once)
    unsigned	end;		// last seed (set once)
    uint8_t	threshold;	// print-if-placed-≥ threshold (hot)
    uint8_t	flags;		// hot-path bitfield (see F_*)

    static constexpr uint8_t F_PRINT_ALL	= 1 << 0;
    static constexpr uint8_t F_DEBUG		= 1 << 1;
    static constexpr uint8_t F_END_SET		= 1 << 2;
};

static state S;

static_assert(sizeof(state)  == 64, "state should fit one cache line");
static_assert(alignof(state) == 64, "state should be cache-line aligned");

inline bool isDebug() { return S.flags & state::F_DEBUG; }

struct suit {
    uint8_t value;

    static constexpr char letters[4]		= {'C','D','H','S'};
    static constexpr const char *glyphs[4]	= {"♣", "♦", "♥", "♠"};

    suit(uint8_t value) : value(value) {}
    char letter() const	      { return letters[value]; }
    const char *glyph() const { return glyphs[value];  }
};

// Encoding matches KPat / MS Freecell:
//   value = (rank_idx * 4) + suit_idx
//   rank_idx: 0=A, 1=2, ..., 9=T, 10=J, 11=Q, 12=K
//   suit_idx: 0=C, 1=D, 2=H, 3=S
struct card {
    uint8_t value;

    static constexpr char ranks[13] = {'A','2','3','4','5','6','7','8','9','T','J','Q','K'};

    card()		: value(0) {}
    card(uint8_t value) : value(value) {}
    operator uint8_t() const	{ return value; }
    char rankChar() const	{ return ranks[value / 4]; }
    auto suit() const		{ return ::suit(value % 4); }
    uint8_t rank() const	{ return value / 4; }
    card &operator=(uint8_t value) {
	assert(value < 52 || value == 0xff);
	this->value = value;
	return *this;
    }

    void toString(char *dest) const {
	dest[0] = rankChar();
	dest[1] = suit().letter();
	dest[2] = 0;
    }
};

template<unsigned N, uint8_t C>
struct alignas(64) deck {
    uint8_t stack[4];	// Holds the rank it is expecting next
    uint8_t n;
    ::card cards[N];


    deck() :n(N) {
	for (unsigned i = 0; i < N; ++i)
	    cards[i] = i;
	memset(stack, 0, sizeof(stack));
    }

    auto card(unsigned i) const { return cards[i]; }
    auto &card(unsigned i)	{ return cards[i]; }
    void swap(unsigned a, unsigned b) {
	uint8_t tmp = cards[a];
	cards[a] = cards[b];
	cards[b] = tmp;
    }

    // Microsoft Freecell LCG + top-down Fisher-Yates.
    // Matches KPat's KpatShuffle::shuffled() so deal numbers agree with MS Freecell.
    void shuffle(unsigned int seed) {
	for (auto i = N; i > 1; --i) {
	    seed = 214013 * seed + 2531011;
	    unsigned rand = (seed >> 16) & 0x7fff;
	    swap(i - 1, rand % i);
	}
    }

    // Returns remaining cards (0 == fully solvable). Sweeps bottom-to-top
    // repeatedly until a sweep places nothing.
    uint8_t solve() {
	while (n) {
	    bool progress = false;
	    for (uint8_t i = N - 1; i != 0xff; --i) {
		auto &card = this->card(i);
		if (card == 0xff)
		    continue;
		// Multi-step visibility: any non-0xff cell below us in this
		// column covers us. Walk i+C, i+2C, ... while in range.
		bool covered = false;
		for (unsigned k = i + C; k < N; k += C) {
		    if (cards[k] != 0xff) {
			covered = true;
			break;
		    }
		}
		if (covered)
		    continue;

		auto rank = card.rank();
		auto suit = card.suit();
		auto s    = &stack[suit.value];
		if (*s == rank) {
		    ++*s;
		    card = 0xff;
		    --n;
		    progress = true;
		    if (isDebug())
			print();
		}
	    }
	    if (!progress)
		return n;
	}
	return n;
    }

    void makeSolveable() {
	for (uint8_t i = 0; i < 52; ++i) {
	    card(51 - i) = i;
	}
    }


    // Freecell deal layout: 8 columns, row-major.
    // Top row: 4 (empty) free cells then 4 foundation slots from stack[].
    // Tableau: card k -> column (k % 8), row (k / 8). 0xff means consumed.
    void print() const {
	constexpr unsigned cols = 8;

	std::printf("n=%d\n", 52 - n);
	// Free cells: blank, 4 card-widths.
	for (unsigned i = 0; i < 4; ++i)
	    std::fputs("   ", stdout);
	// Foundations: stack[s] is the rank EXPECTED next, so the visible
	// top is stack[s] - 1. stack[s] == 0 -> empty slot.
	for (unsigned s = 0; s < 4; ++s) {
	    if (stack[s] == 0) {
		std::fputs("   ", stdout);
	    } else {
		auto cd = ::card(static_cast<uint8_t>((stack[s] - 1) * 4 + s));
		std::printf(" %c%s", cd.rankChar(), cd.suit().glyph());
	    }
	}
	std::putchar('\n');

	const unsigned rows = (N + cols - 1) / cols;
	for (unsigned r = 0; r < rows; ++r) {
	    for (unsigned c = 0; c < cols; ++c) {
		unsigned k = r * cols + c;
		if (k >= N) break;
		uint8_t v = cards[k];
		if (v == 0xff) {
		    std::fputs("   ", stdout);
		} else {
		    auto cd = ::card(v);
		    std::printf(" %c%s", cd.rankChar(), cd.suit().glyph());
		}
	    }
	    std::putchar('\n');
	}
    }
};

typedef deck<52, 8> freecell_deck;

static_assert(sizeof(freecell_deck) == 64,  "deck<52> should pad to one cache line");
static_assert(alignof(freecell_deck) == 64, "deck<52> should be cache-line aligned");

// MS Freecell deal numbers are 1..2^31-1.
constexpr unsigned MAX_SEED = 0x7fffffff;

static void usage(const char *prog) {
    std::fprintf(stderr,
	"Usage: %s [-s START] [-e END] [-p]\n"
	"  -s START   first seed (default: 1, max: 0x7fffffff)\n"
	"  -e END     last seed, inclusive (default: START)\n"
	"  -p         print each deal (default: only print solvable ones)\n"
	"  -d         debug spew\n",
	prog);
}

int main(int argc, char *argv[])
{
    S.start = 1;

    int opt;
    while ((opt = getopt(argc, argv, "s:e:t:pdh")) != -1) {
	switch (opt) {
	case 's': S.start	= static_cast<unsigned>(std::strtoul(optarg, nullptr, 10));
		  break;
	case 'e': S.end		= static_cast<unsigned>(std::strtoul(optarg, nullptr, 10));
		  S.flags	|= state::F_END_SET;
		  break;
	case 't': S.threshold	= static_cast<uint8_t>(std::strtoul(optarg, nullptr, 10)); break;
	case 'p': S.flags	|= state::F_PRINT_ALL;	break;
	case 'd': S.flags	|= state::F_DEBUG;	break;
	case 'h': usage(argv[0]); return 0;
	default:  usage(argv[0]); return 2;
	}
    }
    if (!(S.flags & state::F_END_SET))
	S.end = S.start;
    if (!S.start || !S.end || S.start > MAX_SEED || S.end > MAX_SEED) {
	std::fprintf(stderr, "Seeds must be in 1..0x%x\n", MAX_SEED);
	return 2;
    }

    freecell_deck ordered;
    assert((reinterpret_cast<uintptr_t>(&ordered) & 63) == 0);
    assert((reinterpret_cast<uintptr_t>(&S)       & 63) == 0);

    S.loop_end = (S.end + 1) & MAX_SEED;
    for (S.seed = S.start; S.seed != S.loop_end; ++S.seed, S.seed &= MAX_SEED) {
	auto deal = ordered;
	deal.shuffle(S.seed);

	const uint8_t solved = deal.solve();
	if (solved == 0)
	    ++S.solvable;

	if ((S.flags & state::F_PRINT_ALL) || solved <= S.threshold) {
	    std::printf("Deal #%u%s\n", S.seed, solved == 0 ? "  [solvable]" : "");
	    deal.print();
	}
    }

    std::printf("Scanned seeds %u..%u: %u solvable\n", S.start, S.end, S.solvable);
    return 0;
}

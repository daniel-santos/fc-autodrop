#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdint>
#include <cassert>
#include <unistd.h>

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

    card(uint8_t value) : value(value) {}
    char rankChar() const { return ranks[value / 4]; }
    auto suit() const	  { return ::suit(value % 4); }
    uint8_t rank() const  { return value / 4; }

    void toString(char *dest) const {
	dest[0] = rankChar();
	dest[1] = suit().letter();
	dest[2] = 0;
    }
};

template<unsigned N, uint8_t C>
struct alignas(64) deck {
    uint8_t n = N;
    uint8_t stack[4];	// Holds the rank it is expecting next
    uint8_t cards[N];


    deck() :n(N) {
	for (unsigned i = 0; i < N; ++i)
	    cards[i] = static_cast<uint8_t>(i);
	memset(stack, 0, sizeof(stack));
    }

    auto card(unsigned i) const { return ::card(cards[i]); }
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

    // Returns true if this deal is solvable. (solves in place)
    bool solve() {
	uint8_t start = N - 1;
	// uint8_t visible[C];
	// memset(visible, 1, C);

	while (n) {
	    uint8_t i, last_visible;
	    for (i = last_visible = start; start != 0xff; --i) {
		if (cards[i] == 0xff) {
		    --start;
		    --last_visible;
		    continue;
		}
		if (start - i > C && cards[i + C] != 0xff) {
		    if (last_visible - i > C)
			return false;
		    continue;
		}
		last_visible = i;
		auto card = this->card(i);
		auto rank = card.rank();
		auto suit = card.suit();
		auto s    = &stack[suit.value];
		if (*s == rank) {
		    ++*s;
		    cards[i] = 0xff;
		    --n;
		}
	    }
	}
	return true;
    }

    // Freecell deal layout: 8 columns, row-major.
    // Top row: 4 (empty) free cells then 4 foundation slots from stack[].
    // Tableau: card k -> column (k % 8), row (k / 8). 0xff means consumed.
    void print() const {
	constexpr unsigned cols = 8;

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
	"  -p         print each deal (default: only print solvable ones)\n",
	prog);
}

int main(int argc, char *argv[])
{
    unsigned start = 1;
    unsigned end   = 0;        // sentinel: "not set yet"
    bool end_set   = false;
    bool print_all = false;

    int opt;
    while ((opt = getopt(argc, argv, "s:e:ph")) != -1) {
	switch (opt) {
	case 's': start = static_cast<unsigned>(std::strtoul(optarg, nullptr, 10)); break;
	case 'e': end   = static_cast<unsigned>(std::strtoul(optarg, nullptr, 10));
		  end_set = true; break;
	case 'p': print_all = true; break;
	case 'h': usage(argv[0]); return 0;
	default:  usage(argv[0]); return 2;
	}
    }
    if (!end_set)
	end = start;
    if (!start || !end || start > MAX_SEED || end > MAX_SEED) {
	std::fprintf(stderr, "Seeds must be in 1..0x%x\n", MAX_SEED);
	return 2;
    }


    freecell_deck ordered;
    assert((reinterpret_cast<uintptr_t>(&ordered) & 63) == 0);

    unsigned solvable = 0;
    const unsigned loop_end = (end + 1) & MAX_SEED;
    for (unsigned seed = start; seed != loop_end; ++seed, seed &= MAX_SEED) {
	auto deal = ordered;
	deal.shuffle(seed);

	const bool solved = deal.solve();
	if (solved)
	    ++solvable;

	if (print_all || solved) {
	    std::printf("Deal #%u%s\n", seed, solved ? "  [solvable]" : "");
	    deal.print();
	}
    }

    std::printf("Scanned seeds %u..%u: %u solvable\n", start, end, solvable);
    return 0;
}

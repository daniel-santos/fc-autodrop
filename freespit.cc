#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <cassert>
#include <unistd.h>

// Encoding matches KPat / MS Freecell:
//   value = (rank_idx * 4) + suit_idx
//   rank_idx: 0=A, 1=2, ..., 9=T, 10=J, 11=Q, 12=K
//   suit_idx: 0=C, 1=D, 2=H, 3=S
struct card {
    uint8_t value;

    static constexpr char ranks[13] = {'A','2','3','4','5','6','7','8','9','T','J','Q','K'};
    static constexpr char suits[4]  = {'C','D','H','S'};
    static constexpr const char *suitGlyphs[4] = {"♣", "♦", "♥", "♠"};

    char rankChar() const { return ranks[value / 4]; }
    char suitChar() const { return suits[value % 4]; }
    const char *suitGlyph() const { return suitGlyphs[value % 4]; }

    void toString(char *dest) const {
        dest[0] = rankChar();
        dest[1] = suitChar();
        dest[2] = '\0';
    }
};

template<unsigned N>
struct alignas(64) deck {
    uint8_t cards[N];

    deck() {
        for (unsigned i = 0; i < N; ++i)
            cards[i] = static_cast<uint8_t>(i);
    }

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

    // Returns true if this deal is solvable. TODO: actually solve.
    bool solve() const {
        return false;
    }

    // Freecell deal layout: 8 columns, row-major.
    // Card k goes to column (k % 8), row (k / 8).
    void print() const {
        constexpr unsigned cols = 8;
        const unsigned rows = (N + cols - 1) / cols;
        for (unsigned r = 0; r < rows; ++r) {
            for (unsigned c = 0; c < cols; ++c) {
                unsigned k = r * cols + c;
                if (k >= N) break;
                card cd{cards[k]};
                std::printf(" %c%s", cd.rankChar(), cd.suitGlyph());
            }
            std::putchar('\n');
        }
    }
};

static_assert(sizeof(deck<52>) == 64,  "deck<52> should pad to one cache line");
static_assert(alignof(deck<52>) == 64, "deck<52> should be cache-line aligned");

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
    ++end;
    end &= MAX_SEED;

    deck<52> ordered;
    assert((reinterpret_cast<uintptr_t>(&ordered) & 63) == 0);

    unsigned solvable = 0;
    for (unsigned seed = start; seed != end; ++seed, seed &= MAX_SEED) {
        deck<52> deal = ordered;
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

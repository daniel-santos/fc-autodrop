#include <cstdio>
#include <cstdlib>
#include <cstdint>

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

static_assert(sizeof(deck<52>) == 64, "deck<52> should pad to one cache line");

int main(int argc, char *argv[])
{
    unsigned seed = (argc > 1) ? static_cast<unsigned>(std::strtoul(argv[1], nullptr, 10)) : 1;

    deck<52> ordered;
    deck<52> deal = ordered;
    deal.shuffle(seed);

    std::printf("Deal #%u\n", seed);
    deal.print();
    return 0;
}

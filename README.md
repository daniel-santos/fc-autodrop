# fc-autodrop

Brute-force scanner for **self-solving** Freecell deals: deals that
fully clear with no tactical play, using only direct auto-drop to
the foundations.

## Compatibility

Deal numbers match KPatience (kpat), PySol, GNOME Aisleriot, and every other
Freecell that adopted the canonical numbering. Deal `N` in
fc-autodrop is the same tableau as deal `N` in any of those — load it
in your favorite game and the cards line up exactly.

The algorithm is documented in the [fc-solve FAQ][1] and traces back
to a certain proprietary implementation by the folks in Redmond, who at
least did the world the favor of using a simple LCG that everyone could
clone.

[1]: https://fc-solve.shlomifish.org/faq.html#what_are_ms_deals

## Build

```sh
make
```

Requires a C++17 compiler. Tested with GCC 12 on Debian Bookworm
(`-march=native -O2`).

## Usage

```
./fc-autodrop [OPTIONS]
  -s, --start     SEED   first seed (default: 1, max: 0x7fffffff)
  -e, --end       SEED   last seed, inclusive (default: START)
  -t, --threshold N      print deals with at most N cards remaining
                         (default: 0; i.e. only print fully-solved deals)
  -p, --print-all        print every deal scanned
  -d, --debug            trace each card played
  -h, --help             show this message
```

### Examples

```sh
# Print one specific deal (matches kpat's "New Game by Number" for the same SEED)
./fc-autodrop -s 155388374 -p

# Scan the first million deals, print those with ≤5 cards left after auto-drop
./fc-autodrop -e 1000000 -t 5

# Full deal-number range, print anything that auto-drops at least 16 cards
./fc-autodrop -e 2147483647 -t 36

# Same as above but only print deals that fully clear (the hunt itself)
./fc-autodrop -e 2147483647
```

On a Ryzen 5 PRO 8640HS the full 2³¹-1 sweep runs in ~6 minutes,
single-threaded. Easy to parallelize across cores by splitting the seed
range and combining outputs.

## What "auto-drop only" means

A real Freecell solver uses the 4 free cells and builds tableau
sequences (red on black descending). fc-autodrop intentionally does
*neither*. A card is played only when:

1. It is the bottom card of its column (no card covers it), AND
2. Its rank exactly matches the next rank expected on its suit's foundation.

So the scanner finds deals winnable with **zero tactical thought** — a
strict subset of solvable deals. Most Freecell deals are solvable; the
auto-drop-only subset is much smaller.

## Output format

```
Deal #1195233675
n=23
             7♣ 5♦ 5♥ 6♠
 T♠ T♥ 7♦ J♣ 7♠ K♦ K♠ J♥
 T♦ 9♥    Q♥ 8♣ 6♥ J♠ 7♥
 T♣ 8♦    9♣ Q♦ Q♣ 9♠ K♣
 6♦       8♠          8♥
 K♥       9♦          J♦
          Q♠
```

The top row shows 4 blank free-cell slots followed by 4 foundation
slots; the rest is the residual tableau. Blank slots in the tableau
mark cards that were auto-dropped. `n=` prints the number of cards
actually placed on foundations (so `n=52` means a fully-solved deal).

## How many self-solvable games are there?

I'm glad you asked, because it's that one magical number -- 0! The closest is
the game is displayed above.

## License

GPL-3.0-or-later. See [LICENSE](LICENSE).

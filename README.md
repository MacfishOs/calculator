# Calculator

The calculator of Cutefish Desktop. It works out expressions typed or
entered on its keypad and keeps a history of the calculations. It computes
in decimal, so 0.1 + 0.2 is 0.3: whole numbers keep every digit up to
10^1000, everything else 160 significant digits.

Besides + − × ÷ and parentheses it knows powers (^, ², ³), factorials (!),
percentages (200 + 10% is 220), mod and div, square and cube roots (√, ∛),
pi, e and phi, variables (x = 5) and the last result (ans), and functions
such as sin, cos, tan and their inverses in degrees, ln, log, exp, sqrt,
round, floor, min, max, sum, gcd and ncr.

## Dependencies

Calculator uses cutefish-toolkit; GoogleTest for the tests.

```sh
sudo apt install cmake libgtest-dev
```

## Build and install

```sh
mkdir build
cd build
cmake -DCMAKE_INSTALL_PREFIX=/usr ..
make
sudo make install
```

## Tests

```sh
cmake -DBUILD_TESTING=ON ..
make
ctest --output-on-failure
```

## License

BSD-3-Clause; see [LICENSE](LICENSE).

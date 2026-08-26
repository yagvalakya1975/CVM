# CVM

CVM executes a small, Java-type-inspired language. Programs use semicolon-terminated declarations, assignments, `if`, `while`, `print`, and `input`.

## Types

Supported declaration types are `byte`, `short`, `int`, `long`, `float`, `double`, `char`, `String`, and `boolean`.

- Integer literals are `int`; append `L` for `long`.
- Decimal literals are `double`; append `F` for `float`.
- Safe numeric widening is implicit. Use a cast such as `(int) 3.9` for narrowing.
- `String` equality compares contents. `input("prompt")` returns a `String`.
- Conditions must be `boolean`.

`string` and `bool` are no longer type keywords. Use `String` and `boolean` instead. String and character literals retain their raw-literal behavior; Java escape decoding is intentionally not implemented.

Build and run:

```sh
cmake -S . -B build
cmake --build build
./build/cvmpp example.pi
```

Run the test suite with `ctest --test-dir build --output-on-failure`.

# CVM

CVM executes a small custom language with C-like syntax and primitive types. Programs use semicolon-terminated declarations, assignments, `if`, `while`, `print`, and `input`.

## Types

Supported declaration types are `byte`, `short`, `int`, `long`, `float`, `double`, `char`, `String`, and `boolean`.

- Integer literals are `int`; append `L` for `long`.
- Decimal literals are `double`; append `F` for `float`.
- Safe numeric widening is implicit. Use a cast such as `(int) 3.9` for narrowing.
- `String` is CVM's text type; equality compares contents. `input("prompt")` returns a `String`.
- Conditions must be `boolean`.

`string` and `bool` are no longer type keywords. Use `String` and `boolean` instead. String and character literals retain their raw-literal behavior; escape decoding is intentionally not implemented.

## Arrays (lexer and parser)

The frontend accepts C-like typed declarations and bracket literals, including
index reads and writes:

```c
int[] scores = [10, 20, 30];
print(scores[1]);
scores[2] = 42;
```

Nested type brackets such as `int[][] grid` are supported. Array literals are
compiled with a `BUILD_ARRAY` bytecode instruction and may be stored in and
printed from array declarations. Indexed reads and writes are parsed but are not
yet executed by the compiler/VM.

Build and run:

```sh
cmake -S . -B build
cmake --build build
./build/cvmpp example.pi
```

Run the test suite with `ctest --test-dir build --output-on-failure`.

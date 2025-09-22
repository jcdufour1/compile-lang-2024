# Compile Lang 2024
A statically typed systems programming language with modern language features such as sum types, generics, and defer for safe resource management.

## Disclaimer
the language design and implementation are not finished; there will be breaking changes and bugs

## Features
- :white_check_mark: Sum types with pattern matching
- :white_check_mark: `defer` for automatic cleanup for resources
- :white_check_mark: Generics for reusable functions and data structures
- :white_check_mark: Functions, conditionals, loops, and recursion
- :white_check_mark: Compiles to C for portability

## Example Programs
```c

type util import = std.util
type io import = std.io

fn divide(lhs NumT, rhs NumT, NumT Type) util.Optional(<NumT>) {
    if rhs == 0 {
        return .none
    }
    return .some(lhs/rhs)
}

fn main() i32 {
    switch divide(8, 2) {
        case .some(num): io.printf("result is %d\n", num)
        case .none: io.printf("error when dividing (cannot divide by zero\n")
    }
    return 0
}
```

```c

type coll import = std.collections
type io import = std.io

fn main() i32 {
    let nums coll.Darr(<i32>) = coll.darr_new(i32, [94, 23])
    defer coll.darr_free(&nums)

    coll.arr_append(&nums, 3)
    coll.arr_append(&nums, 7)

    for idx u64 in 0..nums.count {
        io.print_num(coll.darr_at(nums, idx))
    }

    return 0
}
```

## Quickstart
1. clone the repo
```sh
git clone https://github.com/jcdufour1/compile-lang-2024
cd my-lang
```
2. build the compiler
```sh
make build
```
3. run a program
```sh
./build/release/main tests2/inputs/char.own --run
```


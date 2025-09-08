# Compile Lang 2024
A statically typed systems programming language with sum types, generics, and defer for safe resource management.

## Features
- [x] Sum types with pattern matching
- [x] `defer` for automatic cleanup for resources
- [x] Generics for reusable functions and data structures
- [x] Functions, conditionals, loops, and recursion
- [x] Compiles to C for portability

## Example Programs
'''c

type io import = std.io

fn divide(lhs i32, rhs i32) io.Optional(<i32>) {
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
'''

# TODO: defer example

## Quickstart
1. clone the repo
'''sh
git clone https://github.com/jcdufour1/compile-lang-2024
cd my-lang
'''
2. build the compiler
'''sh
make build
'''
3. run a program
'''sh
./build/release/main tests2/inputs/char.own --run
'''


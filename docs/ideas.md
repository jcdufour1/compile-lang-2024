
# these features may or may not be implemented


hello world 
```c
fn void main() {
    io::println("hello");
}
```

struct
```c
struct Token {
    String name;
    int num;
}
```

# struct (generics)
```c
def Token_types = String | int | uint;

struct Token <T1: Token_types> {
    String name;
    T1 value;
}
```
## example 2
```c
type Optional sum(<ValueType>) {
    String name;
    ValueType value;
}
```

## example 3
```c
type vector_int(<ValueType: $int>) {
    String name;
    T1 value;
}

type Darr struct (<ValueType, IndexType: $int = usize>) {
    items* ItemType
    count IndexType
    capacity IndexType
}
```

for loop (index (0 inclusive, 10 exclusive))
```c
for idx in [0, 10) {
    print(idx);
}
```

for loop (index (0 inclusive, 10 inclusive))
```c
for idx in [0, 10] {
    print(idx);
}
```

for loop (iterate over each item)
```c
Vec<int> vector = {89, 1893, 1, 8};
defer free(vector);
for num in vector.iter() {
    println(idx);
}
```

for loop (iterate over each item with index)
```c
Vec<int> vector = {89, 1893, 1, 8};
defer free(vector);
for index, num in vector.iter() {
    println("at index ", index, ":", num);
}
```

for loop (index (0 inclusive, 10 inclusive))
```c
for idx in (0, 10) {
    print(idx);
}
```

function (generics)
```c
fn Token<Type> get_token_value(Token& token) {
    return token.value;
}
```

# generics idea 2
# generics idea struct 2.1
type Darr struct('ItemType, 'IndexType = usize) {
    items ItemType*
    count IndexType
    capacity IndexType
}
# generics idea struct 2.2
// constraints
'IndexType: $int
type Darr struct('ItemType, 'IndexType = usize) {
    items ItemType*
    count IndexType
    capacity IndexType
}

type Arr struct('ItemType, 'count usize) {
    items ItemType*
}

# generics idea struct 2.3
// likely not, because needlessly verbose
type Darr struct(''ItemType, ''IndexType = usize) {
    items 'ItemType*
    count 'IndexType
    capacity 'IndexType
}

type Arr struct(''ItemType, ''count usize) {
    items ItemType*
    'count usize
}

# generics idea function 2.1
// probably not the way
fn append(darr Darr*, item darr.ItemType) {
    reserve(darr, 1)
    darr.items[darr.count] = item
    darr.count += 1
}


# generics idea function 2.2
// probably not the way
fn append(darr Darr(T, _)*, item T, T' = auto) {
    reserve(darr, 1)
    darr.items[darr.count] = item
    darr.count += 1
}

# generics idea function 2.3
// use `'T` to define T, and use `T` otherwise
fn append(darr Darr('T, _)*, item T) {
    reserve(darr, 1)
    darr.items[darr.count] = item
    darr.count += 1
}

fn at(darr Darr('T, 'S)*, index S) S {
    assert(index < darr.count && "out of bounds")
    return darr.items[darr.count]
}

# generics idea function 2.4
fn print_sizeof_item(darr Darr) {
    io.print("%d\n", sizeof darr.ItemT)
}

fn at(darr Darr('T, 'S)*, index S) S {
    assert(index < darr.count && "out of bounds")
    return darr.items[darr.count]
}

# generics idea function 2.5
fn append(<>)(darr Darr*, item T) {
    reserve(darr, 1)
    darr.items[darr.count] = item
    darr.count += 1
}

fn at(darr Darr('T, 'S)*, index S) S {
    assert(index < darr.count && "out of bounds")
    return darr.items[darr.count]
}

# generics idea function 2.6
fn append(<>)(darr Darr(<$T, _>)*, item T) {
    reserve(darr, 1)
    darr.items[darr.count] = item
    darr.count += 1
}

fn at(darr Darr(_, $S)*, index S) S {
    assert(index < darr.count && "out of bounds")
    return darr.items[darr.count]
}

allocate thing (error)
```c
fn void main() {
    int[1000] items = alloc(int, 1000);
    for item in items {
        io::println("hello");
    }
}
```

allocate thing (correct)
```c
fn void main() {
    int[1000] items = alloc<int>(1000);
    defer free(items);
    ... // put items in items
    for item in items {
        println("hello");
    }
}
```

function returning an error (idea 1)
```c
fn void main() {
    Result<File> file = open("hello.txt", FILE::READ);
    if file.type == err {
        println("file could not be opened:", err_text(err));
        return;
    }
    defer close(file);
    for idx, line in file.iter_lines() {
        io::print("line ", idx, ":", line);
    }
}
```

function returning an error (idea 2)
```c
fn void main() {
    Result<File> file_result = open("hello.txt", FILE::READ);
    File file = if file_result == {
        Err (err) {
            println("file could not be opened:", err_text(err));
            return;
        }
        File (file) {
            break file;
        }
    }
    ...
}
```


function returning an error (idea 2.1)
```c
fn void main() {
    let file_result = open("hello.txt", FILE::READ);
    File file = if file_result == {
        Err (err) {
            println("file could not be opened:", err_text(err));
            return;
        }
        File (file) break file; 
    }
    ...
}
```


function returning an error (idea 2.2)
```c
fn void main() {
    File file = if open("hello.txt", FILE::READ) == {
        Err (err) {
            println("file could not be opened:", err_text(err));
            return;
        }
        File (file) break file; 
    }
    ...
}
```

function returning an error (idea 3)
```c
fn void main() {
    File! file = open("hello.txt", FILE::READ);
    if type(file) == err {
        println("file could not be opened:", err_text(file));
        return;
    }
    // file is now a normal file
    ...
}
```

function returning an error (idea 4)
```c
fn void main() {
    File file = open("hello.txt", FILE::READ) orelse {
        println("file could not be opened:", err_text(file));
        return;
    }
    // file is a normal file
    ...
}

function returning an error (idea 5)
```c
fn void main() {
    File file = open("hello.txt", FILE::READ) orelse(error) {
        println("file could not be opened:", error);
        return;
    }
    // file is a normal file
    ...
}

```
# constraints
## array library
```c
struct Array {
    int* buf;
    usize count;
    usize capacity;
}

fn int array_at(Array* array, usize @{idx < array.count} idx) {
    ...
}
```

## example 1
```c
struct Array {
    int* buf;
    usize count;
    usize capacity;
}

fn int array_at(Array* array, usize @{idx < array.count} idx) {
    ...
}

fn void main() {
    Array array = array_new();
    array_push(array, 34);
    array_push(array, 78);

    // compiler gives error because idx is not < array.count
    int num = array_at(array, 2);
}
```

## example 2
```c
fn void main() {
    Array array = array_get_with_random_count();

    // compiler gives error because idx is not < array.count
    int num = array_at(array, 2);
}

## example 3
this compiles
```c
fn void main() {
    Array array = array_get_with_random_count();

    if (array.len < 2) {
        println("error: array has too few elements");
        return;
    }
    int num = array_at(array, 2);
}

## example 4
this compiles
```c
fn void main() {
    Array array = array_get_with_random_count();

    if (array.len < 2) {
        println("error: array has too few elements");
        return;
    }
    int num = array_at(array, 2);
}



```

# print formatting
print num1
```c
fn main() i32 {
    let num1: i32 = 89;
    println("num1: {num1}");
}
```
print num1 + num2
```c
fn main() i32 {
    let num1: i32 = 89;
    let num2: i32 = 76;
    println("num1: {num1 + num2}");
}
```


# function overloading (idea 1)
```c
fn string_append(darray Darray<(u8*)>, character overload u8) {
}

fn string_append(darray Darray<(u8*)>, string overload Darray<(u8)>) {
}

fn string_append(darray Darray<(u8*)>, string overload [u8]) {
}
```

# function overloading (idea 2)
```c
fn string_append<(ItemType = overload)>(string Darray<(u8*)>) {
}
```

# function overloading (idea 3)
```c
fn string_append overload u8(string Darray<(u8)>, overload item) {
}

fn string_append overload Darray<(u8)>(string Darray<(u8)>, overload item) {
}

fn string_append overload [u8](string Darray<(u8)>, overload item) {
}
```

# function overloading (idea 4)
```c
fn string_append u8(string Darray<(u8)>, overload item) {
}

fn string_append Darray<(u8)>(string Darray<(u8)>, overload item) {
}

fn string_append [u8](string Darray<(u8)>, overload item) {
}
```

# function overloading (idea 5)
```c
@overload
fn string_append u8(string Darray<(u8)>, overload item) {
}

@overload
fn string_append Darray<(u8)>(string Darray<(u8)>, overload item) {
}

@overload
fn string_append [u8](string Darray<(u8)>, overload item) {
}
```

# function overloading (idea 6)
```c
fn string_append u8(string Darray<(u8)>, oload(0) item) {
}

fn string_append Darray<(u8)>(string Darray<(u8)>, oload(0) item) {
}

fn string_append [u8](string Darray<(u8)>, oload(0) item) {
}
```

# operator overloading (idea 1)
```c
fn operator [] (lhs String, rhs i32) {
}

fn operator [] (lhs String, rhs i32) {
}

# operator overloading (idea 2)
```c
fn [] (lhs String, rhs i32) {
}

fn [] (lhs String, rhs i32) {
}

# operator overloading (idea 3)
```c
fn [] String, i32 (lhs, rhs) {
}
```

# operator overloading (idea 4)
```c
// constraints
T: (+) | (-) | (*) | (/)

fn binary i32, Vec2 (oper T, lhs, rhs) {
    switch oper {
        case (+): return vec2_add(lhs, rhs)
        case (-): return vec2_sub(lhs, rhs)
        case (*): return vec2_mul(lhs, rhs)
        case (/): return vec2_div(lhs, rhs)
    }
}
```

# operator overloading (idea 4.1)
```c
// constraints
T: "+" | "-" | "*" | "/"

fn binary i32, Vec2 ('T, lhs, rhs) {
    switch oper {
        case "+": return vec2_add(lhs, rhs)
        case "-": return vec2_sub(lhs, rhs)
        case "*": return vec2_mul(lhs, rhs)
        case "/": return vec2_div(lhs, rhs)
    }
}
```

# using (put struct members directly in namespace)
```c
type Token struct {
    string u8*
    pos Pos
}

fn token_print(using token Token) {
    pos_print(pos)
    string_print(string)
}
```

# 

# this style of defining types is used for easier grepping
```c
type Token struct {
}
```

# foreach
```c
type struct Things {
    items Thing[]
}

fn foreach Things(things Things) {
    for (usize idx = 0; idx < things.items.count; idx++) {
        for_yield things.items[idx]
    }
}

```

# inout
// allow count and mut items to be passed to same function
```c
fn darr_at(darr Darr(inout 'T, 'I), index I) inout T {
}
```
# method
```c
// this function can be used as either a method or a regular function
@method
fn at(darr Darr(<i32>), index i32) {
    return darr.buf[index]
}

fn main() i32 {
    let darr Darr(<i32>) = ..
    let num i32 = darr.at(0)
    return 0
}
```

# ownership
pointer could use generic with associated arena (this could reduce frustrations)
type Expr struct('alloc) {
    ptr(Expr, alloc)
}
## syntax sugar:
type Expr struct('alloc) {
    Expr&alloc
}


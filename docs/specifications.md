

hello world 
'''c
fn void main() {
    io::println("hello");
}
'''

struct
'''c
struct Token {
    String name;
    int num;
}
'''

struct (generics)
'''c
def Token_types = String | int | uint;

struct Token <T1: Token_types> {
    String name;
    T1 value;
}
'''

for loop (index (0 inclusive, 10 exclusive))
'''c
for idx in [0, 10) {
    print(idx);
}
'''

for loop (index (0 inclusive, 10 inclusive))
'''c
for idx in [0, 10] {
    print(idx);
}
'''

for loop (iterate over each item)
'''c
Vec<int> vector = {89, 1893, 1, 8};
defer free(vector);
for num in vector.iter() {
    println(idx);
}
'''

for loop (iterate over each item with index)
'''c
Vec<int> vector = {89, 1893, 1, 8};
defer free(vector);
for index, num in vector.iter() {
    println("at index ", index, ":", num);
}
'''

for loop (index (0 inclusive, 10 inclusive))
'''c
for idx in (0, 10) {
    print(idx);
}
'''

function (generics)
'''c
fn Token<Type> get_token_value(Token& token) {
    return token.value;
}
'''

allocate thing (error)
'''c
fn void main() {
    int[1000] items = alloc(int, 1000);
    for item in items {
        io::println("hello");
    }
}
'''

allocate thing (correct)
'''c
fn void main() {
    int[1000] items = alloc<int>(1000);
    defer free(items);
    ... // put items in items
    for item in items {
        println("hello");
    }
}
'''

function returning an error (idea 1)
'''c
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
'''

function returning an error (idea 2)
'''c
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
'''


function returning an error (idea 2.1)
'''c
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
'''


function returning an error (idea 2.2)
'''c
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
'''

function returning an error (idea 3)
'''c
fn void main() {
    File! file = open("hello.txt", FILE::READ);
    if type(file) == err {
        println("file could not be opened:", err_text(file));
        return;
    }
    // file is now a normal file
    ...
}
'''

# constraints
## array library
'''c
struct Array {
    int* buf;
    usize count;
    usize capacity;
}

fn int array_at(Array* array, usize @{idx < array.count} idx) {
    ...
}
'''

## example 1
'''c
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
'''

## example 2
'''c
fn void main() {
    Array array = array_get_with_random_count();

    // compiler gives error because idx is not < array.count
    int num = array_at(array, 2);
}

## example 3
this compiles
'''c
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
'''c
fn void main() {
    Array array = array_get_with_random_count();

    if (array.len < 2) {
        println("error: array has too few elements");
        return;
    }
    int num = array_at(array, 2);
}



'''

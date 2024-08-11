

hello world 
'''c
fn void main() {
    io::println("hello");
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


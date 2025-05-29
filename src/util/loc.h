#ifndef LOC_H
#define LOC_H

typedef struct {
    const char* file;
    int line;
} Loc;

#define loc_new() ((Loc) {.file = __FILE__, .line = __LINE__})

#endif // LOC_H

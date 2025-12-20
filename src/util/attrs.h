#ifndef ATTRS_H
#define ATTRS_H

typedef uint16_t Attrs;

#define ATTR_COUNT 1
#define ATTR_ALLOW_UNINIT (1 << 0)

static_assert(__LINE__ == 9, "update ATTR_COUNT");

#endif // ATTRS_H

#include "app/app_buffer.h"
#include <assert.h>
#include <string.h>

int main(void)
{
    char str[16];
    Buffer buffer;
    app_buffer_init(&buffer, 16);

    app_buffer_write(&buffer, "Hello, world!", 13);

    assert(buffer.len == 13);
    assert(buffer.start == 0);

    app_buffer_read(&buffer, str, 7);

    assert(buffer.len == 6);
    assert(buffer.start == 7);

    app_buffer_write(&buffer, "Eat cake!", 9);

    assert(buffer.len == 15);
    assert(buffer.start == 7);

    app_buffer_read(&buffer, str, 15);

    assert(buffer.len == 0);
    assert(buffer.start == 6);
    assert(strncmp(str, "world!Eat cake!", 15) == 0);

    return 0;
}

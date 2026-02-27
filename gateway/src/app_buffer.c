#include "../include/app_buffer.h"
#include <string.h>
#include <stdlib.h>
#include "../thirdparty/log.c/log.h"

int app_buffer_init(Buffer *buffer, int size)
{
    if (!buffer) {
        log_warn("Buffer pointer is NULL");
        return -1;
    }
    
    buffer->ptr = malloc(size);
    if (!buffer->ptr)
    {
        log_warn("Not enough memory for buffer %p", buffer);
        return -1;
    }

    if (pthread_mutex_init(&buffer->lock, NULL) != 0) {
        log_warn("Failed to initialize mutex for buffer");
        free(buffer->ptr);
        buffer->ptr = NULL;
        return -1;
    }
    
    buffer->size = size;
    buffer->start = 0;
    buffer->len = 0;
    log_debug("Buffer %p created", buffer);

    return 0;
}

int app_buffer_read(Buffer *buffer, void *buf, int len)
{
    if (!buffer || !buf)
    {
        log_warn("Buffer or buf not valid");
        return -1;
    }

    pthread_mutex_lock(&buffer->lock);
    if (len > buffer->len)
    {
        len = buffer->len;
    }
    if (len == 0)
    {
        pthread_mutex_unlock(&buffer->lock);
        return 0;
    }

    if (buffer->start + len <= buffer->size)
    {
        memcpy(buf, buffer->ptr + buffer->start, len);
        buffer->start += len;
    }
    else
    {
        int first_len = buffer->size - buffer->start;
        memcpy(buf, buffer->ptr + buffer->start, first_len);
        memcpy(buf + first_len, buffer->ptr, len - first_len);
        buffer->start = len - first_len;
    }
    buffer->len -= len;

    pthread_mutex_unlock(&buffer->lock);
    log_trace("Buffer status after read: start %d, len %d", buffer->start, buffer->len);
    return len;
}


int app_buffer_peek(Buffer *buffer, void *buf, int len)
{
    if (!buffer || !buf)
    {
        log_warn("Buffer or buf not valid");
        return -1;
    }

    pthread_mutex_lock(&buffer->lock);
    
    if (len > buffer->len)
    {
        len = buffer->len;
    }
    if (len == 0)
    {
        pthread_mutex_unlock(&buffer->lock);
        return 0;
    }

    if (buffer->start + len <= buffer->size)
    {
        memcpy(buf, buffer->ptr + buffer->start, len);
    }
    else
    {
        int first_len = buffer->size - buffer->start;
        memcpy(buf, buffer->ptr + buffer->start, first_len);
        memcpy(buf + first_len, buffer->ptr, len - first_len);
    }

    pthread_mutex_unlock(&buffer->lock);
    return len;
}


int app_buffer_write(Buffer *buffer, void *buf, int len)
{
    if (!buffer || !buf)
    {
        log_warn("Buffer or buf not valid");
        return -1;
    }
    pthread_mutex_lock(&buffer->lock);
    if (len > buffer->size - buffer->len)
    {
        pthread_mutex_unlock(&buffer->lock);
        log_warn("Buffer %p is not enough", buffer);
        return -1;
    }

    int write_offset = buffer->start + buffer->len;
    if (write_offset >= buffer->size)
    {
        write_offset -= buffer->size;
    }

    if (write_offset + len <= buffer->size)
    {
        memcpy(buffer->ptr + write_offset, buf, len);
    }
    else
    {
        int first_len = buffer->size - write_offset;
        memcpy(buffer->ptr + write_offset, buf, first_len);
        memcpy(buffer->ptr, buf + first_len, len - first_len);
    }
    buffer->len += len;
    pthread_mutex_unlock(&buffer->lock);
    log_trace("Buffer status after write: start %d, len %d", buffer->start, buffer->len);
    return 0;
}

void app_buffer_close(Buffer *buffer)
{
    if (!buffer) return;
    
    if (buffer->ptr)
    {
        free(buffer->ptr);
        buffer->ptr = NULL;
    }

    pthread_mutex_destroy(&buffer->lock);

    buffer->size = 0;
    buffer->len = 0;
    buffer->start = 0;
}

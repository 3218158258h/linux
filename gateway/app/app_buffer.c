#include "app_buffer.h"
#include <string.h>
#include <stdlib.h>
#include "thirdparty/log.c/log.h"

static pthread_mutex_t lock_initializer = PTHREAD_MUTEX_INITIALIZER;

int app_buffer_init(Buffer *buffer, int size)
{
    buffer->ptr = malloc(size);
    if (!buffer->ptr)
    {
        log_warn("Not enough memory for buffer %p", buffer);
        return -1;
    }

    memcpy(&buffer->lock, &lock_initializer, sizeof(pthread_mutex_t));
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

    // 首先要确定实际读取的数据长度有多长
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

    // 再次判断是否能够一次读取
    if (buffer->start + len <= buffer->size)
    {
        // 读一次
        memcpy(buf, buffer->ptr + buffer->start, len);
        buffer->start += len;
    }
    else
    {
        // 读两次
        // 第一次读取的长度为
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

int app_buffer_write(Buffer *buffer, void *buf, int len)
{
    if (!buffer || !buf)
    {
        log_warn("Buffer or buf not valid");
        return -1;
    }
    pthread_mutex_lock(&buffer->lock);
    // 判断数据能不能写进去
    if (len > buffer->size - buffer->len)
    {
        // 剩余空间无法写入
        pthread_mutex_unlock(&buffer->lock);
        log_warn("Buffer %p is not enough", buffer);
        return -1;
    }
    // 考虑数据是分一次写还是两次写
    // 首先找到写入的起点
    int write_offset = buffer->start + buffer->len;
    if (write_offset > buffer->size)
    {
        write_offset -= buffer->size;
    }

    // 判断写入起点到尾部还有多少空间
    if (write_offset + len <= buffer->size)
    {
        // 一次就能写入
        memcpy(buffer->ptr + write_offset, buf, len);
    }
    else
    {
        // 需要两次写入
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
    if (buffer->ptr)
    {
        free(buffer->ptr);
        buffer->ptr = NULL;
    }

    buffer->size = 0;
    buffer->len = 0;
}

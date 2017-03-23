#include <string.h>
#include <sys/types.h>
#include <util/ring.h>

ssize_t ring_write(char* buf, size_t head, volatile size_t* tail, size_t size, const char* data, size_t len) {
	if(*tail < head) {
		size_t len0 = head - *tail - 1;
		if(len0 > len)
			len0 = len;
		
		memcpy(buf + *tail, data, len0);
		*tail += len0;

		return len0;
	} else {
		size_t len1 = size - *tail;
		size_t len2 = head;

		if(len1 + len2 > len) {
			if(len1 >= len) {
				len1 = len;
				len2 = 0;
			} else
				len2 = len - len1;
		} else {
			len1 = 0;
			len2 = 0;
		}
		
		memcpy(buf + *tail, data, len1);
		memcpy(buf, data + len1, len2);
		
		*tail = (*tail + len1 + len2) % size;
		return len1 + len2;
	}
}

ssize_t ring_read(char* buf, volatile size_t *head, size_t tail, size_t size, char* data, size_t len) {
	if(*head <= tail) {
		size_t len0 = tail - *head;
		if(len0 > len)
			len0 = len;
		
		memcpy(data, buf + *head, len0);
		*head += len0;
		
		return len0;
	} else {
		size_t len1 = size - *head;
		size_t len2 = tail;
		
		if(len1 >= len) {
			len1 = len;
			len2 = 0;
		} else if(len1 + len2 > len) {
			len2 = len - len1;
		}
		
		memcpy(data, buf + *head, len1);
		memcpy(data + len1, buf, len2);
		
		*head = (*head + len1 + len2) % size;
		
		return len1 + len2;
	}
}

size_t ring_readable(size_t head, size_t tail, size_t size) {
	return head <= tail ? tail - head : size - head + tail;
}

size_t ring_writable(size_t head, size_t tail, size_t size) {
	return tail < head ? head - tail - 1 : size - tail + head;
}

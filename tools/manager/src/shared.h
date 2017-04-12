#ifndef __SHARED_H__
#define __SHARED_H__

#include <stdint.h>
#include "mp.h"

#define SHARED_MAGIC		0x481230420134f090

struct _FIFO;
typedef struct {
	struct _FIFO*		icc_queue;
	volatile uint8_t 	icc_queue_lock;
} ICC;

typedef struct {
	uint8_t	mp_cores[MP_MAX_CORE_COUNT];
		
	volatile uint8_t	sync[3];

	struct _FIFO*		icc_pool;
	volatile uint8_t	icc_lock_alloc;
	volatile uint8_t	icc_lock_free;
	ICC*			icc_queues;

	uint64_t		magic;
} __attribute__ ((packed)) Shared;

Shared* shared;

int shared_init();

#endif /* __SHARED_H__ */

#include <stdio.h>
#include <util/arraylist.h>
#include <util/linkedlist.h>
#include <util/arrayqueue.h>
#include <util/hashmap.h>

int main(int argc, const char *argv[])
{
	ArrayList* list = arraylist_create(DATATYPE_UINT64, POOLTYPE_LOCAL, 1000);
	LinkedList* list2 = linkedlist_create(DATATYPE_UINT64, POOLTYPE_LOCAL);
	ArrayQueue* queue = arrayqueue_create(DATATYPE_UINT64, POOLTYPE_LOCAL, 1000);
	HashMap* map = hashmap_create(DATATYPE_UINT64, POOLTYPE_LOCAL, 1000);

	map->put(map, (void*)4, 10);
	map->put(map, (void*)5, 11);
	map->put(map, (void*)6, 12);
	map->put(map, (void*)7, 13);
	map->put(map, (void*)8, 14);
	map->put(map, (void*)9, 15);
	map->put(map, (void*)0, 16);
	map->put(map, (void*)4, 17);
	map->put(map, (void*)4, 18);
	map->put(map, (void*)4, 19);
	map->put(map, (void*)4, 20);

/*
 *        void* element;
 *        typeof(*list2->iter) iter;
 *        printf("Size : %d\n", sizeof(iter));
 *        Iterator* iterOps = list2->iter;
 *        printf("Size : %p\n", iterOps);
 *        iterOps->init(&iter, list2);
 *        element = iterOps->next(&iter);
 *
 *        for_each(element, list2) {
 *                printf("%d\n", (int)element);
 *        }
 */
	return 0;
}

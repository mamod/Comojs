#include "data/queue.h"
#include "data/heap.h"

#define container_of(ptr, type, member) \
  ((type *) ((char *) (ptr) - offsetof(type, member)))

unsigned int _data_counter = 0;

typedef struct {
    unsigned int id;
    QUEUE queue;
} data_queue;

typedef struct {
    void* min;
	unsigned int nelts;
} data_heap;

typedef struct {
	unsigned int id;
    void* heap_node[3];
} data_heap_node;

static const int queue_init(duk_context *ctx) {
    data_queue *t = (data_queue *)malloc(sizeof(data_queue));
	t->id = ++_data_counter;
    QUEUE_INIT(&t->queue);
	
	duk_push_global_stash(ctx);
	duk_get_prop_string(ctx, -1, "bindings");
	duk_push_number(ctx, (unsigned int)_data_counter);
	duk_dup(ctx, 0);
	duk_put_prop(ctx, -3);
	
    duk_push_pointer(ctx, t);
	return 1;
}

static const int queue_insert_tail(duk_context *ctx) {
    QUEUE *t1 = (void *)duk_get_pointer(ctx, 0);
    QUEUE *t2 = (void *)duk_get_pointer(ctx, 1);
    QUEUE_INSERT_TAIL(t1, t2);
	return 1;
}

static const int queue_data(duk_context *ctx) {
    data_queue *data = (void *)duk_get_pointer(ctx, 0);
	
	if (data == NULL){
		printf("DATA UNDFINED\n");
		duk_push_undefined(ctx);
		return 1;
	}
	
	duk_push_global_stash(ctx);
	duk_get_prop_string(ctx, -1, "bindings");
	duk_push_number(ctx, (unsigned int)data->id);
	duk_get_prop(ctx, -2);
	return 1;
}

static const int queue_isempty(duk_context *ctx) {
    data_queue *data = (void *)duk_get_pointer(ctx, 0);
    if (QUEUE_EMPTY(&data->queue) == 0){
        duk_push_boolean(ctx,0);
    } else {
        duk_push_boolean(ctx, 1);
    }
	return 1;
}

static const int queue_empty(duk_context *ctx) {
    QUEUE *ptr = (void *)duk_get_pointer(ctx, 0);
    free(ptr);
    ptr = NULL;
	return 1;
}

static const int queue_remove(duk_context *ctx) {
    data_queue *data = (void *)duk_get_pointer(ctx, 0);
    QUEUE_REMOVE(&data->queue);
    duk_push_global_stash(ctx);
	duk_get_prop_string(ctx, -1, "bindings");
	duk_push_number(ctx, (unsigned int)data->id);
	duk_del_prop(ctx, -2);
	free(&data->queue);
	free(data);
	data = NULL;
	return 1;
}

const duk_function_list_entry queue_funcs[] = {
	{ "init", queue_init, 1 },
    { "insert_tail", queue_insert_tail, 2 },
    { "isEmpty", queue_isempty, 1 },
    { "empty", queue_empty, 1 },
    { "remove", queue_remove, 1 },
	{ "data", queue_data, 1 },
    { NULL, NULL, 0 }
};

static const int init_binding_queue(duk_context *ctx) {
	duk_push_object(ctx);
	duk_put_function_list(ctx, -1, queue_funcs);
	return 1;
}


/* HEAP */

static const int d_heap_init(duk_context *ctx) {
    data_heap *h = (data_heap *)malloc(sizeof(data_heap));
    heap_init((struct heap*) h);
    duk_push_pointer(ctx, h);
	return 1;
}

static const int d_heap_init_node(duk_context *ctx) {
    data_heap_node *h = (data_heap_node *)malloc(sizeof(data_heap_node));
	h->id = ++_data_counter;
	
	duk_push_global_stash(ctx);
	duk_get_prop_string(ctx, -1, "bindings");
	duk_push_number(ctx, _data_counter);
	duk_dup(ctx, 0);
	duk_put_prop(ctx, -3);
	
    duk_push_pointer(ctx, h->heap_node);
	return 1;
}


static int timer_less_than(const struct heap_node* ha,
							const struct heap_node* hb) {
	//const uv_timer_t* a;
	//const uv_timer_t* b;
	//a = container_of(ha, const uv_timer_t, heap_node);
	//b = container_of(hb, const uv_timer_t, heap_node);
	//if (a->timeout < b->timeout)
	//return 1;
	//if (b->timeout < a->timeout)
	//return 0;
	///* Compare start_id when both have the same timeout. start_id is
	//* allocated with loop->timer_counter in uv_timer_start().
	//*/
	//if (a->start_id < b->start_id)
	//return 1;
	//if (b->start_id < a->start_id)
	//return 0;
	return 0;
}

static const int d_heap_insert(duk_context *ctx) {
	struct heap *heap = (void *)duk_get_pointer(ctx, 0);
	struct heap_node *heap_node = (void *)duk_get_pointer(ctx, 1);
    heap_insert(heap,heap_node,timer_less_than);
	return 1;
}

static const int d_heap_remove(duk_context *ctx) {
	struct heap *heap = (void *)duk_get_pointer(ctx, 0);
	struct heap_node *heap_node = (void *)duk_get_pointer(ctx, 1);
    heap_insert(heap,heap_node,timer_less_than);
	return 1;
}

static const int d_heap_min(duk_context *ctx) {
	const struct heap_node* heap_node;
	const struct heap* heap = (void *)duk_get_pointer(ctx, 0);
	heap_node = heap_min(heap);
	const data_heap_node* handle;
	if (heap_node == NULL){
		duk_push_undefined(ctx);
		return 1;
	}
	
	handle = container_of(heap_node, data_heap_node, heap_node);
	printf("XXXXX: %i\n", handle->id);
	
	duk_push_global_stash(ctx);
	duk_get_prop_string(ctx, -1, "bindings");
	duk_push_number(ctx, (unsigned int)handle->id);
	duk_get_prop(ctx, -2);
	
	return 1;
}

const duk_function_list_entry heap_funcs[] = {
	{ "init", d_heap_init, 1 },
	{ "init_node", d_heap_init_node, 1 },
	{ "min", d_heap_min, 1 },
	{ "insert", d_heap_insert, 2 },
	{ "remove", d_heap_remove, 2 },
    { NULL, NULL, 0 }
};

static const int init_binding_heap(duk_context *ctx) {
	duk_push_object(ctx);
	duk_put_function_list(ctx, -1, heap_funcs);
	return 1;
}

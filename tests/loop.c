/* TODO */

#include "../src/loop/core.h"


int test_cb (evHandle *handle){
    evTimer *timer = handle->ev;
    printf("just called %i\n", timer->start_id);
}

int COUNTER = 0;
int test_cb2 (evHandle *handle){
    evTimer *timer = handle->ev;
    printf("cb2 %i, %p\n", COUNTER++, timer);
    timer_stop(handle);
}

int test_cb3 (evHandle *handle){
    evTimer *timer = handle->ev;
    printf("cb2xxxxxxxxxxxxxxxxxxxxxxxxxxx\n");
    // timer_stop(handle);
    exit(1);
}

int main (){
    
    printf("start\n");
    
    evLoop *loop = loop_init();
    
    evHandle *handle  = handle_init(loop,test_cb);

    evHandle *handle2 = handle_init(loop,test_cb3);
    
    timer_start(handle, 1, 1);
    timer_start(handle2, 1000, 0);

    evHandle *handlex = handle_init(loop,test_cb3);
    handle_start(handlex);
    
    loop_start(loop, 0);
    printf("TIME : %u\n", loop->time);
    
    printf("end\n");
}

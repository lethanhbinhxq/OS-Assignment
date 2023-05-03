#include<stdio.h>

struct queue{
    int * proc[10];
    int size;
};

int empty(struct queue * q) {
        if (q == NULL) return 1;
	return (q->size == 0);
}

void enqueue(struct queue * q, int * data){
    if(q == NULL || q->size == 10) return;
    q->proc[q->size] = data;
    q->size++;
}

void dequeue(struct queue * q){
    if(q == NULL || q->size == 0) return;
    int get = 0;
    for(int i = 1; i < q->size; i++){
        if(*(q->proc[get]) > *(q->proc[i])){
            get = i;
        }
    }
    for(int i = get; i < q->size; i++){
        q->proc[i] = q->proc[i+1];
    }
    q->size--;
}

int main()
{
    struct queue q;
    q.size = 0;
    int arr[] = {3,8,9,5,7,4,3,6,7,1};
    for(int i = 0; i < 10; i++){
        enqueue(&q, &arr[i]);
    }
    dequeue(&q);
    dequeue(&q);
    for(int i = 0; i < q.size; i++){
        printf("q[%d] = %d\n", i, *(q.proc[i]));
    }
    return 0;
}

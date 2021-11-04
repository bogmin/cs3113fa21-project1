#include <stdio.h>
#include <string.h>
#include<stdlib.h>
#include<stdbool.h>
#include<stdint.h>
#include <assert.h>

typedef struct thread_t {
    bool started; //false until at least one instruction in the thread has started executing
    int turnaround_time; //number of time units from when this instruction was queued until it was finished
    int wait_time; //number of time units this instruction spends waiting on the ready queue
    int response_time; //number of time units from when this instruction was queued until it started
    int time_last_running; //the time at which the last instruction in this thread was running
    bool executing; //true if the thread is currently being executed in the simulation, false otherwise
    int nonvoluntary_context_switches; //the number of times this thread has had a nonvoluntary context switch imposed on it
} thread_t;


typedef struct data_t {
    int pid; //the process id for this instruction
    int burst; //the number of time units this instruction needs to run
    int priority; //the priority of this instruction (0 has highest priority)

    //bool started; //true if this instruction
    //  double turnaround_time;
    //   double wait_time; //number of time units this instruction spends waiting on the ready queue
    //   double response_time; //number of time units from when this instruction was queued until it started
    //  bool last; //true if this instruction_t is the last with this pid in the simulation, false otherwise
} data_t;

typedef struct execution_element_t {
    bool working; //indicates whether this execution element is currently working a task
    data_t * current_instruction; //the current task this execution element is working on, NULL if working is false
    uint32_t work_time; //the number of time units this element has spent working since simulation start
    uint32_t idle_time; //the number of time units this element has spent idle since simulation start
    uint32_t tasks_completed; //the number of instructions this element has completed since simulation start
} execution_element_t;

typedef struct element_t {
    void * prev; //the previous element in the list
    void * next; //the next element in the list
    data_t data; //the current data element of this linked list
} element_t;

typedef struct linked_list_t {
    element_t * head; //pointer to the current element the linked list is on
    element_t * first; //the first element on the list
    element_t * last; //the last element on the list
} linked_list_t;

//create a new linked list and return a pointer to its control block
linked_list_t * ll_create(void){
    linked_list_t * to_return = malloc(sizeof(linked_list_t));

    to_return->head = NULL;
    to_return->first = NULL;
    to_return->last = NULL;
    return to_return;

}

//copy the contents of ele and append it to the linked list tail
void ll_append(linked_list_t * list, data_t * data){

    element_t * new_ele = malloc(sizeof(element_t));
    memcpy(&new_ele->data, data, sizeof(data_t));

    element_t * last = list->last;
    if (last != NULL){
        last->next = (void *)new_ele;
    }
    new_ele->prev = (void *)list->last;
    list->last = new_ele;
    new_ele->next = NULL;
    if (list->first == NULL){list->first = new_ele;}
    //printf("new element created at 0x%x\n", new_ele);

}

//return the first element in the linked list
data_t * ll_first(linked_list_t * list){
    list->head = list->first;

    element_t * head = list->head;
    return &(head->data);
}

//get the next element of the linked list, return NULL if we are at the end of the list
data_t * ll_next(linked_list_t * list){
    element_t * head = list->head;
    //printf("current head 0x%x\n", list->head);

    element_t * next = (element_t *)(head->next);
    list->head = next;
    //printf("new head 0x%x\n", list->head);

    if (next == NULL){
        return NULL;
    }
    data_t * to_return = &(next->data);

    return to_return;
}

double find_cpu_util(int num_processors){
    double cpu = 100.00 / num_processors;
    return cpu;
}

//pop an element off the front of the linked list, return NULL if there is no element to return
//data_t * ll_pop_front() {}


//void find_throughput(float totalBurst){
//    throughput = p / totalBurst;
//}


int main(int argc, char **argv) {

    FILE *fp = argc > 1 ? fopen (argv[1], "r") : stdin; //read in file given as argv[1] or read stdin
    if (!fp){
        fprintf(stderr, "file open failed '%s'\n",argv[1]);
    }
    int i;
    int num_processors;
    int num_threads;
    int num_instructions;
    //read in first two lines
    fscanf(fp,"%d",&num_processors);
    fscanf(fp,"%d",&num_threads);
    fscanf(fp,"%d",&num_instructions);



    thread_t threads[num_threads]; //create an array of threads that will store the results of each thread's execution
    memset(threads, 0, sizeof(thread_t)*num_threads);

    linked_list_t * input_queue = ll_create(); //store instructions as they are read from stdin


    //int array[N];
    int pid;
    int burst;
    int priority;
    double cpu;
    //float totalBurst;

    //read in the inputs

    data_t data;
    memset(&data, 0, sizeof(data_t));

    //while (!feof(stdin)){
    for (i=0; i<num_instructions; i++){
        fscanf(fp,"%d",&pid);
        fscanf(fp,"%d",&burst);
        fscanf(fp,"%d",&priority);
        data.pid = pid;
        data.burst = burst;
        data.priority = priority;
        ll_append(input_queue,&data);
    }
    //}

    //input_queue is filled now

    //simulation starts here

    uint32_t simulation_counter=0; //keep track of the number of time units the simulation has ran
    data_t * current_instruction;
    data_t * prev_instruction = NULL;
    thread_t * t; //the current thread in simulation

    //iterate over the instructions
    current_instruction = ll_first(input_queue);
    while (current_instruction != NULL){
        //printf("pid %d burst %d\n",current_instruction->pid, current_instruction->burst );
        pid = current_instruction->pid;

        assert(pid>0);
        assert(pid <= num_threads); //check for an error condition caused by malformed inputs

        t = &threads[pid-1];

        //the simulation_counter equals the moment this new instruction began running

        if (t->started == false){ //if this is the first instruction for the thread
            t->started = true;
            t->response_time = simulation_counter;
            t->wait_time = simulation_counter;
        }
        else if (t->time_last_running > 0) { //this is not the first instruction in this thread to run
            t->wait_time += (simulation_counter - t->time_last_running); //wait time accrued for this thread while it wasn't running

            if (prev_instruction != NULL) {
                if (prev_instruction->pid != current_instruction->pid) {
                    t->nonvoluntary_context_switches++; //this thread has a non-voluntary context switch imposed on it
                }
            }

        }

        simulation_counter += current_instruction->burst;
        //the simulation_counter equals the moment this new instruction finished

        t->time_last_running = simulation_counter; //this will mark the time at which this thread was last being executed

        t->turnaround_time = simulation_counter; //if this is the last instruction for the thread, then this is its turnaround time

        prev_instruction = current_instruction;
        current_instruction = ll_next(input_queue);
        if (current_instruction == NULL){break;}
    }

    //calculate the final values that will be output
    int total_nonvoluntary_context_switches = 0;
    double avg_turnaround_time;
    double avg_wait_time;
    double avg_response_time;
    double avg_throughput;

    //for(i=0; i<num_threads; i++){printf("%d %d %d\n", threads[i].turnaround_time, threads[i].wait_time, threads[i].response_time);}

    for(i=0; i<num_threads; i++){
        total_nonvoluntary_context_switches += threads[i].nonvoluntary_context_switches;
        avg_turnaround_time += threads[i].turnaround_time;
        avg_wait_time += threads[i].wait_time;
        avg_response_time += threads[i].response_time;
    }

    avg_turnaround_time = avg_turnaround_time / num_threads;
    avg_wait_time       = avg_wait_time / num_threads;
    avg_response_time   = avg_response_time / num_threads;

    cpu = find_cpu_util(num_processors);

    printf("%d\n", num_threads);
    printf("%d\n", total_nonvoluntary_context_switches);
    printf("%.2f\n", cpu);
    printf("%.2f\n", (num_threads * 1.0)/(simulation_counter * 1.0));
    printf("%.2f\n", avg_turnaround_time);
    printf("%.2f\n", avg_wait_time);
    printf("%.2f\n", avg_response_time);

    if (fp != stdin){
        fclose(fp);
    }
    //pidArray(list, array);
    //totalBurst = find_total_burst(list);
    //find_wait(totalBurst);
    //find_response(totalBurst);
    //find_throughput(totalBurst);
}

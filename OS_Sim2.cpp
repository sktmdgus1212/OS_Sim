// OS_Sim.cpp : �� ���Ͽ��� 'main' �Լ��� ���Ե˴ϴ�. �ű⼭ ���α׷� ������ ���۵ǰ� ����˴ϴ�.
//��ǻ�Ͱ��к�
//2018920017
//���α�

//block�� �Ǿ����� �ʰ� ready_q �켱������ ���� q�� ���־�� ���� Ȯ���� ������

#include <iostream>
#include <thread>
#include <Windows.h>
#include <mutex>              // std::mutex, std::unique_lock
#include <condition_variable>

#define TIME_QUANTUM    5

#define PROCESS_READY   0
#define PROCESS_RUN     1
#define PROCESS_BLOCK   2

#define SCHED_IO_REQ            0
#define SCHED_QUANTUM_EXPIRED   1
#define NUMBER_OF_PRIORITY   5
#define PROC_NUM   5

volatile int cur_proc;

std::condition_variable cv;
std::mutex cv_m;

struct proc_tbl_t {
    int id;
    int priority;
    int state;
    int time_quantum;
    int waiting_time;
    std::thread th;
    std::mutex mu_lock;
    struct proc_tbl_t* prev;
    struct proc_tbl_t* next;
} proc_tbl[10];

struct proc_tbl_t ready_q[NUMBER_OF_PRIORITY];
struct proc_tbl_t block_q;
int ready_q_dekker[2]; // ready_q ��ȣ������ ���� �迭 
int ready_q_turn = 0;

void Put_Tail_Q(proc_tbl_t*, proc_tbl_t*);
proc_tbl_t* Get_Head_Q(proc_tbl_t* head);
void Print_Q(proc_tbl_t*);
void syscall_reqio();
void sys_timer_int();
void sys_scheduler(int why);
void sys_aging_time(int id);
void IO_Completion_Interrupt(int id);
void proc_1(int id);
void proc_2(int id);
void proc_3(int id);
void proc_4(int id);
void proc_5(int id);



void proc_1(int id)
{
    int r;
    std::unique_lock<std::mutex> lk(cv_m);

    for (;;) {
        cv.wait(lk, [=] {return cur_proc == id; });
       // std::cout << id << ' ';
        r = rand() % 100;
        if (r < 2) syscall_reqio();  // call IO with 10% probability
        else Sleep(10);
    }
}

void proc_2(int id)  // CPU-bound process, Do computation only, No IO Request
{
    std::unique_lock<std::mutex> lk(cv_m);

    for (;;) {
        cv.wait(lk, [=] {return cur_proc == id; });
      //  std::cout << id << ' ';
        Sleep(10);
    }
}

void proc_3(int id)
{
    int r;
    std::unique_lock<std::mutex> lk(cv_m);

    for (;;) {
        cv.wait(lk, [=] {return cur_proc == id; });
       // std::cout << id << ' ';
        r = rand() % 100;
        if (r < 3) syscall_reqio();  // call IO with 15% probability
        else Sleep(10);
    }
}

void proc_4(int id)
{
    int r;
    std::unique_lock<std::mutex> lk(cv_m);

    for (;;) {
        cv.wait(lk, [=] {return cur_proc == id; });
       // std::cout << id << ' ';
        r = rand() % 100;
        if (r < 3) syscall_reqio();  // call IO with 20% probability
        else Sleep(10);
    }
}

void proc_5(int id)
{
    int r;
    std::unique_lock<std::mutex> lk(cv_m);

    for (;;) {
        cv.wait(lk, [=] {return cur_proc == id; });
       // std::cout << id << ' ';
        r = rand() % 100;
        if (r < 3) syscall_reqio();  // call IO with 5% probability
        else Sleep(10);
    }
}

int main()
{
    printf("��ǻ�Ͱ��к�\n");
    printf("2018920017\n");
    printf("���α�\n");
    printf("----------------------------------------------------------------------\n");
    proc_tbl_t* p;

    for (int i = 0; i < NUMBER_OF_PRIORITY; i++) {
        ready_q[i].next = ready_q[i].prev = &(ready_q[i]);
    }
    block_q.next = block_q.prev = &(block_q);
    cur_proc = -1;

    p = &(proc_tbl[0]);
    p->id = 0;
    p->priority = 0;
    p->state = PROCESS_READY;
    p->th = std::thread(IO_Completion_Interrupt, 0);

    //    Print_Q(&ready_q);

    p = &(proc_tbl[1]);
    p->id = 1;
    p->priority = 0;
    p->state = PROCESS_READY;
    p->th = std::thread(proc_1, 1);

    Put_Tail_Q(&ready_q[0], p);
    //    Print_Q(&ready_q);

    p = &(proc_tbl[2]);
    p->id = 2;
    p->priority = 0;
    p->state = PROCESS_READY;
    p->th = std::thread(proc_2, 2);

    Put_Tail_Q(&ready_q[0], p);
    //    Print_Q(&ready_q);

    p = &(proc_tbl[3]);
    p->id = 3;
    p->priority = 0;
    p->state = PROCESS_READY;
    p->th = std::thread(proc_3, 3);

    Put_Tail_Q(&ready_q[0], p);
    //    Print_Q(&ready_q);

    p = &(proc_tbl[4]);
    p->id = 4;
    p->priority = 0;
    p->state = PROCESS_READY;
    p->th = std::thread(proc_4, 4);

    Put_Tail_Q(&ready_q[0], p);

    p = &(proc_tbl[5]);
    p->id = 5;
    p->priority = 0;
    p->state = PROCESS_READY;
    p->th = std::thread(proc_5, 5);

    Put_Tail_Q(&ready_q[0], p);

    p = &(proc_tbl[9]);
    p->id = 9;
    p->priority = 0;
    p->state = PROCESS_READY;
    p->th = std::thread(sys_aging_time, 0);

    //    Print_Q(&ready_q);
        // The member function will be executed in a separate thread

        // Wait for the thread to finish, this is a blocking operation

        // Now Begin Schedule
    
    sys_timer_int();

    return 0;
}

void syscall_reqio() // �����
{
    // Invoke IO Operation, 
    // Then schedule to other process
    sys_scheduler(SCHED_IO_REQ);
}

void sys_timer_int() // ���μ��� ����� �����μ��� �Ҵ��� ���� ���ǰ˻�
{
    proc_tbl_t* p;
    proc_tbl_t* wait_time_p;
    proc_tbl_t* temp;
    for (;;) {
        /* do Time Service */
        if (cur_proc == -1) {
            for (int i = 1; i <= PROC_NUM; i++) {
                wait_time_p = &(proc_tbl[i]);
                wait_time_p->waiting_time += 1; // �� ���μ������� waiting_time�� �÷���
            }
            printf("�������� ���μ���: ����\n");
            for (int i = 1; i <= PROC_NUM; i++) {
                temp = &(proc_tbl[i]);
                printf("%d��° ���μ���: ready_q_priority->%d, waiting_time->%d, state->%d\n", i, temp->priority, temp->waiting_time, temp->state);
            }
            printf("\n");
            sys_scheduler(SCHED_QUANTUM_EXPIRED);
        }
        else {
            p = &(proc_tbl[cur_proc]);
            p->time_quantum--;
            for (int i = 1; i <= PROC_NUM; i++) {
                if (i == p->id) {
                    continue; // ���� ���� ���� ���μ����� waiting_time�� �÷����� ����
                }
                wait_time_p = &(proc_tbl[i]);
                wait_time_p->waiting_time += 1; // �� ���μ������� waiting_time�� �÷���
            }
            printf("�������� ���μ���: %d,(time quantum: %d)\n", p->id, p->time_quantum);
            for (int i = 1; i <= PROC_NUM; i++) {
                temp = &(proc_tbl[i]);
                printf("%d��° ���μ���: ready_q_priority->%d, waiting_time->%d, state->%d\n", i, temp->priority, temp->waiting_time, temp->state);
            }
            printf("\n");
            if (p->time_quantum <= 0)
                sys_scheduler(SCHED_QUANTUM_EXPIRED);
        }
        Sleep(100);
    }
}

void sys_aging_time(int id) { // ���� ���� ��ٸ� ���μ��� �켱���� ������

    proc_tbl_t* wait_time_p; // waiting_time�� ����ϱ� ���� ���μ���
    proc_tbl_t* Aging_Up_p; // ���� ���� ��ٸ�, ��, �켱������ �÷������ ���μ���
    int max_index;
    int max_val;

    for (;;) {
        Sleep(1000); //�����ð����� aging�ǽ�
        max_val = -1;
        for (int i = 1; i <= PROC_NUM; i++) {
            wait_time_p = &(proc_tbl[i]);
            if (max_val < wait_time_p->waiting_time) {
                max_index = wait_time_p->id; // ��ٸ� �ð��� ���� ū ���μ��� �ε���
                max_val = wait_time_p->waiting_time; // ��ٸ� �ð��� ���� ū ���μ����� ��ٸ� �ð�
            }
        }

        if (max_val != -1) {
            Aging_Up_p = &(proc_tbl[max_index]); // ���� ���� ��ٸ� ���μ���
            Aging_Up_p->waiting_time = 0; // ��ٸ� �ð� �ʱ�ȭ
            if (Aging_Up_p->priority != 0) {
                Aging_Up_p->priority -= 1; // ���� ���� ��ٸ� ���μ��� �켱���� ������
            }
            printf("���μ��� �켱���� ���(Aging): %d\n\n", Aging_Up_p->id);
        }
        Sleep(2000); //�����ð����� aging�ǽ�
    }
    
}

void sys_scheduler(int why) // ���� ������ ���μ��� ����
{
    proc_tbl_t* p;
    /* �л����� �� �Լ������� �ڵ带 �ۼ��ؾ� �Ѵ� */

    if (why == SCHED_IO_REQ && cur_proc != -1) { //syscall_reqio���� ȣ�� -> ����� ����
        p = &(proc_tbl[cur_proc]);
        //printf("���� ���̴� ���μ��� block(%d) ", p->id);
        p->state = PROCESS_BLOCK; // ���� ���� ���̴� ���μ��� block
        Put_Tail_Q(&block_q, p); // block_q�� ���� ���̴� ���μ��� ����
        printf("Block�� ���μ���: %d\n\n", p->id);
        for (int i = 0; i < NUMBER_OF_PRIORITY; i++) {
            p = Get_Head_Q(&ready_q[i]); // �켱������ ������� ready_q�� �� �� ���μ��� ������
            if (p != NULL) {
                break; // ready_q�� �����ϸ� �ݺ��� ��������
            }
        }
        if (p == NULL) {
            cur_proc = -1; //��� ready_q�� ����
            return;
        }
        cur_proc = p->id; // ready_q���� ������ ���μ����� ���� ���μ����� ����
        p->state = PROCESS_RUN; // ���μ��� run ���·� ����
        p->time_quantum = TIME_QUANTUM; // time�� ���� ��
        printf("%d��° ���μ��� ���� ����\n\n", p->id);
        cv.notify_all(); // ���� ����
        return;
    }

    // -------------------------------------
    //���μ����� ������ ������ ��
    if (cur_proc != -1) {
        p = &(proc_tbl[cur_proc]);
        p->state = PROCESS_READY;
        if (p->priority != NUMBER_OF_PRIORITY - 1) { // �켱���� ���� ���� ������
            p->priority += 1; //������ ���� ���μ��� �켱���� ������
        }
        ready_q_dekker[0] = 1; //��ȣ������ ���� dekker �˰���(ready_q�� ������ ���ÿ� ���ϰ� ��)
        while (ready_q_dekker[1]) {
            if (ready_q_turn == 1) {
                ready_q_dekker[0] = 0;
                while (ready_q_turn == 1);
                ready_q_dekker[0] = 1;
            }
        }
        Put_Tail_Q(&ready_q[p->priority], p);
        ready_q_dekker[0] = 0;
        ready_q_turn = 1;
        printf("������ ��ģ ���μ���: %d\n\n", p->id);
    }
    //    Print_Q(&ready_q);
    for (int i = 0; i < NUMBER_OF_PRIORITY; i++) {
        p = Get_Head_Q(&ready_q[i]); // �켱������ ������� ready_q�� �� �� ���μ��� ������
        if (p != NULL) {
            break; // ready_q�� �����ϸ� �ݺ��� ��������
        }
    }
    if (p == NULL) { // ready_q�� �ƹ��͵� ���� ���
        cur_proc = -1;
        return;
    }

    cur_proc = p->id;
    p->state = PROCESS_RUN;
    p->time_quantum = TIME_QUANTUM;
    printf("%d��° ���μ��� ���� ����\n\n", p->id);
    cv.notify_all();   // switch to process p->id and run */
}

/* IO Interrupt Generator */
void IO_Completion_Interrupt(int id)
{
    proc_tbl_t* p;

    for (;;) {

        /* �л����� �̰��� �ڵ带 �ۼ��ؾ� �Ѵ� */
        p = Get_Head_Q(&block_q); // block_q���� ready��ų ���μ��� ������
        if (p != NULL) { // block_q�� ������� Ȯ��(������� �ʴٸ� if�� ����)
            //printf("���μ��� IO ��û �Ϸ�Ǿ����Ƿ� READY(%d) ", p->id);
            p->state = PROCESS_READY; // block_q�� �ִ� ���μ����� ready���·� ����
            p->waiting_time += 1;
            if (p->priority != 0) { // �켱���� ���� ���� ������
                p->priority -= 1; // IO ��û���� ������Ƿ� �켱���� ������
            }
            printf("%d��° ���μ��� block ����\n\n", p->id);

            ready_q_dekker[1] = 1; //��ȣ������ ���� dekker �˰���(ready_q�� ������ ���ÿ� ���ϰ� ��)
            while (ready_q_dekker[0]) {
                if (ready_q_turn == 0) {
                    ready_q_dekker[1] = 0;
                    while (ready_q_turn == 0);
                    ready_q_dekker[1] = 1;
                }
            }
            Put_Tail_Q(&ready_q[p->priority], p); // ready_q�� ����
            ready_q_dekker[1] = 0;
            ready_q_turn = 0;
        }
        Sleep(1000);   /* Wakeup in every 3 sec */
    }
}

void Put_Tail_Q(proc_tbl_t* head, proc_tbl_t* item)
{
    (head->mu_lock).lock();
    item->prev = head->prev;
    head->prev->next = item;
    item->next = head;
    head->prev = item;
    (head->mu_lock).unlock();
}

proc_tbl_t* Get_Head_Q(proc_tbl_t* head)
{
    proc_tbl_t* item;

    (head->mu_lock).lock();
    if (head->next == head) {
        (head->mu_lock).unlock();
        return NULL;
    }
    item = head->next;

    item->next->prev = head;
    head->next = item->next;

    (head->mu_lock).unlock();
    return item;
}

void Print_Q(proc_tbl_t* head)
{
    proc_tbl_t* item;

    item = head->next;
    while (item != head) {
        std::cout << item->id << ' ';
        item = item->next;
    }
    std::cout << '\n';
}


// ���α׷� ����: <Ctrl+F5> �Ǵ� [�����] > [��������� �ʰ� ����] �޴�
// ���α׷� �����: <F5> Ű �Ǵ� [�����] > [����� ����] �޴�

// ������ ���� ��: 
//   1. [�ַ�� Ž����] â�� ����Ͽ� ������ �߰�/�����մϴ�.
//   2. [�� Ž����] â�� ����Ͽ� �ҽ� ��� �����մϴ�.
//   3. [���] â�� ����Ͽ� ���� ��� �� ��Ÿ �޽����� Ȯ���մϴ�.
//   4. [���� ���] â�� ����Ͽ� ������ ���ϴ�.
//   5. [������Ʈ] > [�� �׸� �߰�]�� �̵��Ͽ� �� �ڵ� ������ ����ų�, [������Ʈ] > [���� �׸� �߰�]�� �̵��Ͽ� ���� �ڵ� ������ ������Ʈ�� �߰��մϴ�.
//   6. ���߿� �� ������Ʈ�� �ٽ� ������ [����] > [����] > [������Ʈ]�� �̵��ϰ� .sln ������ �����մϴ�.

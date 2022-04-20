// OS_Sim.cpp : 이 파일에는 'main' 함수가 포함됩니다. 거기서 프로그램 실행이 시작되고 종료됩니다.
//컴퓨터과학부
//2018920017
//나인규

//block이 되어있지 않고 ready_q 우선순위가 높은 q에 들어가있어야 실행 확률이 높아짐

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
int ready_q_dekker[2]; // ready_q 상호배제를 위한 배열 
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
    printf("컴퓨터과학부\n");
    printf("2018920017\n");
    printf("나인규\n");
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

void syscall_reqio() // 입출력
{
    // Invoke IO Operation, 
    // Then schedule to other process
    sys_scheduler(SCHED_IO_REQ);
}

void sys_timer_int() // 프로세스 실행과 새프로세스 할당을 위한 조건검사
{
    proc_tbl_t* p;
    proc_tbl_t* wait_time_p;
    proc_tbl_t* temp;
    for (;;) {
        /* do Time Service */
        if (cur_proc == -1) {
            for (int i = 1; i <= PROC_NUM; i++) {
                wait_time_p = &(proc_tbl[i]);
                wait_time_p->waiting_time += 1; // 각 프로세스마다 waiting_time을 올려줌
            }
            printf("실행중인 프로세스: 없음\n");
            for (int i = 1; i <= PROC_NUM; i++) {
                temp = &(proc_tbl[i]);
                printf("%d번째 프로세스: ready_q_priority->%d, waiting_time->%d, state->%d\n", i, temp->priority, temp->waiting_time, temp->state);
            }
            printf("\n");
            sys_scheduler(SCHED_QUANTUM_EXPIRED);
        }
        else {
            p = &(proc_tbl[cur_proc]);
            p->time_quantum--;
            for (int i = 1; i <= PROC_NUM; i++) {
                if (i == p->id) {
                    continue; // 현재 실행 중인 프로세스는 waiting_time을 올려주지 않음
                }
                wait_time_p = &(proc_tbl[i]);
                wait_time_p->waiting_time += 1; // 각 프로세스마다 waiting_time을 올려줌
            }
            printf("실행중인 프로세스: %d,(time quantum: %d)\n", p->id, p->time_quantum);
            for (int i = 1; i <= PROC_NUM; i++) {
                temp = &(proc_tbl[i]);
                printf("%d번째 프로세스: ready_q_priority->%d, waiting_time->%d, state->%d\n", i, temp->priority, temp->waiting_time, temp->state);
            }
            printf("\n");
            if (p->time_quantum <= 0)
                sys_scheduler(SCHED_QUANTUM_EXPIRED);
        }
        Sleep(100);
    }
}

void sys_aging_time(int id) { // 가장 오래 기다린 프로세스 우선순위 높여줌

    proc_tbl_t* wait_time_p; // waiting_time을 계산하기 위한 프로세스
    proc_tbl_t* Aging_Up_p; // 가장 오래 기다린, 즉, 우선순위를 올려줘야할 프로세스
    int max_index;
    int max_val;

    for (;;) {
        Sleep(1000); //일정시간마다 aging실시
        max_val = -1;
        for (int i = 1; i <= PROC_NUM; i++) {
            wait_time_p = &(proc_tbl[i]);
            if (max_val < wait_time_p->waiting_time) {
                max_index = wait_time_p->id; // 기다린 시간이 제일 큰 프로세스 인덱스
                max_val = wait_time_p->waiting_time; // 기다린 시간이 제일 큰 프로세스의 기다린 시간
            }
        }

        if (max_val != -1) {
            Aging_Up_p = &(proc_tbl[max_index]); // 가장 오래 기다린 프로세스
            Aging_Up_p->waiting_time = 0; // 기다린 시간 초기화
            if (Aging_Up_p->priority != 0) {
                Aging_Up_p->priority -= 1; // 가장 오래 기다린 프로세스 우선순위 높여줌
            }
            printf("프로세스 우선순위 상승(Aging): %d\n\n", Aging_Up_p->id);
        }
        Sleep(2000); //일정시간마다 aging실시
    }
    
}

void sys_scheduler(int why) // 다음 실행할 프로세스 선택
{
    proc_tbl_t* p;
    /* 학생들은 이 함수내에서 코드를 작성해야 한다 */

    if (why == SCHED_IO_REQ && cur_proc != -1) { //syscall_reqio에서 호출 -> 입출력 수행
        p = &(proc_tbl[cur_proc]);
        //printf("실행 중이던 프로세스 block(%d) ", p->id);
        p->state = PROCESS_BLOCK; // 현재 실행 중이던 프로세스 block
        Put_Tail_Q(&block_q, p); // block_q에 실행 중이던 프로세스 삽입
        printf("Block된 프로세스: %d\n\n", p->id);
        for (int i = 0; i < NUMBER_OF_PRIORITY; i++) {
            p = Get_Head_Q(&ready_q[i]); // 우선순위가 빠른대로 ready_q에 맨 앞 프로세스 가져옴
            if (p != NULL) {
                break; // ready_q에 존재하면 반복문 빠져나옴
            }
        }
        if (p == NULL) {
            cur_proc = -1; //모든 ready_q에 없음
            return;
        }
        cur_proc = p->id; // ready_q에서 꺼내온 프로세스를 현재 프로세스로 변경
        p->state = PROCESS_RUN; // 프로세스 run 상태로 변경
        p->time_quantum = TIME_QUANTUM; // time을 새로 줌
        printf("%d번째 프로세스 새로 시작\n\n", p->id);
        cv.notify_all(); // 실행 시작
        return;
    }

    // -------------------------------------
    //프로세스가 실행이 끝났을 때
    if (cur_proc != -1) {
        p = &(proc_tbl[cur_proc]);
        p->state = PROCESS_READY;
        if (p->priority != NUMBER_OF_PRIORITY - 1) { // 우선순위 가장 낮지 않으면
            p->priority += 1; //실행이 끝난 프로세스 우선순위 낮혀줌
        }
        ready_q_dekker[0] = 1; //상호배제를 위한 dekker 알고리즘(ready_q에 삽입을 동시에 못하게 함)
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
        printf("실행을 마친 프로세스: %d\n\n", p->id);
    }
    //    Print_Q(&ready_q);
    for (int i = 0; i < NUMBER_OF_PRIORITY; i++) {
        p = Get_Head_Q(&ready_q[i]); // 우선순위가 빠른대로 ready_q에 맨 앞 프로세스 가져옴
        if (p != NULL) {
            break; // ready_q에 존재하면 반복문 빠져나옴
        }
    }
    if (p == NULL) { // ready_q에 아무것도 없는 경우
        cur_proc = -1;
        return;
    }

    cur_proc = p->id;
    p->state = PROCESS_RUN;
    p->time_quantum = TIME_QUANTUM;
    printf("%d번째 프로세스 새로 시작\n\n", p->id);
    cv.notify_all();   // switch to process p->id and run */
}

/* IO Interrupt Generator */
void IO_Completion_Interrupt(int id)
{
    proc_tbl_t* p;

    for (;;) {

        /* 학생들은 이곳에 코드를 작성해야 한다 */
        p = Get_Head_Q(&block_q); // block_q에서 ready시킬 프로세스 가져옴
        if (p != NULL) { // block_q가 비었는지 확인(비어있지 않다면 if문 실행)
            //printf("프로세스 IO 요청 완료되었으므로 READY(%d) ", p->id);
            p->state = PROCESS_READY; // block_q에 있던 프로세스를 ready상태로 변경
            p->waiting_time += 1;
            if (p->priority != 0) { // 우선순위 가장 높지 않으면
                p->priority -= 1; // IO 요청에서 깨어났으므로 우선순위 높여줌
            }
            printf("%d번째 프로세스 block 해제\n\n", p->id);

            ready_q_dekker[1] = 1; //상호배제를 위한 dekker 알고리즘(ready_q에 삽입을 동시에 못하게 함)
            while (ready_q_dekker[0]) {
                if (ready_q_turn == 0) {
                    ready_q_dekker[1] = 0;
                    while (ready_q_turn == 0);
                    ready_q_dekker[1] = 1;
                }
            }
            Put_Tail_Q(&ready_q[p->priority], p); // ready_q에 삽입
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


// 프로그램 실행: <Ctrl+F5> 또는 [디버그] > [디버깅하지 않고 시작] 메뉴
// 프로그램 디버그: <F5> 키 또는 [디버그] > [디버깅 시작] 메뉴

// 시작을 위한 팁: 
//   1. [솔루션 탐색기] 창을 사용하여 파일을 추가/관리합니다.
//   2. [팀 탐색기] 창을 사용하여 소스 제어에 연결합니다.
//   3. [출력] 창을 사용하여 빌드 출력 및 기타 메시지를 확인합니다.
//   4. [오류 목록] 창을 사용하여 오류를 봅니다.
//   5. [프로젝트] > [새 항목 추가]로 이동하여 새 코드 파일을 만들거나, [프로젝트] > [기존 항목 추가]로 이동하여 기존 코드 파일을 프로젝트에 추가합니다.
//   6. 나중에 이 프로젝트를 다시 열려면 [파일] > [열기] > [프로젝트]로 이동하고 .sln 파일을 선택합니다.

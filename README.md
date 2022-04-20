# OS_Sim
Create OS simulator (Scheduling, I/O Interrupt) 


- OS_Sim: ready_queue가 1개 존재한다. <br>
  프로세스가 ready_queue에 들어있는 순서대로 실행한다.<br>
  중간중간 I/O Interrupt 발생하여 block_queue에 삽입되었다가 일정 시간 이후 다시 복귀한다.

- OS_Sim2: multi level feedback queue 구현,<br>
각 ready_queue는 우선순위가 존재하며 우선순위가 높은 ready_queue부터 실행한다.<br>
  중간중간 I/O Interrupt 발생하여 block_queue에 삽입되었다가 일정 시간 이후 다시 복귀한다.<br>
오랫동안 실행되지 않는 프로세스는 aging에 의해 우선순위가 상승한다.

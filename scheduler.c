#include "scheduler.h"
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

int compareByArrival(const void* a, const void* b) { // for qsort; if two tasks arrive at the same time, we use PID as tie-breaker.
    Process* p1 = (Process*)a;
    Process* p2 = (Process*)b;

    if (p1->arrivalTime < p2->arrivalTime) return -1;
    if (p1->arrivalTime > p2->arrivalTime) return 1;
    
    if (p1->pid < p2->pid) return -1;
    return 1;
}

// ---------------- Scheduling Algorithms ----------------

// FCFS Scheduling
Metrics fcfs_metrics(Process proc[], int n) {
    Metrics results = {0.0, 0.0, 0.0};

    Process* processes = (Process*)malloc(n * sizeof(Process));
    memcpy(processes, proc, n * sizeof(Process));
    qsort(processes, n, sizeof(Process), compareByArrival);

    float totalTurnaround = 0.0;
    float totalWaiting = 0.0;
    float totalResponse = 0.0;
    int currentTime = 0;

    for (int i = 0; i < n; i++) {
        Process* p = &processes[i];

        if (p->arrivalTime > currentTime) {
            currentTime = p->arrivalTime;
        }

        p->startTime = currentTime;
        currentTime += p->burstTime;
        p->completionTime = currentTime;

        int turnaroundTime = p->completionTime - p->arrivalTime;
        int waitingTime = turnaroundTime - p->burstTime;
        int responseTime = p->startTime - p->arrivalTime;

        totalTurnaround += turnaroundTime;
        totalWaiting += waitingTime;
        totalResponse += responseTime;
    }

    results.avgTurnaround = totalTurnaround / n;
    results.avgWaiting = totalWaiting / n;
    results.avgResponse = totalResponse / n;

    free(processes);
    return results;
}

// SJF Scheduling (Non-preemptive)
Metrics sjf_metrics(Process proc[], int n) {
    Metrics results = {0.0, 0.0, 0.0};

    Process* processes = (Process*)malloc(n * sizeof(Process));
    memcpy(processes, proc, n * sizeof(Process));
    qsort(processes, n, sizeof(Process), compareByArrival);

    float totalTurnaround = 0.0;
    float totalWaiting = 0.0;
    float totalResponse = 0.0;
    int currentTime = 0;

    for (int i = 0; i < n; i++) {
        Process* p = &processes[i];

        if (p->arrivalTime > currentTime) {
            currentTime = p->arrivalTime;
        }

        // find shortest job that has arrived
        int shortestIndex = -1;
        for (int j = i; j < n; j++) {
            if (processes[j].arrivalTime <= currentTime && 
                (shortestIndex == -1 || processes[j].burstTime < processes[shortestIndex].burstTime)) {
                shortestIndex = j;
            }
        }

        if (shortestIndex != -1) {
            Process* shortestProcess = &processes[shortestIndex];
            if (shortestIndex != i) {
                // swap and bring the shortest process to the front
                Process temp = processes[i];
                processes[i] = *shortestProcess;
                *shortestProcess = temp;
            }

            p = &processes[i]; // change to new shortest process

            p->startTime = currentTime;
            currentTime += p->burstTime;
            p->completionTime = currentTime;

            int turnaroundTime = p->completionTime - p->arrivalTime;
            int waitingTime = turnaroundTime - p->burstTime;
            int responseTime = p->startTime - p->arrivalTime;

            totalTurnaround += turnaroundTime;
            totalWaiting += waitingTime;
            totalResponse += responseTime;
        }
    }

    results.avgTurnaround = totalTurnaround / n;
    results.avgWaiting = totalWaiting / n;
    results.avgResponse = totalResponse / n;

    free(processes);
    return results;
}

// Round Robin Scheduling
Metrics rr_metrics(Process proc[], int n, int timeQuantum) {
    Metrics result = {0.0f, 0.0f, 0.0f};

    Process* procs = (Process*)malloc(n * sizeof(Process));
    memcpy(procs, proc, n * sizeof(Process));

    for (int i = 0; i < n; i++) {
        procs[i].remainingTime = procs[i].burstTime;
        procs[i].startTime = -1; 
    }

    qsort(procs, n, sizeof(Process), compareByArrival);

    int* ready_queue = (int*)malloc(n * sizeof(int));
    int q_head = 0, q_tail = 0, q_count = 0;

    int currentTime = 0;
    int completed_count = 0;
    int next_arrival_check_idx = 0; 

    float totalTurnaroundTime = 0;
    float totalWaitingTime = 0;
    float totalResponseTime = 0;
    
    if (n > 0 && procs[0].arrivalTime > currentTime) {
        currentTime = procs[0].arrivalTime;
    }

    while (completed_count < n) {
        while(next_arrival_check_idx < n && procs[next_arrival_check_idx].arrivalTime <= currentTime) {
            ready_queue[q_tail] = next_arrival_check_idx;
            q_tail = (q_tail + 1) % n;
            q_count++;
            next_arrival_check_idx++;
        }

        if (q_count == 0) {
            if (completed_count < n) { 
                if (next_arrival_check_idx < n) {
                     currentTime = procs[next_arrival_check_idx].arrivalTime; 
                } else {
                     break; 
                }
            }
            continue; 
        }

        int current_proc_array_idx = ready_queue[q_head];
        q_head = (q_head + 1) % n;
        q_count--;

        Process* current_p = &procs[current_proc_array_idx];

        if (current_p->startTime == -1) {
            current_p->startTime = currentTime;
        }

        int time_slice_used = timeQuantum;
        if (current_p->remainingTime <= timeQuantum) {
            time_slice_used = current_p->remainingTime;
            
            currentTime += time_slice_used;
            current_p->remainingTime = 0;
            current_p->completionTime = currentTime;
            completed_count++;

            int turnaroundTime = current_p->completionTime - current_p->arrivalTime;
            int waitingTime = turnaroundTime - current_p->burstTime;
            int responseTime = current_p->startTime - current_p->arrivalTime;

            totalTurnaroundTime += turnaroundTime;
            totalWaitingTime += waitingTime;
            totalResponseTime += responseTime;
            
        } else { 
            currentTime += time_slice_used; 
            current_p->remainingTime -= time_slice_used;

            while(next_arrival_check_idx < n && procs[next_arrival_check_idx].arrivalTime <= currentTime) {
                ready_queue[q_tail] = next_arrival_check_idx;
                q_tail = (q_tail + 1) % n;
                q_count++;
                next_arrival_check_idx++;
            }
            
            ready_queue[q_tail] = current_proc_array_idx;
            q_tail = (q_tail + 1) % n;
            q_count++;
        }
    }

    result.avgTurnaround = totalTurnaroundTime / n;
    result.avgWaiting = totalWaitingTime / n;
    result.avgResponse = totalResponseTime / n;

    free(procs);
    free(ready_queue);
    return result;
}

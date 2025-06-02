#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "scheduler.h"
// ---------------- Scheduling Algorithms ----------------
void copy_processes(Process *dest, Process *src, int n) {
    for (int i = 0; i < n; i++) {
        dest[i] = src[i];
        dest[i].remainingTime = src[i].burstTime; // Used by RR to track remaining execution time
        dest[i].startTime = -1; // -1 indicates the process has not started yet
        dest[i].completionTime = 0; // Initialize completion time
    }
}
// FCFS Scheduling
Metrics fcfs_metrics(Process proc[], int n) {
    // مرتب‌سازی فرآیندها بر اساس زمان ورود (arrivalTime)
    for (int i = 0; i < n - 1; ++i) {
        for (int j = 0; j < n - i - 1; ++j) {
            if (proc[j].arrivalTime > proc[j + 1].arrivalTime) {
                Process temp = proc[j];
                proc[j] = proc[j + 1];
                proc[j + 1] = temp;
            }
        }
    }

    int currentTime = 0;
    float totalTurnaround = 0, totalWaiting = 0, totalResponse = 0;

    for (int i = 0; i < n; ++i) {
        if (currentTime < proc[i].arrivalTime)
            currentTime = proc[i].arrivalTime;

        proc[i].startTime = currentTime;
        proc[i].completionTime = currentTime + proc[i].burstTime;

        int turnaround = proc[i].completionTime - proc[i].arrivalTime;
        int waiting = proc[i].startTime - proc[i].arrivalTime;
        int response = waiting;

        totalTurnaround += turnaround;
        totalWaiting += waiting;
        totalResponse += response;

        currentTime += proc[i].burstTime;
    }

    Metrics m;
    m.avgTurnaround = totalTurnaround / n;
    m.avgWaiting = totalWaiting / n;
    m.avgResponse = totalResponse / n;

    return m;
}

int is_completed(int completed[], int index) {
    return completed[index];
}

Metrics sjf_metrics(Process proc[], int n) {
    int completed[1000] = {0};  // آرایه‌ای برای علامت‌گذاری فرآیندهای اجرا شده
    int completed_count = 0;
    int currentTime = 0;
    float totalTurnaround = 0, totalWaiting = 0, totalResponse = 0;

    // برای ذخیره مقادیر شروع فرآیند
    int started[1000];
    for (int i = 0; i < n; i++) {
        started[i] = -1;  // هنوز شروع نشده
    }

    while (completed_count < n) {
        // انتخاب فرآیندی که آماده است (arrivalTime <= currentTime)
        // و کمترین burstTime دارد و کامل نشده است
        int idx = -1;
        int min_burst = 1 << 30; // عدد خیلی بزرگ

        for (int i = 0; i < n; i++) {
            if (!completed[i] && proc[i].arrivalTime <= currentTime) {
                if (proc[i].burstTime < min_burst) {
                    min_burst = proc[i].burstTime;
                    idx = i;
                } else if (proc[i].burstTime == min_burst) {
                    // اگر burstTime برابر بود، اولویت با arrival کمتر
                    if (proc[i].arrivalTime < proc[idx].arrivalTime)
                        idx = i;
                }
            }
        }

        if (idx == -1) {
            // هیچ فرآیندی حاضر نیست، زمان را افزایش بده (idle)
            currentTime++;
            continue;
        }

        // فرآیند انتخاب شده اجرا می‌شود
        proc[idx].startTime = currentTime;
        proc[idx].completionTime = currentTime + proc[idx].burstTime;

        int turnaround = proc[idx].completionTime - proc[idx].arrivalTime;
        int waiting = proc[idx].startTime - proc[idx].arrivalTime;
        int response = waiting;

        totalTurnaround += turnaround;
        totalWaiting += waiting;
        totalResponse += response;

        currentTime += proc[idx].burstTime;
        completed[idx] = 1;
        completed_count++;
    }

    Metrics m;
    m.avgTurnaround = totalTurnaround / n;
    m.avgWaiting = totalWaiting / n;
    m.avgResponse = totalResponse / n;

    return m;
}

// Round Robin Scheduling (Revised)
Metrics rr_metrics(Process* procs, int n, int timeQuantum) {
    Metrics m = {0, 0, 0};

    Process *plist = malloc(sizeof(Process) * n);
    if (!plist) {
        fprintf(stderr, "Memory allocation failed for RR\n");
        exit(EXIT_FAILURE);
    }
    copy_processes(plist, procs, n);

    int current_time = 0;
    int completed = 0;

    // صف ساده اما بزرگ‌تر از n (مثلا n*100) تا جلوگیری از overflow
    int max_queue_size = n * 100;
    int *queue = malloc(max_queue_size * sizeof(int));
    if (!queue) {
        fprintf(stderr, "Memory allocation failed for RR queue\n");
        free(plist);
        exit(EXIT_FAILURE);
    }
    int front = 0, rear = 0;

    // برای جلوگیری از اضافه شدن چندباره یک پردازش به صف
    bool *in_queue = calloc(n, sizeof(bool));
    if (!in_queue) {
        fprintf(stderr, "Memory allocation failed for RR in_queue\n");
        free(plist);
        free(queue);
        exit(EXIT_FAILURE);
    }

    // پردازش‌های رسیده در زمان صفر به صف اضافه می‌شوند
    for (int i = 0; i < n; i++) {
        if (plist[i].arrivalTime == 0) {
            queue[rear++] = i;
            in_queue[i] = true;
        }
    }

    while (completed < n) {
        if (front == rear) {
            // صف خالی، یعنی CPU idle؛ به زمان رسیدن پردازش بعدی بریم
            int next_arrival = -1;
            for (int i = 0; i < n; i++) {
                if (plist[i].remainingTime > 0 && !in_queue[i]) {
                    if (next_arrival == -1 || plist[i].arrivalTime < next_arrival) {
                        next_arrival = plist[i].arrivalTime;
                    }
                }
            }
            if (next_arrival == -1) break; // همه پردازش‌ها تموم شدند
            current_time = (next_arrival > current_time) ? next_arrival : current_time + 1;

            // اضافه کردن پردازش‌های رسیده به صف
            for (int i = 0; i < n; i++) {
                if (plist[i].remainingTime > 0 && !in_queue[i] && plist[i].arrivalTime <= current_time) {
                    queue[rear++] = i;
                    in_queue[i] = true;
                }
            }
            continue;
        }

        int idx = queue[front++]; // پردازش بعدی از صف خارج می‌شود

        // ثبت زمان شروع پاسخ‌دهی (اولین بار اجرا)
        if (plist[idx].startTime == -1) {
            plist[idx].startTime = current_time;
        }

        int exec_time = (plist[idx].remainingTime < timeQuantum) ? plist[idx].remainingTime : timeQuantum;
        plist[idx].remainingTime -= exec_time;
        current_time += exec_time;

        // اضافه کردن پردازش‌های رسیده در زمان جاری به صف
        for (int i = 0; i < n; i++) {
            if (plist[i].remainingTime > 0 && !in_queue[i] && plist[i].arrivalTime <= current_time) {
                queue[rear++] = i;
                in_queue[i] = true;
            }
        }

        if (plist[idx].remainingTime > 0) {
            // اگر هنوز پردازش تمام نشده، دوباره به انتهای صف اضافه شود
            queue[rear++] = idx;
        } else {
            // پردازش تمام شده، زمان پایان ثبت می‌شود
            plist[idx].completionTime = current_time;
            completed++;
            in_queue[idx] = false; // از صف خارج شد
        }
    }

    float totalTurnaround = 0, totalWaiting = 0, totalResponse = 0;
    for (int i = 0; i < n; i++) {
        int turnaround = plist[i].completionTime - plist[i].arrivalTime;
        int waiting = turnaround - plist[i].burstTime;
        int response = plist[i].startTime - plist[i].arrivalTime;

        totalTurnaround += turnaround;
        totalWaiting += waiting;
        totalResponse += response;
    }

    m.avgTurnaround = totalTurnaround / n;
    m.avgWaiting = totalWaiting / n;
    m.avgResponse = totalResponse / n;

    free(plist);
    free(queue);
    free(in_queue);

    return m;
}


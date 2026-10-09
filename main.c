/*
 * Module: Scheduling and OS-DBMS Integration
 * Developed by: Garvit Pandey
 * Project: Smart Machine and Job Scheduling System
 *
 * Responsibilities:
 * 1. FCFS Scheduling
 * 2. SJF Scheduling
 * 3. Priority Scheduling
 * 4. Job ordering
 * 5. Synchronization using POSIX Mutex
 * 6. MySQL Transaction handling
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <mysql/mysql.h>

/* -----------------------------
   Job Structure
   ----------------------------- */

typedef struct {
    int id;
    int processing_time;
    int priority;
    int machine_id;
    int start_time;
    int finish_time;
    int waiting_time;
} Job;


/* -----------------------------
   Scheduling Algorithm Names
   ----------------------------- */

const char *algorithm_name(int algorithm)
{
    if (algorithm == 1)
        return "FCFS";

    if (algorithm == 2)
        return "SJF";

    return "PRIORITY";
}


/* -----------------------------
   Job Sorting
   ----------------------------- */

void sort_jobs(Job *jobs, size_t count, int algorithm)
{
    size_t i, j;
    Job temp;

    for (i = 0; i < count; i++)
    {
        for (j = i + 1; j < count; j++)
        {
            int swap = 0;

            /* SJF */
            if (algorithm == 2 &&
                jobs[j].processing_time < jobs[i].processing_time)
            {
                swap = 1;
            }

            /* Priority Scheduling */
            else if (algorithm == 3 &&
                     jobs[j].priority < jobs[i].priority)
            {
                swap = 1;
            }

            /* Same priority -> smaller job ID first */
            else if (algorithm == 3 &&
                     jobs[j].priority == jobs[i].priority &&
                     jobs[j].id < jobs[i].id)
            {
                swap = 1;
            }

            if (swap)
            {
                temp = jobs[i];
                jobs[i] = jobs[j];
                jobs[j] = temp;
            }
        }
    }
}


/* -----------------------------
   Scheduling Selection
   ----------------------------- */

int choose_scheduling_algorithm()
{
    int algorithm;

    printf("\nScheduling Algorithm\n");
    printf("--------------------\n");
    printf("1. FCFS (First Come, First Served)\n");
    printf("2. SJF  (Shortest Job First)\n");
    printf("3. Priority Scheduling\n");

    printf("Choose algorithm: ");
    scanf("%d", &algorithm);

    if (algorithm < 1 || algorithm > 3)
    {
        printf("Invalid scheduling algorithm.\n");
        return 0;
    }

    return algorithm;
}


/* -----------------------------
   Calculate Job Times
   ----------------------------- */

void calculate_job_times(Job *job, int current_time)
{
    job->start_time = current_time;

    job->finish_time =
        job->start_time + job->processing_time;

    job->waiting_time =
        job->start_time;
}


/* -----------------------------
   POSIX Mutex
   ----------------------------- */

pthread_mutex_t allocation_mutex =
    PTHREAD_MUTEX_INITIALIZER;


/* -----------------------------
   Database Transaction
   ----------------------------- */

int start_transaction(MYSQL *conn)
{
    if (mysql_query(conn, "START TRANSACTION"))
    {
        fprintf(stderr,
                "Could not start transaction: %s\n",
                mysql_error(conn));

        return 0;
    }

    return 1;
}


int commit_transaction(MYSQL *conn)
{
    if (mysql_query(conn, "COMMIT"))
    {
        fprintf(stderr,
                "Could not commit transaction: %s\n",
                mysql_error(conn));

        mysql_query(conn, "ROLLBACK");

        return 0;
    }

    return 1;
}


void rollback_transaction(MYSQL *conn)
{
    mysql_query(conn, "ROLLBACK");
}


/* -----------------------------
   Protected Machine Allocation
   ----------------------------- */

void process_job(Job *job,
                 int machine_id,
                 int current_time,
                 MYSQL *conn)
{
    /*
     * Mutex protects the critical section.
     */

    pthread_mutex_lock(&allocation_mutex);

    job->machine_id = machine_id;

    calculate_job_times(job, current_time);

    printf("\nJob %d scheduled", job->id);
    printf("\nMachine      : %d", job->machine_id);
    printf("\nStart Time   : %d", job->start_time);
    printf("\nFinish Time  : %d", job->finish_time);
    printf("\nWaiting Time : %d\n",
           job->waiting_time);

    /*
     * Database operation can be performed
     * while the scheduling transaction is active.
     */

    pthread_mutex_unlock(&allocation_mutex);
}


/* -----------------------------
   Scheduling Demonstration
   ----------------------------- */

void run_scheduling(Job *jobs,
                    size_t job_count,
                    int algorithm,
                    MYSQL *conn)
{
    size_t i;
    int current_time = 0;

    if (job_count == 0)
    {
        printf("No jobs available.\n");
        return;
    }

    printf("\nScheduling using %s...\n",
           algorithm_name(algorithm));

    /*
     * Arrange jobs according to
     * selected scheduling algorithm.
     */

    sort_jobs(jobs, job_count, algorithm);

    /*
     * Start DBMS transaction.
     */

    if (!start_transaction(conn))
        return;

    for (i = 0; i < job_count; i++)
    {
        process_job(
            &jobs[i],
            1,
            current_time,
            conn
        );

        current_time =
            jobs[i].finish_time;
    }

    /*
     * Commit all scheduling updates.
     */

    if (!commit_transaction(conn))
    {
        rollback_transaction(conn);

        printf("\nScheduling failed.\n");
        return;
    }

    printf("\nScheduling completed successfully.\n");
}


/* -----------------------------
   Main Function
   ----------------------------- */

int main()
{
    printf("=====================================\n");
    printf(" Scheduling Module - Garvit Pandey\n");
    printf("=====================================\n");

    printf("\nThis module handles:\n");
    printf("1. FCFS Scheduling\n");
    printf("2. SJF Scheduling\n");
    printf("3. Priority Scheduling\n");
    printf("4. POSIX Mutex Synchronization\n");
    printf("5. MySQL Transactions\n");

    return 0;
}
/*
 * Module: Machine Allocation and System Workflow
 * Developed by: Krishna Pandey
 * Project: Smart Machine and Job Scheduling System
 *
 * Responsibilities:
 * 1. Machine availability checking
 * 2. Machine allocation
 * 3. Job-to-machine assignment
 * 4. Start and finish time calculation
 * 5. Overall scheduling workflow
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_MACHINES 20
#define MAX_JOBS 50
#define NAME_LEN 100


/* -----------------------------
   Structures
   ----------------------------- */

typedef struct
{
    int id;
    char name[NAME_LEN];
    int available_at;
} Machine;


typedef struct
{
    int id;
    char name[NAME_LEN];
    int processing_time;
    int priority;

    int machine_id;
    int start_time;
    int finish_time;
    int waiting_time;

} Job;


/* -----------------------------
   Display Machines
   ----------------------------- */

void display_machines(
    Machine machines[],
    int machine_count
)
{
    int i;

    printf("\nAvailable Machines\n");
    printf("---------------------------\n");

    for (i = 0; i < machine_count; i++)
    {
        printf(
            "Machine ID: %d | Name: %s | "
            "Available At: %d\n",
            machines[i].id,
            machines[i].name,
            machines[i].available_at
        );
    }
}


/* -----------------------------
   Find Earliest Available Machine
   ----------------------------- */

int find_available_machine(
    Machine machines[],
    int machine_count
)
{
    int i;
    int selected = 0;

    for (i = 1; i < machine_count; i++)
    {
        if (machines[i].available_at <
            machines[selected].available_at)
        {
            selected = i;
        }
    }

    return selected;
}


/* -----------------------------
   Allocate Machine to Job
   ----------------------------- */

void allocate_machine(
    Job *job,
    Machine machines[],
    int machine_count
)
{
    int selected;

    if (machine_count <= 0)
    {
        printf("No machines available.\n");
        return;
    }

    selected = find_available_machine(
        machines,
        machine_count
    );

    /*
     * Assign selected machine to job
     */

    job->machine_id =
        machines[selected].id;

    /*
     * Job starts when the selected
     * machine becomes available.
     */

    job->start_time =
        machines[selected].available_at;

    /*
     * Calculate finish time.
     */

    job->finish_time =
        job->start_time +
        job->processing_time;

    /*
     * Waiting time is the time spent
     * waiting for a machine.
     */

    job->waiting_time =
        job->start_time;

    /*
     * Update machine availability.
     */

    machines[selected].available_at =
        job->finish_time;

    printf(
        "\nJob %d assigned to Machine %d",
        job->id,
        job->machine_id
    );

    printf(
        "\nStart Time  : %d",
        job->start_time
    );

    printf(
        "\nFinish Time : %d",
        job->finish_time
    );

    printf(
        "\nWaiting Time: %d\n",
        job->waiting_time
    );
}


/* -----------------------------
   Process All Jobs
   ----------------------------- */

void process_jobs(
    Job jobs[],
    int job_count,
    Machine machines[],
    int machine_count
)
{
    int i;

    printf("\n==============================\n");
    printf(" Starting Machine Allocation\n");
    printf("==============================\n");

    for (i = 0; i < job_count; i++)
    {
        printf(
            "\nProcessing Job %d - %s",
            jobs[i].id,
            jobs[i].name
        );

        allocate_machine(
            &jobs[i],
            machines,
            machine_count
        );
    }
}


/* -----------------------------
   Display Final Results
   ----------------------------- */

void display_results(
    Job jobs[],
    int job_count
)
{
    int i;

    printf("\n\nFinal Scheduling Results\n");
    printf("--------------------------------------------------\n");

    printf(
        "%-5s %-15s %-10s %-10s %-10s\n",
        "ID",
        "Job",
        "Machine",
        "Start",
        "Finish"
    );

    printf(
        "--------------------------------------------------\n"
    );

    for (i = 0; i < job_count; i++)
    {
        printf(
            "%-5d %-15s %-10d %-10d %-10d\n",
            jobs[i].id,
            jobs[i].name,
            jobs[i].machine_id,
            jobs[i].start_time,
            jobs[i].finish_time
        );
    }
}


/* -----------------------------
   Complete System Workflow
   ----------------------------- */

void run_workflow()
{
    Machine machines[3] =
    {
        {1, "Machine A", 0},
        {2, "Machine B", 0},
        {3, "Machine C", 0}
    };

    Job jobs[5] =
    {
        {1, "Job A", 20, 1, 0, 0, 0, 0},
        {2, "Job B", 10, 2, 0, 0, 0, 0},
        {3, "Job C", 15, 1, 0, 0, 0, 0},
        {4, "Job D", 25, 3, 0, 0, 0, 0},
        {5, "Job E", 12, 2, 0, 0, 0, 0}
    };

    int machine_count = 3;
    int job_count = 5;

    /*
     * Step 1:
     * Display available machines.
     */

    display_machines(
        machines,
        machine_count
    );

    /*
     * Step 2:
     * Process jobs and allocate machines.
     */

    process_jobs(
        jobs,
        job_count,
        machines,
        machine_count
    );

    /*
     * Step 3:
     * Display final results.
     */

    display_results(
        jobs,
        job_count
    );

    /*
     * Step 4:
     * Display updated machine status.
     */

    printf("\nUpdated Machine Availability\n");
    display_machines(
        machines,
        machine_count
    );
}


/* -----------------------------
   Main Function
   ----------------------------- */

int main()
{
    printf(
        "============================================\n"
    );

    printf(
        " Smart Machine and Job Scheduling System\n"
    );

    printf(
        " Machine Allocation Module\n"
    );

    printf(
        " Developed by: Krishna Pandey\n"
    );

    printf(
        "============================================\n"
    );

    run_workflow();

    return 0;
}
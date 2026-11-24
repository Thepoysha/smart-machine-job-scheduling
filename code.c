#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <mysql/mysql.h>

#define MAX_JOBS 500
#define MAX_MACHINES 100
#define JOB_NAME_LEN 100

typedef struct {
    int id;
    char name[JOB_NAME_LEN];
    int processing_time;
    int priority;
    int machine_id;
    int start_time;
    int finish_time;
    int waiting_time;
} Job;

typedef struct {
    int id;
    char name[JOB_NAME_LEN];
    int available_at;
} Machine;


MYSQL *conn;
pthread_mutex_t allocation_mutex = PTHREAD_MUTEX_INITIALIZER;


void trim_newline(char *s) {
    s[strcspn(s, "\r\n")] = '\0';
}


int db_query(const char *sql) {
    if (mysql_query(conn, sql)) {
        fprintf(stderr, "MySQL error: %s\n", mysql_error(conn));
        return 0;
    }
    return 1;
}


int db_connect(void) {
    char host[100] = "localhost", user[100] = "root", password[100] = "", database[100] = "manufacturing_scheduler";
    const char *env;
    if ((env = getenv("SMJS_DB_HOST")) && *env) snprintf(host, sizeof(host), "%s", env);
    if ((env = getenv("SMJS_DB_USER")) && *env) snprintf(user, sizeof(user), "%s", env);
    if ((env = getenv("SMJS_DB_PASSWORD")) && *env) snprintf(password, sizeof(password), "%s", env);
    if ((env = getenv("SMJS_DB_NAME")) && *env) snprintf(database, sizeof(database), "%s", env);

    conn = mysql_init(NULL);
    if (!conn) {
        puts("MySQL initialization failed.");
        return 0;
    }
    if (!mysql_real_connect(conn, host, user, password, database, 0, NULL, 0)) {
        fprintf(stderr, "Connection failed: %s\n", mysql_error(conn));
        mysql_close(conn);
        return 0;
    }
    printf("Connected to MySQL database: %s\n", database);
    return 1;
}


void add_machine(void) {
    char name[JOB_NAME_LEN], escaped[JOB_NAME_LEN * 2 + 1], sql[512];
    printf("Machine name: ");
    if (!fgets(name, sizeof(name), stdin)) return;
    trim_newline(name);
    if (!name[0]) { puts("Name cannot be empty."); return; }
    mysql_real_escape_string(conn, escaped, name, (unsigned long)strlen(name));
    snprintf(sql, sizeof(sql), "INSERT INTO machines(machine_name,available_at) VALUES('%s',0)", escaped);
    if (db_query(sql)) printf("Machine '%s' added.\n", name);
}


void add_job(void) {
    char name[JOB_NAME_LEN], escaped[JOB_NAME_LEN * 2 + 1], sql[512];
    int duration, priority;
    printf("Job name: ");
    if (!fgets(name, sizeof(name), stdin)) return;
    trim_newline(name);
    if (!name[0]) { puts("Name cannot be empty."); return; }
    printf("Processing time (positive integer): ");
    if (scanf("%d", &duration) != 1 || duration <= 0) { puts("Invalid processing time."); while(getchar()!='\n'); return; }
    printf("Priority (1=highest, 5=lowest): ");
    if (scanf("%d", &priority) != 1 || priority < 1 || priority > 5) { puts("Priority must be 1-5."); while(getchar()!='\n'); return; }
    while (getchar() != '\n');
    mysql_real_escape_string(conn, escaped, name, (unsigned long)strlen(name));
    snprintf(sql, sizeof(sql), "INSERT INTO jobs(job_name,processing_time,priority,status) VALUES('%s',%d,%d,'PENDING')", escaped, duration, priority);
    if (db_query(sql)) printf("Job '%s' added as pending.\n", name);
}


int load_pending_jobs(Job jobs[]) {
    MYSQL_RES *res;
    MYSQL_ROW row;
    int n = 0;
    if (!db_query("SELECT job_id,job_name,processing_time,priority FROM jobs WHERE status='PENDING' ORDER BY job_id")) return -1;
    res = mysql_store_result(conn);
    if (!res) { fprintf(stderr, "Result error: %s\n", mysql_error(conn)); return -1; }
    while ((row = mysql_fetch_row(res)) && n < MAX_JOBS) {
        jobs[n].id = atoi(row[0]);
        snprintf(jobs[n].name, sizeof(jobs[n].name), "%s", row[1] ? row[1] : "");
        jobs[n].processing_time = atoi(row[2]);
        jobs[n].priority = atoi(row[3]);
        jobs[n].machine_id = 0;
        jobs[n].start_time = jobs[n].finish_time = jobs[n].waiting_time = 0;
        n++;
    }
    mysql_free_result(res);
    return n;
}


int load_machines(Machine machines[]) {
    MYSQL_RES *res;
    MYSQL_ROW row;
    int n = 0;
    if (!db_query("SELECT machine_id,machine_name,available_at FROM machines ORDER BY machine_id")) return -1;
    res = mysql_store_result(conn);
    if (!res) { fprintf(stderr, "Machine result error: %s\n", mysql_error(conn)); return -1; }
    while ((row = mysql_fetch_row(res)) && n < MAX_MACHINES) {
        machines[n].id = atoi(row[0]);
        snprintf(machines[n].name, sizeof(machines[n].name), "%s", row[1] ? row[1] : "");
        machines[n].available_at = atoi(row[2]);
        n++;
    }
    mysql_free_result(res);
    return n;
}


void sort_jobs(Job jobs[], int count, int algorithm) {
    int i, j;
    Job temp;
    for (i = 0; i < count - 1; i++) {
        for (j = i + 1; j < count; j++) {
            int should_swap = 0;
            if (algorithm == 2 && jobs[j].processing_time < jobs[i].processing_time) should_swap = 1;
            if (algorithm == 3 && jobs[j].priority < jobs[i].priority) should_swap = 1;
            if (algorithm == 3 && jobs[j].priority == jobs[i].priority && jobs[j].id < jobs[i].id) should_swap = 1;
            if (should_swap) { temp = jobs[i]; jobs[i] = jobs[j]; jobs[j] = temp; }
        }
    }
}


int find_earliest_machine(Machine machines[], int count) {
    int i, best = 0;
    for (i = 1; i < count; i++) if (machines[i].available_at < machines[best].available_at) best = i;
    return best;
}


int schedule_jobs(int algorithm) {
    Job jobs[MAX_JOBS];
    Machine machines[MAX_MACHINES];
    int job_count, machine_count, i, m;
    const char *algorithm_name = algorithm == 1 ? "FCFS" : algorithm == 2 ? "SJF" : "PRIORITY";
    pthread_mutex_lock(&allocation_mutex);
    job_count = load_pending_jobs(jobs);
    machine_count = load_machines(machines);
    if (job_count <= 0 || machine_count <= 0) {
        puts(job_count == 0 ? "No pending jobs to schedule." : "Unable to load scheduling data.");
        pthread_mutex_unlock(&allocation_mutex);
        return 0;
    }
    sort_jobs(jobs, job_count, algorithm);
    if (!db_query("START TRANSACTION")) { pthread_mutex_unlock(&allocation_mutex); return 0; }
    for (i = 0; i < job_count; i++) {
        m = find_earliest_machine(machines, machine_count);
        jobs[i].machine_id = machines[m].id;
        jobs[i].start_time = machines[m].available_at;
        jobs[i].finish_time = jobs[i].start_time + jobs[i].processing_time;
        jobs[i].waiting_time = jobs[i].start_time;
        machines[m].available_at = jobs[i].finish_time;
        char sql[768];
        snprintf(sql, sizeof(sql), "UPDATE jobs SET status='SCHEDULED',machine_id=%d,start_time=%d,finish_time=%d,waiting_time=%d,algorithm='%s' WHERE job_id=%d AND status='PENDING'",
                 jobs[i].machine_id, jobs[i].start_time, jobs[i].finish_time, jobs[i].waiting_time, algorithm_name, jobs[i].id);
        if (!db_query(sql)) {
            db_query("ROLLBACK");
            puts("Scheduling database update failed; transaction rolled back.");
            pthread_mutex_unlock(&allocation_mutex);
            return 0;
        }
        snprintf(sql, sizeof(sql), "UPDATE machines SET available_at=%d WHERE machine_id=%d", machines[m].available_at, machines[m].id);
        if (!db_query(sql)) {
            db_query("ROLLBACK");
            puts("Machine update failed; transaction rolled back.");
            pthread_mutex_unlock(&allocation_mutex);
            return 0;
        }
    }
    if (!db_query("COMMIT")) {
        db_query("ROLLBACK");
        pthread_mutex_unlock(&allocation_mutex);
        return 0;
    }
    printf("\nScheduled %d job(s) using %s.\n", job_count, algorithm_name);
    printf("%-6s %-20s %-10s %-10s %-10s %-10s %-10s\n", "JobID", "Job Name", "Machine", "Start", "Finish", "Waiting", "Priority");
    for (i = 0; i < job_count; i++) {
        int k;
        char machine_name[JOB_NAME_LEN] = "?";
        for (k = 0; k < machine_count; k++) if (machines[k].id == jobs[i].machine_id) { snprintf(machine_name, sizeof(machine_name), "%s", machines[k].name); break; }
        printf("%-6d %-20s %-10s %-10d %-10d %-10d %-10d\n", jobs[i].id, jobs[i].name, machine_name, jobs[i].start_time, jobs[i].finish_time, jobs[i].waiting_time, jobs[i].priority);
    }
    pthread_mutex_unlock(&allocation_mutex);
    return 1;
}


void view_jobs(void) {
    if (!db_query("SELECT job_id,job_name,processing_time,priority,status,COALESCE(machine_id,'-'),COALESCE(start_time,'-'),COALESCE(finish_time,'-'),COALESCE(waiting_time,'-'),COALESCE(algorithm,'-') FROM jobs ORDER BY job_id")) return;
    MYSQL_RES *res = mysql_store_result(conn);
    MYSQL_ROW row;
    if (!res) return;
    puts("\n--- JOB LIST ---");
    printf("%-5s %-18s %-8s %-8s %-10s %-8s %-8s %-8s %-8s %-10s\n", "ID", "Name", "Duration", "Priority", "Status", "Machine", "Start", "Finish", "Wait", "Algorithm");
    while ((row = mysql_fetch_row(res))) {
        printf("%-5s %-18s %-8s %-8s %-10s %-8s %-8s %-8s %-8s %-10s\n", row[0],row[1],row[2],row[3],row[4],row[5],row[6],row[7],row[8],row[9]);
    }
    mysql_free_result(res);
}


void view_machines(void) {
    if (!db_query("SELECT machine_id,machine_name,available_at FROM machines ORDER BY machine_id")) return;
    MYSQL_RES *res = mysql_store_result(conn);
    MYSQL_ROW row;
    if (!res) return;
    puts("\n--- MACHINE LIST ---");
    printf("%-5s %-25s %-15s\n", "ID", "Machine", "Available At");
    while ((row = mysql_fetch_row(res))) printf("%-5s %-25s %-15s\n", row[0], row[1], row[2]);
    mysql_free_result(res);
}


void reset_schedule(void) {
    puts("Reset scheduled jobs and machine availability? (y/n): ");
    char answer[10];
    if (!fgets(answer, sizeof(answer), stdin)) return;
    if (answer[0] != 'y' && answer[0] != 'Y') return;
    if (!db_query("START TRANSACTION")) return;
    if (!db_query("UPDATE jobs SET status='PENDING',machine_id=NULL,start_time=NULL,finish_time=NULL,waiting_time=NULL,algorithm=NULL")) { db_query("ROLLBACK"); return; }
    if (!db_query("UPDATE machines SET available_at=0")) { db_query("ROLLBACK"); return; }
    if (db_query("COMMIT")) puts("Schedule reset. Jobs are pending again."); else db_query("ROLLBACK");
}


int main(void) {
    int choice, algorithm;
    puts("==============================================");
    puts(" SMART MACHINE & JOB SCHEDULING SYSTEM");
    puts("==============================================");
    if (!db_connect()) {
        puts("Start MySQL, import schema.sql, and verify credentials.");
        return 1;
    }
    do {
        puts("\nMAIN MENU");
        puts("1. Add Job");
        puts("2. Add Machine");
        puts("3. View Jobs");
        puts("4. View Machines");
        puts("5. Schedule Pending Jobs");
        puts("6. Reset Schedule (make all jobs pending)");
        puts("0. Exit");
        printf("Enter choice: ");
        if (scanf("%d", &choice) != 1) { puts("Invalid input."); while (getchar() != '\n'); continue; }
        while (getchar() != '\n');
        switch (choice) {
            case 1: add_job(); break;
            case 2: add_machine(); break;
            case 3: view_jobs(); break;
            case 4: view_machines(); break;
            case 5:
                puts("\nScheduling algorithm:");
                puts("1. FCFS");
                puts("2. SJF");
                puts("3. Priority (1 is highest)");
                printf("Choose: ");
                if (scanf("%d", &algorithm) == 1 && algorithm >= 1 && algorithm <= 3) { while (getchar() != '\n'); schedule_jobs(algorithm); }
                else { puts("Invalid algorithm."); while (getchar() != '\n'); }
                break;
            case 6: reset_schedule(); break;
            case 0: puts("Exiting."); break;
            default: puts("Invalid menu option.");
        }
    } while (choice != 0);
    mysql_close(conn);
    pthread_mutex_destroy(&allocation_mutex);
    return 0;
}

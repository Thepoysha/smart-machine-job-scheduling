/*
 * Module: Database Management
 * Developed by: Pragati Gupta
 * Project: Smart Machine and Job Scheduling System
 *
 * Responsibilities:
 * 1. Database connection
 * 2. Job table operations
 * 3. Machine table operations
 * 4. Schedule data management
 * 5. Database queries
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mysql/mysql.h>

#define NAME_LEN 100

/* Database connection */
MYSQL *conn = NULL;


/* -----------------------------
   Database Connection
   ----------------------------- */

int connect_database()
{
    char host[NAME_LEN];
    char user[NAME_LEN];
    char password[NAME_LEN];
    char database[NAME_LEN];

    printf("MySQL Host [localhost]: ");
    fgets(host, sizeof(host), stdin);
    host[strcspn(host, "\n")] = '\0';

    if (host[0] == '\0')
        strcpy(host, "localhost");

    printf("MySQL Username [root]: ");
    fgets(user, sizeof(user), stdin);
    user[strcspn(user, "\n")] = '\0';

    if (user[0] == '\0')
        strcpy(user, "root");

    printf("MySQL Password: ");
    fgets(password, sizeof(password), stdin);
    password[strcspn(password, "\n")] = '\0';

    printf("Database Name [manufacturing_scheduler]: ");
    fgets(database, sizeof(database), stdin);
    database[strcspn(database, "\n")] = '\0';

    if (database[0] == '\0')
        strcpy(database, "manufacturing_scheduler");

    conn = mysql_init(NULL);

    if (conn == NULL)
    {
        printf("MySQL initialization failed.\n");
        return 0;
    }

    if (!mysql_real_connect(
            conn,
            host,
            user,
            password,
            database,
            0,
            NULL,
            0))
    {
        printf("Database connection failed: %s\n",
               mysql_error(conn));

        mysql_close(conn);
        conn = NULL;

        return 0;
    }

    printf("Connected to MySQL successfully.\n");

    return 1;
}


/* -----------------------------
   Execute SQL Query
   ----------------------------- */

int run_query(const char *query)
{
    if (mysql_query(conn, query))
    {
        fprintf(stderr,
                "Database error: %s\n",
                mysql_error(conn));

        return 0;
    }

    return 1;
}


/* -----------------------------
   Add Job
   ----------------------------- */

void add_job()
{
    char name[NAME_LEN];
    char query[512];
    int processing_time;
    int priority;

    printf("\nJob Name: ");
    fgets(name, sizeof(name), stdin);
    name[strcspn(name, "\n")] = '\0';

    printf("Processing Time: ");
    scanf("%d", &processing_time);

    printf("Priority: ");
    scanf("%d", &priority);

    getchar();

    snprintf(
        query,
        sizeof(query),
        "INSERT INTO jobs "
        "(name, processing_time, priority, status) "
        "VALUES ('%s', %d, %d, 'PENDING')",
        name,
        processing_time,
        priority
    );

    if (run_query(query))
    {
        printf("Job added successfully.\n");
        printf("Job ID: %llu\n",
               (unsigned long long)
               mysql_insert_id(conn));
    }
}


/* -----------------------------
   Add Machine
   ----------------------------- */

void add_machine()
{
    char name[NAME_LEN];
    char query[512];

    printf("\nMachine Name: ");

    fgets(name, sizeof(name), stdin);
    name[strcspn(name, "\n")] = '\0';

    snprintf(
        query,
        sizeof(query),
        "INSERT INTO machines "
        "(name, available) "
        "VALUES ('%s', 1)",
        name
    );

    if (run_query(query))
    {
        printf("Machine added successfully.\n");
        printf("Machine ID: %llu\n",
               (unsigned long long)
               mysql_insert_id(conn));
    }
}


/* -----------------------------
   Display Jobs
   ----------------------------- */

void show_jobs()
{
    MYSQL_RES *result;
    MYSQL_ROW row;

    const char *query =
        "SELECT id, name, processing_time, "
        "priority, status, "
        "COALESCE(assigned_machine_id, 0) "
        "FROM jobs ORDER BY id";

    if (!run_query(query))
        return;

    result = mysql_store_result(conn);

    if (result == NULL)
    {
        printf("Unable to read job data.\n");
        return;
    }

    printf("\n");
    printf("ID   Job   Processing Time   Priority   Status\n");
    printf("-----------------------------------------------\n");

    while ((row = mysql_fetch_row(result)) != NULL)
    {
        printf("%s   %s   %s   %s   %s\n",
               row[0],
               row[1],
               row[2],
               row[3],
               row[4]);
    }

    mysql_free_result(result);
}


/* -----------------------------
   Display Machines
   ----------------------------- */

void show_machines()
{
    MYSQL_RES *result;
    MYSQL_ROW row;

    const char *query =
        "SELECT id, name, available "
        "FROM machines ORDER BY id";

    if (!run_query(query))
        return;

    result = mysql_store_result(conn);

    if (result == NULL)
    {
        printf("Unable to read machine data.\n");
        return;
    }

    printf("\n");
    printf("ID   Machine Name   Available\n");
    printf("-----------------------------\n");

    while ((row = mysql_fetch_row(result)) != NULL)
    {
        printf("%s   %s   %s\n",
               row[0],
               row[1],
               strcmp(row[2], "1") == 0
                   ? "Yes"
                   : "No");
    }

    mysql_free_result(result);
}


/* -----------------------------
   View Schedule History
   ----------------------------- */

void show_schedule_history()
{
    MYSQL_RES *result;
    MYSQL_ROW row;

    const char *query =
        "SELECT s.id, "
        "j.name, "
        "m.name, "
        "s.algorithm, "
        "s.start_time, "
        "s.finish_time, "
        "s.waiting_time "
        "FROM schedules s "
        "JOIN jobs j ON j.id = s.job_id "
        "JOIN machines m ON m.id = s.machine_id "
        "ORDER BY s.id";

    if (!run_query(query))
        return;

    result = mysql_store_result(conn);

    if (result == NULL)
    {
        printf("Unable to read schedule history.\n");
        return;
    }

    printf("\n");
    printf("Schedule History\n");
    printf("-------------------------------\n");

    while ((row = mysql_fetch_row(result)) != NULL)
    {
        printf(
            "ID: %s | Job: %s | Machine: %s | "
            "Algorithm: %s | Start: %s | "
            "Finish: %s | Wait: %s\n",
            row[0],
            row[1],
            row[2],
            row[3],
            row[4],
            row[5],
            row[6]
        );
    }

    mysql_free_result(result);
}


int main()
{
    printf("=====================================\n");
    printf(" Database Module - Pragati Gupta\n");
    printf("=====================================\n");

    if (!connect_database())
        return EXIT_FAILURE;

    printf("\nDatabase module initialized.\n");

    mysql_close(conn);

    return EXIT_SUCCESS;
}

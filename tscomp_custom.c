/* External definitions for time-shared computer model. */
#include "simlib.h" /* Required for use of simlib.c. */

#define EVENT_ARRIVAL 1         /* Event type for arrival of job to CPU. */
#define EVENT_END_CPU_1_RUN 2   /* Event type for end of a CPU run. */
#define EVENT_END_CPU_2_RUN 3   /* Event type for end of a CPU run. */
#define EVENT_END_SIMULATION 4  /* Event type for end of the simulation. */
#define LIST_QUEUE_MAIN 1       /* List number for main queue. */
#define LIST_QUEUE_1 2          /* List number for CPU queue. */
#define LIST_QUEUE_2 3          /* List number for CPU queue. */
#define LIST_CPU_1 4            /* List number for CPU_1. */
#define LIST_CPU_2 5            /* List number for CPU_2. */
#define SAMPST_RESPONSE_TIMES 1 /* sampst variable for response times. */
#define STREAM_THINK 1          /* Random-number stream for think times. */
#define STREAM_SERVICE 2        /* Random-number stream for service times. */
#define STREAM_UNIFORM 3        /* Random-number stream for random uniform. */
#define QUEUE_CPU_LIMIT 10      /* Maximum number of jobs in one CPU queue. */

/* Declare non-simlib global variables. */
int min_terms, max_terms, incr_terms, num_terms, num_responses,
    num_responses_required, term;
float mean_think, mean_service, quantum, swap;
FILE *infile, *outfile;

/* Declare non-simlib functions. */
void arrive(void);
void start_CPU_run(int queue_number);
void end_CPU_run(int queue_number);
void report(void);

int main() /* Main function. */
{
    /* Open input and output files. */
    infile = fopen("tscomp.in", "r");
    outfile = fopen("tscomp.out", "w");

    /* Read input parameters. */
    int test_case = 1;

    while (fscanf(infile, "%d %d %d %d %f %f %f %f",
                  &min_terms, &max_terms, &incr_terms, &num_responses_required,
                  &mean_think, &mean_service, &quantum, &swap) == 8)
    {

        /* Write report heading and input parameters. */
        fprintf(outfile, "\n\nTest Case %d\n", test_case);
        fprintf(outfile, "Time-shared computer model\n\n");
        fprintf(outfile, "Number of terminals%9d to%4d by %4d\n\n",
                min_terms, max_terms, incr_terms);
        fprintf(outfile, "Mean think time  %11.3f seconds\n\n", mean_think);
        fprintf(outfile, "Mean service time%11.3f seconds\n\n", mean_service);
        fprintf(outfile, "Quantum          %11.3f seconds\n\n", quantum);
        fprintf(outfile, "Swap time        %11.3f seconds\n\n", swap);
        fprintf(outfile, "Number of jobs processed%12d\n\n\n",
                num_responses_required);
        fprintf(outfile, "Number of      Average         Average        Average         Average");
        fprintf(outfile, "         Utilization\n");
        fprintf(outfile, "terminals   response time  N in queue 1       of CPU 1     N in queue 2       of CPU 2");

        /* Run the simulation varying the number of terminals. */
        for (num_terms = min_terms; num_terms <= max_terms;
             num_terms += incr_terms)
        {

            /* Initialize simlib */
            init_simlib();

            /* Set maxatr = max(maximum number of attributes per record, 4) */
            maxatr = 8; /* NEVER SET maxatr TO BE SMALLER THAN 4. */

            /* Initialize the non-simlib statistical counter. */
            num_responses = 0;

            /* Schedule the first arrival to the CPU from each terminal. */
            for (term = 1; term <= num_terms; ++term)
                event_schedule(expon(mean_think, STREAM_THINK), EVENT_ARRIVAL);

            /* Run the simulation until it terminates after an end-simulation event
           (type EVENT_END_SIMULATION) occurs. */
            do
            {

                /* Determine the next event. */
                timing();

                /* Invoke the appropriate event function. */
                switch (next_event_type)
                {
                case EVENT_ARRIVAL:
                    arrive();
                    break;
                case EVENT_END_CPU_1_RUN:
                    end_CPU_run(LIST_QUEUE_1);
                    break;
                case EVENT_END_CPU_2_RUN:
                    end_CPU_run(LIST_QUEUE_2);
                    break;
                case EVENT_END_SIMULATION:
                    report();
                    break;
                }

                /* If the event just executed was not the end-simulation event (type
           EVENT_END_SIMULATION), continue simulating.  Otherwise, end the
           simulation. */
            } while (next_event_type != EVENT_END_SIMULATION);
        }
        test_case++;
    }

    fclose(infile);
    fclose(outfile);

    return 0;
}

void arrive(void) /* Event function for arrival of job at CPU after think
                      time. */
{

    /* Place the arriving job at the end of the CPU queue.
       Note that the following attributes are stored for each job record:
            1. Time of arrival to the computer.
            2. The (remaining) CPU service time required (here equal to the
               total service time since the job is just arriving). */
    transfer[1] = sim_time;
    transfer[2] = expon(mean_service, STREAM_SERVICE);

    double random_number = uniform(0, 1, STREAM_UNIFORM);

    list_file(LAST, LIST_QUEUE_MAIN);

    int queue_number;
    if (list_size[LIST_QUEUE_1] != QUEUE_CPU_LIMIT || list_size[LIST_QUEUE_2] != QUEUE_CPU_LIMIT) {
        if (list_size[LIST_QUEUE_1] != QUEUE_CPU_LIMIT) {
            queue_number = LIST_QUEUE_2;
        } else if (list_size[LIST_QUEUE_2] != QUEUE_CPU_LIMIT) {
            queue_number = LIST_QUEUE_1;
        } else {
            queue_number = (random_number < 0.5) ? LIST_QUEUE_1 : LIST_QUEUE_2;
        }

        list_file(LAST, queue_number);

        int list_cpu = (queue_number == LIST_QUEUE_1) ? LIST_CPU_1 : LIST_CPU_2;
        
        /* If the CPU is idle, start a CPU run. */
        if (list_size[list_cpu] == 0)
            start_CPU_run(queue_number);
    }
}

void start_CPU_run(int queue_number) /* Non-event function to start a CPU run of a job. */
{
    float run_time;
    /* Remove the first job from the queue. */
    list_remove(FIRST, queue_number);

    /* Determine the CPU time for this pass, including the swap time. */
    if (quantum < transfer[2])
        run_time = quantum + swap;
    else
        run_time = transfer[2] + swap;

    /* Decrement remaining CPU time by a full quantum.  (If less than a full
       quantum is needed, this attribute becomes negative.  This indicates that
       the job, after exiting the CPU for the current pass, will be done and is
       to be sent back to its terminal.) */
    transfer[2] -= quantum;

    /* Place the job into the CPU. */
    int list_cpu = (queue_number == LIST_QUEUE_1) ? LIST_CPU_1 : LIST_CPU_2;
    list_file(FIRST, list_cpu);

    /* Schedule the end of the CPU run. */
    int event_end_cpu = (queue_number == LIST_QUEUE_1) ? EVENT_END_CPU_1_RUN : EVENT_END_CPU_2_RUN;;
    event_schedule(sim_time + run_time, event_end_cpu);
}

void end_CPU_run(int queue_number) /* Event function to end a CPU run of a job. */
{

    int list_cpu = (queue_number == LIST_QUEUE_1) ? LIST_CPU_1 : LIST_CPU_2;

    /* Remove the job from the CPU. */
    list_remove(FIRST, list_cpu);

    /* Check to see whether this job requires more CPU time. */
    if (transfer[2] > 0.0)
    {

        /* This job requires more CPU time, so place it at the end of the queue
           and start the first job in the queue. */
        list_file(LAST, queue_number);
        start_CPU_run(queue_number);
    }

    else
    {

        /* This job is finished, so collect response-time statistics and send it
           back to its terminal, i.e., schedule another arrival from the same
           terminal. */
        sampst(sim_time - transfer[1], SAMPST_RESPONSE_TIMES);

        event_schedule(sim_time + expon(mean_think, STREAM_THINK),
                       EVENT_ARRIVAL);

        /* Increment the number of completed jobs. */
        ++num_responses;

        /* Check to see whether enough jobs are done. */
        if (num_responses >= num_responses_required)

            /* Enough jobs are done, so schedule the end of the simulation
               immediately (forcing it to the head of the event list). */
            event_schedule(sim_time, EVENT_END_SIMULATION);

        else

            /* Not enough jobs are done; if the queue is not empty, start
               another job. */
            if (list_size[queue_number] > 0)
            start_CPU_run(queue_number);
    }
}

void report(void) /* Report genenext_event_typerator function. */
{
    /* Get and write out estimates of desired measures of performance. */
    fprintf(outfile, "\n%5d%16.3f%16.3f%16.3f%16.3f%16.3f\n", num_terms,
            sampst(0.0, -SAMPST_RESPONSE_TIMES), filest(LIST_QUEUE_1),
            filest(LIST_CPU_1), filest(LIST_QUEUE_2), filest(LIST_CPU_2));
}
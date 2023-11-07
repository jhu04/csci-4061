Project group number: 13

Group member names and x500s:
- Jeffrey Hu (hu000557)
- Devajya Khanna (khann077)
- Sameen Rahman (rahma262)

The name of the CSELabs computer that you tested your code on: csel-kh1260-03.cselabs.umn.edu

Any changes you made to the Makefile or existing files that would affect grading: N/A

Plan outlining individual contributions for each member of your group:
* We plan to meet on Zoom and have one person share their screen while the others assist by reviewing, looking up information in the writeup/documentation, etc.
* Every 45 minutes, we will switch the screen sharer/typer. This should result in equal contributions for each member.

Plan on how you are going to construct the worker threads and how you will make use of mutex locks and unlock and Conditions variables.
```
Construct worker threads:
    Keep a global worker_threads pointer (pointing to an array stored on heap).
    In main, keep an integer array for the worker thread arguments. pthread_create [the processing thread and] all the worker threads, passing in the arguments via the array.

Mutex locks:
1. Lock for queue operations
2. Lock for output directory operations (writing)
3. Lock for log file (writing)

Condition variables:
1. cons_cond: variable that is signalled if request queue is enqueued or worker should terminate
    Note: will need an additional (not conditional) variable orders_complete to tell if worker should terminate
2. prod_cond: variable that is signalled if request queue is dequeued (in order to avoid overflowing the queue buffer)
```

Pseudocode:
```
worker:
    ...
    
    while queue empty:
        // termination
        if processing thread done (via orders_complete variable)
            pthread_exit()

main:
    Parse command arguments (number of worker threads)
    Open log file
    Spawn processing thread (pthread_create)
    Spawn an array of worker threads (pthread_create)
    pthread_join on all threads
    Close log file
```

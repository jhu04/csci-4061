Project group number: 13

Group member names and x500s:
- Jeffrey Hu (hu000557)
- Devajya Khanna (khann077)
- Sameen Rahman (rahma262)

The name of the CSELabs computer that you tested your code on: csel-kh1260-03.cselabs.umn.edu

Any changes you made to the Makefile or existing files that would affect grading: Added a few commands to attempt section 2.7

Plan outlining individual contributions for each member of your group:
* We plan to meet on Zoom and have one person share their screen while the others assist by reviewing, looking up information in the writeup/documentation, etc.
* Every 45 minutes, we will switch the screen sharer/typer. This should result in equal contributions for each member.

Plan on how you are going to construct the worker threads and how you will make use of mutex locks and unlock and Conditions variables.
```
Construct worker threads:
    Keep a global worker_threads pointer (pointing to an array stored on heap).
    In main, keep an integer array for the worker thread arguments. pthread_create [the processing thread and] all the worker threads, passing in the arguments via the array.

Mutex locks:
1. queue_lock: lock queue ops
2. term_lock: specifically for the termination condition variable (check if processor broadcasted termination signal)
3. log_lock: lock for log file
4. worker_done_lock: the unique mutex for each worker, when working acknowledging image-processing completion 


Condition variables:
1. cons_cond: variable that is signalled if request queue is enqueued or worker should terminate
    Note: will need an additional (not conditional) variable orders_complete to tell if worker should terminate
2. prod_cond: variable that is signalled if request queue is dequeued (in order to avoid overflowing the queue buffer)
3. term_cond: dedicated only for termination variable
4. worker_done_cv: array of unique conditional variables for each worker thread: checking if worker done processing
```

Pseudocode:
```
**: anything related to parallel processing

processor:
    while there is a reg file (assumed to be png) found
	lock on the queue
        wait for empty slot 
	add the file to queue
	unlock on queue

    broadcast to workers that no more things added to queue **

    for each worker thread **
	lock on the worker's unique mutex
	wait for the worker's acknowledgement 
	unlock on the worker's unique mutex
	update the number of total files processed

    compare number of enqueued files and number of files processed 

    broadcast termination signal **

worker:
    ...
    
    while queue empty:
     
            if processing thread done (via request_complete variable)
	        lock on worker's unique mutex
                signal processing thread for acknowledgement about finishing **
	        wait for termination broadcast signal from processor **
	        unlock on worker's unique mutex

	        terminate thread

	wait for a signal from processing thread **

    dequeue an entry from queue
    signal to the processor about new empty slot **
    increment number of files this thread processed on the processed_count_array (each index is a thread id) **
    
    image processing...

    output logging info to logfile



main:
    Parse command arguments (number of worker threads)
    Open log file
    Spawn processing thread (pthread_create)
    Spawn an array of worker threads (pthread_create)
    pthread_join on all threads
    Close log file

    free all necessary resources
```
Link to program analysis document, google drive:
https://docs.google.com/document/d/1E5tTJzgQ4gp9CiR3SohR02Da2QTEcpB3dcvin4jY3Uw/edit


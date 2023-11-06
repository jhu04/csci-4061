Project group number: 13

Group member names and x500s:
- Jeffrey Hu (hu000557)
- Devajya Khanna (khann077)
- Sameen Rahman (rahma262)

The name of the CSELabs computer that you tested your code on:  // TODO

Any changes you made to the Makefile or existing files that would affect grading: // TODO

Plan outlining individual contributions for each member of your group:
We plan to meet on Zoom and have one person share their screen while the
others assist by reviewing, looking up information in the writeup/documentation, etc.

Every 45 minutes, we will switch the screen sharer/typer. This should result in equal contributions for each member.

Plan on how you are going to construct the worker threads and how you will make use of mutex locks and unlock and Conditions variables.
```
Construct worker threads: keep array

Mutex locks:
1. Queue
2. Output directory

Condition variables:
1. Test if request queue empty
2. Test if request queue full (assuming # of images >= queue buffer size)
3. "Done Traversing" --> no more elements will be added to queue 

Lock request queue, log file, global variables


```
```
worker:
*Make a global array of worker threads
*
    


check if processing thread done --> a broadcasted signal
    check if queue empty
        pthread_exit()

main:
    Parse command arguments (number of worker threads)
    Open log file
    Spawn processing thread (pthread_create)
    Spawn an array of worker threads (pthread_create)
    pthread_join on all threads
    Close log file
```

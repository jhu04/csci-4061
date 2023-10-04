Project group number: 13

Group member names and x500s:
- Jeffrey Hu (hu000557)
- Devajya Khanna (khann077)
- Sameen Rahman (rahma262)

The name of the CSELabs computer that you tested your code on: csel-kh1260-03.cselabs.umn.edu

Any changes you made to the Makefile or existing files that would affect grading: N/A

Plan outlining individual contributions for each member of your group:
We plan to meet on Zoom and have one person share their screen while the 
others assist by reviewing, looking up information in the writeup/documentation, etc.

Every 30 minutes, we will switch the screen sharer/typer.

Generally, the above process will result in the following assignments.
- README: Jeffrey Hu, Sameen Rahman
- 3.1 Partitioning Input Data into N files: Jeffrey Hu, Devajya Khanna, Sameen Rahman
- 3.2 Creating Merkle Process Tree and Hashing Files: Switch every 30 minutes
    - 3.2.1 Leaf Process: Switch every 30 minutes
    - 3.2.2 Non-Leaf Process: Switch every 30 minutes
- merkle.c: Switch every 30 minutes
- Error Handling: Switch every 30 minutes
- Code Review: Jeffrey Hu, Devajya Khanna, Sameen Rahman

Plan on how you are going to implement the process tree component of creating the Merkle tree (high-level pseudocode would be acceptable/preferred for this part):
main:
    if process is leaf:
        open corresponding block file
        if open fails:
            exit(-1)
        
        read from corresponding block file
        if read fails:
            close all files
            exit(-1)

        hash block file

        open corresponding hash file
        if open fails:
            close all files
            exit(-1)

        write hash to corresponding hash file
        if write fails:
            close all files
            exit(-1)

        close all files
        exit
    else:
        for i from 0 to 1:
            pid = fork()
            if pid is child:
                exec ./child_process
                exit(-1)
        
        wait for both child processes to terminate
        if wait fails:
            close all files
            exit(-1)
        
        generate parent hash
        
        open hash file
        if open fails:
            close all files
            exit(-1)
        
        write parent hash to corresponding hash file (force full write)
        if write fails:
            close all files
            exit(-1)
        
        close all files
        exit

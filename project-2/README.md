Project group number: 13

Group member names and x500s:
- Jeffrey Hu (hu000557)
- Devajya Khanna (khann077)
- Sameen Rahman (rahma262)

The name of the CSELabs computer that you tested your code on: TODO

Any changes you made to the Makefile or existing files that would affect grading: TODO

Plan outlining individual contributions for each member of your group:
We plan to meet on Zoom and have one person share their screen while the
others assist by reviewing, looking up information in the writeup/documentation, etc.

Every 45 minutes, we will switch the screen sharer/typer. This should result in equal contributions for each member.

Plan on how you are going to construct the pipes and inter-process communication:
```
root_process:
    // Initialization
    Parse argument <dir>

    // Pipe
    Create a pipe
    
    Fork
    
    If child:
        Close pipe read end
        Exec nonleaf_process
    Else:
        Close pipe write end
        Read string from pipe to all_filepath_hashvalue
        Close pipe read end
    
    // File logic
    // ...

nonleaf_process:
    // Initialization
    Parse arguments <file_path>, <pipe_write_end>
    
    // Forking and obtaining read_ends for each entry
    Create array read_ends of size MAX_ENTRIES = 10
    Let i = 0
    
    while there are still entries in directory:
        Create a pipe
        
        Fork
        
        If child:
            Close pipe read end
            If entry is directory:
                Exec nonleaf_process
            Else:
                Exec leaf_process
        Else:
            Close pipe write end
            read_ends[i] = pipe read end
            i += 1
    
    // Reading from children
    Malloc BUFSIZE for buffer
    Initialize buffer
    For read_end in read_ends:
        Concatenate read data to buffer
        Close read_end
    
    // Writing to parent
    Write buffer to pipe_write_end
    Close pipe_write_end
    
    // Waiting
    While there are still children left, wait

leaf_process:
    
```
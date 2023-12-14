Project group number: 13

Group member names and x500s:
- Jeffrey Hu (hu000557)
- Devajya Khanna (khann077)
- Sameen Rahman (rahma262)

The name of the CSELabs computer that you tested your code on: csel-kh1260-03.cselabs.umn.edu

Any changes you made to the Makefile or existing files that would affect grading:
OBJS = utils.o sha256.o
LOBJS = $(addprefix $(LIBDIR)/, $(OBJS))

These lines add sha256.o to server & client

Plan outlining individual contributions for each member of your group:
* We plan to meet on Zoom and have one person share their screen while the others assist by reviewing, looking up information in the writeup/documentation, etc.
* Every 1hr, we will switch the screen sharer/typer. This should result in equal contributions for each member.

Plan on how you are going to construct the client handling thread and how
you plan to send the package according to the protocol

Server Listening Thread:
Opening a listening socket
bind port # to it
loop (listen --> accept connection and invoke client handling thread)
In the loop after spawning client thread, detach it

__Client Handling Thread__:
Create Connection Socket

while True {
    Receive packet

    recv a packet from client

    if received a IMG_OP_EXIT packet
        break;
    
    otherwise, it's and IMG_OP_ROTATE packet (indicates image data packets incoming)
    receive image data packets in a loop

    Image Processing

    if error occurs
        send IMG_OP_NAK packet

    send IMG_OP_ACK packet
    read image file data into chunks of a byte stream and send to client in a loop
}

Close connection socket
Clean-up


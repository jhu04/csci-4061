### Project group number: 13

### Group member names and x500s:
- Jeffrey Hu (hu000557)
- Devajya Khanna (khann077)
- Sameen Rahman (rahma262)

### The name of the CSELabs computer that you tested your code on: csel-kh1260-03.cselabs.umn.edu

### Any changes you made to the Makefile or existing files that would affect grading:
We added the following lines.
```
OBJS = utils.o sha256.o
LOBJS = $(addprefix $(LIBDIR)/, $(OBJS))
```
We then used these lines are used to add sha256.o to server & client by changing their make commands to the following.
```
server: $(LOBJS) $(INCDIR)/utils.h $(SRCDIR)/server.c
	$(CC) $(CFLAGS) -I$(INCDIR) -o $@ $(LOBJS) $(SRCDIR)/server.c -lm

client: $(LOBJS) $(INCDIR)/utils.h $(SRCDIR)/client.c
	$(CC) $(CFLAGS) -I$(INCDIR) -o $@ $(LOBJS) $(SRCDIR)/client.c -lm
```

### Plan outlining individual contributions for each member of your group:
* We plan to meet on Zoom and have one person share their screen while the others assist by reviewing, looking up information in the writeup/documentation, etc.
* Every 1 hr, we will switch the screen sharer/typer. This should result in equal contributions for each member.

### Plan on how you are going to construct the client handling thread and how you plan to send the package according to the protocol
Server Listening Thread:
```
Opening a listening socket
bind port # to it
loop (listen --> accept connection and invoke client handling thread)
In the loop after spawning client thread, detach it
```

Client Handling Thread:
```
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
```

### Any assumptions that you made that weren’t outlined in section 7
- We assume that the number of active clients at any given point in time is less than or equal to 1024. This is reflected in `server.c`, where our local `conn_fd` array in `main` is of length 1024.

### How you designed your program for Package sending and processing (again, high-level pseudocode would be acceptable/preferred for this part)
See "Client Handling Thread" pseudocode above for a high level overview of the code. Our design did not change much from our intermediate submission.

For each file, we had our client first send an initial packet containing operations, flags, file size, and checksum.
Our client would then send many packets consisting of smaller parts of the file (e.g. like a stream of bytes).

Our server had a similar protocol in which it first sends an ackpacket to the client with information (including file size), then sends the file in multiple chunks.

### How you implement error detection/correction and the package encryption
We implemented our error detection checksum by computing a sha256 hash (using the `sha256.o` library provided in Project 1) of the file data before sending the file.
We then sent the sha256 hash in the packet preceding the file data, switching the `IMG_FLAG_CHECKSUM` flag on.

Specifically, we used `sha256_init` to initialize a `SHA256_CTX` struct, used `sha256_update` to update the hash while reading in file data, and used `sha256_final` to get the final hash as a byte array.

After receiving all the file data (say in the server), we would calculate the sha256 hash of the file data.
If the `IMG_FLAG_CHECKSUM` was on, we would then compare the two hashes character by character, confirming that the hashes are equal.
If the hashes are unequal, the server prints out that there is a failed checksum.

We had a very similar procedure for when the server sends a file to the client; the server would compute the sha256 hash and send it to the client as a checksum.

We implemented encryption using an [affine cipher](https://en.wikipedia.org/wiki/Affine_cipher) on each byte. We used keys `a = 53, b = 41` and an "implicit" modulus of `m = 256`.
This modulus is "implict" since addition and multiplication on bytes with "integer" overflow are isomorphic to addition and multiplication modulo `256`.

In other words, to encrypt a string, we mapped each byte of the string `s[i]` to `(char) 53 * s[i] + (char) 41`.

To decrypt a string, it can be proven that mapping an encrypted byte `s_prime[i]` to `(char) 29 * (s_prime[i] - (char) 41)` (see link above).
Note that `29` is the modular inverse of `53` modulo `256`.

Our code can be extended to other encryption algorithms by modifying the provided `enc` and `dec` functions in `client.h` and `server.h`.

Use gcc -o printer printer.c -lrt -lpthread 
and gcc -o client client.c -lrt -lpthread to run printer and client.
I used multiple terminals to test my code, it worked perfect.
At the first time running printer, it asks for an input for buffer size.
The client will ask for a source ID before it puts the job to buffer.
These printer and client will print the trace to a file called trace.txt


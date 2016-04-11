There are multiple sources of Fluidanimate.
The one using multidependencies requires nanox 0.7.9 and 
mercurium 1.99.7.
Change the source to compile in Makefile.ompss by changing 
the .o file used:

TARGET   = fluidanimate-ompss
OBJS     = ompss-multideps-nobar.o cellpool.o

If you want to try another source, change ompss-multideps-nobar.o 
by the name of the corresponding file with .cpp extension.

To do a fair comparison, Pthreads must have disabled the use of 
impenetrable wall (comment out #define USE_ImpeneratableWall), 
as it was relocating cells to their original 
grid partition if during simulation ended out of it. In that way, 
Fluidanimate pthreads achieved load balance but was altering 
the simulation.

#
# Set compile and link options below.
#
# BEGIN Solaris opts
#     CC = mpicc
# END Solaris opts
# BEGIN AIX opts
#     CC = mpcc
# LDFLAGS = -lm
# END AIX opts
# BEGIN Linux opts
#     CC = mpicc
#     LDFLAGS = -lm
# END Linux opts
# BEGIN SGI IRIX opts
#     CC = cc
#     LDFLAGS = -lmpi
# END SGI IRIX opts


     CC = mpicc
     LDFLAGS = -lm

OBJS = listrates.o second.o 


listrates:$(OBJS)
	$(CC) -o $@ $(OBJS) $(LDFLAGS)

listrates.o:listrates.c
	$(CC) -c listrates.c $(FFLAGS)

second.o:second.c
	$(CC) -c second.c $(CFLAGS)

clean:
	rm -f $(OBJS) listrates

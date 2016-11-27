# MAKEFILE FOR virtualMaemoryManagement
#

EXEA = vmapp
EXEB = mmanage


CC = /usr/bin/gcc

CFLAG  = -pthread
CFLAG += -g
CFLAG += -Wall

LDFLAG = -lpthread
LDFLAG = -lrt

SRCB = mmanage.c
OBJB = $(SRCB:%.c=%.o) 
HEADERB = $(SRCB:%.c=%.h) vmem.h

SRCA = vmappl.c vmaccess.c
OBJA = $(SRCA:%.c=%.o) 
HEADERA= $(SRCA:%.c=%.h) vmem.h

# targets (all is the default, because its on top.)

all: $(EXEB) $(EXEA)

$(EXEB): $(OBJB) $(HEADERB)
	$(CC) $(CFLAG) $(LDFLAG) -o $@ $^

$(EXEA): $(OBJA) $(HEADERA)
	$(CC) $(CFLAG) $(LDFLAG) -o $@ $^

%.o: %.c
	$(CC) -c $(CFLAG) $(LDFLAG) -o $@ $^

clean:
	rm -f $(EXEB)
	rm -f $(EXEA)
	rm -f ./pagefile.bin
	rm -f ./logfile.txt
	rm -f *.o

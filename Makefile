CC=gcc

TARGET = linux-scalability

MYFLAGS =  -m32 -g3 -L./ -O0 -Wall  -fno-builtin-malloc -fno-builtin-calloc -fno-builtin-realloc -fno-builtin-free 

# uncomment this to link with hoard memory allicator 
MYLIBS = libmtmm.a

# uncomment this to link with simple memory allocator
#MYLIBS = libSimpleMTMM.a

# uncomment this to link with standard memory allocator
#MYLIBS = 


all: libSimpleMTMM.a $(TARGET)

libSimpleMTMM.a: 
	$(CC) $(MYFLAGS) -c mtmm.c 
	ar rcu libSimpleMTMM.a mtmm.o
	ranlib libSimpleMTMM.a


$(TARGET): $(TARGET).c
	$(CC) $(CCFLAGS) $(MYFLAGS) $(MYLIBS) $(TARGET).c -o $(TARGET) -lpthread -lm -lSimpleMTMM

clean:
	rm -f $(TARGET)  *.o libSimpleMTMM.a *core*
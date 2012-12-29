CXXFLAGS=-I. -ggdb -D _REENTRANT 
#CPPFLAGS=
LDFLAGS=-L.
SYSLIBS=-lpthread -lrt

OBJS=INETAddr.o inireader.o ScopeLock.o TcpClient.o ThreadBase.o RollingFile.o RingBuffer.o osutil.o TCPServer.o

.PHONY :  all clean

all: zhutil.a

zhutil.a: ${OBJS}
	ar -rcs libzhutil.a ${OBJS} 
	
clean:
	-rm -f *.o libzhutil.a	
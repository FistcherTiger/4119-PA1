CFLAGS = -Wall
OBJECTS = ChatApp.o udp.o interface.o user.o


ChatApp : $(OBJECTS)
	cc $(CFLAGS) -o ChatApp $(OBJECTS) -lpthread

ChatApp.o : ChatApp.c
	cc -Wall -c ChatApp.c

udp.o : udp.c udp.h
	cc -Wall -c udp.c

interface.o : interface.c interface.h udp.c udp.h
	cc -Wall -c interface.c

user.o : user.c user.h
	cc -Wall -c user.c

.PHONY : clean
clean :
	rm -rf *.o ChatApp

.PHONY : remove
remove :
	rm -rf *.o *.c *.h ChatApp

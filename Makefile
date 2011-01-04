CFLAGS = -Wall

OBJS = pngutil.o viewer.o 

LIBS = -lpng -lncurses 

default: compile

compile: viewer

viewer: $(OBJS)
	$(CC) $(LIBS) -o $@ $(OBJS)

clean:
	rm -f *.o viewer

pngutil.o: pngutil.c pngutil.h 
	$(CC) -c $(CFLAGS) -o $@ $<

viewer.o: viewer.c pngutil.h
	$(CC) -c $(CFLAGS) -o $@ $<

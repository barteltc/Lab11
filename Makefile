CC=gcc
CFLAGS=-c -Wall -g
LDFLAGS=-ljpeg -lm
SOURCES=mandel.c jpegrw.c
OBJECTS=$(SOURCES:.c=.o)
EXECUTABLE=mandel

all: $(EXECUTABLE)

# Pull in dependency info for existing .o files
-include $(OBJECTS:.o=.d)

$(EXECUTABLE): $(OBJECTS)
	$(CC) $(OBJECTS) $(LDFLAGS) -o $@

.c.o:
	$(CC) $(CFLAGS) $< -o $@
	$(CC) -MM $< > $*.d

clean:
	rm -rf $(OBJECTS) $(EXECUTABLE) *.d
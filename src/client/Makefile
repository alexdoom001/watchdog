TARGET = neo-watchdog-client

CFLAGS += -I.


SOURCES = main.c
OBJECTS=$(SOURCES:%.c=%.o)

CFLAGS+=$(shell pkg-config --cflags glib-2.0)
LDFLAGS+=$(shell pkg-config --libs glib-2.0)


all: $(TARGET)

$(TARGET): $(OBJECTS) 
	$(CC) -o $(TARGET) $(LDFLAGS) $(CXXFLAGS) ${LIBS} $(OBJECTS)

clean:
	$(RM) $(OBJECTS) $(TARGET)

.PHONY: all clean

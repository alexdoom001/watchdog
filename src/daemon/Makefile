TARGET = neo-watchdogd

CFLAGS += -I.

SOURCES = watchdogCtl/base.c watchdogCtl/lanner.c  watchdogCtl/std.c socket_events.c watchdog_event.c
SOURCES += main.c
OBJECTS=$(SOURCES:%.c=%.o)

INCLUDES = $(wildcard *.h) $(wildcard watchdogCtl/*.h) ../config.h

CFLAGS+=$(shell pkg-config --cflags glib-2.0 gio-2.0)
LDFLAGS+=$(shell pkg-config --libs glib-2.0 gio-2.0)


all: $(TARGET)

# $(OBJECTS): $(SOURCES)
%.o: %.c ${INCLUDES}
	$(CC) -c -o $@ $(CFLAGS) $<


$(TARGET): $(OBJECTS) 
	$(CC) -o $(TARGET) $(LDFLAGS) $(CXXFLAGS) ${LIBS} $(OBJECTS)

clean:
	$(RM) $(OBJECTS) $(TARGET)

.PHONY: all clean

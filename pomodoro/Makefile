P=pomodoro
OBJECTS=
CFLAGS=-g -Wall -Werror -O2 -std=c11 -Wno-format-truncation
LDLIBS=-pthread

CFLAGS += $(shell pkg-config --cflags json-c)
LDFLAGS += $(shell pkg-config --libs json-c)

$(P): $(OBJECTS)

all: hbuilder

CFLAGS := -Wall -Werror -g
OBJS := data.o calc.o

hbuilder: main.o $(OBJS)
	$(CC) $(CFLAGS) $(CPPFLAGS) $< $(OBJS) -o $@ -lm $(LDFLAGS)

%.o: %.c %.h list.h
	$(CC) $(CFLAGS) $(CPPFLAGS) -o $@ -c $<

main.o: data.h calc.h list.h

all: hbuilder

CFLAGS := -Wall -Werror -g
OBJS := data.o calc.o edit.o save.o parse.o

hbuilder: main.o $(OBJS)
	$(CC) $(CFLAGS) $(CPPFLAGS) $< $(OBJS) -o $@ -lm $(LDFLAGS)

%.o: %.c %.h list.h
	$(CC) $(CFLAGS) $(CPPFLAGS) -o $@ -c $<

calc.o: data.h edit.h

data.o: parse.h

edit.o: calc.h data.h save.h

save.o: calc.h data.h edit.h parse.h

main.o: $(OBJS:.o=.h) list.h

all: hbuilder

CFLAGS := -g
OBJS := data.o

hbuilder: main.o $(OBJS)
	$(CC) $(CFLAGS) $(CPPFLAGS) $< $(OBJS) -o $@ $(LDFLAGS)

%.o: %.c %.h list.h
	$(CC) $(CFLAGS) $(CPPFLAGS) -o $@ -c $<

main.o: data.h list.h

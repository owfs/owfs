
LDFLAGS = -L/opt/owfs/lib -Wl,--rpath -Wl,/opt/owfs/lib -lowcapi
CFLAGS = -g -I/opt/owfs/include

OBJS = owdir.o

owdir: $(OBJS)
	gcc $(CFLAGS) $(LDFLAGS) -o $@ $(OBJS)

%.o: %.c
	gcc $(CFLAGS) -c -o $@ $<

clean:
	$(RM) -f owdir *.o *~ .~

CSRCS := nsm-app.c

all: $(CSRCS)
	$(CC) $(CFLAGS) $(LDFLAGS) -lnsm -pie  \
		$(CSRCS) -o nsm-app
clean:
	rm -f nsm-app

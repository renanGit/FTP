all: mytcpclient mytcpserver

mytcpclient: mytcpclient.c
	gcc $< -o $@

mytcpserver: mytcpserver.c
	gcc $< -o $@

clean:
	rm -f mytcpclient mytcpserver *.o *~ core ServerClient/*~ ClientFiles/*~

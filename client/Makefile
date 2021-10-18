client:
	gcc *.c -g -o client

check: client
	./client http://www.microsoft.com 
	./client http://www.google.com 
	./client http://www.github.com 
	./client http://www.facebook.com

clean: client
	rm client
.PHONY=check
.PHONY+=clean

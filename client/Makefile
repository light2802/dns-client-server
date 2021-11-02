client:
	gcc *.c -g -o client.out

check: client
	./client http://www.microsoft.com 
	./client http://www.google.com 
	./client http://www.github.com 
	./client http://www.facebook.com

clean: client
	rm -f client
.PHONY=check clean

#Pretty noob Makelfile :`(
client:
	cd src & gcc *.c -g -o client.out

check: client
	cd src
	./client http://www.microsoft.com 
	./client http://www.google.com 
	./client http://www.github.com 
	./client http://www.facebook.com

clean: client
	cd src
	rm -f client
.PHONY=check clean

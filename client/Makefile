#Pretty noob Makelfile :`( sorry
client.out:
	cd src
	gcc *.c -g -o ../client.out

check: client.out
	cd src
	./client.out http://www.microsoft.com 
	./client.out http://www.google.com 
	./client.out http://www.github.com 
	./client.out http://www.facebook.com

clean: client.out
	rm -f client.out
.PHONY=check clean

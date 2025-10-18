CPP = g++ -std=c++23

all:
	$(CPP) pinentry-whiptail.cpp -o pinentry-whiptail -Wall
	sh tests/test.sh

install:
	install -m 0755 pinentry-whiptail /usr/bin

uninstall:
	rm -f /usr/bin/pinentry-whiptail

clean:
	rm -f pinentry-whiptail

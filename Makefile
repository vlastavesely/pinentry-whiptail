CPP = g++ -std=c++23

prefix = /usr
bindir = $(prefix)/bin

all:
	$(CPP) pinentry-whiptail.cpp -o pinentry-whiptail -Wall

install:
	install -m 0755 -d $(bindir)
	install -m 0755 pinentry-whiptail $(bindir)

uninstall:
	rm -f $(bindir)/pinentry-whiptail

clean:
	rm -f pinentry-whiptail

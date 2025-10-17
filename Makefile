CPP = g++ -std=c++23

all:
	$(CPP) pinentry-whiptail.cpp -o pinentry-whiptail
	sh test

clean:
	rm -f pinentry-whiptail

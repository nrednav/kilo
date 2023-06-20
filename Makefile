CC=g++

kilo: kilo.cpp
	$(CC) kilo.cpp -o kilo -Wall -Wextra -pedantic -std=c++20

clean:
	rm kilo

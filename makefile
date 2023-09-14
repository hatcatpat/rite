all: rite

rite: *.c *.h
	gcc -g -o rite *.c -Wall -ansi
	# tcc -o rite *.c -Wall

run: all
	./rite

clean:
	rm rite

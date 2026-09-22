.PHONY: all

all: out/c out/vm

out/c: out/lang.o out/c.o
	gcc -o out/c out/lang.o out/c.o

out/vm: out/lang.o out/vm.o
	gcc -o out/vm out/lang.o out/vm.o

out/lang.o: src/lang.c header/lang.h
	gcc -c -o out/lang.o src/lang.c

out/vm.o: src/vm.c header/lang.h src/lang.c
	gcc -c -o out/vm.o src/vm.c

out/c.o: src/c.c src/lang.c header/lang.c
	gcc -c -o out/c.o src/c.c

.PHONY: test
test:
	echo "No tests yet"

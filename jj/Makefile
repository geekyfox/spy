OBJS := build/jj.o build/test.o

.PHONY: test
test : build/test
	build/test

.PHONY: grind
grind : build/test
	valgrind --leak-check=full --show-leak-kinds=all build/test

build/test: $(OBJS)
	gcc --std=gnu99 -g -o $@ $(OBJS)

build/%.o : %.c jj.h
	@mkdir -p build
	gcc --std=gnu99 -Wall -g -c -o $@ $<

format:
	clang-format -i *.c *.h

clean:
	rm -rf build


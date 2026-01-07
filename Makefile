SRCS := $(wildcard src/*.c)
OBJS := $(patsubst src/%.c,build/spy_%.o,$(SRCS)) build/jj.o

spy: $(OBJS)
	gcc --std=gnu99 -g -o spy $(OBJS) -lcurl

build/spy_%.o: src/%.c src/spy.h jj/jj.h
	@mkdir -p build
	gcc --std=gnu99 -Wall -Werror -g -c -o $@ $<

build/jj.o: jj/jj.c jj/jj.h
	@mkdir -p build
	gcc --std=gnu99 -Wall -Werror -g -c -o $@ $<

format:
	clang-format -i src/*

clean:
	rm -rf build
	rm -f spy

install: spy
	cp spy ~/.local/bin/spy


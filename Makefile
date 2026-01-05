SRCS := $(wildcard src/*.c)
OBJS := $(patsubst src/%.c,build/%.o,$(SRCS))

spy: $(OBJS)
	gcc --std=gnu99 -g -o spy $(OBJS) -lcurl

build/%.o: src/%.c src/spy.h
	@mkdir -p build
	gcc --std=gnu99 -Wall -Werror -pedantic -g -c -o $@ $<

format:
	clang-format -i src/*

clean:
	rm -rf build
	rm -f spy

install: spy
	cp spy ~/.local/bin/spy


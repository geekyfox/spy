SRCS := $(wildcard src/*.c)
OBJS := $(patsubst src/%.c,build/%.o,$(SRCS))

spy: $(OBJS)
	gcc -g -o spy $(OBJS) -lcurl

build/%.o: src/%.c src/spy.h
	@mkdir -p build
	gcc -Wall -g -c -o $@ $<

format:
	clang-format -i src/*

clean:
	rm -rf build
	rm -f spy


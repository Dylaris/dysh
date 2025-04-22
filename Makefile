dysh: src/main.c src/parse.c
	make -C src/builtin/
	gcc -Wall -Wextra -I src/ -o dysh src/main.c src/parse.c -g

clean:
	rm dysh && make -C src/builtin clean

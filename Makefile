dysh: src/main.c src/parse.c
	gcc -Wall -Wextra -I src/ -o dysh src/main.c src/parse.c -g

clean:
	rm dysh
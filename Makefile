# You have to use gnu11 because I used some GNU extensions
# and to be compatible with some linux code, which using some 
# GNU extensions

dysh: src/main.c src/parse.c
	make -C src/builtin/
	gcc -Wall -Wextra -std=gnu11 -I src/ -o dysh src/main.c src/parse.c

clean:
	rm dysh && make -C src/builtin clean

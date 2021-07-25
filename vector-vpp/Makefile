all:
	cscope -Rbq && ctags -R
	gcc -O3 -I../ -I./vppinfra -I./ -march=corei7 -mtune=corei7-avx -o main main.c
	gcc -I../ -I./vppinfra -I./ -march=corei7 -mtune=corei7-avx -g range.c -o range
clean:
	rm -rf a.out main range
	rm -rf cscope.*
	rm -rf tags

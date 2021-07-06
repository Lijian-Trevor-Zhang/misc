all:
	cscope -Rbq && ctags -R
	gcc -O3 -I../ -I./vppinfra -I./ -march=armv8.2-a+crc+crypto -o main main.c
	gcc -I../ -I./vppinfra -I./ -march=armv8.2-a+crc+crypto range.c -g -o range
	gcc -I../ -I./vppinfra -I./ -march=armv8.2-a+crc+crypto saturate.c -g -o saturate
clean:
	rm -rf a.out main range saturate
	rm -rf cscope.*
	rm -rf tags

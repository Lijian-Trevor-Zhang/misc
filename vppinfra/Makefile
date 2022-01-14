all:
	gcc-10 --static -Wall -O2 -g -I../ -march=armv8.5-a+crc+crypto+sve -o main.exe main.c
	gcc-10 --static -Wall -O2 -g -I../ -march=armv8.5-a+crc+crypto+sve -o memcpyle.exe memcpyle.c
	gcc-10 --static -Wall -O2 -g -I../ -march=armv8.5-a+crc+crypto+sve -o memsetux.exe memsetux.c
	gcc-10 --static -Wall -O2 -g -I../ -march=armv8.5-a+crc+crypto+sve -o ceq.exe ceq.c
	gcc-10 --static -Wall -O2 -g -I../ -march=armv8.5-a+crc+crypto+sve -o buffers.exe buffers.c
	gcc-10 --static -Wall -O2 -g -I../ -march=armv8.5-a+crc+crypto+sve -o rdma.exe rdma.c
	gcc-10 --static -Wall -O2 -g -I../ -march=armv8.5-a+crc+crypto+sve -o bond.exe bond.c
clean:
	rm -rf *.exe *.d a.out *.i *.s

all:
	gcc-10 --static -Wall -O2 -g -I../ -march=armv8.5-a+crc+crypto+sve -o main main.c
	gcc-10 --static -Wall -O2 -g -I../ -march=armv8.5-a+crc+crypto+sve -o memcpyle memcpyle.c
	gcc-10 --static -Wall -O2 -g -I../ -march=armv8.5-a+crc+crypto+sve -o memsetux memsetux.c
	gcc-10 --static -Wall -O2 -g -I../ -march=armv8.5-a+crc+crypto+sve -o ceq ceq.c
	gcc-10 --static -Wall -O2 -g -I../ -march=armv8.5-a+crc+crypto+sve -o buffers buffers.c
	gcc-10 --static -Wall -O2 -g -I../ -march=armv8.5-a+crc+crypto+sve -o rdma rdma.c
	gcc-10 --static -Wall -O2 -g -I../ -march=armv8.5-a+crc+crypto+sve -o bond bond.c
clean:
	rm -rf *.exe *.d a.out *.i *.s

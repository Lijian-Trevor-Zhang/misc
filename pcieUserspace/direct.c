#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>

// 举例两种将PCIe memory space映射到用户空间进行访问的技术..

int option_sysfs ()
{
    size_t size = 16 << 20;
    off_t offset = 0;
    void *addr = (void *) 0x383000000000L;
    int fd;

    fd = open ("/sys/bus/pci/devices/0000:5e:00.0/resource0", O_RDWR);
    addr = mmap (addr, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, offset);
    // addr = mmap (NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, offset);
    printf ("%p\n", addr);
    munmap (addr, size);
    close (fd);
    return 0;
}

int option_devmem ()
{
    size_t size = 16 << 20;
    off_t offset = 0;
    void *addr = (void *) 0x383000000000L;
    int fd;

    fd = open ("/dev/mem", O_RDWR);
    addr = mmap (addr, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, (off_t) addr);
    // addr = mmap (NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, (off_t) addr);
    printf ("%p\n", addr);
    munmap (addr, size);
    close (fd);
    return 0;
}

int main (int argc, char *argv[])
{
    option_sysfs ();
    option_devmem ();
    return 0;
}

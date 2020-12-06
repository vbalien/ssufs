#include "ssufs-ops.h"

int main()
{
    char str[] = "!-------32 Bytes of Data-------!!-------32 Bytes of Data-------!!-------32 Bytes of Data-------!!-------32 Bytes of Data-------!";
    char fname[100];
    int fd1;
    ssufs_formatDisk();

    for (int i = 0; i < 8; ++i)
    {
        sprintf(fname, "f%d.txt", i);
        ssufs_create(fname);
        fd1 = ssufs_open(fname);
        printf(">> %s <<\n", fname);
        printf("Write Data: %d\n", ssufs_write(fd1, str, 128));
        printf("Write Data: %d\n\n", ssufs_write(fd1, str, 128));
    }
    ssufs_dump();
}

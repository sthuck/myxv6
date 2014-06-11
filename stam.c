#include "types.h"
#include "stat.h"
#include "user.h"
#include "fs.h"
#include "fcntl.h"


char bla2[1024*1024];
int
main(int argc, char *argv[])
{
  int fd = open("/test.txt",O_CREATE|O_RDWR);

  write(fd,bla2,6*1024);  

  printf(2,"wrote 6kb(direct)\n");
      write(fd,bla2,64*1024);  
  printf(2,"wrote 70kb(indirect)\n");
  /*for (i=0;i<130;i++) {
    for (j=0;j<64;j++)  //write block
      write(fd,bla,8);  
  }*/
  write(fd,bla2,1024*1024);  
  printf(2,"wrote 1MB(indirect)\n");
  close(fd);
  exit();
}

#include "types.h"
#include "stat.h"
#include "user.h"
#include "fs.h"
#include "fcntl.h"



int
main(int argc, char *argv[])
{
  char* bla = "abcdefgf";
  int fd = open("/test.txt",O_CREATE|O_RDWR);
  int i,j ;
  for (i=0;i<12;i++) {
    for (j=0;j<64;j++)  //write block
      write(fd,bla,8);  
  }
  printf(2,"wrote 6kb(direct)\n");
  for (i=0;i<128;i++) {
    for (j=0;j<64;j++)  //write block
      write(fd,bla,8);  
  }
  printf(2,"wrote 70kb(indirect)\n");
  for (i=0;i<130;i++) {
    for (j=0;j<64;j++)  //write block
      write(fd,bla,8);  
  }
  printf(2,"wrote 1MB(indirect)\n");

  exit();
}

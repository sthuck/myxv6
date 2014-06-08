#include "types.h"
#include "stat.h"
#include "user.h"
#include "fs.h"
#include "fcntl.h"



int
main(int argc, char *argv[])
{
  char buf[32];
  symlink("/ls","/bla");
  symlink("/bla","/bla2");
  symlink("/bla2","/bla3");
  readlink("/bla",buf,31);
  printf(2,"%s \n",buf);
  readlink("/bla3",buf,31);
  printf(2,"%s \n",buf);
  symlink("/noexsist","/bla4");
  symlink("/bla4","/bla5");
  readlink("/bla5",buf,31);
  printf(2,"%s \n",buf);
  int ret = symlink("/ls","/mknod");
  printf(2,"overwrite file ret:%d\n",ret);
  exit();
}

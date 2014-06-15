#include "types.h"
#include "stat.h"
#include "user.h"
#include "fs.h"
#include "fcntl.h"

typedef enum { PLUS, MINUS, EQUAL } action_t;

void usage() {
    printf(1, "usage: find <path> <options> <predicates>\n");
    printf(1, "Options:\n");
    printf(1, "    -follow - follow symlink links.\n");
    printf(1, "    -help - Print a summary of the command-line usage of find.\n");
    printf(1, "Predicates:\n");
    printf(1, "    -name <file_name> - \n");
    printf(1, "    -size (+/-)n - File is of size n (exactly), +n (more than n), -n (less than n).\n");
    printf(1, "    -type c - File type:\n");
    printf(1, "          d - directory\n");
    printf(1, "          f - regular file\n");
    printf(1, "          s - symlink\n");
    exit();
    return;
}

static int
findLastSlash(char* buf) {
  int i;
  for (i=strlen(buf);i>=0;i--) {
    if (buf[i]=='/')
      return i+1;
  }
  return 0;
}

void
printFile(char *name,char* filename, action_t sizeOperator, int size, int type, struct stat st) {
  int flag=1;
  if (filename!=0) {
    int i = findLastSlash(name);
    if (strcmp(name+i,filename))
      flag=0;
  }
  if (flag && size!=-1) 
      if (!((sizeOperator == MINUS && st.size < size) ||  (sizeOperator == PLUS && st.size > size) || (sizeOperator == EQUAL && st.size == size)))
        flag=0;
  if (flag && type!=-1)
    if (!(type ==st.type))
      flag=0;
  if (flag)
    printf(1,"%s\n",name);
  return;
}

void 
find(char *name,int follow,char* filename, action_t sizeOperator, int size, int type){
    char buf[1024], *p;
    int fd;
    struct dirent de;
    struct stat st;
    if (follow) {
      if ((fd = open(name,O_RDONLY)) < 0) {
        printf(2, "find: cannot open %s\n", name);
        return;
      }
    }
    else {
      if ((fd = openNoFollow(name,O_RDONLY)) < 0) {
          printf(2, "find: cannot open %s\n", name);
          return;
      }
    }
    if (fstat(fd, &st) < 0) {
        printf(2, "find: cannot stat %s\n", name);
        close(fd);
        return;
    }
    switch (st.type) {
    case T_SYMLINK:
    case T_FILE:
        printFile(name ,filename, sizeOperator, size, type, st);
        break;
    case T_DIR:
        strcpy(buf, name);
        p = buf + strlen(name);
        if (*(p-1)!='/')
          *p++ = '/'; 
        printFile(name ,filename, sizeOperator, size, type, st);
        while (read(fd, &de, sizeof(de)) == sizeof(de)) {
            if (de.inum == 0)
                continue; 
            if (de.name[0] == '.') //dont scan .. and ./
                continue;
            memmove(p, de.name, DIRSIZ); 
            *(p+DIRSIZ) = 0;
            find(buf,follow,filename,sizeOperator,size,type);
        }
        break;
      default:  //device
        ;
    }
    close (fd);
    return;
}

int
main(int argc, char* argv[])
{
  char* path;
  char* filename=0;
  int follow=0;
  action_t sizeOperator;
  int size=-1;
  int type=-1;

  if (argc<2 || (strcmp(argv[1],"-help")==0))
    usage();
  
  path = argv[1];
  int i=2;
  while(i<argc) {

    if (strcmp(argv[i],"-help")==0) 
      usage();

    if (strcmp(argv[i],"-follow")==0) {
      follow=1;
      i++;
      continue;
    }
    if (strcmp(argv[i],"-name")==0) {
        if(++i<argc)
          filename = argv[i];
        else
          usage();
        i++;
        continue;
    }
    if (strcmp(argv[i],"-size")==0) {
      if (++i>=argc)
        usage();
      char* sizeinput = argv[i];
      switch(*sizeinput){
        case '+':
          sizeOperator = PLUS; 
          size = atoi(sizeinput+1);
          break;
        case '-':
          sizeOperator = MINUS;
          size = atoi(sizeinput+1);
          break;
        default:
          sizeOperator = EQUAL;
          size = atoi(sizeinput);
          break;
      }
      i++;
      continue;
    }
      else if (strcmp(argv[i],"-type")==0) {
        char* typeString = argv[++i];
        switch(typeString[0]){
        case 'f':
          type = T_FILE; 
          break;
        case 'd':
          type = T_DIR; 
          break;
        case 's':
          type = T_SYMLINK; 
          break;
        default:
          usage();
        }
        i++;
        continue;
      }
      usage();
  }
  find (path,follow,filename,sizeOperator,size,type);
  exit();
}
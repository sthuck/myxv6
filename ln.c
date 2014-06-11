#include "types.h"
#include "stat.h"
#include "user.h"

void usage() {
	printf(2, "Usage: ln [-s] old new\n");
    exit();
}

int
main(int argc, char *argv[])
{
	if (argc<1) 
		usage();
	if (strcmp("-s",argv[1])==0) {  //symlink
		if(argc != 4)
			usage();
		else {
			if(symlink(argv[2], argv[3]) < 0)
	    		printf(2, "symlink %s %s: failed\n", argv[2], argv[3]);
	  		exit();
		}
  	}
  	else if (strcmp("-c",argv[1])==0) {  //symlink
		if(argc != 3)
			usage();
		else {
			char buf[128];
			if(readlink(argv[2], buf,127) < 0)
	    		printf(2, "symlink check %s : failed\n", argv[2]);
	    	else
	    		printf(2,"%s\n",buf);
	  		exit();
		}
  	}
  	else { 							//hard link
  		if(argc != 3)
  			usage();
	  	if(link(argv[1], argv[2]) < 0)
	    	printf(2, "link %s %s: failed\n", argv[1], argv[2]);
	  	exit();
	}
	return 0;
}

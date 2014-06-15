#include "types.h"
#include "stat.h"
#include "user.h"

#include "param.h"
#include "fcntl.h"


int

main(int argc, char *argv[])

{
	
	int fd = open("README", O_RDWR);
	close(fd);
	printf(1,"a.txt===>%d\n",fd);

	// father put password
	int res = fprot("README", "12345678");
	printf(1,"set password: ==> %d\n",res);

	int id= fork();
	if (id==0){
		res = funprot("README", "12345678");
		//res = funlock("README", "12345678");
		printf(1,"funlock\\funprot right password ==> %d\n",res);
		fd = open("README", O_RDONLY);
		printf(1,"printing first 30 chars of file, fd is ==> %d\n",fd);
		char buf[31];
		read(fd,buf,30);
		printf(1,"%s\n",buf);
		close(fd);
		exit();
	}
	else{
		wait();
		fd = open("README", O_RDONLY);
		if (fd>0) {
			printf(1,"printing first 30 chars of file, fd is ==> %d\n",fd);
			char buf[31];
			read(fd,buf,30);
			printf(1,"%s\n",buf);
			close(fd);
		}
		else 
			printf(1,"error opening file\n");
		exit();
	}
	exit();
}



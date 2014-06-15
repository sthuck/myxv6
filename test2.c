#include "types.h"
#include "stat.h"
#include "user.h"

#include "param.h"
#include "fcntl.h"


int

main(int argc, char *argv[])

{
	
	int fd = open("a.txt", O_CREATE|O_RDWR);
	close(fd);
	printf(1,"a.txt===>%d\n",fd);

	// father put password
	int res = fprot("a.txt", "12345678");
	printf(1,"set password: ==> %d\n",res);

	res = fprot("a.txt", "aaaa");
	printf(1,"set password 2nd time: ==> %d\n",res);

	res = fprot("/", "aaaa");
	printf(1,"trying to set password on dir: ==> %d\n",res);

	fd = open("b.txt", O_CREATE|O_RDWR);
	close(fd);
	printf(1,"b.txt===>%d\n",fd);
	
	res = fprot("b.txt", "aaaa");
	printf(1,"set password b.txt ==> %d\n",res);
	res = funlock("b.txt", "aaaa");
	printf(1,"funlock on b.txt ==> %d\n",res);

	int id= fork();
	//father  try to open
	if (id!=0){
		fd = open("a.txt", O_RDWR);
		printf(1,"open file while locked ==> %d\n",fd);

		res = funlock("a.txt", "123478");
		printf(1,"funlock wrong password ==> %d\n",res);

		res = funlock("a.txt", "12345678");
		printf(1,"funlock right password ==> %d\n",res);

		fd = open("a.txt", O_RDWR);
		printf(1,"open unlocked file ==> %d\n",fd);
		wait();

		res = funprot("b.txt","aaaaa");
		printf(1,"disable password on file, wrong password ==> %d\n",res);
		res = funprot("b.txt","aaaa");
		printf(1,"disable password on file ==> %d\n",res);

		exit();
	}
	else{
		sleep(100);
		fd = open("a.txt", O_RDWR);
		printf(1,"open file unlocked after fork ==> %d\n",fd);

		fd = open("b.txt", O_RDWR);
		printf(1,"open file unlocked before fork ==> %d\n",fd);
		close(fd);
		exit();
	}
	exit();
}



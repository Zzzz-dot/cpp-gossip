#include "net/wrapped.h"
#include <iostream>
#include <cstring>
#include <time.h>
#include <thread>
using namespace std;

int main(){
	int fd[2];
	Pipe(fd);
	cout <<fd[0]<<" "<<fd[1]<<endl;
	auto f=[](int fdin){
		char buf[]="1231";
		sleep(3);
		Write(fdin,buf,strlen(buf));
		sleep(3);
		Write(fdin,buf,strlen(buf));
	};
	thread s(f,fd[1]);
	char buf[1024];
	Read(fd[0],buf,1024);
	close(fd[0]);
	close(fd[1]);
	cout<<buf<<endl;
	s.join();
	return 0;
}
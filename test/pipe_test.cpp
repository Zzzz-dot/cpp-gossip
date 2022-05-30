// 这个文件用来测试 pipe 写 0 字节是否可读；
// pipe 写端口关闭是否会向所有读端口发送信号，使得其可读；
// O_DIRECT 是否会产生多个读信号

#include <unistd.h>
#include <fcntl.h>
#include <iostream>
#include <thread>

using namespace std;
int main()
{
	fd_set rset;
	FD_ZERO(&rset);

	int msgfd[2];
	int shutdownfd[2];

	//这种方式可以多次读取
	//pipe2(msgfd, O_DIRECT);

	//这种方式只能读取一次
	pipe(msgfd);

	pipe(shutdownfd);

	auto f1 = [msgfd]
	{
		for (int i = 0; i < 10; i++)
		{
			write(msgfd[1], "f1", 2);
		}
	};

	auto f2 = [shutdownfd]
	{
		sleep(5);
		// 这种方式的写调用 read 不可读取
		//write(shutdownfd[1], nullptr, 0)

		// 这种方式的写调用 read 不可读取
		//write(shutdownfd[1], nullptr, 1)

		// 这种方式的写调用 read 只能读取一次
		//write(shutdownfd[1], "", 1);

		// 这种方式会通知到每一个 read
		close(shutdownfd[1]);
	};

	auto f3=[shutdownfd]
	{
		char buf[10];
		read(shutdownfd[0], buf, sizeof(buf));
		cout<<"f3"<<endl;
	};

	auto t1 = thread(f1);
	auto t2 = thread(f2);
	auto t3=thread(f3);

	sleep(1);

	for (;;)
	{
		FD_SET(msgfd[0], &rset);
		FD_SET(shutdownfd[0], &rset);
		int maxfd = msgfd[0] > shutdownfd[0] ? msgfd[0] : shutdownfd[0];
		select(maxfd + 1, &rset, nullptr, nullptr, nullptr);
		if (FD_ISSET(shutdownfd[0], &rset))
		{
			char buf[10];
			read(shutdownfd[0], buf, sizeof(buf));
			cout << "f2" << endl;
			break;
		}
		if (FD_ISSET(msgfd[0], &rset))
		{
			char buf[1000];
			int n = read(msgfd[0], buf, sizeof(buf));
			buf[n] = 0;
			cout << buf << endl;
		}
	}

	t1.join();
	t2.join();
	t3.join();
	return 0;
}
#include <stdio.h> // for perror
#include <string.h> // for memset
#include <unistd.h> // for close
#include <arpa/inet.h> // for htons
#include <netinet/in.h> // for sockaddr_in
#include <sys/socket.h> // for socket
#include <list>
#include <vector>
#include <thread>
#include <mutex>
using namespace std;

list<int> fdlist;
mutex mtx_lock;

void usage() {
	printf("syntax: echo_server <port> [-b]\n");
	printf("sample: echo_server 1234 -b\n");
}

void fun(int childfd)
{
	while (true) {
		const static int BUFSIZE = 1024;
		char buf[BUFSIZE];

		ssize_t received = recv(childfd, buf, BUFSIZE - 1, 0);
		if (received == 0 || received == -1) {
			perror("recv failed");
			break;
		}
		buf[received] = '\0';
		if(strcmp(buf, "quit") != 0)
			printf("%s\n", buf);

		ssize_t sent = send(childfd, buf, strlen(buf), 0);
		if(strcmp(buf, "quit") == 0)
			continue;
		if (sent == 0) {
			perror("send failed");
			break;
		}	
	}
	close(childfd);
}

void bfun(int childfd)
{
	mtx_lock.lock();
	fdlist.push_back(childfd);
	mtx_lock.unlock();
	while (true) {
		const static int BUFSIZE = 1024;
		char buf[BUFSIZE];

		ssize_t received = recv(childfd, buf, BUFSIZE - 1, 0);
		if (received == 0 || received == -1) {
			perror("recv failed");
			mtx_lock.lock();
			fdlist.remove(childfd);
			mtx_lock.unlock();
			break;
		}
		buf[received] = '\0';
		if(strcmp(buf, "quit") == 0)
		{
			send(childfd, buf, strlen(buf), 0);
			continue;
		}
		printf("%s\n", buf);
		mtx_lock.lock();
		for(list<int>::iterator iter = fdlist.begin(); iter != fdlist.end(); iter++){
			ssize_t sent = send(*iter, buf, strlen(buf), 0);
			if (sent == 0) {
				perror("send failed");
				fdlist.remove(*iter);
				continue;
			}
		}
		mtx_lock.unlock();
	}
	close(childfd);
}

int main(int argc, char* argv[]) {
	if (argc < 2) {
		usage();
		return -1;
	}
	int port = atoi(argv[1]);
	int chk = 0;
	if (argc == 3 && strncmp(argv[2], "-b", 2) == 0)
		chk = 1;

	int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd == -1) {
		perror("socket failed");
		return -1;
	}

	int optval = 1;
	setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR,  &optval , sizeof(int));

	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	memset(addr.sin_zero, 0, sizeof(addr.sin_zero));

	int res = bind(sockfd, reinterpret_cast<struct sockaddr*>(&addr), sizeof(struct sockaddr));
	if (res == -1) {
		perror("bind failed");
		return -1;
	}

	res = listen(sockfd, 2);
	if (res == -1) {
		perror("listen failed");
		return -1;
	}
	
	vector<thread> workers;
	while (true) {
		struct sockaddr_in addr;
		socklen_t clientlen = sizeof(sockaddr);
		int childfd = accept(sockfd, reinterpret_cast<struct sockaddr*>(&addr), &clientlen);
		if (childfd < 0) {
			perror("ERROR on accept");
			break;
		}
		printf("connected\n");

		if(chk)
			workers.push_back(thread(bfun, childfd));
		else
			workers.push_back(thread(fun, childfd));
	}
	int len = workers.size();
	for(int i=0; i<len; i++)
		workers[i].join();
	close(sockfd);
}

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>

#define CTL_PORT    18082
#define LOG_PATH    "/var/log/libhermit-kata-agent.log"

static ssize_t write_all(int fd, const char* buf, size_t len)
{
	size_t pos = 0;
    ssize_t r;

	while (pos < len) {
		r = write(fd, buf+pos, len-pos);
		if (r <= 0)
			goto out;
        pos += r;
	}
    r = len;

out:
	return r;
}

#if 0
static void logger(const char *logbuffer)
{
	int fd;

	fd = open(LOG_PATH, O_CREAT| O_WRONLY | O_APPEND, 0644);
	if (fd < 0) {
		printf("Log fail\n");
		exit(-1);
	}
	if (write_all(fd, logbuffer, strlen(logbuffer)) <= 0) {
		printf("Log fail\n");
		exit(-1);
	}
	if (write_all(fd, "\n", 1) <= 0) {
		printf("Log fail\n");
		exit(-1);
	}
	close(fd);
}
#endif

static void logger(const char *logbuffer)
{
	printf("%s\n", logbuffer);
}

pthread_t sandbox_thread;

void* sandbox_func(void* arg)
{
	int socketfd = (int)arg;
	char buf[1];

	logger("Sandbox start");

	read(socketfd, buf, 1);
	close(socketfd);

	logger("Sandbox stop");

	exit(0);
}

pthread_t container_threads[128];

void* container_func(void* arg)
{
	int socketfd = (int)arg;
	char buf[1];

	logger("Container start");

	read(socketfd, buf, 1);
	close(socketfd);

	logger("Container stop");
}

int
main(int argc, char **argv)
{
	int listenfd, socketfd;
	static struct sockaddr_in serv_addr;
	int sandbox_running = 0;
	int container_id = 0;

#if 0
    if (argc != 2) {
        printf("wasm file is not set\n");
        return -1;
    }

    return wamr(argv[1]);
#endif

	listenfd = socket(AF_INET, SOCK_STREAM, 0);
	if (listenfd < 0) {
		logger("socket failed");
		return listenfd;
	}
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_addr.sin_port = htons(CTL_PORT);
	if(bind(listenfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) <0) {
		logger("bind failed");
		return -1;
	}
	if(listen(listenfd, 4) < 0) {
		logger("listen failed");
		return -1;
	}

	printf("To Infinity and Beyond!\n");
	while(1) {
		int ret;

		logger("wait client");
		socketfd = accept(listenfd, NULL, NULL);
		if (socketfd < 0) {
			logger("accept failed");
			return -1;
		}
		logger("new client");

		if (!sandbox_running) {
			ret = pthread_create(&sandbox_thread, NULL, sandbox_func, (void *)socketfd);
			sandbox_running = 1;
		} else {
			ret = pthread_create(&container_threads[container_id], NULL, container_func, (void *)socketfd);
			container_id ++;
			if (container_id >= 128)
				break;
		}
		if (ret) {
			printf("pthread_create failed error =  %d\n", ret);
			return ret;
		}

	}
}

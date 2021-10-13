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

#if 0
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

#define va_list __builtin_va_list
#define va_start __builtin_va_start
#define va_arg __builtin_va_arg
#define va_end __builtin_va_end

static void logger(const char *fmt, ...)
{
	int real_fmt_max = strlen(fmt) + 50;
	char real_fmt[real_fmt_max];
	va_list ap;

	real_fmt[0] = '\0';
	strcat(real_fmt, fmt);
	strcat(real_fmt, "\n");
	va_start(ap, real_fmt);
	vprintf(real_fmt, ap);
	va_end(ap);
}

int
os_printf(const char *format, ...)
{
    int ret = 0;
    va_list ap;

    va_start(ap, format);
    ret += vprintf(format, ap);
    va_end(ap);

    return ret;
}

pthread_t container_threads[128];

void* container_func(void* arg)
{
	logger("Container start");

	wamr("/home/t4/teawater/hello.wasm");

	logger("Container stop");
}

int socketfd;

int
main(int argc, char **argv)
{
	int listenfd, ret;
	static struct sockaddr_in serv_addr;
	int sandbox_running = 0;
	int container_count = 0;
	char buf[1025];

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

	logger("wait kata");
	socketfd = accept(listenfd, NULL, NULL);
	if (socketfd < 0) {
		logger("accept failed");
		return -1;
	}
	logger("new client");

	while(1) {
		int buf_len;

		buf_len = lwip_recv(socketfd & ~LWIP_FD_BIT, &buf, 1024, MSG_DONTWAIT);
		if (buf_len == 0) {
			logger("read close");
			break;
		}
		if (buf_len == -1) {
			if (errno != EWOULDBLOCK) {
				sleep(1);
				continue;
			}
			logger("read fail %d", errno);
			break;
		}

#if 0
		buf_len = read(socketfd, buf, 1024);
		if (buf_len <= 0) {
			logger("read fail %d", buf_len);
			break;
		}
#endif
		buf[buf_len] = '\0';
		logger("read got %s", buf);

		switch(buf[0]) {
		case 'c':
			logger("new container");
			ret = pthread_create(&container_threads[container_count], NULL, container_func, NULL);
			if (ret < 0) {
				logger("pthread_create fail %d", ret);
				continue;
			}
			container_count ++;
			//logger("sleep");
			//sleep(10);
			break;
		default:
			goto out;
			break;
		}
	}

out:
	logger("quit");
	exit(0);
}

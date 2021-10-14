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

static pthread_mutex_t logger_mutex;

#define va_list __builtin_va_list
#define va_start __builtin_va_start
#define va_arg __builtin_va_arg
#define va_end __builtin_va_end

void logger(const char *fmt, ...)
{
	int real_fmt_max = strlen(fmt) + 50;
	char real_fmt[real_fmt_max];
	va_list ap;

	real_fmt[0] = '\0';
	strcat(real_fmt, fmt);
	strcat(real_fmt, "\n");
	va_start(ap, real_fmt);
	pthread_mutex_lock(&logger_mutex);
	vprintf(real_fmt, ap);
	pthread_mutex_unlock(&logger_mutex);
	va_end(ap);
}

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

int socketfd;
static pthread_mutex_t socketfd_mutex;

pthread_t container_threads[128];
struct container_struct
{
	char *id;
	char *exec;
};
struct container_struct container[128];

__thread struct container_struct *current_c;
void* container_func(void* arg)
{
	current_c = arg;
	logger("Container start %s %s", current_c->id, current_c->exec);

	wamr(current_c->exec);
	#if 0
	while(1) {
		logger("Container running");
		sleep(1);
	}
	#endif

	logger("Container stop");
}

#define HEADER_MAX	256
int netsend(const char *data, size_t len)
{
	int buf_max = len + HEADER_MAX;
	char buf[buf_max];
	int header_len;
	int ret;

	strcpy(buf, current_c->id);
	strcat(buf, ":");
	header_len = strlen(buf);
	if (header_len >= HEADER_MAX) {
		logger("netsend header %s is too big", buf);
		return -1;
	}

	memcpy(buf + header_len, data, len);
	buf[header_len + len] = '\0';
	//logger(buf);

	pthread_mutex_lock(&socketfd_mutex);
	ret = write_all(socketfd, buf, header_len + len);
	pthread_mutex_unlock(&socketfd_mutex);
	if (ret >= header_len)
		ret = len;

	return ret;
}

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
	pthread_mutex_init(&logger_mutex, 0);
	pthread_mutex_init(&socketfd_mutex, 0);

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

		//logger("lwip_recv");
		pthread_mutex_lock(&socketfd_mutex);
		buf_len = lwip_recv(socketfd & ~LWIP_FD_BIT, &buf, 1024, MSG_DONTWAIT);
		pthread_mutex_unlock(&socketfd_mutex);
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

		buf[buf_len] = '\0';
		logger("read got %s", buf);

		switch(buf[0]) {
		case 'c': {
			char *id_tail = strchr(buf + 1, ',');
			if (id_tail == NULL) {
				logger("format is not right");
				continue;
			}
			id_tail[0] = '\0';
			container[container_count].id = strdup(buf + 1);
			container[container_count].exec = strdup(id_tail + 1);
			
			logger("new container %s %s",
				   container[container_count].id,
				   container[container_count].exec);
			ret = pthread_create(&container_threads[container_count], NULL, container_func,
								 &container[container_count]);
			if (ret < 0) {
				logger("pthread_create fail %d", ret);
				continue;
			}
			container_count ++;
		}
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

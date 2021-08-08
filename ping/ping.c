#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#define BUFSIZE 1024
#define ECHO_HDR_SIZE 8

static int SendPing(int soc, char *name, int len, unsigned short sqc,
                    struct timespec *sendtime);
static int RecvPing(int soc, int len, unsigned short sqc,
                    struct timespec *sendtime, int timeoutSec);

static int CheckPacket(char *rbuff, int nbytes, int len,
                       struct sockaddr_in *from, unsigned short sqc, int *ttl,
                       struct timespec *sendtime, struct timespec *recvtime,
                       double *diff);

static int CalcChecksum(unsigned short *ptr, int nbytes);

int PingCheck(char *name, int len, int times, int timeoutSec);

int main(int argc, char **argv) {
  if (argc < 2) {
    fprintf(stderr, "usage: ./ping ipaddr\n");
    return (EXIT_FAILURE);
  }

  int ret = PingCheck(argv[1], 64, 5, 1);

  if (ret >= 0) {
    printf("RTT: %dms\n", ret);
    return (EXIT_SUCCESS);
  }
  printf("error: %d\n", ret);
  return (EXIT_FAILURE);
}

int PingCheck(char *name, int len, int times, int timeoutSec) {
  int soc;

  if ((soc = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP)) < 0) {
    return (-300);
  }

  int total = 0;
  int total_no = 0;

  for (int i = 0; i < times; i++) {
    // send Echo Request
    struct timespec sendtime;
    int ret = SendPing(soc, name, len, i+1, &sendtime);

    // on success sending
    if (ret == 0) {
      ret = RecvPing(soc, len, i+1, &sendtime, timeoutSec);

      // on success receiving
      if (ret >= 0) {
        total += ret;
        total_no++;
      }
    }
    sleep(1);
  }

  close(soc);

  if (total_no > 0) {
    return (total / total_no);
  } else {
    return (-1);
  }
}

static int SendPing(int soc, char *name, int len, unsigned short sqc,
                    struct timespec *sendtime) {
  struct hostent *host;
  struct sockaddr_in *sinp;
  struct sockaddr sa;
  struct icmp *icmph;
  unsigned char *ptr;
  int psize;
  int n;
  char sbuff[BUFSIZE];

  sinp = (struct sockaddr_in *)&sa;
  sinp->sin_family = AF_INET;

  // decide target host
  if ((sinp->sin_addr.s_addr = inet_addr(name)) == INADDR_NONE) {
    host = gethostbyname(name);
    if (host == NULL) {
      return (-100);
    }
    sinp->sin_family = host->h_addrtype;
    memcpy(&(sinp->sin_addr), host->h_addr, host->h_length);
  }

  // send time
  clock_gettime(CLOCK_REALTIME, sendtime);

  // create send data
  memset(sbuff, 0, BUFSIZE);
  icmph = (struct icmp *)sbuff;
  icmph->icmp_type = ICMP_ECHO;
  icmph->icmp_code = 0;
  icmph->icmp_id = htons((unsigned short)getpid());
  icmph->icmp_seq = htons(sqc);
  ptr = (unsigned char *)&sbuff[ECHO_HDR_SIZE];
  psize = len - ECHO_HDR_SIZE;

  for (; psize; psize--) {
    *ptr++ = (unsigned char)0xA5;
  }
  ptr = (unsigned char *)&sbuff[ECHO_HDR_SIZE];
  memcpy(ptr, sendtime, sizeof(struct timespec));
  icmph->icmp_cksum = CalcChecksum((u_short *)icmph, len);

  // send
  n = sendto(soc, sbuff, len, 0, &sa, sizeof(struct sockaddr));

  if (n == len) {
    return (0);
  } else {
    return (-1000);
  }
}

static int RecvPing(int soc, int len, unsigned short sqc,
                    struct timespec *sendtime, int timeoutSec) {
  struct pollfd targets[1];
  double diff;
  int nready, ret, nbytes, ttl;
  struct sockaddr_in from;
  socklen_t fromlen;
  struct timespec recvtime;
  char rbuff[BUFSIZE];

  memset(rbuff, 0, BUFSIZE);

  for (;;) {
    targets[0].fd = soc;
    targets[0].events = POLLIN | POLLERR;

    nready = poll(targets, 1, timeoutSec * 1000);
    if (nready == 0) {
      // timeout!
      return (-2000);
    }
    if (nready == -1) {
      if (errno == EINTR) {
        continue;
      } else {
        return (-2010);
      }
    }

    // recv
    fromlen = sizeof(from);
    nbytes = recvfrom(soc, rbuff, sizeof(rbuff), 0, (struct sockaddr *)&from,
                      &fromlen);

    // recv time
    clock_gettime(CLOCK_REALTIME, &recvtime);

    // check recv packet
    ret = CheckPacket(rbuff, nbytes, len, &from, sqc, &ttl, sendtime, &recvtime,
                      &diff);

    switch (ret) {
      case 0:  // recv correctly
        return ((int)(diff * 1000.0));

      case 1:  // packet is for other process
        if (diff > (timeoutSec * 1000)) {
          return (-2020);
        }
        break;

      default:  // other panic
          ;
    }
  }
}

static int CheckPacket(char *rbuff, int nbytes, int len,
                       struct sockaddr_in *from, unsigned short sqc, int *ttl,
                       struct timespec *sendtime, struct timespec *recvtime,
                       double *diff) {
  // extract ip header
  struct ip *iph = (struct ip *)rbuff;

  // extract icmp header
  struct icmp *icmph = (struct icmp *)(rbuff + iph->ip_len * 4);

  // checks
  if (ntohs(icmph->icmp_id) != (unsigned short)getpid()) {
    // incorrect icmp id
    return (1);
  }
  if (nbytes < len + iph->ip_len * 4) {
    // incorrect packet size
    return (-3000);
  }
  if (icmph->icmp_type != ICMP_ECHOREPLY) {
    // incorrect icmp type
    return (-3010);
  }
  if (ntohs(icmph->icmp_seq) != sqc) {
    // incorrect icmp sequence number
    return (-3030);
  }

  // icmp data offset
  unsigned short offset = iph->ip_len * 4 + ECHO_HDR_SIZE;

  // set ptr to offset if icmp payload
  unsigned char *ptr = (unsigned char *)(rbuff + offset);

  // copy timespec data from packet to sendtime variable
  memcpy(sendtime, ptr, sizeof(struct timespec));

  // seek to over timespec struct
  ptr += sizeof(struct timespec);

  for (int i = nbytes - offset - sizeof(struct timespec); i; i++) {
    // on our ping program, rest of data is filled by 0xA5, so if we find data
    // rather than 0xA5 then return error
    if (*ptr++ != 0xA5) {
      return (-3040);
    }
  }
  // calc RTT
  *diff = (double)(recvtime->tv_sec - sendtime->tv_sec) +
          (double)(recvtime->tv_nsec - sendtime->tv_nsec) / 1000000000.0;

  printf("%d bytes from %s: icmp_seq=%d ttl=%d time=%.4f ms\n",
         nbytes - iph->ip_len * 4, inet_ntoa(from->sin_addr), sqc, *ttl,
         *diff * 1000.0);
  return (0);
}

static int CalcChecksum(unsigned short *ptr, int nbytes) {
  long sum = 0;

  while (nbytes > 1) {
    sum += *ptr++;
    nbytes -= 2;
  }

  unsigned short oddbyte;
  if (nbytes == 1) {
    oddbyte = 0;
    *((unsigned char*)&oddbyte)=*(unsigned char*)ptr;
    sum += oddbyte;
  }

  sum = (sum >> 16) + (sum & 0xFFFF);
  sum += (sum >> 16);

  return (unsigned short) ~sum;
}
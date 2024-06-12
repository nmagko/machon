/*
 * Ping ameteur program is for users who need to test hosts with icmp packets
 * Copyright (C) 2006,2012  Victor C Salas Pumacayo (aka nmag)
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Compiling little library:
 *   gcc -Wall -Os -c pinga.c
 * Compiling for testing library:
 *   gcc -Wall -Os -DTEST_PING -o pinga pinga.c
 *
 *     Victor C Salas Pumacayo (aka nmag) <nmagko@gmail.com>
 */
 
#define _GNU_SOURCE

#include <stdio.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <netdb.h>
#include "pinga.h"

/* icmp packet size */
#define AT_LEAST_SIZE 46
#define TIMEOUT_ALARM 2
#define TCP_TIMEOUT_ALARM 6
#ifdef MINPACK_GCC
#define MINPACK MINPACK_GCC
#else
#define MINPACK 32
#endif
#define PINGA_RAW_TIMEOUT 255

static int shnd;
struct icmpreply resp;
static int infinito = 1;
static int minpack = MINPACK;
static int alter_packet_size = 0;

/* icmp packet checksum */
static int icmp_cksum (unsigned short *pbyte) {
  long sum = 0;
  unsigned short impar;
  /* int nidx = sizeof (struct icmphdr); */
  //  int nidx = MINPACK;
  int nidx = minpack;
  while ( nidx > 1 ) {
    sum += *pbyte++;
    nidx -= 2;
  }
  if ( nidx == 1 ) {
    impar = 0;
    *((unsigned char *)&impar) = *(unsigned char *)pbyte;
    sum += impar;
  }
  sum = (sum >> 16) + (sum & 0xffff);
  sum += (sum >> 16);
  return ~sum;
}

/* timeout alarm */
void timeouta (int sigrecv) {
  shutdown (shnd, 2); 
  close (shnd);
  infinito = 0;
}

/* tcp pinga funcion that fuck you :) */
int tcp_pinga(char *hostname, uint16_t port, float *resptime) {
  struct sockaddr_in sockin;
  struct hostent *hostinfo;
  struct timeval initime, endtime;

  shnd = socket(PF_INET, SOCK_STREAM, 0);
  if (shnd < 0) {
    perror("socket()");
    return EXIT_FAILURE;
  }
  sockin.sin_family = AF_INET;
  sockin.sin_port = htons(port);
  if ( ( hostinfo = gethostbyname(hostname) ) == NULL ) {
#ifdef TEST_PING
    fprintf ( stderr, "invalid host %s\n", hostname );
#endif
    return EXIT_FAILURE;
  }
  sockin.sin_addr = *(struct in_addr *) hostinfo->h_addr;
  /* waiting connect */
  gettimeofday(&initime, NULL); /* start time */
  signal (SIGALRM, timeouta);
  alarm (TCP_TIMEOUT_ALARM);
  if ( 0 > connect (shnd, (struct sockaddr *) &sockin, sizeof(sockin)))
    return EXIT_FAILURE;
  close(shnd);
  gettimeofday(&endtime, NULL); /* end time */
  *resptime = ( ( endtime.tv_sec - initime.tv_sec ) * 1000000 + ( endtime.tv_usec - initime.tv_usec ) ) / 1000.0;
  return EXIT_SUCCESS;
}

/* pinga function that fuck you :) */
struct icmpreply *pinga (int id, unsigned short seq, char *hostname) {
  struct sockaddr_in saddr;
  struct hostent *hent;
  int nidx;
  struct protoent *prent;
  struct icmphdr *icmp;
  unsigned char paq[AT_LEAST_SIZE + minpack];
  struct iphdr *ip;
  struct timeval initime, endtime;
  
  /* searching and checking host */
  if ( ! inet_aton (hostname, &saddr.sin_addr) ) {
    if ( ( hent = gethostbyname (hostname) ) == NULL ) {
#ifdef TEST_PING
      fprintf ( stderr, "invalid host %s\n", hostname );
#endif
      return NULL;
    }
    bcopy ( hent->h_addr, &saddr.sin_addr, hent->h_length );
  }
  saddr.sin_family = AF_INET;
  saddr.sin_port = 0;
  
  /* making icmp packet */
  bzero (paq, sizeof(paq));
  ip = (struct iphdr *)&paq;
  ip->ttl = (u_int8_t)127;
  icmp = (struct icmphdr *)paq;
  icmp->type = ICMP_ECHO; /* echo request */
  icmp->code = 0;
  icmp->checksum = 0;
  icmp->un.echo.sequence = seq;
  icmp->un.echo.id = id; /* (getpid() & 0xFFFF); */

  /* making socket */
  if ( ! ( prent = getprotobyname("icmp") ) ) {
#ifdef TEST_PING
    perror ("getprotobyname");
#endif
    return NULL;
  }
  setprotoent(0);
  if ( ( shnd = socket (AF_INET, SOCK_RAW, prent->p_proto ) ) < 0 ) {
#ifdef TEST_PING
    perror ("socket");
#endif
    return NULL;
  }
  
  /* icmp packet checksum */
  icmp->checksum = icmp_cksum ( (unsigned short *)paq );

  /* sending packet */
  gettimeofday(&initime, NULL); /* start time */
  /* if ( sendto (shnd, &paq, sizeof(struct icmphdr), 0, (struct sockaddr *)&saddr, sizeof(saddr)) < 0 ) { */
  /* if ( sendto (shnd, &paq, MINPACK, 0, (struct sockaddr *)&saddr, sizeof(saddr)) < 0 ) { */
  if ( sendto (shnd, &paq, minpack, 0, (struct sockaddr *)&saddr, sizeof(saddr)) < 0 ) {
#ifdef TEST_PING
    perror ("sendto");
#endif
    shutdown (shnd, 2);
    close (shnd);
    return NULL;
  }
  if ( alter_packet_size )
    printf ("sending %d bytes\n", minpack);
  
  /* waiting hit */
  signal (SIGALRM, timeouta);
  alarm (TIMEOUT_ALARM);
  do {
    bzero (&saddr, sizeof(saddr));
    bzero (paq, sizeof(paq));
    /* if ( recvfrom (shnd, &paq, sizeof(paq), 0, (struct sockaddr_in *)&saddr, (socklen_t *)&nidx) >= 0 ) { */
    if ( recvfrom (shnd, &paq, sizeof(paq), 0, (struct sockaddr *)&saddr, (socklen_t *)&nidx) >= 0 ) {
      ip = (struct iphdr *)&paq;
      icmp = (struct icmphdr *)(&paq[0] + (ip->ihl << 2));
      if ( icmp->un.echo.id == id && icmp->un.echo.sequence == seq ) {
        alarm (0);
        gettimeofday(&endtime, NULL); /* end time */
        resp.id = icmp->un.echo.id;
        resp.type = icmp->type;
        resp.code = icmp->code;
        resp.time = ( ( endtime.tv_sec - initime.tv_sec ) * 1000000 + ( endtime.tv_usec - initime.tv_usec ) ) / 1000.0;
        shutdown (shnd, 2);
        close (shnd);
        return &resp;
      }
    }
  } while (infinito);
  infinito = 1; /* EXCEEDED bug */
  resp.type = 255; /* timeout */
  resp.time = -1;
  return &resp;
}

/* main program */
#ifdef TEST_PING
char *nans = (char *)&"no answer from";
char *help = (char *)&"usage: pinga host [packet_size]\n"
  "usage: pinga -p host [port]\n\n"
  "     host           Host to send packet\n"
  "     packet_size    Default 16 bytes\n\n"
  "     -p             Entry test TCP mode\n"
  "     port           TCP port used with -p option\n\n"
  " * IP Timeout 2000 ms\n"
  " * TCP Timeout 6000 ms\n\n";
int main (int argc, char *argv[]) {
  struct icmpreply *resp;
  float resptime;
  if (argc < 2 || argc > 4) {
    fprintf(stderr, "%s", help);
    exit (EXIT_FAILURE);
  }
  if ( ( strcmp(argv[1], "-p") == 0 ) && ( argc != 4 ) ) {
    fprintf(stderr, "%s", help);
    exit (EXIT_FAILURE);
  }
  if ( argc == 3 ) {
    alter_packet_size = 1;
    minpack = strtol(argv[2], NULL, 10);
  }
  if ( strcmp(argv[1], "-p") == 0 ) {
    if ( tcp_pinga (argv[2], strtol(argv[3], NULL, 10), &resptime) == 0 ) {
      fprintf (stderr, "%s:%s is alive [%.2f ms]\n", argv[2], argv[3], resptime);
      exit (EXIT_SUCCESS);
    } else {
      fprintf (stderr, "%s %s:%s\n", nans, argv[2], argv[3]);
      exit (EXIT_FAILURE);
    }
  }
  if ( ( resp = pinga ( (getpid() & 0xFFFF), 48000, argv[1]) ) != NULL )
    switch (resp->type) {
    case ICMP_ECHOREPLY:
      fprintf (stderr, "%s is alive [%.2f ms]\n", argv[1], resp->time);
      exit (EXIT_SUCCESS);
      break;
    case ICMP_DEST_UNREACH: fprintf (stderr, "%s %s", nans, argv[1]);
      switch (resp->code) {
      case ICMP_NET_UNREACH:   fprintf (stderr, " [net_unreach]\n"); break;
      case ICMP_HOST_UNREACH:  fprintf (stderr, " [host_unreach]\n"); break;
      case ICMP_PROT_UNREACH:  fprintf (stderr, " [prot_unreach]\n"); break;
      case ICMP_PORT_UNREACH:  fprintf (stderr, " [port_unreach]\n"); break;
      case ICMP_SR_FAILED:     fprintf (stderr, " [sr_failed]\n"); break;
      case ICMP_PKT_FILTERED:  fprintf (stderr, " [pkt_filtered]\n"); break;
      case ICMP_FRAG_NEEDED:   fprintf (stderr, " [frag_needed]\n"); break;
      case ICMP_NET_UNKNOWN:   fprintf (stderr, " [net_unknown]\n"); break;
      case ICMP_HOST_UNKNOWN:  fprintf (stderr, " [host_unknown]\n"); break;
      case ICMP_HOST_ISOLATED: fprintf (stderr, " [host_isolated]\n"); break;
      case ICMP_NET_ANO:       fprintf (stderr, " [net_ano]\n"); break;
      case ICMP_HOST_ANO:      fprintf (stderr, " [host_ano]\n"); break;
      case ICMP_NET_UNR_TOS:   fprintf (stderr, " [net_unr_tos]\n"); break;
      case ICMP_HOST_UNR_TOS:  fprintf (stderr, " [host_unr_tos]\n"); break;
      case ICMP_PREC_VIOLATION: fprintf (stderr, " [prec_violation]\n"); break;
      case ICMP_PREC_CUTOFF:   fprintf (stderr, " [prec_cutoff]\n"); break;
      default:
        fprintf (stderr, " [dest_unreach]\n"); break;
      }
      break;
    case PINGA_RAW_TIMEOUT: fprintf (stderr, "%s %s\n", nans, argv[1]); break;
    case ICMP_TIME_EXCEEDED: fprintf (stderr, "%s %s [time_exceeded]\n", nans, argv[1]); break;
    case ICMP_SOURCE_QUENCH: fprintf (stderr, "%s %s [source_quench]\n", nans, argv[1]); break;
    case ICMP_REDIRECT: fprintf (stderr, "%s %s [redirect]\n", nans, argv[1]); break;
    case ICMP_ECHO:
      fprintf (stderr, "%s is alive [echo %.2f ms]\n", argv[1], resp->time);
      exit (EXIT_SUCCESS);
      break;
    case ICMP_PARAMETERPROB: fprintf (stderr, "%s %s [parameterprob]\n", nans, argv[1]); break;
    case ICMP_TIMESTAMP: fprintf (stderr, "%s %s [timestamp]\n", nans, argv[1]); break;
    case ICMP_TIMESTAMPREPLY: fprintf (stderr, "%s %s [timestampreply]\n", nans, argv[1]); break;
    case ICMP_INFO_REQUEST: fprintf (stderr, "%s %s [info_request]\n", nans, argv[1]); break;
    case ICMP_INFO_REPLY: fprintf (stderr, "%s %s [info_reply]\n", nans, argv[1]); break;
    case ICMP_ADDRESS: fprintf (stderr, "%s %s [address]\n", nans, argv[1]); break;
    case ICMP_ADDRESSREPLY: fprintf (stderr, "%s %s [addressreply]\n", nans, argv[1]); break;
    default:
      fprintf (stderr, "no answer from %s [%d]\n", argv[1], resp->type); break;
    }
  exit (EXIT_FAILURE);
}
#endif

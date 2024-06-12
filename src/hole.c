/*
 * HOst LEss
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
 *   gcc -Wall -O2 -o hole hole.c
 *
 *     Victor C Salas Pumacayo (aka nmag) <nmagko@gmail.com>
 */

#define _GNU_SOURCE

#include <stdlib.h>
#include <sys/unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>

#ifndef program_invocation_short_name
#  define program_invocation_short_name "hole"
#endif

int main(int argc, char *argv[]) {
  char *hostname, *detail, *wdet;
  int i;
  struct hostent *he;
  struct in_addr **addr_list;
  in_addr_t ipaddr;
  if ( argc != 2 ) {
    printf("usage: %s <hostname|address>\n", program_invocation_short_name);
    exit(EXIT_FAILURE);
  }
  hostname = argv[1];
  
  if ( ( ipaddr = inet_addr(hostname) ) == -1 ) {
    // do some error checking
    if ( ( he = gethostbyname(hostname) ) == NULL ) {
      herror("gethostbyname"); // herror(), NOT perror()
      return 1;
    }
    
    // printf("hostname: %s\n", he->h_name);
    // printf("IP: %s\n", inet_ntoa(*(struct in_addr*)he->h_addr));
    addr_list = (struct in_addr **)he->h_addr_list;
    
    printf("%s has address", he->h_name);
    for(i = 0; addr_list[i] != NULL; i++) {
      if ( i == 0 )
        asprintf(&detail, " %s", inet_ntoa(*addr_list[i]));
      else {
        wdet = detail;
        asprintf(&detail, "%s, %s", detail, inet_ntoa(*addr_list[i]));
        free (wdet);
      }
    }
    
    if ( i > 1 )
      printf("es%s\n", detail);
    else
      printf("%s\n", detail);
    
  } else {
    if ( ( he = gethostbyaddr((char *)&ipaddr, sizeof(in_addr_t), AF_INET) ) == NULL ) {
      herror("Host not found");
      return 1;
    }

    printf("%s domain name pointer", argv[1]);
    i = 0;
    asprintf(&detail, " %s.", he->h_name);
    if ( he->h_aliases != NULL && *he->h_aliases != NULL ) {
      for (i = 0; he->h_aliases[i] != NULL; i++) {
        wdet = detail;
        asprintf(&detail, "%s, %s", detail, he->h_aliases[i]);
        free(wdet);
      }
    }

    if ( i >= 1 )
      printf("s%s\n", detail);
    else
      printf("%s\n", detail);
  }

  free(detail);
  
  return 0;
}

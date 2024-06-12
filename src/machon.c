/*
 * MACHine MONitor tests computers availability with icmp packets
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
 * Compiling:
 *   gcc -Wall -Os -lncurses -luregex -lpthread -o machon pinga.o machon.c
 *
 *     Victor C Salas Pumacayo (aka nmag) <nmagko@gmail.com>
 */
 
#define _GNU_SOURCE

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
//#include <ncurses/ncurses.h>
#include <ncurses.h>
#include <string.h>
#include <sys/wait.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <arpa/inet.h>
//#include <pthread.h>
#include <aarray.h>
#include <uconfi.h>
#include <time.h>
#include "pinga.h"

#ifdef CYGWIN32
#define program_invocation_short_name "machon"
#endif

#define VERSION    0
#define SUBVERSION 1
#define PATCHLEVEL 3
#define MACHON_TIMEOUT 255

/* Espacio reservado para hosts a chequear */
#define HOSTCOUNT  1000

/* Entorno compartido de archivo */
FILE * logF;
char logFoutput[64];
time_t it;
struct tm *tmp;
char logfilename[128];
char lognewfname[128];

/* Entorno ncurses */
WINDOW *wHndTitle;
WINDOW *wHndMain[HOSTCOUNT];

/* Estructuras compartidas para mostrar en cada ventana */
char *tHndMain[HOSTCOUNT];
u_int16_t toCounter[HOSTCOUNT];
//u_int8_t pHndMain;
struct AArray *hosts;

/* Estructuras para el taskbar */
char *title;
char *step;
int istep;

/* Estructuras para el body */
int vilines, vicols, visible;
int lastvilines, lastvicols;
int ic, il, iv;
int pic, pil, piv;
int global_seq = 1;
long c_hosts;
char **k_hosts;
char **v_hosts;
pthread_t pingthread;
int firsthread = 1;
int pingret;

/* Entrada de funciones */
static void finish(int);
static void title_refresh(void);
static void body_refresh(void);
static void mach_test(int, const char *, char *);
void *before_test(void);

int main(int argc, char *argv[]) {
  /* inicializar sus estructuras no ncurses aquí */
  char sufix[11];
  (void) signal(SIGINT, finish);      /* SIGINT o 2, CTRL-C */
  hosts = get_config("/etc/machon.hosts");
  k_hosts = keys_aa(hosts, &c_hosts);
  v_hosts = values_aa(hosts, &c_hosts);
  it = time(NULL);
  tmp = localtime(&it);
  strftime(sufix, sizeof(sufix), "%F", tmp);
	sprintf(logfilename, "/var/log/machon-%s.log", sufix);
  if ( (logF = fopen(logfilename, "a")) == NULL ) {
    perror("fopen():");
    exit(EXIT_FAILURE);
  }
  for ( istep = 0; istep < HOSTCOUNT; istep++ ) toCounter[istep] = (u_int16_t)0;
  
  /* iniciando marcador de procesos */
  //pHndMain = (u_int8_t) 0;

  /* inicializando ncurses */
  (void) initscr();
  lastvilines = 0;
  lastvicols = 0;
  
  if (has_colors()) {
    start_color();
    /* El par 0 no se puede redefinir, queda como blanco sobre negro. */
    /* init_pair(1, COLOR_RED,     COLOR_WHITE);
     * init_pair(2, COLOR_GREEN,   COLOR_BLUE);
     * init_pair(3, COLOR_YELLOW,  COLOR_BLUE);
     * init_pair(3, COLOR_YELLOW,  COLOR_BLACK);
     * init_pair(4, COLOR_BLUE,    COLOR_WHITE);
     * init_pair(6, COLOR_MAGENTA, COLOR_BLUE);
     */
    init_pair(1, COLOR_YELLOW,  COLOR_BLACK);  // 80 ms > time <= 200 ms
    init_pair(2, COLOR_BLACK,   COLOR_YELLOW); // 200 ms > time <= 640 ms
    init_pair(3, COLOR_RED,     COLOR_YELLOW); // 640 ms > time <= 1280 ms
    init_pair(4, COLOR_WHITE,   COLOR_RED);    // 1280 ms > time
    init_pair(5, COLOR_CYAN,    COLOR_BLUE);
    init_pair(6, COLOR_YELLOW,  COLOR_RED);    // timeout
    init_pair(7, COLOR_BLACK,   COLOR_WHITE);
  }
  
  for(;;) {
    title_refresh(); /* barra de título */
    body_refresh();
    doupdate(); /* general update */
  }
  exit(EXIT_SUCCESS);
}

static void finish(int sig) {
  endwin();
  fclose(logF);
  exit(EXIT_SUCCESS);
}

static void title_refresh(void) {
  if ( wHndTitle == NULL )
    wHndTitle = newwin(1, COLS, 0, 0);
  wattrset(wHndTitle, COLOR_PAIR(5));
  title = malloc(COLS + 1);
  step = title;
  for ( istep = 1; istep < COLS ; istep++ )
    bcopy (" ", step++, 1);
  strcpy (step, " ");
  mvwprintw(wHndTitle, 0, 0, "%s", title);
  free(title);
  asprintf(&title, " MACHine MONitor %d.%d.%d (C) 2006  Victor C. Salas P. (GPL)  [%010d] ",
                     VERSION, SUBVERSION, PATCHLEVEL, global_seq);
  mvwprintw(wHndTitle, 0, 0, "%s", title);
  free(title);
  wnoutrefresh(wHndTitle);
}

static void body_refresh(void) {
  vilines = LINES - 1;
  vicols = (int)(COLS / 40);
  visible = vilines * vicols;
  
  if ( lastvilines != vilines || lastvicols != vicols ) {
    //while ( pHndMain != (u_int8_t) 0 ) { sleep(1); }
    iv = 0;
    for ( ic = 0; ic < lastvicols; ic++ )
      for ( il = 1; il <= lastvilines; il++ ) {
        if ( wHndMain[iv] != NULL ) {
          wclear ( wHndMain[iv] );
          wnoutrefresh(wHndMain[iv]);
          delwin ( wHndMain[iv] );
          wHndMain[iv] = NULL;
        }
        iv++;
      }
    lastvilines = vilines;
    lastvicols = vicols;
  }
        
  iv = 0;
  for ( ic = 0; ic < vicols; ic++ )
    for ( il = 1; il <= vilines; il++ )
      if ( iv < visible || iv < HOSTCOUNT ) {
        if ( tHndMain[iv] == NULL ) {
          tHndMain[iv] = malloc(41);
          sprintf(tHndMain[iv], " ");
        }
        if ( wHndMain[iv] == NULL ) {
          wHndMain[iv] = newwin(1, 40, il, (40 * ic));
          wattrset(wHndMain[iv], A_NORMAL);
        }
        mvwprintw(wHndMain[iv], 0, 0, "%-40s", tHndMain[iv]);
        wnoutrefresh(wHndMain[iv]);
        iv++;
      }
      
  /* if ( pHndMain == (u_int8_t) 0 ) {
    if ( firsthread )
      firsthread = 0;
    else
      pthread_join (pingthread, NULL);
    pHndMain = (u_int8_t) 1;
    pingret = pthread_create (&pingthread, NULL, (void *)before_test, NULL);
  } */
	before_test();
  //sleep(1);
}

static void only_windows_refresh(void) {
  iv = 0;
  for ( ic = 0; ic < vicols; ic++ )
    for ( il = 1; il <= vilines; il++ )
      if ( iv < visible || iv < HOSTCOUNT ) {
        mvwprintw(wHndMain[iv], 0, 0, "%-40s", tHndMain[iv]);
        wnoutrefresh(wHndMain[iv]);
        iv++;
      }
}

void *before_test(void) {
  piv = 0;
  for ( pic = 0; pic < vicols; pic++ )
    for ( pil = 1; pil <= vilines; pil++ )
      if ( piv < visible && piv < HOSTCOUNT && piv < c_hosts ) {
        mach_test(piv, k_hosts[piv], v_hosts[piv]);
        piv++;
        //if ( ( piv % 8 ) == 0 ) sleep(1); /* enviando en bloques de 8 hosts x 8 bytes = 64 bytes */
      }
  sleep(1);
  //pHndMain = (u_int8_t) 0;
  global_seq++;
  //pthread_exit(NULL);
	return NULL;
}

static void mach_test(int iv, const char *hostname, char *ip) {
  struct icmpreply *resp;
  struct tm *tmpm;
  char hora[3];
  char minutos[3];
  char newsfix[11];
  u_int8_t redisplay = 0;
  if ( ( resp = pinga (iv, global_seq, ip) ) != NULL ) {
    it = time(NULL);
    tmpm = localtime(&it);
    strftime(newsfix, sizeof(newsfix), "%F", tmpm);
    strftime(hora, sizeof(hora), "%H", tmpm);
    strftime(minutos, sizeof(minutos), "%M", tmpm);
    sprintf(lognewfname, "/var/log/machon-%s.log", newsfix);
    if ( strcmp(lognewfname, logfilename) != 0 ) {
      fclose(logF);
      strcpy(logfilename, lognewfname);
      if ( (logF = fopen(logfilename, "a")) == NULL ) {
        endwin();
        perror("fopen():");
        exit(EXIT_FAILURE);
      }
    }
    switch (resp->type) {
      case ICMP_ECHOREPLY:
        if ( resp->time <= 60 ) {
          wattrset(wHndMain[iv], A_NORMAL);
        } else if ( resp->time > 60 && resp->time <= 136 ) {
          wattrset(wHndMain[iv], COLOR_PAIR(1));
        } else if ( resp->time > 136 && resp->time <= 620 ) {
          wattrset(wHndMain[iv], COLOR_PAIR(2));
        } else if ( resp->time > 620 && resp->time <= 1280 ) {
          wattrset(wHndMain[iv], COLOR_PAIR(3));
        } else if ( resp->time > 1280 ) {
          wattrset(wHndMain[iv], COLOR_PAIR(4));
          redisplay = 1;
        }
        sprintf (tHndMain[iv], "%-13s[%05d/%2.2f] %.2f ms", hostname, toCounter[iv], 99.99-(toCounter[iv]*99.99/global_seq), resp->time);
        sprintf (logFoutput, "%s,%s,%s,%d,%2.2f,%.2f\r\n", hostname, hora, minutos, toCounter[iv], 99.99-(toCounter[iv]*99.99/global_seq), resp->time);
        break;
      case ICMP_DEST_UNREACH: sprintf (tHndMain[iv], "%-13s[%05d/%2.2f] UnreaDST", hostname, ++toCounter[iv], 99.99-(toCounter[iv]*99.99/global_seq));
        sprintf (logFoutput, "%s,%s,%s,%d,%2.2f,UnreaDST\r\n", hostname, hora, minutos, toCounter[iv], 99.99-(toCounter[iv]*99.99/global_seq));
        switch (resp->code) {
          case ICMP_NET_UNREACH:
            wattrset(wHndMain[iv], COLOR_PAIR(6));
            sprintf (tHndMain[iv], "%-13s[%05d/%2.2f] UnreaNET", hostname, ++toCounter[iv], 99.99-(toCounter[iv]*99.99/global_seq));
            sprintf (logFoutput, "%s,%s,%s,%d,%2.2f,UnreaNET\r\n", hostname, hora, minutos, toCounter[iv], 99.99-(toCounter[iv]*99.99/global_seq));
            break;
          case ICMP_HOST_UNREACH:
            wattrset(wHndMain[iv], COLOR_PAIR(6));
            sprintf (tHndMain[iv], "%-13s[%05d/%2.2f] UnreaHST", hostname, ++toCounter[iv], 99.99-(toCounter[iv]*99.99/global_seq));
            sprintf (logFoutput, "%s,%s,%s,%d,%2.2f,UnreaHST\r\n", hostname, hora, minutos, toCounter[iv], 99.99-(toCounter[iv]*99.99/global_seq));
            break;
          case ICMP_PROT_UNREACH:
            wattrset(wHndMain[iv], COLOR_PAIR(6));
            sprintf (tHndMain[iv], "%-13s[%05d/%2.2f] UnreaPRO", hostname, ++toCounter[iv], 99.99-(toCounter[iv]*99.99/global_seq));
            sprintf (logFoutput, "%s,%s,%s,%d,%2.2f,UnreaPRO\r\n", hostname, hora, minutos, toCounter[iv], 99.99-(toCounter[iv]*99.99/global_seq));
            break;
          case ICMP_PORT_UNREACH:
            wattrset(wHndMain[iv], COLOR_PAIR(6));
            sprintf (tHndMain[iv], "%-13s[%05d/%2.2f] UnreaPOR", hostname, ++toCounter[iv], 99.99-(toCounter[iv]*99.99/global_seq));
            sprintf (logFoutput, "%s,%s,%s,%d,%2.2f,UnreaPOR\r\n", hostname, hora, minutos, toCounter[iv], 99.99-(toCounter[iv]*99.99/global_seq));
            break;
          case ICMP_SR_FAILED:
            wattrset(wHndMain[iv], COLOR_PAIR(6));
            sprintf (tHndMain[iv], "%-13s[%05d/%2.2f] UnreaSRC", hostname, ++toCounter[iv], 99.99-(toCounter[iv]*99.99/global_seq));
            sprintf (logFoutput, "%s,%s,%s,%d,%2.2f,UnreaSRC\r\n", hostname, hora, minutos, toCounter[iv], 99.99-(toCounter[iv]*99.99/global_seq));
            break;
          case ICMP_PKT_FILTERED:
            wattrset(wHndMain[iv], COLOR_PAIR(6));
            sprintf (tHndMain[iv], "%-13s[%05d/%2.2f] UnreaFLT", hostname, ++toCounter[iv], 99.99-(toCounter[iv]*99.99/global_seq));
            sprintf (logFoutput, "%s,%s,%s,%d,%2.2f,UnreaFLT\r\n", hostname, hora, minutos, toCounter[iv], 99.99-(toCounter[iv]*99.99/global_seq));
            break;
          case ICMP_FRAG_NEEDED:
          case ICMP_NET_UNKNOWN:
          case ICMP_HOST_UNKNOWN:
          case ICMP_HOST_ISOLATED:
          case ICMP_NET_ANO:
          case ICMP_HOST_ANO:
          case ICMP_NET_UNR_TOS:
          case ICMP_HOST_UNR_TOS:
          case ICMP_PREC_VIOLATION:
          case ICMP_PREC_CUTOFF:
          default:
            wattrset(wHndMain[iv], COLOR_PAIR(6));
            sprintf (tHndMain[iv], "%-13s[%05d/%2.2f] UnreaUNK", hostname, ++toCounter[iv], 99.99-(toCounter[iv]*99.99/global_seq));
            sprintf (logFoutput, "%s,%s,%s,%d,%2.2f,UnreaUNK\r\n", hostname, hora, minutos, toCounter[iv], 99.99-(toCounter[iv]*99.99/global_seq));
            break;
        }
        break;
      case MACHON_TIMEOUT: 
        wattrset(wHndMain[iv], COLOR_PAIR(6));
        sprintf (tHndMain[iv], "%-13s[%05d/%2.2f] TIMEOUT", hostname, ++toCounter[iv], 99.99-(toCounter[iv]*99.99/global_seq));
        sprintf (logFoutput, "%s,%s,%s,%d,%2.2f,TIMEOUT\r\n", hostname, hora, minutos, toCounter[iv], 99.99-(toCounter[iv]*99.99/global_seq));
        redisplay = 1;
        break;
      case ICMP_TIME_EXCEEDED:
        wattrset(wHndMain[iv], COLOR_PAIR(6));
        sprintf (tHndMain[iv], "%-13s[%05d/%2.2f] EXCEEDED", hostname, ++toCounter[iv], 99.99-(toCounter[iv]*99.99/global_seq));
        sprintf (logFoutput, "%s,%s,%s,%d,%2.2f,EXCEEDED\r\n", hostname, hora, minutos, toCounter[iv], 99.99-(toCounter[iv]*99.99/global_seq));
        break;
      case ICMP_SOURCE_QUENCH:
      case ICMP_REDIRECT:
      case ICMP_ECHO:
      case ICMP_PARAMETERPROB:
      case ICMP_TIMESTAMP:
      case ICMP_TIMESTAMPREPLY:
      case ICMP_INFO_REQUEST:
      case ICMP_INFO_REPLY:
      case ICMP_ADDRESS:
      case ICMP_ADDRESSREPLY:
      default:
        wattrset(wHndMain[iv], COLOR_PAIR(6));
        sprintf (tHndMain[iv], "%-13s[%05d/%2.2f] ICMP%d", hostname, ++toCounter[iv], 99.99-(toCounter[iv]*99.99/global_seq), resp->type);
        sprintf (logFoutput, "%s,%s,%s,%d,%2.2f,ICMP%d\r\n", hostname, hora, minutos, toCounter[iv], 99.99-(toCounter[iv]*99.99/global_seq), resp->type);
        break;
    }
    fputs(logFoutput,logF);
  } else {
    wattrset(wHndMain[iv], COLOR_PAIR(6));
    sprintf (tHndMain[iv], "%-13s[%05d/%2.2f] UNSOLVED", hostname, ++toCounter[iv], 99.99-(toCounter[iv]*99.99/global_seq));
    sprintf (logFoutput, "%s,%s,%s,%d,%2.2f,UNSOLVED\r\n", hostname, hora, minutos, toCounter[iv], 99.99-(toCounter[iv]*99.99/global_seq));
  }
  if ( redisplay ) {
    title_refresh(); /* barra de título */
    only_windows_refresh();
    doupdate(); /* general update */
  }
}

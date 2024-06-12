/* 
 * Name resolver
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
 */

#define _GNU_SOURCE

#define LINE_SIZE 4096

#ifdef CYGWIN32
#define program_invocation_short_name "resolvname"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main (int argc, char *argv[]) {
  FILE *outexec;
  char *cmdexec;
  char ptr[LINE_SIZE + 1];
  if (argc != 2) {
    printf("usar: %s <ip>\n", program_invocation_short_name);
    exit(EXIT_FAILURE);
  }
#ifndef CYGWIN32
  asprintf(&cmdexec, "echo \"Dirección IP: [%s]\"; echo \"\"; host %s", argv[1], argv[1]);
#else
  asprintf(&cmdexec, "nbtstat -a %s", argv[1]);
#endif
  if ( (outexec = popen(cmdexec, "r")) != NULL) {
    memset(&ptr, 0, sizeof(ptr));
    fread(&ptr, LINE_SIZE, 1, outexec);
    ptr[LINE_SIZE] = (char)NULL;
    printf("%s", ptr);
    pclose(outexec);
  } else {
    perror ("popen()");
  }
  free(cmdexec);
  exit(EXIT_SUCCESS);
}

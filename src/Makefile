# Makefile
# Copyright (C) 2006,2012  Victor C Salas Pumacayo (aka nmag)
#
#  This program is free software: you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation, either version 3 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program.  If not, see <http://www.gnu.org/licenses/>.

CC        = gcc
CP	  = cp
RM	  = rm
COPTS     = -Wno-sequence-point
CCFLAGS   = -Wall $(COPTS) 
PERLFLAGS = -B
CURSES    = -lcurses
LIBRARIES = -luregex $(CURSES)
OBJECTDIR = .
INCLUDIR  = -I/usr/include/ncurses
PREFIX    = .
INSTALL   = install
MAKE      = make
STRIP	  = strip
CHMOD     = chmod
MINPACK   = 16

all: lib binary

lib:
	$(CC) $(CCFLAGS) -c $(OBJECTDIR)/pinga.c

clean:
	rm -f *~
	rm -f *.o
	rm -f machonanal machon pinga hole

binary:
	$(CC) $(CCFLAGS) $(INCLUDIR) -o machon pinga.o machon.c $(LIBRARIES)
	$(STRIP) machon
	$(CP) machonanal.pl machonanal
	$(CC) $(CCFLAGS) -DTEST_PING -DMINPACK_GCC=$(MINPACK) -o pinga pinga.c
	$(STRIP) pinga
	$(CC) $(CCFLAGS) -o hole hole.c
	$(STRIP) hole

install:
	$(INSTALL) -o root -m 4755 -v machon /usr/local/bin/
	$(INSTALL) -o root -m 755 -v machonanal /usr/local/bin/
	$(INSTALL) -o root -m 4755 -v pinga /usr/local/bin/
	$(INSTALL) -o root -m 755 -v hole /usr/local/bin/

uninstall:
	$(RM) -f /usr/local/bin/machon
	$(RM) -f /usr/local/bin/machonanal
	$(RM) -f /usr/local/bin/pinga
	$(RM) -f /usr/local/bin/hole

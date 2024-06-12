/*
 * PINGA is a little code for users who need test hosts with little icmp packets
 * Copyright (C) 2006,2012  Víctor Clodoaldo Salas Pumacayo (aka nmag)
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

#ifndef _PINGA_H
#define _PINGA_H

struct icmpreply {
  pid_t id;
  u_int8_t type;
  u_int8_t code;
  float time;
};

/*
 * Structure of an icmp header.
 */
#ifdef CYGWIN32
struct icmp {
  u_char  icmp_type;		/* type of message, see below */
  u_char  icmp_code;		/* type sub code */
  u_short icmp_cksum;		/* ones complement cksum of struct */
  union {
    u_char ih_pptr;	       	/* ICMP_PARAMPROB */
    struct in_addr ih_gwaddr;	/* ICMP_REDIRECT */
    struct ih_idseq {
      n_short icd_id;
      n_short icd_seq;
    } ih_idseq;
    int ih_void;
    
    /* ICMP_UNREACH_NEEDFRAG -- Path MTU Discovery (RFC1191) */
    struct ih_pmtu {
      n_short ipm_void;    
      n_short ipm_nextmtu;
    } ih_pmtu;
  } icmp_hun;
#define	icmp_pptr	icmp_hun.ih_pptr
#define	icmp_gwaddr	icmp_hun.ih_gwaddr
#define	icmp_id		icmp_hun.ih_idseq.icd_id
#define	icmp_seq	icmp_hun.ih_idseq.icd_seq
#define	icmp_void	icmp_hun.ih_void
#define	icmp_pmvoid	icmp_hun.ih_pmtu.ipm_void
#define	icmp_nextmtu	icmp_hun.ih_pmtu.ipm_nextmtu
  union {
    struct id_ts {
      n_time its_otime;
      n_time its_rtime;
      n_time its_ttime;
    } id_ts;
    struct id_ip  {
      struct ip idi_ip;
      /* options and then 64 bits of data */
    } id_ip;
    u_long	id_mask;
    char	id_data[1];
  } icmp_dun;
#define	icmp_otime	icmp_dun.id_ts.its_otime
#define	icmp_rtime	icmp_dun.id_ts.its_rtime
#define	icmp_ttime	icmp_dun.id_ts.its_ttime
#define	icmp_ip		icmp_dun.id_ip.idi_ip
#define	icmp_mask	icmp_dun.id_mask
#define	icmp_data	icmp_dun.id_data
};
#endif

/*
 * Lower bounds on packet lengths for various types.
 * For the error advice packets must first insure that the
 * packet is large enought to contain the returned ip header.
 * Only then can we do the check to see if 64 bits of packet
 * data have been returned, since we need to check the returned
 * ip header length.
 */
#define	ICMP_MINLEN	8				/* abs minimum */
#define	ICMP_TSLEN	(8 + 3 * sizeof (n_time))	/* timestamp */
#define	ICMP_MASKLEN	12				/* address mask */
#define	ICMP_ADVLENMIN	(8 + sizeof (struct ip) + 8)	/* min */
#define	ICMP_ADVLEN(p)	(8 + ((p)->icmp_ip.ip_hl << 2) + 8)
/* N.B.: must separately check that ip_hl >= 5 */

/*
 * Definition of type and code field values.
 */
#define	ICMP_ECHOREPLY		0		/* echo reply */
#define	ICMP_UNREACH		3		/* dest unreachable, codes: */
#define		ICMP_UNREACH_NET	0		/* bad net */
#define		ICMP_UNREACH_HOST	1		/* bad host */
#define		ICMP_UNREACH_PROTOCOL	2		/* bad protocol */
#define		ICMP_UNREACH_PORT	3		/* bad port */
#define		ICMP_UNREACH_NEEDFRAG	4		/* IP_DF caused drop */
#define		ICMP_UNREACH_SRCFAIL	5		/* src route failed */
#define		ICMP_UNREACH_NET_UNKNOWN 6		/* unknown net */
#define		ICMP_UNREACH_HOST_UNKNOWN 7		/* unknown host */
#define		ICMP_UNREACH_ISOLATED	8		/* src host isolated */
#define		ICMP_UNREACH_NET_PROHIB	9		/* prohibited access */
#define		ICMP_UNREACH_HOST_PROHIB 10		/* ditto */
#define		ICMP_UNREACH_TOSNET	11		/* bad tos for net */
#define		ICMP_UNREACH_TOSHOST	12		/* bad tos for host */
#define	ICMP_SOURCEQUENCH	4		/* packet lost, slow down */
#define	ICMP_REDIRECT		5		/* shorter route, codes: */
#define		ICMP_REDIRECT_NET	0		/* for network */
#define		ICMP_REDIRECT_HOST	1		/* for host */
#define		ICMP_REDIRECT_TOSNET	2		/* for tos and net */
#define		ICMP_REDIRECT_TOSHOST	3		/* for tos and host */
#define	ICMP_ECHO		8		/* echo service */
#define	ICMP_ROUTERADVERT	9		/* router advertisement */
#define	ICMP_ROUTERSOLICIT	10		/* router solicitation */
#define	ICMP_TIMXCEED		11		/* time exceeded, code: */
#define		ICMP_TIMXCEED_INTRANS	0		/* ttl==0 in transit */
#define		ICMP_TIMXCEED_REASS	1		/* ttl==0 in reass */
#define	ICMP_PARAMPROB		12		/* ip header bad */
#define		ICMP_PARAMPROB_OPTABSENT 1		/* req. opt. absent */
#define	ICMP_TSTAMP		13		/* timestamp request */
#define	ICMP_TSTAMPREPLY	14		/* timestamp reply */
#define	ICMP_IREQ		15		/* information request */
#define	ICMP_IREQREPLY		16		/* information reply */
#define	ICMP_MASKREQ		17		/* address mask request */
#define	ICMP_MASKREPLY		18		/* address mask reply */

#define	ICMP_MAXTYPE		18

#define	ICMP_INFOTYPE(type)                                            \
  ((type) == ICMP_ECHOREPLY || (type) == ICMP_ECHO ||                  \
   (type) == ICMP_ROUTERADVERT || (type) == ICMP_ROUTERSOLICIT ||      \
   (type) == ICMP_TSTAMP || (type) == ICMP_TSTAMPREPLY ||              \
   (type) == ICMP_IREQ || (type) == ICMP_IREQREPLY ||                  \
   (type) == ICMP_MASKREQ || (type) == ICMP_MASKREPLY)

#ifdef KERNEL
void	icmp_error __P((struct mbuf *, int, int, n_long, struct ifnet *));
void	icmp_input __P((struct mbuf *, int));
void	icmp_reflect __P((struct mbuf *));
void	icmp_send __P((struct mbuf *, struct mbuf *));
int	icmp_sysctl __P((int *, u_int, void *, size_t *, void *, size_t));
#endif

/* icmphdr */
#ifdef CYGWIN32
struct icmphdr {
  u_char  type;
  u_char  code;
  u_short checksum;
  union	{
    struct {
      u_short id;
      u_short sequence;
    } echo;
    u_int gateway;
    struct {
      u_short unused;
      u_short mtu;
    } frag;
  } un;
};
#endif

/* iphdr */
#ifdef CYGWIN32
struct iphdr {
#if defined(__LITTLE_ENDIAN_BITFIELD)
  u_char ihl:4,
    version:4;
#elif defined (__BIG_ENDIAN_BITFIELD)
  u_char version:4,
    ihl:4;
#else
#error "Please fix <asm/byteorder.h>"
#endif
  u_char  tos;
  u_short tot_len;
  u_short id;
  u_short frag_off;
  u_char  ttl;
  u_char  protocol;
  u_short check;
  u_int   saddr;
  u_int   daddr;
};
#endif

/* Macros from <linux/types.h> */
#define	ICMP_DEST_UNREACH	3
#define	ICMP_NET_UNREACH	0
#define	ICMP_HOST_UNREACH	1
#define	ICMP_PROT_UNREACH	2
#define	ICMP_PORT_UNREACH	3
#define	ICMP_FRAG_NEEDED	4
#define	ICMP_SR_FAILED		5
#define	ICMP_NET_UNKNOWN	6
#define	ICMP_HOST_UNKNOWN	7
#define	ICMP_HOST_ISOLATED	8
#define	ICMP_NET_ANO		9
#define	ICMP_HOST_ANO		10
#define	ICMP_NET_UNR_TOS	11
#define	ICMP_HOST_UNR_TOS	12
#define	ICMP_PKT_FILTERED	13
#define	ICMP_PREC_VIOLATION	14
#define	ICMP_PREC_CUTOFF	15

#define	ICMP_SOURCE_QUENCH	4
#define ICMP_TIME_EXCEEDED	11
#define	ICMP_PARAMETERPROB	12
#define	ICMP_TIMESTAMP		13
#define	ICMP_TIMESTAMPREPLY	14
#define	ICMP_INFO_REQUEST	15
#define ICMP_INFO_REPLY		16
#define	ICMP_ADDRESS		17
#define	ICMP_ADDRESSREPLY	18

/* making public pinga function thats fuck you :) */
extern struct icmpreply *pinga (int, unsigned short, char *);

#endif /* !_PINGA_h */

/*
 * Copyright (c) 2001-2003 Swedish Institute of Computer Science.
 * All rights reserved.
 * Copyright (c) <2013-2015>, <Huawei Technologies Co., Ltd>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
 * SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
 *
 * This file is part of the lwIP TCP/IP stack.
 *
 * Author: Simon Goldschmidt
 *
 */

/**********************************************************************************
 * Notice of Export Control Law
 * ===============================================
 * Huawei LiteOS may be subject to applicable export control laws and regulations, which
 * might include those applicable to Huawei LiteOS of U.S. and the country in which you
 * are located.
 * Import, export and usage of Huawei LiteOS in any manner by you shall be in compliance
 * with such applicable export control laws and regulations.
 **********************************************************************************/
 
 
 
   /**
 *  @file lwipopts.h
 */
 

#ifndef __LWIPOPTS_H__
#define __LWIPOPTS_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define LWIP_LITEOS_COMPAT  1
#if LWIP_LITEOS_COMPAT
#include "sys/select.h"
#include "sys/socket.h"
#include "sys/ioctl.h"
#include "asm/atomic.h"
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <net/route.h>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arch-arm/include/checksum.h>
#endif

/**
 * LWIP_SO_SNDTIMEO==1: Enable send timeout for sockets/netconns and
 * SO_SNDTIMEO processing.
 */
#ifndef LWIP_SO_SNDTIMEO
#define LWIP_SO_SNDTIMEO                1
#endif

/**
 * LWIP_SO_RCVTIMEO==1: Enable receive timeout for sockets/netconns and
 * SO_RCVTIMEO processing.
 */
#ifndef LWIP_SO_RCVTIMEO
#define LWIP_SO_RCVTIMEO                1
#endif


#ifndef LWIP_SACK
#define LWIP_SACK           0
#endif

#ifndef LWIP_CONFIG_PARAM
#define LWIP_CONFIG_PARAM 1
#endif

#ifndef LWIP_SACK_DATA_SEG_PIGGYBACK
#define LWIP_SACK_DATA_SEG_PIGGYBACK       (LWIP_SACK & 1)
#endif

#ifndef LWIP_WND_SCALE
#define LWIP_WND_SCALE	0
#endif

#if LWIP_WND_SCALE

/*
 * TCP_RCV_SCALE - This holds the shift count value for Window scale
 */
#ifndef TCP_RCV_SCALE
#define TCP_RCV_SCALE   7
#endif

/*
 * TCP_WND_MIN  -If window scaling is enabled then this minimum receive window also
 * should be configured. So that if peer is not supporting window scaling
 * option, then this minimum receive window will be considered.
 * This value should not be greater than 0XFFFF and this value should not
 * be more than TCP_WND.
 */
#ifndef TCP_WND_MIN
#define TCP_WND_MIN        32768
#endif


#endif

/**
 * NO_SYS==1: Provides VERY minimal functionality. Otherwise,
 * use lwIP facilities.
 */
#define NO_SYS                  0

/* ---------- Memory options ---------- */
/* MEM_ALIGNMENT: should be set to the alignment of the CPU for which
   lwIP is compiled. 4 byte alignment -> define MEM_ALIGNMENT to 4, 2
   byte alignment -> define MEM_ALIGNMENT to 2. */
#define MEM_ALIGNMENT           4U

#if LWIP_LITEOS_COMPAT
/**
 * MEM_LIBC_MALLOC==1: Use malloc/free/realloc provided by your C-library
 * instead of the lwip internal allocator. Can save code size if you
 * already use it.
 */
#define MEMP_MEM_MALLOC         1
#define MEM_LIBC_MALLOC         1
#ifndef MEM_SIZE
#define MEM_SIZE                        (4*1024*1024)
#endif
#else
/* MEM_SIZE: the size of the heap memory. If the application will send
a lot of data that needs to be copied, this should be set high. */
#ifndef MEM_SIZE
#define MEM_SIZE               (512*1024)
#endif
#endif

#if MEM_LIBC_MALLOC
#ifndef MEM_PBUF_RAM_SIZE_LIMIT
#define MEM_PBUF_RAM_SIZE_LIMIT   1
#endif
#endif


/* MEMP_NUM_PBUF: the number of memp struct pbufs. If the application
   sends a lot of data out of ROM (or other static memory), this
   should be set high. */
#ifndef MEMP_NUM_PBUF
#define MEMP_NUM_PBUF           64
#endif

/**
  *  Attention: Be careful while configure large sockets number, i.e, greater than 128, for that
  *  LWIP_NUM_SOCKETS is bind with file system descriptons CONFIG_NFILE_DESCRIPTORS.
  *  If socket num is large, file descriptions for file system would be small. See vfs_config.h
  *  for detail.
  */
#ifndef LWIP_NUM_SOCKETS
#define LWIP_NUM_SOCKETS               10// 32
#endif


#ifndef MEMP_NUM_NETCONN
#define MEMP_NUM_NETCONN          10//must is equal or lesser than CONFIG_NSOCKET_DESCRIPTORS 32
#endif

/**
 * MEMP_NUM_NETBUF: the number of struct netbufs.
 * (only needed if you use the sequential API, like api_lib.c)
 */
#ifndef MEMP_NUM_NETBUF
/**
  * MEMP_NUM_ARP_QUEUE: the number of simulateously queued outgoing
  * packets (pbufs) that are waiting for an ARP request (to resolve
  * their destination address) to finish.
  * (requires the ARP_QUEUEING option)
  */
#define MEMP_NUM_NETBUF             200   // 3072
#endif


/* MEMP_NUM_UDP_PCB: the number of UDP protocol control blocks. One
   per active UDP "connection". */
#ifndef MEMP_NUM_UDP_PCB
#define MEMP_NUM_UDP_PCB        5 // 32
#endif

/* MEMP_NUM_TCP_PCB: the number of simulatenously active TCP
   connections. */
#ifndef MEMP_NUM_TCP_PCB
#define MEMP_NUM_TCP_PCB        10   //32
#endif

/* MEMP_NUM_TCP_PCB_LISTEN: the number of listening TCP
   connections. */
#ifndef MEMP_NUM_TCP_PCB_LISTEN
/**
  * MEMP_NUM_RAW_PCB: Number of raw connection PCBs
  * (requires the LWIP_RAW option)
  */
#define MEMP_NUM_TCP_PCB_LISTEN      5
#endif

/* MEMP_NUM_TCP_SEG: the number of simultaneously queued TCP
   segments. */
#ifndef MEMP_NUM_TCP_SEG
#define MEMP_NUM_TCP_SEG         1500//3584
#endif

/* ---------- Pbuf options ---------- */
/* PBUF_POOL_SIZE: the number of buffers in the pbuf pool. */
#ifndef PBUF_POOL_SIZE
#define PBUF_POOL_SIZE          64
#endif

/* PBUF_POOL_BUFSIZE: the size of each pbuf in the pbuf pool. */
#ifndef PBUF_POOL_BUFSIZE
#define PBUF_POOL_BUFSIZE       1550
#endif

/* ---------- TCP options ---------- */

#ifndef LWIP_TCP
#define LWIP_TCP                1
#endif

#ifndef TCP_TTL
#define TCP_TTL                 255
#endif

#ifndef LWIP_RAW
#define LWIP_RAW 1
#endif

#if LWIP_RAW
#ifndef PF_PKT_SUPPORT
#define PF_PKT_SUPPORT  0
#endif
#else
#define PF_PKT_SUPPORT 0
#endif

#if PF_PKT_SUPPORT
#ifndef LWIP_NETIF_PROMISC
#define LWIP_NETIF_PROMISC 1
#endif
#else
#define LWIP_NETIF_PROMISC 0
#endif

/* Controls if TCP should queue segments that arrive out of
   order. Define to 0 if your device is low on memory. */
#define TCP_QUEUE_OOSEQ         LWIP_TCP

/**
 * TCPIP_MBOX_SIZE: The mailbox size for the tcpip thread messages
 * The queue size value itself is platform-dependent, but is passed to
 * sys_mbox_new() when tcpip_init is called.
 */
#ifndef TCPIP_MBOX_SIZE
#if LWIP_NETIF_PROMISC
#define TCPIP_MBOX_SIZE         512
#else
#define TCPIP_MBOX_SIZE         320
#endif
#endif

/**
 * TCPIP_PRTY_MBOX_SIZE: The mailbox size for the tcpip priority thread messages
 * like driver stop and driver wake messages
 * The queue size value itself is platform-dependent, but is passed to
 * sys_mbox_new() when tcpip_init is called.
 */
#ifndef TCPIP_PRTY_MBOX_SIZE
#define TCPIP_PRTY_MBOX_SIZE        16
#endif

/**
 * DEFAULT_TCP_RECVMBOX_SIZE: The mailbox size for the incoming packets on a
 * NETCONN_TCP. The queue size value itself is platform-dependent, but is passed
 * to sys_mbox_new() when the recvmbox is created.
 */
#ifndef DEFAULT_TCP_RECVMBOX_SIZE
#define DEFAULT_TCP_RECVMBOX_SIZE       128
#endif

/**
 * DEFAULT_ACCEPTMBOX_SIZE: The mailbox size for the incoming connections.
 * The queue size value itself is platform-dependent, but is passed to
 * sys_mbox_new() when the acceptmbox is created.
 */
#ifndef DEFAULT_ACCEPTMBOX_SIZE
#define DEFAULT_ACCEPTMBOX_SIZE         32
#endif

#ifndef IP_FRAG_MAX_MTU
#define IP_FRAG_MAX_MTU         1500
#endif

/* As per RFC 791, "Every internet module must be able to forward a datagram of 68
 * octets without further fragmentation.  This is because an internet header
 * may be up to 60 octets, and the minimum fragment is 8 octets." */
#ifndef  IP_FRAG_MIN_MTU
#define IP_FRAG_MIN_MTU	        68
#endif

#ifndef IP_REASS_MAX_PBUFS
#define IP_REASS_MAX_PBUFS   (65535*3/(IP_FRAG_MAX_MTU-20-8)) /*较老的交换机mtu可能是536，最大分片个数相应放大3倍，便于大包不被丢弃*/
#endif

#ifndef MEMP_NUM_REASSDATA
#define MEMP_NUM_REASSDATA  5
#endif

#ifndef LWIP_IPV4_ROUTE_TABLE_SUPPORT
#define LWIP_IPV4_ROUTE_TABLE_SUPPORT   1
#endif

#ifndef LWIP_IPV4_NUM_ROUTE_ENTRIES
#define LWIP_IPV4_NUM_ROUTE_ENTRIES 5
#endif


/* TCP Maximum segment size. */
#ifndef TCP_MSS
#define TCP_MSS                 536//1448
#endif

/* TCP sender buffer space (bytes). */
#ifndef TCP_SND_BUF
#define TCP_SND_BUF           (20 * TCP_MSS) //65535
#endif

/* TCP sender buffer space (pbufs). This must be at least = 2 *
   TCP_SND_BUF/TCP_MSS for things to work. */
#define TCP_SND_QUEUELEN        (4 * TCP_SND_BUF)/TCP_MSS

/* TCP receive window. */
#ifndef TCP_WND
#define TCP_WND            65535
#endif


#ifndef LWIP_CONGCNTRL_INITIAL_CWND
#define LWIP_CONGCNTRL_INITIAL_CWND       20
#endif

#ifndef LWIP_CONGCNTRL_INITIAL_SSTHRESH
#define LWIP_CONGCNTRL_INITIAL_SSTHRESH       1500
#endif


/* ---------- ICMP options ---------- */
/**
 * LWIP_ICMP==1: Enable ICMP module inside the IP stack.
 */
#define LWIP_ICMP                       1

/* ---------- IGMP options ---------- */
#ifndef LWIP_IGMP
#define LWIP_IGMP                       0
#endif

/* ---------- DHCP options ---------- */
/**
 * LWIP_DHCP==1: Enable DHCP module.
 */
#define LWIP_DHCP               0

/* ---------- UDP options ---------- */
/**
 * LWIP_UDP==1: Turn on UDP.
 */
#define LWIP_UDP                1
#define UDP_TTL                 255
/**
 * DEFAULT_UDP_RECVMBOX_SIZE: The mailbox size for the incoming packets on a
 * NETCONN_UDP. The queue size value itself is platform-dependent, but is passed
 * to sys_mbox_new() when the recvmbox is created.
 */

#ifndef DEFAULT_UDP_RECVMBOX_SIZE
#define DEFAULT_UDP_RECVMBOX_SIZE       128
#endif

/* -----------RAW options -----------*/
/**
 * DEFAULT_RAW_RECVMBOX_SIZE: The mailbox size for the incoming packets on a
 * NETCONN_RAW. The queue size value itself is platform-dependent, but is passed
 * to sys_mbox_new() when the recvmbox is created.
 */
#define DEFAULT_RAW_RECVMBOX_SIZE       128

/* -----------LOOPIF options -----------*/
#ifndef LWIP_HAVE_LOOPIF
#define LWIP_HAVE_LOOPIF                0
#endif

/* -----------LOOPBACK options -----------*/
#ifndef LWIP_NETIF_LOOPBACK
#define LWIP_NETIF_LOOPBACK             0
#endif


/* ---------- Statistics options ---------- */
/**
 * LWIP_STATS==1: Enable statistics gathering in lwip_stats.
 */
#ifndef LWIP_STATS
#define LWIP_STATS                     0
#endif

/* ---------- TFTP options ---------- */
#ifndef LWIP_TFTP
#define LWIP_TFTP                      0
#endif

/* ---------- SNTP options ---------- */
#ifndef LWIP_SNTP
#define LWIP_SNTP                      0
#endif

/**
 * SO_REUSE==1: Enable SO_REUSEADDR option.
 */
#ifndef SO_REUSE
#define SO_REUSE                        1
#endif

#define LWIP_ETHERNET               1
#define LWIP_NETCONN                    1
#define LWIP_SOCKET                    1
#define IP_FORWARD                      1

#define LWIP_DHCPS_DISCOVER_BROADCAST   LWIP_LITEOS_COMPAT

#if LWIP_LITEOS_COMPAT
#define TCPIP_THREAD_STACKSIZE          0x6000
#define TCPIP_THREAD_PRIO               5
#ifdef LOSCFG_FS_VFS
#define LWIP_SOCKET_START_NUM CONFIG_NFILE_DESCRIPTORS
#endif
#define LWIP_ENABLE_LOS_SHELL_CMD     0
#define ERRNO
#define LWIP_PROVIDE_ERRNO 0
/* Currently pthread_cond_timedwait() in liteos doesn't support absolute time */
#define LWIP_USE_POSIX_COND_WAIT_WITH_ABS_TIMEOUT 0
#define LWIP_INET_ADDR_FUNC         1
#define LWIP_INET_NTOA_FUNC         1
#define LWIP_INET_ATON_FUNC         1
#define LWIP_CHKSUM_ALGORITHM       4
#define LWIP_CHKSUM_COPY_ALGORITHM  2
#define LWIP_PLATFORM_BYTESWAP      1
#define LWIP_CHECKSUM_ON_COPY       1
#else
#define ERRNO
#define LWIP_PROVIDE_ERRNO 1
#endif



/* Requesting application to configure a cryptographically strong random number generator to LWIP_RAND macro */
#ifndef LWIP_RAND
#define LWIP_RAND g_stlwIPSecSspCallbacks.pfRand
#endif

#ifndef LWIP_STATIC
#define LWIP_STATIC static
#endif

#ifndef LWIP_ALWAYS_SEND_HWTYPE_AS_ETHER_IN_ARP
#define LWIP_ALWAYS_SEND_HWTYPE_AS_ETHER_IN_ARP 1
#endif

#ifndef LWIP_ALWAYS_SEND_HWTYPE_AS_ETHER_IN_DHCP
#define LWIP_ALWAYS_SEND_HWTYPE_AS_ETHER_IN_DHCP    1
#endif

#ifndef USE_ONLY_PRIMARY_IFACE
#define USE_ONLY_PRIMARY_IFACE 0
#endif

#ifndef DRIVER_STATUS_CHECK
#define DRIVER_STATUS_CHECK    0
#endif

#if DRIVER_STATUS_CHECK
#ifndef DRIVER_WAKEUP_INTERVAL
#define DRIVER_WAKEUP_INTERVAL 120000
#endif
#endif

#ifndef PBUF_LINK_CHKSUM_LEN
#define PBUF_LINK_CHKSUM_LEN  0
#endif

#ifndef LWIP_SACK_PERF_OPT
#if LWIP_SACK
#define LWIP_SACK_PERF_OPT 1
#else
#define LWIP_SACK_PERF_OPT 0
#endif
#endif /* LWIP_SACK_PERF_OPT */

#ifndef LWIP_SACK_CWND_OPT
#define LWIP_SACK_CWND_OPT LWIP_SACK_PERF_OPT
#endif

#ifndef ENABLE_NETCONN_INTERFACE
#define ENABLE_NETCONN_INTERFACE 0
#endif


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __LWIPOPTS_H__ */




#ifndef __HIETH_MAC_H
#define __HIETH_MAC_H

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif /* __cplusplus */
#endif /* __cplusplus */

#define U_MAC_PORTSEL        0x0200
#define D_MAC_PORTSEL        0x2200
#define U_MAC_RO_STAT        0x0204
#define D_MAC_RO_STAT        0x2204
#define U_MAC_PORTSET        0x0208
#define D_MAC_PORTSET        0x2208
#define U_MAC_STAT_CHANGE    0x020C
#define D_MAC_STAT_CHANGE    0x220C
#define U_MAC_SET        0x0210
#define D_MAC_SET        0x2210
#define U_MAC_RX_IPGCTRL    0x0214
#define D_MAC_RX_IPGCTRL    0x2214
#define U_MAC_TX_IPGCTRL    0x0218
#define D_MAC_TX_IPGCTRL    0x2218

/* bits of UD_MAC_PORTSET and UD_MAC_RO_STAT */
#define BITS_MACSTAT    MK_BITS(0, 3)

/* bits of U_MAC_PORTSEL and D_MAC_PORTSEL */
#define BITS_NEGMODE    MK_BITS(0, 1)
#define BITS_MII_MODE    MK_BITS(1, 1)

/* bits of U_MAC_TX_IPGCTRL and D_MAC_TX_IPGCTRL */
#define BITS_PRE_CNT_LIMIT    MK_BITS(23, 3)
#define BITS_IPG    MK_BITS(16, 7)
#define BITS_FC_INTER    MK_BITS(0, 16)

#define HIETH_SPD_100M    (1<<2)
#define HIETH_LINKED    (1<<1)
#define HIETH_DUP_FULL    1

#define HIETH_MAX_RCV_LEN    1535
#define BITS_LEN_MAX    MK_BITS(0, 10)

unsigned int hieth_set_mac_leadcode_cnt_limit(struct hieth_netdev_local *ld, unsigned int cnt);
unsigned int hieth_set_mac_trans_interval_bits(struct hieth_netdev_local *ld, unsigned int nbits);
unsigned int hieth_set_mac_fc_interval(struct hieth_netdev_local *ld, unsigned int para);

unsigned int hieth_set_linkstat(struct hieth_netdev_local *ld, unsigned int mode);
unsigned int hieth_get_linkstat(struct hieth_netdev_local *ld);

#define HIETH_NEGMODE_CPUSET    1
#define HIETH_NEGMODE_AUTO    0

unsigned int hieth_set_negmode(struct hieth_netdev_local *ld, unsigned int mode);
unsigned int hieth_get_negmode(struct hieth_netdev_local *ld);

#define HIETH_MII_MODE        0
#define HIETH_RMII_MODE        1

unsigned int hieth_set_mii_mode(struct hieth_netdev_local *ld, unsigned int mode);
void hieth_set_rcv_len_max(struct hieth_netdev_local *ld, unsigned int cnt);

#ifdef __cplusplus
#if __cplusplus
}
#endif /* __cplusplus */
#endif /* __cplusplus */

#endif

/* vim: set ts=8 sw=8 tw=78: */

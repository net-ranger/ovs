#include <linux/version.h>

#if LINUX_VERSION_CODE < KERNEL_VERSION(3,18,0)

#include <net/udp.h>

/* Function to set UDP checksum for an IPv4 UDP packet. This is intended
 * for the simple case like when setting the checksum for a UDP tunnel.
 */
void rpl_udp_set_csum(bool nocheck, struct sk_buff *skb,
		      __be32 saddr, __be32 daddr, int len)
{
	struct udphdr *uh = udp_hdr(skb);

	if (nocheck)
		uh->check = 0;
	else if (skb_is_gso(skb))
		uh->check = ~udp_v4_check(len, saddr, daddr, 0);
	else if (skb_dst(skb) && skb_dst(skb)->dev &&
		 (skb_dst(skb)->dev->features & NETIF_F_V4_CSUM)) {

		BUG_ON(skb->ip_summed == CHECKSUM_PARTIAL);

		skb->ip_summed = CHECKSUM_PARTIAL;
		skb->csum_start = skb_transport_header(skb) - skb->head;
		skb->csum_offset = offsetof(struct udphdr, check);
		uh->check = ~udp_v4_check(len, saddr, daddr, 0);
	} else {
		int l4_offset = skb_transport_offset(skb);
		__wsum csum;

		BUG_ON(skb->ip_summed == CHECKSUM_PARTIAL);

		uh->check = 0;
		csum = skb_checksum(skb, l4_offset, len, 0);
		uh->check = udp_v4_check(len, saddr, daddr, csum);
		if (uh->check == 0)
			uh->check = CSUM_MANGLED_0;

		skb->ip_summed = CHECKSUM_UNNECESSARY;
	}
}
EXPORT_SYMBOL_GPL(rpl_udp_set_csum);

#endif /* Linux version < 3.16 */

#ifdef OVS_CHECK_UDP_TUNNEL_ZERO_CSUM
void rpl_udp6_csum_zero_error(struct sk_buff *skb)
{
	/* RFC 2460 section 8.1 says that we SHOULD log
	 * this error. Well, it is reasonable.
	 */
	net_dbg_ratelimited("IPv6: udp checksum is 0 for [%pI6c]:%u->[%pI6c]:%u\n",
			&ipv6_hdr(skb)->saddr, ntohs(udp_hdr(skb)->source),
			&ipv6_hdr(skb)->daddr, ntohs(udp_hdr(skb)->dest));
}
#endif

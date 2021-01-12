/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright(c) 2010-2014 Intel Corporation
 */

#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <stdint.h>
#include <unistd.h>
#include <inttypes.h>

#include <sys/queue.h>
#include <sys/stat.h>

#include <rte_common.h>
#include <rte_byteorder.h>
#include <rte_log.h>
#include <rte_debug.h>
#include <rte_cycles.h>
#include <rte_memory.h>
#include <rte_memcpy.h>
#include <rte_launch.h>
#include <rte_eal.h>
#include <rte_per_lcore.h>
#include <rte_lcore.h>
#include <rte_atomic.h>
#include <rte_branch_prediction.h>
#include <rte_mempool.h>
#include <rte_mbuf.h>
#include <rte_interrupts.h>
#include <rte_pci.h>
#include <rte_ether.h>
#include <rte_ethdev.h>
#include <rte_ip.h>
#include <rte_tcp.h>
#include <rte_udp.h>
#include <rte_string_fns.h>
#include <rte_flow.h>

#include <rte_hash.h>
#include <rte_fbk_hash.h>
#include <rte_jhash.h>
#include <rte_hash_crc.h>

#include "testpmd.h"
#include "rand_dist.h"
#include "../test-pmd/nanosleep.h"
#include "../test-pmd/alt_header.h"

struct req_header {
	uint64_t request_id;    // Request identifier
} __attribute__((__packed__)); // or use __rte_packed

struct timestamp_pair{
	struct timespec	tx_timestamp;
	struct timespec	rx_timestamp;
};
struct timestamp_pair ts_array[TS_ARRAY_SIZE];
uint64_t recv_req_index;

uint32_t tx_ip_src_addr = RTE_IPV4(172, 31, 32, 235);
uint32_t tx_ip_dst_addr = RTE_IPV4(172, 31, 34, 51);
//char* mac_src_addr = "06:97:39:b3:67:3f"; //172.31.32.235 06:97:39:b3:67:3f
//char* mac_dst_addr = "06:96:c2:b8:68:09"; //172.31.34.51  06:96:c2:b8:68:09 

static struct rte_ether_hdr eth_hdr;

#define IP_DEFTTL  64   /* from RFC 1340. */

static struct rte_ipv4_hdr pkt_ip_hdr; /**< IP header of transmitted packets. */
static struct rte_udp_hdr pkt_udp_hdr; /**< UDP header of tx packets. */
static struct alt_header pkt_req_hdr;

uint16_t tx_udp_src_port = 7000;
uint16_t tx_udp_dst_port = 7000;
uint16_t port_base = 7000;
uint16_t port_diff = 0;

static inline void
update_checksum(struct rte_ipv4_hdr *ipv4_hdr, struct rte_udp_hdr *udp_hdr){
        udp_hdr->dgram_cksum = 0;
        //udp_hdr->dgram_cksum = rte_ipv4_udptcp_cksum(ipv4_hdr, (void*)udp_hdr);
        ipv4_hdr->hdr_checksum = 0;
        ipv4_hdr->hdr_checksum = rte_ipv4_cksum(ipv4_hdr);
}

static inline void
print_ether_addr(const char *what, const struct rte_ether_addr *eth_addr){
	char buf[RTE_ETHER_ADDR_FMT_SIZE];
	rte_ether_format_addr(buf, RTE_ETHER_ADDR_FMT_SIZE, eth_addr);
	printf("%s%s\n", what, buf);
}

static inline void
print_ipaddr(const char* string, rte_be32_t ip_addr){
        uint32_t ipaddr = rte_be_to_cpu_32(ip_addr);
        uint8_t src_addr[4];
        src_addr[0] = (uint8_t) (ipaddr >> 24) & 0xff;
        src_addr[1] = (uint8_t) (ipaddr >> 16) & 0xff;
        src_addr[2] = (uint8_t) (ipaddr >> 8) & 0xff;
        src_addr[3] = (uint8_t) ipaddr & 0xff;
        printf("%s:%" PRIu8 ".%" PRIu8 ".%" PRIu8 ".%" PRIu8 "\n", string,
                        src_addr[0], src_addr[1], src_addr[2], src_addr[3]);
}

static void
copy_buf_to_pkt_segs(void* buf, unsigned len, struct rte_mbuf *pkt,
		     unsigned offset)
{
	struct rte_mbuf *seg;
	void *seg_buf;
	unsigned copy_len;

	seg = pkt;
	while (offset >= seg->data_len) {
		offset -= seg->data_len;
		seg = seg->next;
	}
	copy_len = seg->data_len - offset;
	seg_buf = rte_pktmbuf_mtod_offset(seg, char *, offset);
	while (len > copy_len) {
		rte_memcpy(seg_buf, buf, (size_t) copy_len);
		len -= copy_len;
		buf = ((char*) buf + copy_len);
		seg = seg->next;
		seg_buf = rte_pktmbuf_mtod(seg, char *);
		copy_len = seg->data_len;
	}
	rte_memcpy(seg_buf, buf, (size_t) len);
}

static inline void
copy_buf_to_pkt(void* buf, unsigned len, struct rte_mbuf *pkt, unsigned offset)
{
	if (offset + len <= pkt->data_len) {
		rte_memcpy(rte_pktmbuf_mtod_offset(pkt, char *, offset),
			buf, (size_t) len);
		return;
	}
	copy_buf_to_pkt_segs(buf, len, pkt, offset);
}

static void
setup_pkt_udp_ip_headers(struct rte_ipv4_hdr *ip_hdr,
			 struct rte_udp_hdr *udp_hdr,
			 uint16_t pkt_data_len)
{
	uint16_t *ptr16;
	uint32_t ip_cksum;
	uint16_t pkt_len;

	/*
	 * Initialize UDP header.
	 */	
	pkt_len = (uint16_t) (pkt_data_len + sizeof(struct rte_udp_hdr));
	printf("udp pkt_len:%" PRIu16 "\n", pkt_len);
	udp_hdr->src_port = rte_cpu_to_be_16(tx_udp_src_port);
	udp_hdr->dst_port = rte_cpu_to_be_16(tx_udp_dst_port);
	udp_hdr->dgram_len      = RTE_CPU_TO_BE_16(pkt_len);
	udp_hdr->dgram_cksum    = 0; /* No UDP checksum. */

	/*
	 * Initialize IP header.
	 */	
	pkt_len = (uint16_t) (pkt_len + sizeof(struct rte_ipv4_hdr));
	printf("ip pkt_len:%" PRIu16 "\n", pkt_len);
	ip_hdr->version_ihl   = RTE_IPV4_VHL_DEF;
	ip_hdr->type_of_service   = 0;
	ip_hdr->fragment_offset = 0;
	ip_hdr->time_to_live   = IP_DEFTTL;
	ip_hdr->next_proto_id = IPPROTO_UDP;
	ip_hdr->packet_id = 0;
	ip_hdr->total_length   = RTE_CPU_TO_BE_16(pkt_len);
	ip_hdr->src_addr = rte_cpu_to_be_32(tx_ip_src_addr);
	ip_hdr->dst_addr = rte_cpu_to_be_32(tx_ip_dst_addr);

	/*
	 * Compute IP header checksum.
	 */
	ptr16 = (unaligned_uint16_t*) ip_hdr;
	ip_cksum = 0;
	ip_cksum += ptr16[0]; ip_cksum += ptr16[1];
	ip_cksum += ptr16[2]; ip_cksum += ptr16[3];
	ip_cksum += ptr16[4];
	ip_cksum += ptr16[6]; ip_cksum += ptr16[7];
	ip_cksum += ptr16[8]; ip_cksum += ptr16[9];

	/*
	 * Reduce 32 bit checksum to 16 bits and complement it.
	 */
	ip_cksum = ((ip_cksum & 0xFFFF0000) >> 16) +
		(ip_cksum & 0x0000FFFF);
	if (ip_cksum > 65535)
		ip_cksum -= 65535;
	ip_cksum = (~ip_cksum) & 0x0000FFFF;
	if (ip_cksum == 0)
		ip_cksum = 0xFFFF;
	ip_hdr->hdr_checksum = (uint16_t) ip_cksum;
}

static inline bool
pkt_burst_prepare(struct rte_mbuf *pkt, struct rte_mempool *mbp,
		struct rte_ether_hdr *eth_hdr, const uint16_t vlan_tci,
		const uint16_t vlan_tci_outer, const uint64_t ol_flags,
		const uint16_t idx, const struct fwd_stream *fs)
{
	struct rte_mbuf *pkt_segs[RTE_MAX_SEGS_PER_PKT];
	struct rte_mbuf *pkt_seg;
	uint32_t nb_segs, pkt_len;
	uint8_t i;

	if (unlikely(tx_pkt_split == TX_PKT_SPLIT_RND))
		nb_segs = rte_rand() % tx_pkt_nb_segs + 1;
	else
		nb_segs = tx_pkt_nb_segs;

	if (nb_segs > 1) {
		if (rte_mempool_get_bulk(mbp, (void **)pkt_segs, nb_segs - 1))
			return false;
	}

	rte_pktmbuf_reset_headroom(pkt);
	pkt->data_len = tx_pkt_seg_lengths[0];
	pkt->ol_flags &= EXT_ATTACHED_MBUF;
	pkt->ol_flags |= ol_flags;
	pkt->vlan_tci = vlan_tci;
	pkt->vlan_tci_outer = vlan_tci_outer;
	pkt->l2_len = sizeof(struct rte_ether_hdr);
	pkt->l3_len = sizeof(struct rte_ipv4_hdr);

	pkt_len = pkt->data_len;
	pkt_seg = pkt;
	for (i = 1; i < nb_segs; i++) {
		pkt_seg->next = pkt_segs[i - 1];
		pkt_seg = pkt_seg->next;
		pkt_seg->data_len = tx_pkt_seg_lengths[i];
		pkt_len += pkt_seg->data_len;
	}
	pkt_seg->next = NULL; /* Last segment of packet. */

	//ip layer setup
	pkt_ip_hdr.src_addr = fs->client_alt_header.actual_src_ip; // client_self_ip
	//pkt_ip_hdr.dst_addr = fs->client_alt_header.alt_dst_ip;

	int ret;
	// lookup routing table for ToR's ip as dest ip
	void* lookup_result;
	uint32_t* nexthop_ptr;
	uint32_t dest_addr = fs->client_alt_header.alt_dst_ip;
	ret = rte_hash_lookup_data(fs->routing_table, (void*) &dest_addr, &lookup_result);
	if(ret >= 0){
		nexthop_ptr = (uint32_t*) lookup_result;
		pkt_ip_hdr.dst_addr = *nexthop_ptr;
		//print_ipaddr("ToR ip addr", pkt_ip_hdr.dst_addr);
	}
	else{
		if (errno == ENOENT){
			print_ipaddr("dst ip addr", dest_addr);
			printf("value not found\n");
		}
		else if(errno == EINVAL){
			printf("invalid parameters\n");
		}
		else{
			printf("unknown!\n");
		}
	}

	pkt_ip_hdr.hdr_checksum = 0;
    pkt_ip_hdr.hdr_checksum = rte_ipv4_cksum(&pkt_ip_hdr);

	//MAC layer setup
	//set src mac addr
	void* mac_lookup_result;
	ret = rte_hash_lookup_data(fs->ip2mac_table, (void*) &pkt_ip_hdr.src_addr, &mac_lookup_result);
	if(ret >= 0){
		struct rte_ether_addr* lookup1 = (struct rte_ether_addr*)(uintptr_t) mac_lookup_result;
		//print_ipaddr("src_ipaddr", pkt_ip_hdr.src_addr);
		//print_ether_addr("eth_addr lookup", lookup1);
		rte_ether_addr_copy(lookup1, &eth_hdr->s_addr); // ether_header->s_addr = lookup1
	}
	else{
		if (errno == ENOENT){
				print_ipaddr("src_ipaddr", pkt_ip_hdr.src_addr);
				printf("value not found\n");
		}
		else if(errno == EINVAL){
				printf("invalid parameters\n");
		}
		else{
				printf("unknown!\n");
		}
	}
	
	//set dst mac addr
	ret = rte_hash_lookup_data(fs->ip2mac_table, (void*) &pkt_ip_hdr.dst_addr, &mac_lookup_result);
	if(ret >= 0){
		struct rte_ether_addr* lookup1 = (struct rte_ether_addr*)(uintptr_t) mac_lookup_result;
		//print_ipaddr("dst_ipaddr", pkt_ip_hdr.dst_addr);
		//print_ether_addr("eth_addr lookup", lookup1);
		rte_ether_addr_copy(lookup1, &eth_hdr->d_addr); // ether_header->d_addr = lookup1
	}
	else{
		if (errno == ENOENT){
				print_ipaddr("dst_ipaddr", pkt_ip_hdr.dst_addr);
				printf("value not found\n");
		}
		else if(errno == EINVAL){
				printf("invalid parameters\n");
		}
		else{
				printf("unknown!\n");
		}
	}	

	//update_checksum(&pkt_ip_hdr, &pkt_udp_hdr);
	
	//UDP port increment for each requesets
	pkt_udp_hdr.src_port = rte_cpu_to_be_16(port_base + port_diff);
	pkt_udp_hdr.dst_port = rte_cpu_to_be_16(port_base + port_diff);
	port_diff = (port_diff + 1)%MAX_SINGLE_THREAD_CONNECTIONS;

	//update_checksum(&pkt_ip_hdr, &pkt_udp_hdr);

	//alt_header setup
	pkt_req_hdr.msgtype_flags = SINGLE_PKT_REQ_PASSTHROUGH;
	pkt_req_hdr.redirection = 0;
	pkt_req_hdr.header_size = sizeof(struct alt_header);

	pkt_req_hdr.feedback_options = 0;
	pkt_req_hdr.service_id = fs->client_alt_header.service_id;
	pkt_req_hdr.request_id = (pkt_req_hdr.request_id + 1)%TS_ARRAY_SIZE;

	pkt_req_hdr.alt_dst_ip = fs->client_alt_header.alt_dst_ip;	
	pkt_req_hdr.actual_src_ip = fs->client_alt_header.actual_src_ip;
	for(int i = 0; i < NUM_REPLICA; i++){
		pkt_req_hdr.replica_dst_list[i] = fs->client_alt_header.replica_dst_list[i];
	}

	//update_checksum(&pkt_ip_hdr, &pkt_udp_hdr);
	// pkt_udp_hdr.dgram_cksum = 0;
    // pkt_udp_hdr.dgram_cksum = rte_ipv4_udptcp_cksum(&pkt_ip_hdr, (void*)&pkt_udp_hdr);
	// printf("csum: %x\n", rte_cpu_to_be_16(pkt_udp_hdr.dgram_cksum));
    //pkt_ip_hdr.hdr_checksum = 0;
    //pkt_ip_hdr.hdr_checksum = rte_ipv4_cksum(&pkt_ip_hdr);


	/*
	 * Copy headers in first packet segment(s).
	 */
	copy_buf_to_pkt(eth_hdr, sizeof(*eth_hdr), pkt, 0);
	copy_buf_to_pkt(&pkt_ip_hdr, sizeof(pkt_ip_hdr), pkt,
			sizeof(struct rte_ether_hdr));

	copy_buf_to_pkt(&pkt_udp_hdr, sizeof(struct rte_udp_hdr), pkt,
			sizeof(struct rte_ether_hdr) +
			sizeof(struct rte_ipv4_hdr));
			
	copy_buf_to_pkt(&pkt_req_hdr, sizeof(struct alt_header), pkt,
			sizeof(struct rte_ether_hdr) +
			sizeof(struct rte_ipv4_hdr)  +
			sizeof(struct rte_udp_hdr));	

	/*
	 * Complete first mbuf of packet and append it to the
	 * burst of packets to be transmitted.
	 */
	pkt->nb_segs = nb_segs;
	pkt->pkt_len = pkt_len;

	return true;
}

/*
 * Transmit a burst of multi-segments packets.
 */
static void
pkt_burst_transmit(struct fwd_stream *fs)
{
	nb_pkt_per_burst = 1;
	struct timespec ts1, ts2, ts3, sleep_ts1, sleep_ts2;
	struct rte_mbuf *recv_burst[MAX_PKT_BURST];
	struct rte_mbuf *pkts_burst[MAX_PKT_BURST];
	struct rte_port *txp;
	struct rte_mbuf *pkt;
	struct rte_mempool *mbp;
	//struct rte_ether_hdr eth_hdr;
	uint16_t nb_tx;
	uint16_t nb_pkt;
	uint16_t vlan_tci, vlan_tci_outer;
	uint32_t retry;
	uint64_t ol_flags = 0;
	uint64_t tx_offloads;

	mbp = current_fwd_lcore()->mbp;
	txp = &ports[fs->tx_port];
	tx_offloads = txp->dev_conf.txmode.offloads;
	vlan_tci = txp->tx_vlan_id;
	vlan_tci_outer = txp->tx_vlan_id_outer;
	if (tx_offloads	& DEV_TX_OFFLOAD_VLAN_INSERT)
		ol_flags = PKT_TX_VLAN_PKT;
	if (tx_offloads & DEV_TX_OFFLOAD_QINQ_INSERT)
		ol_flags |= PKT_TX_QINQ_PKT;
	if (tx_offloads & DEV_TX_OFFLOAD_MACSEC_INSERT)
		ol_flags |= PKT_TX_MACSEC;


	for (nb_pkt = 0; nb_pkt < nb_pkt_per_burst; nb_pkt++) {		
		pkt = rte_mbuf_raw_alloc(mbp);
		if (pkt == NULL)
			break;
		if (unlikely(!pkt_burst_prepare(pkt, mbp, &eth_hdr,
						vlan_tci,
						vlan_tci_outer,
						ol_flags,
						nb_pkt, fs))) {
			rte_pktmbuf_free(pkt);
			break;
		}
		pkts_burst[nb_pkt] = pkt;
	}

	if (nb_pkt == 0)
		return;
	
	clock_gettime(CLOCK_REALTIME, &ts1);
	ts_array[pkt_req_hdr.request_id].tx_timestamp = ts1;
	nb_tx = rte_eth_tx_burst(fs->tx_port, fs->tx_queue, pkts_burst, 1);

	/*
	 * Retry if necessary
	 */
	if (unlikely(nb_tx < nb_pkt) && fs->retry_enabled) {
		retry = 0;
		while (nb_tx < nb_pkt && retry++ < burst_tx_retry_num) {
			rte_delay_us(burst_tx_delay_time);
			nb_tx += rte_eth_tx_burst(fs->tx_port, fs->tx_queue,
					&pkts_burst[nb_tx], nb_pkt - nb_tx);
		}
	}
	fs->tx_packets += nb_tx;

	if (unlikely(nb_tx < nb_pkt)) {
		if (verbose_level > 0 && fs->fwd_dropped == 0)
			printf("port %d tx_queue %d - drop "
			       "(nb_pkt:%u - nb_tx:%u)=%u packets\n",
			       fs->tx_port, fs->tx_queue,
			       (unsigned) nb_pkt, (unsigned) nb_tx,
			       (unsigned) (nb_pkt - nb_tx));
		fs->fwd_dropped += (nb_pkt - nb_tx);
		do {
			rte_pktmbuf_free(pkts_burst[nb_tx]);
		} while (++nb_tx < nb_pkt);
	}

	// 1 (X) 2 (X) 3 (X) 4 (V) for vector sse -> mlx5_rx_burst_vec
	// 1 (V) for scalar -> mlx5_rx_burst
	int nb_rx = rte_eth_rx_burst(fs->rx_port, fs->rx_queue, recv_burst, 4); 
	/*if(nb_rx > 0){
		printf("rte_eth_rx_burst rx %" PRIu16 " pkt\n", nb_rx);
	}*/	
	clock_gettime(CLOCK_REALTIME, &ts2);

	fs->rx_packets += nb_rx;
	for (int i = 0; i < nb_rx; i++){
		struct alt_header* recv_req_ptr = rte_pktmbuf_mtod_offset(recv_burst[i], struct alt_header *, 
			sizeof(struct rte_ether_hdr) + sizeof(struct rte_ipv4_hdr) + sizeof(struct rte_udp_hdr));
		uint32_t req_id = recv_req_ptr->request_id;	
		req_id = req_id%TS_ARRAY_SIZE;
		ts_array[req_id].rx_timestamp = ts2;		
		uint64_t latency = clock_gettime_diff_ns(&ts_array[req_id].rx_timestamp, &ts_array[req_id].tx_timestamp);		
		printf("req id:%" PRIu32 ", latency:%" PRIu64 "\n", req_id, latency);	
		// if(ts1.tv_sec == ts2.tv_sec){
		// 	//fprintf(fp, "%" PRIu64 "\n", ts2.tv_nsec - ts1.tv_nsec);
		// 	printf("%" PRIu64 "\n", ts_array[req_id].rx_timestamp.tv_nsec - ts_array[req_id].tx_timestamp.tv_nsec);
		// }
		// else{
		// 	uint64_t ts1_nsec = ts_array[req_id].tx_timestamp.tv_nsec + 1000000000*ts_array[req_id].tx_timestamp.tv_sec;
		// 	uint64_t ts2_nsec = ts_array[req_id].rx_timestamp.tv_nsec + 1000000000*ts_array[req_id].rx_timestamp.tv_sec;
		// 	//fprintf(fp, "%" PRIu64 "\n", ts2_nsec - ts1_nsec);
		// 	printf("%" PRIu64 "\n", ts2_nsec - ts1_nsec);
		// 	//printf("queue_id %" PRIu16 ":%" PRIu64 "\n", fs->rx_queue, ts2_nsec - ts1_nsec);
		// }

		rte_pktmbuf_free(recv_burst[i]);
	}
	
	clock_gettime(CLOCK_REALTIME, &ts3);
	sleep_ts1=ts3;
	//realnanosleep(500*1000*1000, &sleep_ts1, &sleep_ts2); // 500 ms
	realnanosleep(5*1000, &sleep_ts1, &sleep_ts2); // 5 us
}

static void
tx_only_begin(portid_t pi)
{
	uint16_t pkt_data_len;
	int dynf;

	pkt_data_len = (uint16_t) (tx_pkt_length - (
					sizeof(struct rte_ether_hdr) +
					sizeof(struct rte_ipv4_hdr) +
					sizeof(struct rte_udp_hdr)));
	setup_pkt_udp_ip_headers(&pkt_ip_hdr, &pkt_udp_hdr, pkt_data_len);

	/*
	 * Initialize Ethernet header.
	 */
	// rte_ether_unformat_addr(mac_src_addr, &eth_hdr.s_addr);
	// print_ether_addr("ETH_SRC_ADDR:", &eth_hdr.s_addr);
	// rte_ether_unformat_addr(mac_dst_addr, &eth_hdr.d_addr);
	// print_ether_addr("ETH_DST_ADDR:", &eth_hdr.d_addr);

	//set up ether src and dst address
	eth_hdr.ether_type = rte_cpu_to_be_16(RTE_ETHER_TYPE_IPV4);

	// printf("pkt_data_len:%" PRIu16 "\n", pkt_data_len);
	// pkt_req_hdr.request_id = 0;
	// //for(uint16_t host_index = 0; host_index < NUM_REPLICA; host_index++){
	// 	pkt_req_hdr.replica_dst_list[host_index];
	// //}
	
}

struct fwd_engine tx_only_engine = {
	.fwd_mode_name  = "txonly",
	.port_fwd_begin = tx_only_begin,
	.port_fwd_end   = NULL,
	.packet_fwd     = pkt_burst_transmit,
};

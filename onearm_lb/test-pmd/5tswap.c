/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright 2014-2020 Mellanox Technologies, Ltd
 */

#include <stdarg.h>
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <inttypes.h>

#include <sys/queue.h>
#include <sys/stat.h>

#include <rte_common.h>
#include <rte_ether.h>
#include <rte_ethdev.h>
#include <rte_ip.h>
#include <rte_flow.h>

#include "macswap_common.h"
#include "testpmd.h"

//ST: for packet/request redirection
#define REDIRECT_ENABLED 1
#define THRESHOLD_REDIRECT 1
//#define REDIRECT_DEBUG_PRINT 1
//[load++,load--] tags mark the code sections where we use switch-based counters to record load

#include "nanosleep.h"
#include "alt_header.h"
#include <rte_hash.h>
#include <rte_fbk_hash.h>
#include <rte_jhash.h>
#include <rte_hash_crc.h>

uint32_t empty_addr = RTE_IPV4(0, 0, 0, 0);
int udp_counter = 0;
struct timespec start_ts[] = {
	[0] = {.tv_nsec = 0, .tv_sec = 0},
	[1] = {.tv_nsec = 0, .tv_sec = 0},
};
struct timespec end_ts[] = {
	[0] = {.tv_nsec = 0, .tv_sec = 0},
	[1] = {.tv_nsec = 0, .tv_sec = 0},
};
struct timespec ts1 ={
	.tv_nsec = 0,
	.tv_sec = 0,
};

struct timespec redir_ts ={
	.tv_nsec = 0,
	.tv_sec = 0,
};
//#ifdef REDIRECT_DEBUG_PRINT
static inline void
print_macaddr(const char* string, struct rte_ether_addr *macaddr){
	printf("%s:%02" PRIx8 " %02" PRIx8 " %02" PRIx8
	" %02" PRIx8 " %02" PRIx8 " %02" PRIx8 "\n", string,
	macaddr->addr_bytes[0], macaddr->addr_bytes[1],
	macaddr->addr_bytes[2], macaddr->addr_bytes[3],
	macaddr->addr_bytes[4], macaddr->addr_bytes[5]);
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

static inline void
print_ip2load(rte_be32_t ip_addr, uint64_t load){
	uint32_t ipaddr = rte_be_to_cpu_32(ip_addr);
	uint8_t src_addr[4];
	src_addr[0] = (uint8_t) (ipaddr >> 24) & 0xff;
	src_addr[1] = (uint8_t) (ipaddr >> 16) & 0xff;
	src_addr[2] = (uint8_t) (ipaddr >> 8) & 0xff;
	src_addr[3] = (uint8_t) ipaddr & 0xff;
	printf("%" PRIu8 ".%" PRIu8 ".%" PRIu8 ".%" PRIu8 "->",
			src_addr[0], src_addr[1], src_addr[2], src_addr[3]);
	printf("%" PRIu64 "\n", load);
}
//#endif

static inline void
update_checksum(struct rte_ipv4_hdr *ipv4_hdr, struct rte_udp_hdr *udp_hdr){
	udp_hdr->dgram_cksum = 0;
	udp_hdr->dgram_cksum = rte_ipv4_udptcp_cksum(ipv4_hdr, (void*)udp_hdr);
	ipv4_hdr->hdr_checksum = 0;
	ipv4_hdr->hdr_checksum = rte_ipv4_cksum(ipv4_hdr);	
}

static inline int
min_load(uint64_t x, uint64_t y, uint64_t z){
  return x < y ? (x < z ? 1 : 3) : (y < z ? 2 : 3);
}

static inline void
swap_mac(struct rte_ether_hdr *eth_hdr)
{
	struct rte_ether_addr addr;

	/* Swap dest and src mac addresses. */
	rte_ether_addr_copy(&eth_hdr->d_addr, &addr);
	rte_ether_addr_copy(&eth_hdr->s_addr, &eth_hdr->d_addr);
	rte_ether_addr_copy(&addr, &eth_hdr->s_addr);
}

static inline void
swap_ipv4(struct rte_ipv4_hdr *ipv4_hdr)
{
	rte_be32_t addr;

	/* Swap dest and src ipv4 addresses. */
	addr = ipv4_hdr->src_addr;
	ipv4_hdr->src_addr = ipv4_hdr->dst_addr;
	ipv4_hdr->dst_addr = addr;
}

static inline void
swap_ipv6(struct rte_ipv6_hdr *ipv6_hdr)
{
	uint8_t addr[16];

	/* Swap dest and src ipv6 addresses. */
	memcpy(&addr, &ipv6_hdr->src_addr, 16);
	memcpy(&ipv6_hdr->src_addr, &ipv6_hdr->dst_addr, 16);
	memcpy(&ipv6_hdr->dst_addr, &addr, 16);
}

static inline void
swap_tcp(struct rte_tcp_hdr *tcp_hdr)
{
	rte_be16_t port;

	/* Swap dest and src tcp port. */
	port = tcp_hdr->src_port;
	tcp_hdr->src_port = tcp_hdr->dst_port;
	tcp_hdr->dst_port = port;
}

static inline void
swap_udp(struct rte_udp_hdr *udp_hdr)
{
	rte_be16_t port;

	/* Swap dest and src udp port */
	port = udp_hdr->src_port;
	udp_hdr->src_port = udp_hdr->dst_port;
	udp_hdr->dst_port = port;
}

/*
 * 5 tuple swap forwarding mode: Swap the source and the destination of layers
 * 2,3,4. Swaps source and destination for MAC, IPv4/IPv6, UDP/TCP.
 * Parses each layer and swaps it. When the next layer doesn't match it stops.
 */
static void
pkt_burst_5tuple_swap(struct fwd_stream *fs)
{
	struct rte_mbuf  *pkts_burst[MAX_PKT_BURST];
	struct rte_port  *txp;
	struct rte_mbuf *mb;
	uint16_t next_proto;
	uint64_t ol_flags;
	uint16_t proto;
	uint16_t nb_rx;
	uint16_t nb_tx;
	uint32_t retry;

	int i;
	union {
		struct rte_ether_hdr *eth;
		struct rte_vlan_hdr *vlan;
		struct rte_ipv4_hdr *ipv4;
		struct rte_ipv6_hdr *ipv6;
		struct rte_tcp_hdr *tcp;
		struct rte_udp_hdr *udp;
		uint8_t *byte;
	} h;

	/*
	 * Receive a burst of packets and forward them.
	 */
	nb_rx = rte_eth_rx_burst(fs->rx_port, fs->rx_queue, pkts_burst, 4);
	if (unlikely(nb_rx == 0))
		return;

	fs->rx_packets += nb_rx;
	txp = &ports[fs->tx_port];
	ol_flags = ol_flags_init(txp->dev_conf.txmode.offloads);
	vlan_qinq_set(pkts_burst, nb_rx, ol_flags,
			txp->tx_vlan_id, txp->tx_vlan_id_outer);
	for (i = 0; i < nb_rx; i++) {
		if (likely(i < nb_rx - 1))
			rte_prefetch0(rte_pktmbuf_mtod(pkts_burst[i+1],
					void *));
		mb = pkts_burst[i];
		h.eth = rte_pktmbuf_mtod(mb, struct rte_ether_hdr *);
		proto = h.eth->ether_type;
		swap_mac(h.eth);
		mb->l2_len = sizeof(struct rte_ether_hdr);
		h.eth++;
		while (proto == RTE_BE16(RTE_ETHER_TYPE_VLAN) ||
		       proto == RTE_BE16(RTE_ETHER_TYPE_QINQ)) {
			proto = h.vlan->eth_proto;
			h.vlan++;
			mb->l2_len += sizeof(struct rte_vlan_hdr);
		}
		if (proto == RTE_BE16(RTE_ETHER_TYPE_IPV4)) {
			swap_ipv4(h.ipv4);
			next_proto = h.ipv4->next_proto_id;
			mb->l3_len = (h.ipv4->version_ihl & 0x0f) * 4;
			h.byte += mb->l3_len;
		} else if (proto == RTE_BE16(RTE_ETHER_TYPE_IPV6)) {
			swap_ipv6(h.ipv6);
			next_proto = h.ipv6->proto;
			h.ipv6++;
			mb->l3_len = sizeof(struct rte_ipv6_hdr);
		} else {
			mbuf_field_set(mb, ol_flags);
			continue;
		}

		if (next_proto == IPPROTO_UDP) {
			swap_udp(h.udp);
			//printf("udp\n");
			mb->l4_len = sizeof(struct rte_udp_hdr);
		} else if (next_proto == IPPROTO_TCP) {
			swap_tcp(h.tcp);
			mb->l4_len = (h.tcp->data_off & 0xf0) >> 2;
		}
		mbuf_field_set(mb, ol_flags);
	}
	nb_tx = rte_eth_tx_burst(fs->tx_port, fs->tx_queue, pkts_burst, nb_rx);
	/*
	 * Retry if necessary
	 */
	if (unlikely(nb_tx < nb_rx) && fs->retry_enabled) {
		retry = 0;
		while (nb_tx < nb_rx && retry++ < burst_tx_retry_num) {
			rte_delay_us(burst_tx_delay_time);
			nb_tx += rte_eth_tx_burst(fs->tx_port, fs->tx_queue,
					&pkts_burst[nb_tx], nb_rx - nb_tx);
		}
	}
	fs->tx_packets += nb_tx;
	if (unlikely(nb_tx < nb_rx)) {
		fs->fwd_dropped += (nb_rx - nb_tx);
		do {
			rte_pktmbuf_free(pkts_burst[nb_tx]);
		} while (++nb_tx < nb_rx);
	}
}

struct fwd_engine five_tuple_swap_fwd_engine = {
	.fwd_mode_name  = "5tswap",
	.port_fwd_begin = NULL,
	.port_fwd_end   = NULL,
	.packet_fwd     = pkt_burst_5tuple_swap,
};

uint64_t rack_load[HOST_PER_RACK] = {0};

static void
pkt_burst_redirection(struct fwd_stream *fs){
	struct rte_mbuf  *pkts_burst[MAX_PKT_BURST];
	struct rte_port  *txp;
	struct rte_mbuf *mb;
	uint16_t next_proto;
	uint64_t ol_flags;
	uint16_t proto;
	uint16_t nb_rx;
	uint16_t nb_tx;
	uint32_t retry;
	
	struct table_key ip_service_key;
	void* lookup_result;
	int drop_index_list[MAX_PKT_BURST]; // 0 for tx, 1 for drop

	int i;
	union {
		struct rte_ether_hdr *eth;
		struct rte_vlan_hdr *vlan;
		struct rte_ipv4_hdr *ipv4;
		struct rte_ipv6_hdr *ipv6;
		struct rte_tcp_hdr *tcp;
		struct rte_udp_hdr *udp;
		struct alt_header* alt;
		uint8_t *byte;
	} h;

	struct rte_ether_hdr *ether_header;
	struct rte_ipv4_hdr *ipv4_header;
	struct rte_udp_hdr *udp_header;

	/*
	 * Receive a burst of packets and forward them.
	 */
	nb_rx = rte_eth_rx_burst(fs->rx_port, fs->rx_queue, pkts_burst, 4);
	if (unlikely(nb_rx == 0))
		return;

	clock_gettime(CLOCK_REALTIME, &ts1);

	fs->rx_packets += nb_rx;
	txp = &ports[fs->tx_port];
	ol_flags = ol_flags_init(txp->dev_conf.txmode.offloads);
	vlan_qinq_set(pkts_burst, nb_rx, ol_flags,
			txp->tx_vlan_id, txp->tx_vlan_id_outer);
	for (i = 0; i < nb_rx; i++) {
		if (likely(i < nb_rx - 1))
			rte_prefetch0(rte_pktmbuf_mtod(pkts_burst[i+1],
					void *));
		mb = pkts_burst[i];
		drop_index_list[i] = 0; // all packets are default to be forwarded!
		h.eth = rte_pktmbuf_mtod(mb, struct rte_ether_hdr *);
		ether_header = h.eth;
		//ether_header = rte_pktmbuf_mtod(mb, struct rte_ether_hdr *);

		proto = h.eth->ether_type;
		//printf("eth\n");
		swap_mac(h.eth);
		mb->l2_len = sizeof(struct rte_ether_hdr);
		h.eth++;

		while (proto == RTE_BE16(RTE_ETHER_TYPE_VLAN) ||
		       proto == RTE_BE16(RTE_ETHER_TYPE_QINQ)) {
			proto = h.vlan->eth_proto;
			h.vlan++;
			mb->l2_len += sizeof(struct rte_vlan_hdr);
		}

		if (proto == RTE_BE16(RTE_ETHER_TYPE_IPV4)) {
			//printf("ipv4\n");
			ipv4_header = h.ipv4;
			swap_ipv4(h.ipv4);
			next_proto = h.ipv4->next_proto_id;
			mb->l3_len = (h.ipv4->version_ihl & 0x0f) * 4;
			h.byte += mb->l3_len;
		} else if (proto == RTE_BE16(RTE_ETHER_TYPE_IPV6)) {
			swap_ipv6(h.ipv6);
			next_proto = h.ipv6->proto;
			h.ipv6++;
			mb->l3_len = sizeof(struct rte_ipv6_hdr);
		} else {
			mbuf_field_set(mb, ol_flags);
			continue;
		}

		uint64_t local_load = 0;
		int ret;
		void* mac_lookup_result;
		if (next_proto == IPPROTO_UDP) {
			udp_header = h.udp;
			//swap_udp(h.udp);
			//printf("udp");
			udp_counter++;
			mb->l4_len = sizeof(struct rte_udp_hdr);
			//#ifdef REDIRECT_ENABLED
			h.byte += mb->l4_len; // move the pointer!
			uint8_t msgtype = h.alt->msgtype_flags;

			#ifdef REDIRECT_DEBUG_PRINT
			printf("req_id:%" PRIu32 ",msgtype:%" PRIu8 "\n", h.alt->request_id, msgtype);
			#endif
			if(msgtype == SINGLE_PKT_REQ){
				//ST:piggyback counter
				rte_atomic16_inc(&request_counter);
				rte_smp_mb();

				#if FORWARD_LATENCY_LOG==1
				gossip_rx_samples[gossip_rx_array_index].switch_index = (uint16_t) fs->rx_queue;
				clock_gettime(CLOCK_REALTIME, &start_ts[0]);	
				#endif

				if(h.alt->redirection == 0){
					//print_ipaddr("actual_src_ip", ipv4_header->dst_addr);
					h.alt->actual_src_ip = ipv4_header->dst_addr;
				}
				#if redirection_piggyback==1
				else{ // for h.alt->redirection !=0, i.e. > 0
					uint64_t load;
					//parse piggyback load info for each host from the other ToR
					for(uint16_t host_index = 0; host_index < HOST_PER_RACK; host_index++){
						ip_service_key.service_id = h.alt->service_id_list[host_index];
						ip_service_key.ip_dst = h.alt->host_ip_list[host_index];
						load = (uint64_t) h.alt->host_queue_depth[host_index];
						int ret = rte_hash_inplace_update_data_with_key(fs->ip2load_table, (void*) &ip_service_key, &load);
						if(unlikely(ret < 0))
							printf("rte_hash_inplace_update_data_with_key failed!\n");
						// else{
						// 	printf("redir_pgy_recv,");
						// 	print_ip2load(ip_service_key.ip_dst, load);
						// }	
					}
				}
				#endif
				//printf("redirection:%" PRIu8 "\n", h.alt->redirection);


				//#if THRESHOLD_REDIRECT==1
				// the default destination of the request? -> alt_dst_ip
				// set up the key for (service_id, ip) -> load lookups
				ip_service_key.service_id = h.alt->service_id;
				ip_service_key.ip_dst = h.alt->alt_dst_ip;				
				// lookups for (service_id, ip) -> load
				// -> we set missing values to UINT64_MAX, so it's unlikely they'll get selected.
				int ret = rte_hash_lookup_data(fs->ip2load_table, (void*) &ip_service_key, &lookup_result);
				uint64_t* ptr = (uint64_t*) lookup_result;
				if(ret >= 0)
					local_load = *ptr; 
				else
					local_load = UINT64_MAX;
				//#endif

				//redirect requests to the min load replica
				uint64_t remote_min_load = UINT64_MAX;	
				uint64_t current_load;
				uint16_t min_index = 0;
				uint16_t remote_index = 0;
				uint32_t readable_req_array_index = (uint32_t) rte_atomic64_read(&req_qd_array_index);
				for(uint16_t host_index = 0; host_index < NUM_REPLICA; host_index++){
					ip_service_key.ip_dst = h.alt->replica_dst_list[host_index];
					ip_service_key.service_id = h.alt->service_id;

					// we don't compare with ourselves if 
					if(ip_service_key.ip_dst == h.alt->alt_dst_ip)
						continue;

					int ret = rte_hash_lookup_data(fs->ip2load_table, (void*) &ip_service_key, &lookup_result);
					uint64_t* ptr = (uint64_t*) lookup_result;
					//print_ip2load(ip_service_key.ip_dst, *ptr);
					//printf("remote_min_load:%" PRIu64 "\n", remote_min_load);

					if(ret >= 0){
						current_load = *ptr; 
						#if LOAD_COUNTER_LOG==1
						req_qd_samples[readable_req_array_index].remote_load_array[remote_index] = *ptr;
						remote_index++;
						#endif
					}
					else{
						current_load = UINT64_MAX;
						#if LOAD_COUNTER_LOG==1
						req_qd_samples[readable_req_array_index].remote_load_array[remote_index] = UINT64_MAX;						
						remote_index++;
						#endif
					}

					if(current_load < remote_min_load){
						min_index = host_index;
						remote_min_load = current_load;
					}									

					// #ifdef REDIRECT_DEBUG_PRINT					
					// print_ipaddr("ip_service_key.ip_dst", ip_service_key.ip_dst);
					// printf("load:%" PRIu64 "\n", *ptr);
					// printf("remote_min_load:%" PRIu64 "\n", remote_min_load);
					// #endif
				}
				uint32_t dest_addr = h.alt->replica_dst_list[min_index];

				#if LOAD_COUNTER_LOG==1
				req_qd_samples[readable_req_array_index].local_load = local_load;
				//req_qd_samples[req_qd_array_index].remote_min_load = remote_min_load;
				//req_qd_array_index++;
				rte_atomic64_inc(&req_qd_array_index);
				#endif

				h.alt->req_ts_sec = ts1.tv_sec;
				h.alt->req_ts_nsec = ts1.tv_nsec;

				// printf("local_load:");
				// print_ip2load(h.alt->alt_dst_ip, local_load);
				// printf("remote_min_load:");
				// print_ip2load(dest_addr, remote_min_load);

				//#if THRESHOLD_REDIRECT==1
				//if( local queue depth (i.e. default destination's queue depth)  <= remote minimal queue depth + delta || redirection > 1 )
				if(remote_min_load + load_delta >= local_load || h.alt->redirection >= redirect_bound){
					ipv4_header->dst_addr = h.alt->alt_dst_ip;					
					ip_service_key.service_id = h.alt->service_id;
					ip_service_key.ip_dst = ipv4_header->dst_addr;

					//[load++,load--] increment ip2load table's load for ipv4_header->dst_addr
					ret = rte_hash_increment_data_with_key(fs->ip2load_table, (void*) &ip_service_key);
					if(unlikely(ret < 0))
						printf("load++ failed\n");

					#if REQ_TIMESTAMP_LOG==1
					rx_timestamps[rx_ts_index] = ts1;
					rx_ts_index++;
					#endif
				}
				else{ //remote_min_load + LOAD_DELTA < local_load  && h.alt->redirection < redirect_bound
					if(h.alt->alt_dst_ip == dest_addr){
						ipv4_header->dst_addr = h.alt->alt_dst_ip;
						ip_service_key.service_id = h.alt->service_id;
						ip_service_key.ip_dst = ipv4_header->dst_addr;
						int ret = rte_hash_lookup_data(fs->ip2load_table, (void*) &ip_service_key, &lookup_result);						

						//[load++,load--]: increment ip2load table's load for ipv4_header->dst_addr
						ret = rte_hash_increment_data_with_key(fs->ip2load_table, (void*) &ip_service_key);
						if(unlikely(ret < 0))
							printf("load++ failed\n");

						#if REQ_TIMESTAMP_LOG==1
						rx_timestamps[rx_ts_index] = ts1;
						rx_ts_index++;
						#endif

						goto mac_lookup;
					}

					uint32_t* nexthop_ptr;
					//routing lookup for h.alt->replica_dst_list[min_index]
					ret = rte_hash_lookup_data(fs->routing_table, (void*) &dest_addr, &lookup_result);
					if(ret >= 0){
						nexthop_ptr = (uint32_t*) lookup_result;
						ipv4_header->dst_addr = *nexthop_ptr;
						h.alt->alt_dst_ip = dest_addr; // the min load ip becomes the new default dest
					}
					else{ // fall back to default dest.
						//#ifdef REDIRECT_DEBUG_PRINT
						printf("fall back to default dest\n");
						#if REQ_TIMESTAMP_LOG==1
						rx_timestamps[rx_ts_index] = ts1;
						rx_ts_index++;
						#endif						
						//#endif
						ipv4_header->dst_addr = h.alt->alt_dst_ip;	
					}

					#ifdef REDIRECT_DEBUG_PRINT
					//print_ipaddr("min ip_addr:", h.alt->replica_dst_list[min_index]);
					print_ipaddr("next-hop addr:", ipv4_header->dst_addr);
					print_ipaddr("min ip_addr:", h.alt->replica_dst_list[min_index]);
					printf("remote_min_load:%" PRIu64 "\n", remote_min_load);
					#endif

					#if redirection_piggyback==1

					#if REDIR_LOG==1
					int64_t redir_tx_index = rte_atomic64_read(&redir_tx_counter);
					redir_tx_samples[redir_tx_index].req_qd_array_index = rte_atomic64_read(&req_qd_array_index);
					#endif

					uint32_t iter = 0;
					uint32_t index = 0;
					const void *next_key;
					void *next_data;
					clock_gettime(CLOCK_REALTIME, &redir_ts);
					while (rte_hash_iterate(fs->ip2load_table, &next_key, &next_data, &iter) >= 0) {
						struct table_key* ip_service_pair = (struct table_key*) (uintptr_t) next_key;
						int ret = rte_hash_lookup_data(fs->ip2load_table, (void*) ip_service_pair, &lookup_result);
						uint16_t* ptr = (uint16_t*) lookup_result;
						if(unlikely(ret < 0))
							printf("not found in ip2load table for redirection piggyback\n");
						
						for(uint32_t host_index = 0; host_index < HOST_PER_RACK; host_index++){
							if(ip_service_pair->ip_dst == fs->local_ip_list[host_index]){
								h.alt->service_id_list[index] = ip_service_pair->service_id;
								h.alt->host_ip_list[index] = ip_service_pair->ip_dst;
								h.alt->host_queue_depth[index] = (uint16_t) *ptr;
								//record info into redir_tx_samples
								#if REDIR_LOG==1
								redir_tx_samples[redir_tx_index].service_id_list[index] = ip_service_pair->service_id;
								redir_tx_samples[redir_tx_index].host_ip_list[index] = ip_service_pair->ip_dst;
								redir_tx_samples[redir_tx_index].load_list[index] = (uint16_t) *ptr;
								redir_tx_samples[redir_tx_index].redir_ts=redir_ts;
								#endif
								//printf("redir_pgy_sent,");
								//print_ip2load(h.alt->host_ip_list[index],h.alt->host_queue_depth[index]);
								index++;
								break;
							}
						}
					}					
					#endif

					rte_atomic64_inc(&redir_tx_counter);
					h.alt->redirection+=1;
				}

				// print_ipaddr("dst_ipaddr", ipv4_header->dst_addr);
				// look up dst mac addr for our ip dest addr				
mac_lookup:		ret = rte_hash_lookup_data(fs->ip2mac_table, (void*) &ipv4_header->dst_addr, &mac_lookup_result);
				if(ret >= 0){
					struct rte_ether_addr* lookup1 = (struct rte_ether_addr*)(uintptr_t) mac_lookup_result;
					#ifdef REDIRECT_DEBUG_PRINT 
					print_macaddr("eth_addr lookup", lookup1);
					#endif
					// Assign lookup result to dest ether addr
					rte_ether_addr_copy(lookup1, &ether_header->d_addr); // ether_header->d_addr = lookup1
				}
				else{
					if (errno == ENOENT){
						print_ipaddr("dst_ipaddr", ipv4_header->dst_addr);
						printf("value not found\n");
					}
					else if(errno == EINVAL){
						printf("invalid parameters\n");
					}
					else{
						printf("unknown!\n");
					}
				}

				update_checksum(ipv4_header, udp_header); //update checksum!
				drop_index_list[i] = 0;

				#if FORWARD_LATENCY_LOG==1
				clock_gettime(CLOCK_REALTIME, &end_ts[0]);
				uint64_t diff_ns = clock_gettime_diff_ns(&start_ts[0], &end_ts[0]);
				gossip_rx_samples[gossip_rx_array_index].inter_rx_interval = diff_ns;
				gossip_rx_array_index++;
				#endif

				// #ifdef REDIRECT_DEBUG_PRINT 
				// printf("-------- redirection modification start------\n");
				// print_macaddr("src_macaddr", &ether_header->s_addr);
				// print_macaddr("dst_macaddr", &ether_header->d_addr);
				// print_ipaddr( "src_ipaddr",ipv4_header->src_addr);
				// print_ipaddr( "dst_ipaddr",ipv4_header->dst_addr);
				// print_ipaddr( "actual src_ipaddr", h.alt->actual_src_ip);
				// printf("-------- redirection modification end------\n");
				// #endif				
			}
			else if(msgtype == SINGLE_PKT_REQ_PASSTHROUGH){
				//ST:piggyback counter
				rte_atomic16_inc(&request_counter);
				rte_smp_mb();

				#if REQ_TIMESTAMP_LOG==1
				rx_timestamps[rx_ts_index] = ts1;
				rx_ts_index++;
				#endif

				h.alt->req_ts_sec = ts1.tv_sec;
				h.alt->req_ts_nsec = ts1.tv_nsec;				
				
				#if FORWARD_LATENCY_LOG==1
				gossip_rx_samples[gossip_rx_array_index].switch_index = (uint16_t) fs->rx_queue;
				clock_gettime(CLOCK_REALTIME, &start_ts[0]);	
				#endif

				// printf("REQ req-id %" PRIu32 "\n", h.alt->request_id);
				// print_ipaddr("REQ from-swtich dst", h.alt->alt_dst_ip);
				// print_ipaddr("REQ actual_src_ip", h.alt->actual_src_ip);
				
				h.alt->actual_src_ip  = ipv4_header->dst_addr;
				ipv4_header->dst_addr = h.alt->alt_dst_ip;
				// look up dst mac addr for our ip dest addr
				int ret = rte_hash_lookup_data(fs->ip2mac_table, (void*) &ipv4_header->dst_addr, &lookup_result);
				if(ret >= 0){
					struct rte_ether_addr* lookup1 = (struct rte_ether_addr*)(uintptr_t) lookup_result;
					#ifdef REDIRECT_DEBUG_PRINT 
					print_macaddr("eth_addr lookup", lookup1);
					#endif
					// Assign lookup result to dest ether addr
					rte_ether_addr_copy(lookup1, &ether_header->d_addr); // ether_header->d_addr = lookup1
				}
				else{
					if (errno == ENOENT)
						printf("value not found\n");
					else
						printf("invalid parameters\n");
				}

				uint32_t readable_req_array_index = (uint32_t) rte_atomic64_read(&req_qd_array_index);
				uint16_t remote_index = 0;
				for(uint16_t host_index = 0; host_index < NUM_REPLICA; host_index++){
					ip_service_key.ip_dst = h.alt->replica_dst_list[host_index];
					ip_service_key.service_id = h.alt->service_id;

					int ret = rte_hash_lookup_data(fs->ip2load_table, (void*) &ip_service_key, &lookup_result);
					uint64_t* ptr = (uint64_t*) lookup_result;

					#if LOAD_COUNTER_LOG==1
					if(ret >= 0){
						if(ip_service_key.ip_dst == h.alt->alt_dst_ip){
							req_qd_samples[readable_req_array_index].local_load = *ptr;
						}
						else{ 
							req_qd_samples[readable_req_array_index].remote_load_array[remote_index] = *ptr;
							remote_index++;
						}
					}
					#endif
				}
				#if LOAD_COUNTER_LOG==1
				rte_atomic64_inc(&req_qd_array_index);
				//req_qd_array_index++;
				#endif
								
				//[load++,load--]: increment ip2load table's load for ipv4_header->dst_addr
				ip_service_key.service_id = h.alt->service_id;
				ip_service_key.ip_dst = ipv4_header->dst_addr;
				ret = rte_hash_lookup_data(fs->ip2load_table, (void*) &ip_service_key, &lookup_result);
				if(likely(ret >= 0)){
					uint64_t* ptr = (uint64_t*) lookup_result;
					//*ptr = *ptr + 1;
					//rte_hash_add_key_data(fs->ip2load_table, (void*) &ip_service_key, (void *) ptr);
					ret = rte_hash_increment_data_with_key(fs->ip2load_table, (void*) &ip_service_key);
					if(ret < 0)
						printf("load++ failed\n");
					// #ifdef REDIRECT_DEBUG_PRINT
					// printf("load++:%" PRIu64 "\n", *ptr);
					// #endif
					//printf("load++,");
					//print_ip2load(ip_service_key.ip_dst, *ptr);
				}
				else{
					print_ipaddr("req src", ipv4_header->src_addr);
					print_ipaddr("req dst", ipv4_header->dst_addr);
					printf("no dest ip found in ip2load table\n");
				}

				update_checksum(ipv4_header, udp_header); //update checksum!

				#if FORWARD_LATENCY_LOG==1
				clock_gettime(CLOCK_REALTIME, &end_ts[0]);
				uint64_t diff_ns = clock_gettime_diff_ns(&start_ts[0], &end_ts[0]);
				gossip_rx_samples[gossip_rx_array_index].inter_rx_interval = diff_ns;
				gossip_rx_array_index++;
				#endif

				#ifdef REDIRECT_DEBUG_PRINT
				print_macaddr("req_passthrough src", &ether_header->s_addr);
				print_macaddr("req_passthrough dst", &ether_header->d_addr);
				print_ipaddr("req_passthrough src", ipv4_header->src_addr);
				print_ipaddr("req_passthrough dst", ipv4_header->dst_addr);
				#endif

				drop_index_list[i] = 0;
			}
			else if(msgtype == SINGLE_PKT_RESP_PASSTHROUGH || msgtype == SINGLE_PKT_RESP_PIGGYBACK){
				#if RESP_TIMESTAMP_LOG==1
				tx_timestamps[tx_ts_index] = ts1;
				tx_ts_index++;
				#endif

				h.alt->resp_ts_sec = ts1.tv_sec;
				h.alt->resp_ts_nsec = ts1.tv_nsec;

				//after swap_ipv4, dst_addr is the src_addr here	
				rte_be32_t feedback_src_addr = ipv4_header->dst_addr;
				//print_ipaddr("actual_src_addr", ipv4_header->dst_addr);
				//print_ipaddr("actual_dst_addr", h.alt->actual_src_ip);
				// in the response, the actual dest is stored at actual_src_ip;
				ipv4_header->dst_addr = h.alt->actual_src_ip;

				int ret = rte_hash_lookup_data(fs->ip2mac_table, (void*) &ipv4_header->dst_addr, &lookup_result);
				if(ret >= 0){
					struct rte_ether_addr* lookup1 = (struct rte_ether_addr*)(uintptr_t) lookup_result;
					#ifdef REDIRECT_DEBUG_PRINT	
					printf("value not found\n");		 
					print_macaddr("eth_addr lookup", lookup1);
					#endif
					rte_ether_addr_copy(lookup1, &ether_header->d_addr); // ether_header->d_addr = lookup1
				}
				else{
					if (errno == ENOENT)
						printf("value not found\n");
					else
						printf("invalid parameters\n");
				}
				update_checksum(ipv4_header, udp_header); 

				//printf("RESP req-id %" PRIu32 "\n", h.alt->request_id);
				//print_ipaddr("RESP from-server src", feedback_src_addr);
				//print_ipaddr("RESP from-switch dst", ipv4_header->dst_addr);

				//[load++,load--]: decrement ip2load table's load for actual_src_addr												
				// else{ 					
				ip_service_key.service_id = h.alt->service_id;
				ip_service_key.ip_dst = feedback_src_addr;
				ret = rte_hash_lookup_data(fs->ip2load_table, (void*) &ip_service_key, &lookup_result);
				if(ret >= 0){
					uint64_t* ptr = (uint64_t*) lookup_result;
				}
				ret = rte_hash_decrement_data_with_key(fs->ip2load_table, (void*) &ip_service_key);
				if(unlikely(ret < 0)){
					printf("load-- failed\n");
					printf("RESP req-id %" PRIu32 "\n", h.alt->request_id);
					print_ipaddr("RESP from-server src", feedback_src_addr);
					print_ipaddr("RESP from-switch dst", ipv4_header->dst_addr);
				}
				// else{
				// 	printf("load--\n");
				// }

				ret = rte_hash_lookup_data(fs->ip2load_table, (void*) &ip_service_key, &lookup_result);
				if(ret >= 0){
					uint64_t* ptr = (uint64_t*) lookup_result;
					//print_ip2load(feedback_src_addr, *ptr);
				}
				// }
				drop_index_list[i] = 0;
			}
			else if(msgtype == HOST_FEEDBACK_MSG){
				uint64_t load = (uint64_t) h.alt->feedback_options;
				//after swap_ipv4, dst_addr is the src_addr here
				ipv4_header->src_addr = ipv4_header->dst_addr;
				#ifdef REDIRECT_DEBUG_PRINT
				print_ipaddr("feedback src_ipaddr",ipv4_header->src_addr);
				#endif

				ip_service_key.service_id = h.alt->service_id;
				ip_service_key.ip_dst = ipv4_header->src_addr;
				//int ret = 
				rte_hash_lookup_data(fs->ip2load_table, (void*) &ip_service_key, &lookup_result);
				uint64_t* ptr = (uint64_t*) lookup_result;
				*ptr = load;
				printf("HOST_FEEDBACK_MSG:");
				print_ip2load(ip_service_key.ip_dst, *ptr);
				//int ret = rte_hash_add_key_data(fs->ip2load_table, (void*) &ip_service_key, (void *)((uintptr_t) &load));
				//if(ret == 0)
				//	printf("rte_hash_add_key_data okay!\n");

				drop_index_list[i] = 1;
				//drop_index_list[drop_index] = i;
				//drop_index++;
			}
			else if(msgtype == SWITCH_FEEDBACK_MSG){
				uint64_t load;
				//after swap_ipv4, dst_addr is the src addr of the feedback messages
				uint32_t src_switch_addr = ipv4_header->dst_addr;
				uint16_t index;
				for(index = 0; index < fs->switch_ip_list_length; index++){
					if(src_switch_addr == fs->switch_ip_list[index])
						break;
				}
				//Verify gossip intervals
				//gossip_rx_samples: log (index i, rx_time_intervals for index i) to a data sturcture
				//clock_gettime(CLOCK_REALTIME, &end_ts[index]);
                //uint64_t diff_us = clock_gettime_diff_us(&start_ts[index], &end_ts[index]);
				//gossip_rx_samples[gossip_rx_array_index].switch_index = index;
				//gossip_rx_samples[gossip_rx_array_index].inter_rx_interval = diff_us;
				//clock_gettime(CLOCK_REALTIME, &start_ts[index]);
				//gossip_rx_array_index++;

				// SWITCH_FEEDBACK_MSG 
				// It could be very similar to HOST_FEEDBACK_MSG since we need packet drop
				// We may need bulk update to the ip2load hashtable
				int hash_ret;
				for(uint16_t host_index = 0; host_index < HOST_PER_RACK; host_index++){
					ip_service_key.service_id = h.alt->service_id_list[host_index];
					// if(h.alt->host_ip_list[host_index] == empty_addr){
					// 	#ifdef REDIRECT_DEBUG_PRINT
					// 	printf("ip_service_key.ip_dst: empty_addr\n");
					// 	#endif
					// 	continue;
					// }
					ip_service_key.ip_dst = h.alt->host_ip_list[host_index];
					// #ifdef REDIRECT_DEBUG_PRINT	
					// print_ipaddr("ip_service_key.ip_dst",ip_service_key.ip_dst);				
					// #endif
					load = (uint64_t) h.alt->host_queue_depth[host_index];
					//int ret = rte_hash_add_key_data(fs->ip2load_table, (void*) &ip_service_key, (void *)((uintptr_t) &load));
					int ret = rte_hash_inplace_update_data_with_key(fs->ip2load_table, (void*) &ip_service_key, &load);
					//int ret = rte_hash_lookup_data(fs->ip2load_table, (void*) &ip_service_key, &lookup_result);
					//uint64_t* ptr = (uint64_t*) lookup_result;
					//*ptr = load;
					//rte_hash_add_key_data(fs->ip2load_table, (void*) &ip_service_key, (void *) ptr);
					hash_ret += ret;
				}

				//if(hash_ret < 0)
				//	printf("rte_hash_add_key_data %d times okay!\n", HOST_PER_RACK);
					
				drop_index_list[i] = 1;
				//drop_index_list[drop_index] = i;
				//drop_index++;
			}
			else{
				// for unimplemented types of msg
				//MULTI_PKT_REQ,
				//MULTI_PKT_RESP_PIGGYBACK,
				//MULTI_PKT_RESP_PASSTHROUGH,  
				//CONTROL_MSG,	
				printf("unimplemented types of msg\n");	
				drop_index_list[i] = 1;		
			}
			//#endif

		} else if (next_proto == IPPROTO_TCP) {
			swap_tcp(h.tcp);
			mb->l4_len = (h.tcp->data_off & 0xf0) >> 2;
		}
		mbuf_field_set(mb, ol_flags);
	}
	#ifdef REDIRECT_ENABLED
	for (int pkt_index = 0; pkt_index < nb_rx; pkt_index++) {
		if(drop_index_list[pkt_index] == 1){
			#ifdef REDIRECT_DEBUG_PRINT
			printf("rte_pktmbuf_free 1 pkt, udp_counter %d\n", udp_counter);
			#endif
			rte_pktmbuf_free(pkts_burst[pkt_index]);
		}
		else{
			nb_tx = rte_eth_tx_burst(fs->tx_port, fs->tx_queue, &pkts_burst[pkt_index], 1);
			if(nb_tx == 0){
				printf("rte_eth_tx_burst failed to send 1 pkt\n");
				rte_pktmbuf_free(pkts_burst[pkt_index]);
			}
			fs->tx_packets += nb_tx;
		}
	}
	#else
	nb_tx = rte_eth_tx_burst(fs->tx_port, fs->tx_queue, pkts_burst, nb_rx);
	fs->tx_packets += nb_tx;
	#endif

	/*
	 * Retry if necessary
	 */
	// if (unlikely(nb_tx < nb_rx) && fs->retry_enabled) {
	// 	retry = 0;
	// 	while (nb_tx < nb_rx && retry++ < burst_tx_retry_num) {
	// 		rte_delay_us(burst_tx_delay_time);
	// 		nb_tx += rte_eth_tx_burst(fs->tx_port, fs->tx_queue,
	// 				&pkts_burst[nb_tx], nb_rx - nb_tx);
	// 	}
	// }
	// fs->tx_packets += nb_tx;
	// if (unlikely(nb_tx < nb_rx)) {
	// 	fs->fwd_dropped += (nb_rx - nb_tx);
	// 	do {
	// 		rte_pktmbuf_free(pkts_burst[nb_tx]);
	// 	} while (++nb_tx < nb_rx);
	// }
}

struct fwd_engine replica_selection_fwd_engine = {
	.fwd_mode_name = "replica-select",
	.port_fwd_begin = NULL,
	.port_fwd_end   = NULL,
	.packet_fwd     = pkt_burst_redirection,
};

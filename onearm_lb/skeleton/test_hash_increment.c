/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright(c) 2010-2014 Intel Corporation
 */

// how to run it?
// sudo ./build/app/test_hash -l 0-3 -n 4

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>
#include <sys/queue.h>

#include <rte_memory.h>
#include <rte_launch.h>
#include <rte_eal.h>
#include <rte_per_lcore.h>
#include <rte_lcore.h>
#include <rte_debug.h>

#include <inttypes.h>
#include <locale.h>

#include <rte_cycles.h>
#include <rte_hash.h>
#include <rte_hash_crc.h>
#include <rte_malloc.h>
#include <rte_random.h>
#include <rte_spinlock.h>
#include <rte_jhash.h>
#include "nanosleep.h"

/*
 * Check condition and return an error if true. Assumes that "handle" is the
 * name of the hash structure pointer to be freed.
 */
#define RETURN_IF_ERROR(cond, str, ...) do {                            \
	if (cond) {                                                     \
		printf("ERROR line %d: " str "\n", __LINE__,            \
							##__VA_ARGS__);	\
		if (handle)                                             \
			rte_hash_free(handle);                          \
		return -1;                                              \
	}                                                               \
} while (0)

#define RTE_APP_TEST_HASH_MULTIWRITER_FAILED 0

struct {
	uint32_t *keys;
	uint64_t *values;
	uint32_t *found;
	uint32_t nb_tsx_insertion;
	struct rte_hash *h;
} tbl_multiwriter_test_params;

const uint32_t nb_entries = 5*1024; //5*1024*1024;
const uint32_t nb_total_tsx_insertion = 5*1024; //4.5*1024*1024;
uint32_t rounded_nb_total_tsx_insertion;
struct timespec ts1, ts2, sleep_ts1, sleep_ts2;

static rte_atomic64_t gcycles;
static rte_atomic64_t ginsertions;

static int use_htm;

static int
test_hash_add_sub_buggy(void *arg)
{
	uint64_t i;
	uint16_t pos_core;
	uint32_t lcore_id = rte_lcore_id();
	uint16_t *enabled_core_ids = (uint16_t *)arg;

	for (pos_core = 0; pos_core < rte_lcore_count(); pos_core++) {
		if (enabled_core_ids[pos_core] == lcore_id)
			break;
	}

	printf("test_hash_add_sub_buggy: core id:%" PRIu16 "\n", pos_core);

	void* data;
	for (i = 0; i < nb_entries; i++){
		if(pos_core%2==0){ //even-th cores, i == even ++, i == odd --
			if(i%2==0){
				int ret = rte_hash_lookup_data(
					tbl_multiwriter_test_params.h, 
					&tbl_multiwriter_test_params.keys[i],
					&data);

				if(ret < 0){
					printf("increment_data_with_key failed\n");
					//printf("lookup failed\n");
					if(ret == - EINVAL){
						printf("-EINVAL\n");
					}
					if(ret == -ENOENT){
						printf("-ENOENT\n");
					}
					return -1;
				}
				else{
					uint64_t* ptr = (uint64_t*) data;
					*ptr = *ptr + 1;
				}
			}
			else{				
				int ret = rte_hash_lookup_data(
					tbl_multiwriter_test_params.h, 
					&tbl_multiwriter_test_params.keys[i],
					&data);
					
				if(ret < 0){
					printf("decrement_data_with_key failed\n");
					//printf("lookup failed\n");
					if(ret == - EINVAL){
						printf("-EINVAL\n");
					}

					if(ret == -ENOENT){
						printf("-ENOENT\n");
					}
					return -1;
				}
				else{
					uint64_t* ptr = (uint64_t*) data;
					*ptr = *ptr - 1;
				}

			}
		}
		else{ //odd-th cores, i == even --, i == odd ++
			if(i%2==0){
				int ret = rte_hash_lookup_data(
					tbl_multiwriter_test_params.h, 
					&tbl_multiwriter_test_params.keys[i],
					&data);
					
				if(ret < 0){
					//printf("lookup failed\n");
					printf("decrement_data_with_key failed\n");
					if(ret == - EINVAL){
						printf("-EINVAL\n");
					}
					if(ret == -ENOENT){
						printf("-ENOENT\n");
					}
					return -1;
				}
				else{
					uint64_t* ptr = (uint64_t*) data;
					*ptr = *ptr - 1;
				}
			}
			else{
				int ret = rte_hash_lookup_data(
					tbl_multiwriter_test_params.h, 
					&tbl_multiwriter_test_params.keys[i],
					&data);

				if(ret < 0){					
					printf("increment_data_with_key failed\n");
					//printf("lookup failed\n");
					if(ret == - EINVAL){
						printf("-EINVAL\n");
					}
					if(ret == -ENOENT){
						printf("-ENOENT\n");
					}
					return -1;
				}
				else{
					uint64_t* ptr = (uint64_t*) data;
					*ptr = *ptr + 1;
				}
			}
		}
	}
	return 0;
}

static int
test_hash_inplace_update_worker(void *arg)
{
	uint64_t i;
	int ret = 0;
	uint16_t pos_core;
	uint32_t lcore_id = rte_lcore_id();
	uint16_t *enabled_core_ids = (uint16_t *)arg;
	void* lookup_result;

	for (pos_core = 0; pos_core < rte_lcore_count(); pos_core++) {
		if (enabled_core_ids[pos_core] == lcore_id)
			break;
	}

	printf("test_hash_inplace_update_worker: core id:%" PRIu16 "\n", pos_core);

	for (i = 0; i < nb_entries; i++){
		if(pos_core == 1){ //pos_core == 1, the first core launched!
			//inplace_update for values[i]*10
			uint64_t val = tbl_multiwriter_test_params.keys[i]*10;
			//printf("value:%" PRIu64 "\n", val);
			ret = rte_hash_inplace_update_data_with_key( tbl_multiwriter_test_params.h, 
				&tbl_multiwriter_test_params.keys[i], &val);
			if(unlikely(ret < 0))
				break;
			//sleep(1us)
			clock_gettime(CLOCK_REALTIME, &ts1);
			sleep_ts1=ts1;
			realnanosleep(1000, &sleep_ts1, &sleep_ts2);
		}
		else{//other cores
			//sleep(1us)
			clock_gettime(CLOCK_REALTIME, &ts1);
			sleep_ts1=ts1;
			realnanosleep(2000, &sleep_ts1, &sleep_ts2);
			//lookup
			ret = rte_hash_lookup_data(tbl_multiwriter_test_params.h,
				&tbl_multiwriter_test_params.keys[i], &lookup_result);
			if(unlikely(ret < 0))
				break;
			uint64_t value = *(uint64_t*) lookup_result;
			if(value != tbl_multiwriter_test_params.keys[i]*10){
				printf("lcore:%" PRIu16 "key:%" PRIu32 ", value:%" PRIu64 ", values[i]:%" PRIu64 "\n",
				pos_core, tbl_multiwriter_test_params.keys[i], 
				value, tbl_multiwriter_test_params.values[i]);
			}
		}
	}
	return 0;
}

static int
test_hash_add_sub_worker(void *arg)
{
	uint64_t i;
	uint16_t pos_core;
	uint32_t lcore_id = rte_lcore_id();
	uint16_t *enabled_core_ids = (uint16_t *)arg;

	for (pos_core = 0; pos_core < rte_lcore_count(); pos_core++) {
		if (enabled_core_ids[pos_core] == lcore_id)
			break;
	}

	printf("test_hash_add_sub_worker: core id:%" PRIu16 "\n", pos_core);

	for (i = 0; i < nb_entries; i++){
		if(pos_core%2==0){ //even-th cores, i == even ++, i == odd --
			if(i%2==0){
				int ret = rte_hash_increment_data_with_key( //int ret = rte_hash_lookup(
					tbl_multiwriter_test_params.h, 
					&tbl_multiwriter_test_params.keys[i]);

				if(ret < 0){
					printf("increment_data_with_key failed\n");
					//printf("lookup failed\n");
					if(ret == - EINVAL){
						printf("-EINVAL\n");
					}
					if(ret == -ENOENT){
						printf("-ENOENT\n");
					}
					return -1;
				}
			}
			else{				
				int ret = rte_hash_decrement_data_with_key( //int ret = rte_hash_lookup(
					tbl_multiwriter_test_params.h, 
					&tbl_multiwriter_test_params.keys[i]);
					
				if(ret < 0){
					printf("decrement_data_with_key failed\n");
					//printf("lookup failed\n");
					if(ret == - EINVAL){
						printf("-EINVAL\n");
					}

					if(ret == -ENOENT){
						printf("-ENOENT\n");
					}
					return -1;
				}

			}
		}
		else{ //odd-th cores, i == even --, i == odd ++
			if(i%2==0){
				int ret = rte_hash_decrement_data_with_key( //int ret = rte_hash_lookup(
					tbl_multiwriter_test_params.h, 
					&tbl_multiwriter_test_params.keys[i]);
					
				if(ret < 0){
					//printf("lookup failed\n");
					printf("decrement_data_with_key failed\n");
					if(ret == - EINVAL){
						printf("-EINVAL\n");
					}
					if(ret == -ENOENT){
						printf("-ENOENT\n");
					}
					return -1;
				}
			}
			else{
				int ret = rte_hash_increment_data_with_key( //int ret = rte_hash_lookup(
					tbl_multiwriter_test_params.h, 
					&tbl_multiwriter_test_params.keys[i]);

				if(ret < 0){					
					printf("increment_data_with_key failed\n");
					//printf("lookup failed\n");
					if(ret == - EINVAL){
						printf("-EINVAL\n");
					}
					if(ret == -ENOENT){
						printf("-ENOENT\n");
					}
					return -1;
				}
			}
		}
	}

	return 0;
}

static int
test_hash_multiwriter_worker(void *arg)
{
	uint64_t i, offset;
	uint16_t pos_core;
	uint32_t lcore_id = rte_lcore_id();
	uint64_t begin, cycles;
	uint16_t *enabled_core_ids = (uint16_t *)arg;
	
	for (pos_core = 0; pos_core < rte_lcore_count(); pos_core++) {
		if (enabled_core_ids[pos_core] == lcore_id)
			break;
	}

	printf("test_hash_multiwriter_worker: core id:%" PRIu16 "\n", pos_core);

	/*
	 * Calculate offset for entries based on the position of the
	 * logical core, from the master core (not counting not enabled cores)
	 */
	offset = pos_core * tbl_multiwriter_test_params.nb_tsx_insertion;

	printf("Core #%d inserting %d: %'"PRId64" - %'"PRId64"\n",
	       lcore_id, tbl_multiwriter_test_params.nb_tsx_insertion,
	       offset,
	       offset + tbl_multiwriter_test_params.nb_tsx_insertion - 1);

	begin = rte_rdtsc_precise();

	for (i = offset;
	     i < offset + tbl_multiwriter_test_params.nb_tsx_insertion;
	     i++) {
		if (rte_hash_add_key_data(tbl_multiwriter_test_params.h,
				     tbl_multiwriter_test_params.keys + i, 
					 tbl_multiwriter_test_params.values + i) < 0)
			break;
	}

	cycles = rte_rdtsc_precise() - begin;
	rte_atomic64_add(&gcycles, cycles);
	rte_atomic64_add(&ginsertions, i - offset);

	for (; i < offset + tbl_multiwriter_test_params.nb_tsx_insertion; i++)
		tbl_multiwriter_test_params.keys[i]
			= RTE_APP_TEST_HASH_MULTIWRITER_FAILED;

	return 0;
}


static int
test_hash_multiwriter(void)
{
	unsigned int i, rounded_nb_total_tsx_insertion;
	static unsigned calledCount = 1;
	uint16_t enabled_core_ids[RTE_MAX_LCORE];
	uint16_t core_id;

	uint32_t *keys;
	uint64_t *values;
	uint32_t *found;

	struct rte_hash_parameters hash_params = {
		.entries = nb_entries,
		.key_len = sizeof(uint32_t),
		.hash_func = rte_jhash,
		.hash_func_init_val = 0,
		.socket_id = rte_socket_id(),
	};
	if (use_htm)
		hash_params.extra_flag =
			RTE_HASH_EXTRA_FLAGS_TRANS_MEM_SUPPORT
				| RTE_HASH_EXTRA_FLAGS_MULTI_WRITER_ADD;
	else
		hash_params.extra_flag =
			RTE_HASH_EXTRA_FLAGS_MULTI_WRITER_ADD;

	struct rte_hash *handle;
	char name[RTE_HASH_NAMESIZE];

	const void *next_key;
	void *next_data;
	uint32_t iter = 0;

	uint32_t duplicated_keys = 0;
	uint32_t lost_keys = 0;
	uint32_t count;

	snprintf(name, 32, "test%u", calledCount++);
	hash_params.name = name;

	handle = rte_hash_create(&hash_params);
	RETURN_IF_ERROR(handle == NULL, "hash creation failed");

	tbl_multiwriter_test_params.h = handle;
	tbl_multiwriter_test_params.nb_tsx_insertion =
		nb_total_tsx_insertion / rte_lcore_count();

	rounded_nb_total_tsx_insertion =
	(nb_total_tsx_insertion /
		tbl_multiwriter_test_params.nb_tsx_insertion)
		* tbl_multiwriter_test_params.nb_tsx_insertion;

	rte_srand(rte_rdtsc());

	keys = rte_malloc(NULL, sizeof(uint32_t) * nb_entries, 0);
	values = rte_malloc(NULL, sizeof(uint64_t) * nb_entries, 0);

	if (keys == NULL) {
		printf("RTE_MALLOC failed\n");
		goto err1;
	}

	if (values == NULL){
		printf("RTE_MALLOC failed\n");
		goto err1;
	}

	for (i = 0; i < nb_entries; i++){
		keys[i] = i;
		values[i] = (uint64_t) i;
	}

	tbl_multiwriter_test_params.keys = keys;
	tbl_multiwriter_test_params.values = values;

	found = rte_zmalloc(NULL, sizeof(uint32_t) * nb_entries, 0);
	if (found == NULL) {
		printf("RTE_ZMALLOC failed\n");
		goto err2;
	}

	tbl_multiwriter_test_params.found = found;

	rte_atomic64_init(&gcycles);
	rte_atomic64_clear(&gcycles);

	rte_atomic64_init(&ginsertions);
	rte_atomic64_clear(&ginsertions);

	/* Get list of enabled cores */
	i = 0;
	for (core_id = 0; core_id < RTE_MAX_LCORE; core_id++) {
		if (i == rte_lcore_count())
			break;

		if (rte_lcore_is_enabled(core_id)) {
			enabled_core_ids[i] = core_id;
			i++;
		}
	}

	if (i != rte_lcore_count()) {
		printf("Number of enabled cores in list is different from "
				"number given by rte_lcore_count()\n");
		goto err3;
	}

	if(rte_lcore_count()%2 != 0 ){
		printf("Need even numbers of lcores\n");
		goto err3;
	}

	/* Fire all threads. */
	rte_eal_mp_remote_launch(test_hash_multiwriter_worker,
				 enabled_core_ids, CALL_MASTER );
	rte_eal_mp_wait_lcore();

	count = rte_hash_count(handle);
	if (count != rounded_nb_total_tsx_insertion) {
		printf("rte_hash_count returned wrong value %u, %d\n",
				rounded_nb_total_tsx_insertion, count);
		goto err3;
	}

	while (rte_hash_iterate(handle, &next_key, &next_data, &iter) >= 0) {
		/* Search for the key in the list of keys added .*/
		i = *(const uint32_t *)next_key;
		//printf("keys[i]%" PRIu32 ",key:%" PRIu32 "\n", tbl_multiwriter_test_params.keys[i], i);
		//printf("tbl_multiwriter_test_params[%u]:%"PRIu32 "\n",i , tbl_multiwriter_test_params.keys[i]);
		tbl_multiwriter_test_params.found[i]++;
	}

	for (i = 0; i < rounded_nb_total_tsx_insertion; i++) {
		if (tbl_multiwriter_test_params.keys[i]
		    != RTE_APP_TEST_HASH_MULTIWRITER_FAILED) {
			//printf("tbl_multiwriter_test_params[%u]:%"PRIu32 "\n",i , tbl_multiwriter_test_params.keys[i]);
			if (tbl_multiwriter_test_params.found[i] > 1) {
				duplicated_keys++;
				break;
			}
			if (tbl_multiwriter_test_params.found[i] == 0) {
				lost_keys++;
				printf("key %d is lost\n", i);
				break;
			}
		}
	}

	if (duplicated_keys > 0) {
		printf("%d key duplicated\n", duplicated_keys);
		goto err3;
	}

	if (lost_keys > 0) {
		printf("%d key lost\n", lost_keys);
		goto err3;
	}

	printf("No key corrupted during multiwriter insertion.\n");

	unsigned long long int cycles_per_insertion =
		rte_atomic64_read(&gcycles)/
		rte_atomic64_read(&ginsertions);

	printf(" cycles per insertion: %llu\n", cycles_per_insertion);

	printf("multiwriter hash table increment/decrement.\n");
	rte_eal_mp_remote_launch(test_hash_add_sub_worker, enabled_core_ids, CALL_MASTER);
	rte_eal_mp_wait_lcore();

	printf("hash table increment/decrement done.\n");

	const void *next_key2;
	void *next_data2;
	uint32_t iter2 = 0;

	while (rte_hash_iterate(handle, &next_key2, &next_data2, &iter2) >= 0) {
		/* Search for the key in the list of keys added .*/
		uint32_t key = *(const uint32_t *)next_key2;
		uint64_t value = *(uint64_t *)next_data2;
		if(key != value){
			printf("incrementing/decrementing values%" PRIu64 " on key%" PRIu32 "doesn't have the correct value\n", value, key);
		}
		// if(key == value){
		// 	printf("key:%" PRIu32 ", value:%" PRIu64 "passed!\n", key, value);
		// }
	}

	// printf("multiwriter hash table, using pointer to increment/decrement.\n");
	// rte_eal_mp_remote_launch(test_hash_add_sub_buggy, enabled_core_ids, CALL_MASTER);
	// rte_eal_mp_wait_lcore();

	// const void *next_key3;
	// void *next_data3;
	// uint32_t iter3 = 0;

	// uint32_t incorrect_counter = 0;
	// while (rte_hash_iterate(handle, &next_key3, &next_data3, &iter3) >= 0) {
	// 	/* Search for the key in the list of keys added .*/
	// 	uint32_t key = *(const uint32_t *)next_key3;
	// 	uint64_t value = *(uint64_t *)next_data3;
	// 	if(key != value){
	// 		//printf("incrementing/decrementing values%" PRIu64 " on key%" PRIu32 "doesn't have the correct value\n", value, key);
	// 		incorrect_counter++;
	// 	}		
	// 	// if(key == value){
	// 	// 	printf("key:%" PRIu32 ", value:%" PRIu64 "passed!\n", key, value);
	// 	// }
	// }
	// printf("# of incorrect key-value pairs:%" PRIu32 "\n", incorrect_counter);

	printf("multiwriter hash table, in-place updates\n");
	rte_eal_mp_remote_launch(test_hash_inplace_update_worker, enabled_core_ids, CALL_MASTER);
	rte_eal_mp_wait_lcore();

	const void *next_key3;
	void *next_data3;
	uint32_t iter3 = 0;

	uint32_t incorrect_counter = 0;
	while (rte_hash_iterate(handle, &next_key3, &next_data3, &iter3) >= 0) {
		/* Search for the key in the list of keys added .*/
		uint32_t key = *(const uint32_t *)next_key3;
		uint64_t value = *(uint64_t *)next_data3;
		if(key*10 != value){
			//printf("incrementing/decrementing values%" PRIu64 " on key%" PRIu32 "doesn't have the correct value\n", value, key);
			incorrect_counter++;
		}		
		// if(key == value){
		// 	printf("key:%" PRIu32 ", value:%" PRIu64 "passed!\n", key, value);
		// }
	}
	printf("# of incorrect key-value pairs:%" PRIu32 "\n", incorrect_counter);

	rte_free(tbl_multiwriter_test_params.found);
	rte_free(tbl_multiwriter_test_params.keys);
	rte_hash_free(handle);
	return 0;

err3:
	rte_free(tbl_multiwriter_test_params.found);
err2:
	rte_free(tbl_multiwriter_test_params.keys);
err1:
	rte_hash_free(handle);
	return -1;
}

static int
test_hash_multiwriter_main(__rte_unused void *arg)
{
	if (rte_lcore_count() < 2) {
		printf("Not enough cores for distributor_autotest, expecting at least 2\n");
		return -1;
	}

	setlocale(LC_NUMERIC, "");

	if (!rte_tm_supported()) {
		printf("Hardware transactional memory (lock elision) "
			"is NOT supported\n");
	} else {
		printf("Hardware transactional memory (lock elision) "
			"is supported\n");

		printf("Test multi-writer with Hardware transactional memory\n");

		use_htm = 1;
		if (test_hash_multiwriter() < 0)
			return -1;
	}

	printf("Test multi-writer without Hardware transactional memory\n");
	use_htm = 0;
	if (test_hash_multiwriter() < 0)
		return -1;

	return 0;
}

// static int
// lcore_hello(__rte_unused void *arg)
// {
// 	unsigned lcore_id;
// 	lcore_id = rte_lcore_id();
// 	printf("hello from core %u\n", lcore_id);
// 	return 0;
// }

int
main(int argc, char **argv)
{
	int ret;
	//unsigned lcore_id;

	ret = rte_eal_init(argc, argv);
	if (ret < 0)
		rte_panic("Cannot init EAL\n");

	/* call lcore_hello() on every slave lcore */
	// RTE_LCORE_FOREACH_SLAVE(lcore_id) {
	// 	rte_eal_remote_launch(test_hash_multiwriter_main, NULL, lcore_id);
	// }

	/* call it on master lcore too */
	//lcore_hello(NULL);

	test_hash_multiwriter_main(NULL);

	rte_eal_mp_wait_lcore();
	return 0;
}

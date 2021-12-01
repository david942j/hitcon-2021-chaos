/* SPDX-License-Identifier: GPL-2.0 */

#ifndef _CHAOS_H
#define _CHAOS_H

#include <linux/ioctl.h>

#define CHAOS_IOC_MAGIC 0xCA

/*
 * Allocates device DRAM for this client.
 * Mmap() can only be called on clients with buffer allocated.
 *
 * Each client should allocate a buffer exact once.
 *
 * Returns 0 on success.
 */
#define CHAOS_ALLOCATE_BUFFER _IOW(CHAOS_IOC_MAGIC, 0, u_int64_t)

enum chaos_request_algo {
	/* copy input to output, for testing purpose */
	CHAOS_ALGO_ECHO,
	CHAOS_ALGO_MD5,
	CHAOS_ALGO_SHA256,
	CHAOS_ALGO_AES_ENC,
	CHAOS_ALGO_AES_DEC,
	CHAOS_ALGO_RC4_ENC,
	CHAOS_ALGO_RC4_DEC,
	CHAOS_ALGO_BF_ENC,
	CHAOS_ALGO_BF_DEC,
	CHAOS_ALGO_TF_ENC,
	CHAOS_ALGO_TF_DEC,
	CHAOS_ALGO_FFF_ENC,
	CHAOS_ALGO_FFF_DEC,
};

struct chaos_request {
	enum chaos_request_algo algo;
	u_int32_t input;
	u_int32_t in_size;
	u_int32_t key;
	u_int32_t key_size;
	u_int32_t output;
	/*
	 * Size of @output.
	 * Will be set to the "true" output size on success.
	 * e.g. for CHAOS_ALGO_MD5, runtime should have out_size >= 16, and @out_size
	 * is set to 16 when ioctl returned.
	 */
	u_int32_t out_size;
};

#define CHAOS_REQUEST _IOWR(CHAOS_IOC_MAGIC, 0, struct chaos_request)

#endif /* _CHAOS_H */

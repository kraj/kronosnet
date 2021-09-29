/*
 * Copyright (C) 2010-2021 Red Hat, Inc.  All rights reserved.
 *
 * Author: Fabio M. Di Nitto <fabbione@kronosnet.org>
 *
 * This software licensed under LGPL-2.0+
 */

#include "config.h"

#include <strings.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <stdarg.h>
#include <errno.h>
#include <stdio.h>

#include "internals.h"
#include "logging.h"
#include "threads_common.h"

static struct pretty_names subsystem_names[KNET_MAX_SUBSYSTEMS] =
{
	{ "common", KNET_SUB_COMMON },
	{ "handle", KNET_SUB_HANDLE },
	{ "host", KNET_SUB_HOST },
	{ "listener", KNET_SUB_LISTENER },
	{ "link", KNET_SUB_LINK },
	{ "transport", KNET_SUB_TRANSPORT },
	{ "crypto", KNET_SUB_CRYPTO },
	{ "compress", KNET_SUB_COMPRESS },
	{ "filter", KNET_SUB_FILTER },
	{ "dstcache", KNET_SUB_DSTCACHE },
	{ "heartbeat", KNET_SUB_HEARTBEAT },
	{ "pmtud", KNET_SUB_PMTUD },
	{ "tx", KNET_SUB_TX },
	{ "rx", KNET_SUB_RX },
	{ "loopback", KNET_SUB_TRANSP_LOOPBACK },
	{ "udp", KNET_SUB_TRANSP_UDP },
	{ "sctp", KNET_SUB_TRANSP_SCTP },
	{ "nsscrypto", KNET_SUB_NSSCRYPTO },
	{ "opensslcrypto", KNET_SUB_OPENSSLCRYPTO },
	{ "gcryptcrypto", KNET_SUB_GCRYPTCRYPTO },
	{ "zlibcomp", KNET_SUB_ZLIBCOMP },
	{ "lz4comp", KNET_SUB_LZ4COMP },
	{ "lz4hccomp", KNET_SUB_LZ4HCCOMP },
	{ "lzo2comp", KNET_SUB_LZO2COMP },
	{ "lzmacomp", KNET_SUB_LZMACOMP },
	{ "bzip2comp", KNET_SUB_BZIP2COMP },
	{ "zstdcomp", KNET_SUB_ZSTDCOMP },
	{ "none", KNET_SUB_NONE },
	{ "unknown", KNET_SUB_UNKNOWN }		/* unknown MUST always be last in this array */
};

const char *knet_log_get_subsystem_name(uint8_t subsystem)
{
	unsigned int i;

	for (i = 0; i < KNET_MAX_SUBSYSTEMS; i++) {
		if (subsystem_names[i].val == KNET_SUB_UNKNOWN) {
			break;
		}
		if (subsystem_names[i].val == subsystem) {
			errno = 0;
			return subsystem_names[i].name;
		}
	}
	return "unknown";
}

uint8_t knet_log_get_subsystem_id(const char *name)
{
	unsigned int i;

	for (i = 0; i < KNET_MAX_SUBSYSTEMS; i++) {
		if (subsystem_names[i].val == KNET_SUB_UNKNOWN) {
			break;
		}
		if (strcasecmp(name, subsystem_names[i].name) == 0) {
			errno = 0;
			return subsystem_names[i].val;
		}
	}
	return KNET_SUB_UNKNOWN;
}

static int is_valid_subsystem(uint8_t subsystem)
{
	unsigned int i;

	for (i = 0; i < KNET_MAX_SUBSYSTEMS; i++) {
		if ((subsystem != KNET_SUB_UNKNOWN) &&
		    (subsystem_names[i].val == KNET_SUB_UNKNOWN)) {
			break;
		}
		if (subsystem_names[i].val == subsystem) {
			return 0;
		}
	}
	return -1;
}

static struct pretty_names loglevel_names[KNET_LOG_DEBUG + 1] =
{
	{ "ERROR", KNET_LOG_ERR },
	{ "WARNING", KNET_LOG_WARN },
	{ "info", KNET_LOG_INFO },
	{ "debug", KNET_LOG_DEBUG }
};

const char *knet_log_get_loglevel_name(uint8_t level)
{
	unsigned int i;

	for (i = 0; i <= KNET_LOG_DEBUG; i++) {
		if (loglevel_names[i].val == level) {
			errno = 0;
			return loglevel_names[i].name;
		}
	}
	return "ERROR";
}

uint8_t knet_log_get_loglevel_id(const char *name)
{
	unsigned int i;

	for (i = 0; i <= KNET_LOG_DEBUG; i++) {
		if (strcasecmp(name, loglevel_names[i].name) == 0) {
			errno = 0;
			return loglevel_names[i].val;
		}
	}
	return KNET_LOG_ERR;
}

int knet_log_set_loglevel(knet_handle_t knet_h, uint8_t subsystem,
			  uint8_t level)
{
	int savederrno = 0;

	if (!_is_valid_handle(knet_h)) {
		return -1;
	}

	if (is_valid_subsystem(subsystem) < 0) {
		errno = EINVAL;
		return -1;
	}

	if (level > KNET_LOG_DEBUG) {
		errno = EINVAL;
		return -1;
	}

	savederrno = get_global_wrlock(knet_h);
	if (savederrno) {
		log_err(knet_h, subsystem, "Unable to get write lock: %s",
			strerror(savederrno));
		errno = savederrno;
		return -1;
	}

	knet_h->log_levels[subsystem] = level;

	pthread_rwlock_unlock(&knet_h->global_rwlock);
	errno = 0;
	return 0;
}

int knet_log_get_loglevel(knet_handle_t knet_h, uint8_t subsystem,
			  uint8_t *level)
{
	int savederrno = 0;

	if (!_is_valid_handle(knet_h)) {
		return -1;
	}

	if (is_valid_subsystem(subsystem) < 0) {
		errno = EINVAL;
		return -1;
	}

	if (!level) {
		errno = EINVAL;
		return -1;
	}

	savederrno = pthread_rwlock_rdlock(&knet_h->global_rwlock);
	if (savederrno) {
		log_err(knet_h, subsystem, "Unable to get write lock: %s",
			strerror(savederrno));
		errno = savederrno;
		return -1;
	}

	*level = knet_h->log_levels[subsystem];

	pthread_rwlock_unlock(&knet_h->global_rwlock);
	errno = 0;
	return 0;
}


static int send_formatted_msg(knet_handle_t knet_h, struct knet_log_msg *msg)
{
	size_t byte_cnt = 0;
	int len;

	while (byte_cnt < sizeof(struct knet_log_msg)) {
		len = write(knet_h->logfd, msg, sizeof(struct knet_log_msg) - byte_cnt);
		if (len <= 0) {
			return len;
		}
		byte_cnt += len;
	}
	return 0;
}

void log_msg(knet_handle_t knet_h, uint8_t subsystem, uint8_t msglevel,
	     const char *fmt, ...)
{
	va_list ap;
	struct knet_log_msg msg;
	int len;

	if ((!knet_h) ||
	    (subsystem == KNET_MAX_SUBSYSTEMS) ||
	    (msglevel > knet_h->log_levels[subsystem]))
		return;

	if (pthread_mutex_lock(&knet_h->logging_mutex) != 0) {
		return;
	}

	if (msglevel > knet_h->throttle_level) {
		/* Unthrottle after 40 messages just in case it's all been DEBUG messages */
		if (++knet_h->throttled_msg_count < 40) {
			goto out;
		}
		/* Throttle back up one level */
		if (knet_h->throttle_level <  KNET_LOG_MAX) {
			knet_h->throttle_level++;
			knet_h->throttled_msg_count = 0;
		}
	}

	if (knet_h->logfd <= 0)
		goto out;

	/* Try to send the saved msg first */
	if (knet_h->saved_msg.subsystem != KNET_SUB_NONE) {
		(void)send_formatted_msg(knet_h, &knet_h->saved_msg);
		/* If it goes, it goes. If not - we tried. */
		knet_h->saved_msg.subsystem = KNET_SUB_NONE;
	}

	memset(&msg, 0, sizeof(struct knet_log_msg));
	msg.subsystem = subsystem;
	msg.msglevel = msglevel;
	msg.knet_h = knet_h;

	va_start(ap, fmt);
#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wformat-nonliteral"
#endif
	vsnprintf(msg.msg, sizeof(msg.msg), fmt, ap);
#ifdef __clang__
#pragma clang diagnostic pop
#endif
	va_end(ap);

	len = send_formatted_msg(knet_h, &msg);
	if (len < 0 && errno == EAGAIN) {

		/* If it's "important" then save it and try again later */
		if (msglevel < knet_h->throttle_level) {
			memcpy(&knet_h->saved_msg, &msg, sizeof(struct knet_log_msg)); // Save it
		} else {
			knet_h->saved_msg.subsystem = KNET_SUB_NONE; /* Unimportant */
		}

		/* It's getting worse, throttle some more */
		if (knet_h->throttle_level > KNET_LOG_WARN) { // Don't throw away warnings and higher
			knet_h->throttle_level--;
		}
		goto out;
	}

	/* Something was sent, try to unblock a level */
	if (len == 0 && knet_h->throttle_level < KNET_LOG_MAX) {
		knet_h->throttle_level++;
	}
out:
	pthread_mutex_unlock(&knet_h->logging_mutex);
	return;
}

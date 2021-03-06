/*-
 * Copyright (c) 2010 Isilon Systems, Inc.
 * Copyright (c) 2010 iX Systems, Inc.
 * Copyright (c) 2010 Panasas, Inc.
 * Copyright (c) 2013-2015 Mellanox Technologies, Ltd.
 * Copyright (c) 2013 François Tigeot
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice unmodified, this list of conditions, and the following
 *    disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * $FreeBSD$
 */
/* Taken from FreeBSD and modified: Dylan Mayor */
#ifndef UKKREF_KREF_H
#define UKKREF_KREF_H
#ifdef __cplusplus
extern "C" {
#endif

#include <uk/plat/spinlock.h>
#include <uk/mutex.h>
#include <uk/refcount.h>

struct uk_kref {
    __atomic refcount;
};

static inline void
uk_kref_init(struct uk_kref *kref) {

    uk_refcount_init(&kref->refcount, 1);
}

static inline unsigned int
uk_kref_read(const struct uk_kref *kref) {

    return (uk_refcount_read(&kref->refcount));
}

static inline void
uk_kref_get(struct uk_kref *kref) {

    uk_refcount_acquire(&kref->refcount);
}

static inline int
uk_kref_put(struct uk_kref *kref, void (*rel)(struct uk_kref *kref)) {

    if (uk_refcount_release(&kref->refcount)) {
        rel(kref);
        return 1;
    }
    return 0;
}


static inline int
uk_kref_put_lock(struct uk_kref *kref, void (*rel)(struct kref *kref),
              spinlock_t *lock) {

    if (uk_refcount_release(&kref->refcount)) {
        spin_lock(lock);
        rel(kref);
        return (1);
    }
    return (0);
}


static inline int
uk_kref_sub(struct uk_kref *kref, unsigned int count,
         void (*rel)(struct uk_kref *kref)) {

    while (count--) {
        if (uk_refcount_release(&kref->refcount)) {
            rel(kref);
            return 1;
        }
    }
    return 0;
}

static inline int /*__must_check*/
uk_kref_get_unless_zero(struct uk_kref *kref) {

    return atomic_add_unless(&kref->refcount, 1, 0);
}

static inline int uk_kref_put_mutex(struct uk_kref *kref,
                                 void (*release)(struct uk_kref *kref), struct uk_mutex *lock) {
    //WARN_ON(release == NULL);
    if (unlikely(!atomic_add_unless(&kref->refcount, -1, 1))) {
        uk_mutex_lock(lock);
        if (unlikely(!atomic_dec_and_test(&kref->refcount))) {
            uk_mutex_unlock(lock);
            return 0;
        }
        release(kref);
        return 1;
    }
    return 0;
}

#ifdef __cplusplus
}
#endif

#endif /* _LINUX_KREF_H_ */

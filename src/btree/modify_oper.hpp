#ifndef __BTREE_MODIFY_OPER_HPP__
#define __BTREE_MODIFY_OPER_HPP__

#include "utils.hpp"
#include <boost/shared_ptr.hpp>
#include "btree/node.hpp"
#include "btree/slice.hpp"
#include "buffer_cache/buffer_cache.hpp"
#include "buffer_cache/buf_lock.hpp"
#include "buffer_cache/large_buf.hpp"
#include "buffer_cache/transactor.hpp"
#include "buffer_cache/co_functions.hpp"

#define BTREE_MODIFY_OPER_DUMMY_PROPOSED_CAS 0

/* Stats */
extern perfmon_counter_t pm_btree_depth;

class btree_modify_oper_t {
public:
    btree_modify_oper_t() : slice(NULL) { }

    virtual ~btree_modify_oper_t() { }

    /* run_btree_modify_oper() calls operate() when it reaches the leaf node.
     * 'old_value' is the previous value or NULL if the key was not present
     * before. 'old_large_buf' is the large buf for the old value, if the old
     * value is a large value. operate()'s return value indicates whether the
     * leaf should be updated. If it returns true, it's responsible for setting
     * *new_value and *new_large_buf to the correct new values (note that these
     * must match up). If *new_value is NULL, the value will be deleted (and
     * similarly for *new_large_buf).
     */
    virtual bool operate(const boost::shared_ptr<transactor_t>& txor, btree_value *old_value,
        boost::scoped_ptr<large_buf_t>& old_large_buflock, btree_value **new_value, boost::scoped_ptr<large_buf_t>& new_large_buflock) = 0;

    virtual int compute_expected_change_count(const size_t block_size) = 0;

    // These two variables are only used by the get_cas_oper; there should be a
    // nicer way to handle this.
    btree_slice_t *slice;

    // Acquires the old large value; this exists because some
    // btree_modify_opers need to acquire it in a particular way.

    // TIED lb TO lbref  TODO CHECK CALLERS
    virtual void actually_acquire_large_value(large_buf_t *lb) {
        thread_saver_t saver;
        co_acquire_large_buf(saver, lb);
    }

    // This is a dorky name.  This function shall be called
    // immediately after the superblock has been acquired.  The delete
    // queue is a child of the superblock, when it comes to
    // transactional ordering.
    virtual void do_superblock_sidequest(UNUSED boost::shared_ptr<transactor_t>& txor,
                                         UNUSED buf_lock_t& superblock,
                                         UNUSED repli_timestamp recency,
                                         UNUSED const store_key_t *key) { }
};

// Runs a btree_modify_oper_t.
void run_btree_modify_oper(btree_modify_oper_t *oper, btree_slice_t *slice, const store_key_t &key, castime_t castime);

buf_t *get_root(transaction_t *txn, buf_t **sb_buf, block_size_t block_size);
void insert_root(block_id_t root_id, buf_t **sb_buf);
void check_and_handle_split(transaction_t *txn, buf_t **buf, buf_t **last_buf, buf_t **sb_buf,
    const btree_key_t *key, btree_value *new_value, block_size_t block_size);
void check_and_handle_underfull(transaction_t *txn, buf_t **buf, buf_t **last_buf, buf_t **sb_buf,
    const btree_key_t *key, block_size_t block_size);


#endif // __BTREE_MODIFY_OPER_HPP__

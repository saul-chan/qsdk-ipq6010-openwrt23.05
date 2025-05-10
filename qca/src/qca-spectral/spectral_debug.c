#include "spectral_defs.h"

void print_current_prev_message_pools(void)
{
    PER_FREQ_SAMP_MSG_POOL *lower_pool = &lower;
    PER_FREQ_SAMP_MSG_POOL *upper_pool = &upper;

    SPECTRAL_DPRINTF(ATH_DEBUG_SPECTRAL,"%s current freq = %d, lower_pool=%p pool_freq=%d \n", __func__, g_current_freq, lower_pool->pool_ptr, lower_pool->freq);
    print_saved_message_pool(lower_pool->pool_ptr);

    SPECTRAL_DPRINTF(ATH_DEBUG_SPECTRAL,"%s prev freq = %d, upper_pool=%p pool_freq=%d\n", __func__, g_prev_freq, upper_pool->pool_ptr, upper_pool->freq);
    print_saved_message_pool(upper_pool->pool_ptr);
}

void print_saved_message_pool(SAVED_SAMP_MESSAGE *pool)
{
    int i=0;
    struct spectral_samp_msg *msg_ptr;

    for (i=0; i< MAX_SAVED_SAMP_MSGS; i++) {
        msg_ptr=&pool[i].saved_msg;
        SPECTRAL_DPRINTF(ATH_DEBUG_SPECTRAL, "[%d] <%p> msg_valid=%d msg_freq=%d\n", i, &pool[i], pool[i].msg_valid,pool[i].saved_msg.freq);
    }

}


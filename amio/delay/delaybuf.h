#ifndef __ZXY_DELAYBUF_H__
#define __ZXY_DELAYBUF_H__


typedef struct pjmedia_delay_buf pjmedia_delay_buf;
typedef enum pjmedia_delay_buf_flag
{
    PJMEDIA_DELAY_BUF_SIMPLE_FIFO = 1
} pjmedia_delay_buf_flag;


PJ_DECL(pj_status_t) pjmedia_delay_buf_create(pj_pool_t *pool,
											  const char *name,
											  unsigned clock_rate,
											  unsigned samples_per_frame,
											  unsigned channel_count,
											  unsigned max_delay,
											  unsigned options,
											  pjmedia_delay_buf **p_b);

PJ_DECL(pj_status_t) pjmedia_delay_buf_put(pjmedia_delay_buf *b, pj_int16_t frame[]);

PJ_DECL(pj_status_t) pjmedia_delay_buf_get(pjmedia_delay_buf *b, pj_int16_t frame[]);

PJ_DECL(pj_status_t) pjmedia_delay_buf_reset(pjmedia_delay_buf *b);

PJ_DECL(pj_status_t) pjmedia_delay_buf_destroy(pjmedia_delay_buf *b);

#endif	/* __ZXY_DELAYBUF_H__ */

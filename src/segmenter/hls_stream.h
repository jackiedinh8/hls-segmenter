/*
 * Copyright (c) 2015 Jackie Dinh <jackiedinh8@gmail.com>
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *  1 Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  2 Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the 
 *    documentation and/or other materials provided with the distribution.
 *  3 Neither the name of the <organization> nor the 
 *    names of its contributors may be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND 
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE 
 * DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY 
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND 
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * @(#)hls_bitread.c
 */


#ifndef _HLS_STREAM_H_
#define _HLS_STREAM_H_

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>

#include "hls_core.h"
#include "hls_buf.h"
#include "hls_mpegts.h"


typedef struct hls_codec_ctx  hls_codec_ctx_t;
struct hls_codec_ctx {
    uint32_t        width;
    uint32_t        height;
    uint32_t        duration;
    uint32_t        frame_rate;
    uint32_t        video_data_rate;
    uint32_t        video_codec_id;
    uint32_t        audio_data_rate;
    uint32_t        audio_codec_id;
    uint32_t        aac_profile;
    uint32_t        aac_chan_conf;
    uint32_t        aac_sbr;
    uint32_t        aac_ps;
    uint32_t        avc_profile;
    uint32_t        avc_compat;
    uint32_t        avc_level;
    uint32_t        avc_nal_bytes;
    uint32_t        avc_ref_frames;
    uint32_t        sample_rate;    /* 5512, 11025, 22050, 44100 */
    uint32_t        sample_size;    /* 1=8bit, 2=16bit */
    uint32_t        audio_channels; /* 1, 2 */
    char            profile[32];
    char            level[32];

    hls_buf_t      *avc_header;
    hls_buf_t      *aac_header;
    hls_buf_t      *meta;
    uint32_t        meta_version;
};

typedef struct hls_header hls_header_t;
struct hls_header {
};

typedef struct hls_stream hls_stream_t;
struct hls_stream {
   hls_header_t   header;
   uint32_t       dtime;
   uint32_t       len;        /* current fragment length */
   uint8_t        ext; 
   hls_chain_t   *in;
};

struct hls_stream_ctx {
   void              *session;
   AVFormatContext   *pFormatCtx;
   AVCodecContext    *pCodecCtxOrig;
   AVCodecContext    *pCodecCtx;
   AVCodecContext    *pACodecCtx;
   uint32_t           audio_idx;
   uint32_t           video_idx;
};

struct hls_session {
   void                 *ctx; // configuration, to be defined.
   uint32_t              current_ts;
   hls_stream_t         *in_streams;
   hls_codec_ctx_t      *codec_ctx;
   hls_stream_ctx_t     *stream_ctx;
   hls_mpegts_ctx_t     *mpegts_ctx;
};

hls_session_t*
hls_create_session(hls_context_t *ctx);

hls_session_t*
hls_segment_file(hls_context_t *ctx, const char* file);

void
hls_parse_acc_header(hls_session_t *s, hls_chain_t *in);



#endif // _HLS_STREAM_H_









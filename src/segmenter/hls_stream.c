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
 * @(#)hls_stream.c
 */


#include "hls_stream.h"
#include "hls_bitread.h"
#include "hls_utils.h"

hls_session_t*
hls_create_session(hls_context_t *ctx)
{
   hls_session_t *s;
   int len = 0;

   s = (hls_session_t*)malloc(sizeof(*s));
   if (s == NULL)
      return NULL;

   memset(s,0,sizeof(*s));

   s->stream_ctx = (hls_stream_ctx_t*)
                      malloc(sizeof(*s->stream_ctx)); 
   if (s->stream_ctx == NULL)
      return NULL;

   memset(s->stream_ctx,0,sizeof(*s->stream_ctx));

   s->codec_ctx = (hls_codec_ctx_t*)
                      malloc(sizeof(*s->codec_ctx)); 
   if (s->codec_ctx == NULL)
      return NULL;

   memset(s->codec_ctx,0,sizeof(*s->codec_ctx));

   s->mpegts_ctx = (hls_mpegts_ctx_t*)
                      malloc(sizeof(*s->mpegts_ctx)); 
   if (s->mpegts_ctx == NULL)
      return NULL;

   memset(s->mpegts_ctx,0,sizeof(*s->mpegts_ctx));

   s->mpegts_ctx->record = 1;

   s->mpegts_ctx->frags = malloc(sizeof(hls_frag_t)*(ctx->winfrags*2+1));
   if ( s->mpegts_ctx->frags == NULL)
      return NULL;

   memset(s->mpegts_ctx->frags,0,sizeof(hls_frag_t)*(ctx->winfrags*2+1));

   // set default values.
   //s->mpegts_ctx->name.data = "live";
   //s->mpegts_ctx->name.len = strlen("live");
   s->mpegts_ctx->name.data = ctx->name.data;
   s->mpegts_ctx->name.len = ctx->name.len;

   len = s->mpegts_ctx->name.len + ctx->path.len + 1;
   s->mpegts_ctx->stream.data = (char*)malloc(len);
   memset(s->mpegts_ctx->stream.data,0,len);
   
   memcpy(s->mpegts_ctx->stream.data, 
            ctx->path.data, ctx->path.len);
   memcpy(s->mpegts_ctx->stream.data + ctx->path.len, 
            s->mpegts_ctx->name.data, s->mpegts_ctx->name.len);
   s->mpegts_ctx->stream.len = len;
   s->mpegts_ctx->stream.data[len-1] = '-';
   
   len = ctx->path.len + s->mpegts_ctx->name.len + sizeof(".m3u8");
   s->mpegts_ctx->playlist.data = malloc(len);
   memset(s->mpegts_ctx->playlist.data,0,len);
   memcpy(s->mpegts_ctx->playlist.data, ctx->path.data,ctx->path.len);
   memcpy(s->mpegts_ctx->playlist.data + ctx->path.len, 
            s->mpegts_ctx->name.data,s->mpegts_ctx->name.len);
   memcpy(s->mpegts_ctx->playlist.data+ctx->path.len+s->mpegts_ctx->name.len,
            ".m3u8", sizeof(".m3u8")-1);
   s->mpegts_ctx->playlist.len = len;
   
   len = s->mpegts_ctx->playlist.len + sizeof(".bak");

   s->mpegts_ctx->playlist_bak.data = malloc(len);
   memset(s->mpegts_ctx->playlist_bak.data,0,len);
   memcpy(s->mpegts_ctx->playlist_bak.data, 
            s->mpegts_ctx->playlist.data,s->mpegts_ctx->playlist.len);

   memcpy(s->mpegts_ctx->playlist_bak.data+s->mpegts_ctx->playlist.len-1,
            ".bak", sizeof(".bak")-1);
   s->mpegts_ctx->playlist_bak.len = len;

   s->ctx = ctx;

   return s;
}


void
hls_codec_parse_avc_header(hls_session_t *s, char *buf, uint32_t size)
{
    hls_codec_ctx_t        *ctx =  s->codec_ctx;
    hls_bitread_t           br;  
    uint32_t                profile_idc, width, height, crop_left, crop_right,
                            crop_top, crop_bottom, frame_mbs_only, n, cf_idc,
                            num_ref_frames, bit;

    //hexdump(buf,size ,"avc");
    DEBUG("parsing avc header");

    hls_bitread_init(&br, buf, buf+size);
    
    /* see syntax of AVCDecoderConfigurationRecord  
       at ISO-14496-15, section 5.2.4.1.1 */
    //hls_bitread_get(&br, 48); 
    hls_bitread_get(&br, 8); 

    ctx->avc_profile = (uint32_t) hls_bitread_8(&br);
    ctx->avc_compat = (uint32_t) hls_bitread_8(&br);
    ctx->avc_level = (uint32_t) hls_bitread_8(&br);

    /* nal bytes */
    ctx->avc_nal_bytes = (uint32_t) ((hls_bitread_8(&br) & 0x03) + 1);

    /* nnals */
    if ((hls_bitread_8(&br) & 0x1f) == 0) { 
        return;
    }    

    /* nal size */
    hls_bitread_get(&br, 16); 

    /* nal type: see ISO-14496-10, section 7.3.1 of NAL unit syntax.
                 forbidden_zero_bit u(1), always zero if nothing wrong.
                 nal_ref_pic u(2), non-zero indicates SPS, PPS, etc.
                 nal_unit_type u(5), for SPS it is equal to 7.*/
    if (hls_bitread_8(&br) != 0x67) {
        return;
    }

    /* SPS*/

    /* profile idc */
    profile_idc = (uint32_t) hls_bitread_get(&br, 8);

    /* flags */
    hls_bitread_get(&br, 8);

    /* level idc */
    hls_bitread_get(&br, 8);

    /* SPS id */
    hls_bitread_golomb(&br);

    /* TODO: check profile_idc format */
    if (profile_idc == 100 || profile_idc == 110 ||
        profile_idc == 122 || profile_idc == 244 || profile_idc == 44 ||
        profile_idc == 83 || profile_idc == 86 || profile_idc == 118)
    {
        /* chroma format idc */
        cf_idc = (uint32_t) hls_bitread_golomb(&br);

        if (cf_idc == 3) {
            /* separate color plane */
            hls_bitread_get(&br, 1);
        }

        /* bit depth luma - 8 */
        hls_bitread_golomb(&br);

        /* bit depth chroma - 8 */
        hls_bitread_golomb(&br);

        /* qpprime y zero transform bypass */
        hls_bitread_get(&br, 1);

        /* seq scaling matrix present */
        if (hls_bitread_get(&br, 1)) {

            for (n = 0; n < (cf_idc != 3 ? 8u : 12u); n++) {

                /* seq scaling list present */
                if (hls_bitread_get(&br, 1)) {

                    /* TODO: scaling_list()
                    if (n < 6) {
                    } else {
                    }
                    */
                }
            }
        }
   }

    /* log2 max frame num */
    hls_bitread_golomb(&br);

    /* pic order cnt type */
    bit = hls_bitread_golomb(&br);
    switch (bit) {
    case 0:

        /* max pic order cnt */
        hls_bitread_golomb(&br);
        break;

    case 1:

        /* delta pic order alwys zero */
        hls_bitread_get(&br, 1);

        /* offset for non-ref pic */
        hls_bitread_golomb(&br);

        /* offset for top to bottom field */
        hls_bitread_golomb(&br);

        /* num ref frames in pic order */
        num_ref_frames = (uint32_t) hls_bitread_golomb(&br);

        for (n = 0; n < num_ref_frames; n++) {

            /* offset for ref frame */
            hls_bitread_golomb(&br);
        }
    }

    /* num ref frames */
    ctx->avc_ref_frames = (uint32_t) hls_bitread_golomb(&br);

    /* gaps in frame num allowed */
    hls_bitread_get(&br, 1);

    /* pic width in mbs - 1 */
    width = (uint32_t) hls_bitread_golomb(&br);

    /* pic height in map units - 1 */
    height = (uint32_t) hls_bitread_golomb(&br);

    /* frame mbs only flag */
    frame_mbs_only = (uint32_t) hls_bitread_get(&br, 1);

    if (!frame_mbs_only) {

        /* mbs adaprive frame field */
        hls_bitread_get(&br, 1);
    }

    /* direct 8x8 inference flag */
    hls_bitread_get(&br, 1);

    /* frame cropping */
    if (hls_bitread_get(&br, 1)) {

        crop_left = (uint32_t) hls_bitread_golomb(&br);
        crop_right = (uint32_t) hls_bitread_golomb(&br);
        crop_top = (uint32_t) hls_bitread_golomb(&br);
        crop_bottom = (uint32_t) hls_bitread_golomb(&br);

    } else {

        crop_left = 0;
        crop_right = 0;
        crop_top = 0;
        crop_bottom = 0;
    }

    ctx->width = (width + 1) * 16 - (crop_left + crop_right) * 2;
    ctx->height = (2 - frame_mbs_only) * (height + 1) * 16 -
                  (crop_top + crop_bottom) * 2;

}

void
hls_codec_parse_aac_header(hls_session_t *s, char *buf, uint32_t size)
{
   hls_codec_ctx_t *ctx = s->codec_ctx;
   hls_bitread_t    br;
   uint32_t         idx; 
   static uint32_t  aac_sample_rates[] =
                            { 96000, 88200, 64000, 48000,
                              44100, 32000, 24000, 22050,
                              16000, 12000, 11025,  8000,
                               7350,     0,     0,     0 }; 

   //hexdump(buf,size,"aac");
   DEBUG("parsing acc header");

   hls_bitread_init(&br, buf, buf+size);

   //hls_bitread_get(&br, 16); 

   ctx->aac_profile = (uint32_t) hls_bitread_get(&br, 5);
   if (ctx->aac_profile == 31) {
       ctx->aac_profile = (uint32_t) hls_bitread_get(&br, 6) + 32;
   }    

    idx = (uint32_t) hls_bitread_get(&br, 4);
    if (idx == 15) {
        ctx->sample_rate = (uint32_t) hls_bitread_get(&br, 24); 
    } else {
        ctx->sample_rate = aac_sample_rates[idx];
    }    

    ctx->aac_chan_conf = (uint32_t) hls_bitread_get(&br, 4);
    if (ctx->aac_profile == 5 || ctx->aac_profile == 29) {

        if (ctx->aac_profile == 29) {
            ctx->aac_ps = 1;
        }

        ctx->aac_sbr = 1;

        idx = (uint32_t) hls_bitread_get(&br, 4);
        if (idx == 15) {
            ctx->sample_rate = (uint32_t) hls_bitread_get(&br, 24);
        } else {
            ctx->sample_rate = aac_sample_rates[idx];
        }

        ctx->aac_profile = (uint32_t) hls_bitread_get(&br, 5);
        if (ctx->aac_profile == 31) {
            ctx->aac_profile = (uint32_t) hls_bitread_get(&br, 6) + 32;
        }
    }
    /* MPEG-4 Audio Specific Config

       5 bits: object type
       if (object type == 31)
         6 bits + 32: object type
       4 bits: frequency index
       if (frequency index == 15)
         24 bits: frequency
       4 bits: channel configuration

       if (object_type == 5)
           4 bits: frequency index
           if (frequency index == 15)
             24 bits: frequency
           5 bits: object type
           if (object type == 31)
             6 bits + 32: object type
             
       var bits: AOT Specific Config
     */


   return;
}
int32_t
hls_handle_h264_header(hls_session_t *s, int videoStream)
{
   hls_codec_ctx_t   *codec_ctx =  s->codec_ctx;
   hls_stream_ctx_t  *ctx =  s->stream_ctx;
   AVCodecContext    *pCodecCtxOrig;
   AVCodec           *pCodec = NULL;// set NULL to prevent segfaults!
   hls_buf_t         *avc_header;

   pCodecCtxOrig=ctx->pFormatCtx->streams[videoStream]->codec;
   codec_ctx->video_codec_id = 0;
   if (pCodecCtxOrig->codec_id != AV_CODEC_ID_H264 ) {
      ERROR("unsupported video codec, id=%d, AV_CODEC_ID_H264=%d", 
            pCodecCtxOrig->codec_id,AV_CODEC_ID_H264);
      return -1;
   }
   codec_ctx->video_codec_id = AV_CODEC_ID_H264;
/*
   pCodec=avcodec_find_decoder(pCodecCtxOrig->codec_id);
   if(pCodec==NULL) {
       ERROR("Unsupported codec");
       return -1;
   }

   ctx->pCodecCtx = avcodec_alloc_context3(pCodec);
   if(avcodec_copy_context(ctx->pCodecCtx, pCodecCtxOrig) != 0) {
       ERROR("Couldn't copy codec context");
       return -2;
   }

   if(avcodec_open2(ctx->pCodecCtx, pCodec, NULL)<0)
      return -3;
*/
   // TODO: get avc info.
   if (pCodecCtxOrig->extradata) {

      hls_codec_parse_avc_header(s,pCodecCtxOrig->extradata,
                     pCodecCtxOrig->extradata_size);

      avc_header = (hls_buf_t*)malloc(sizeof(*avc_header)); 
      if ( avc_header == NULL )
         return -1;
      avc_header->pos = malloc(pCodecCtxOrig->extradata_size);
      if ( avc_header->pos == NULL )
         return -2;
      avc_header->start = avc_header->pos;
      avc_header->last = avc_header->pos + pCodecCtxOrig->extradata_size;
      memcpy(avc_header->pos,pCodecCtxOrig->extradata, pCodecCtxOrig->extradata_size);

      codec_ctx->avc_header = avc_header;

   } else {
      codec_ctx->avc_profile = pCodecCtxOrig->profile;
      codec_ctx->avc_compat = 0; 
      codec_ctx->avc_level = pCodecCtxOrig->level;
      codec_ctx->avc_nal_bytes = 0; 
      codec_ctx->avc_ref_frames = pCodecCtxOrig->refs;
      codec_ctx->width = pCodecCtxOrig->width;
      codec_ctx->height = pCodecCtxOrig->height;
   }

   DEBUG("avc_profile=%u,avc_compat=%u,"
         "avc_level=%u,avc_nal_bytes=%u,"
         "avc_ref_frames=%u,width=%u,height=%un",
          codec_ctx->avc_profile,
          codec_ctx->avc_compat,
          codec_ctx->avc_level,
          codec_ctx->avc_nal_bytes,
          codec_ctx->avc_ref_frames,
          codec_ctx->width,
          codec_ctx->height);

   return 0; 
}

int32_t
hls_handle_aac_header(hls_session_t *s, int audioStream)
{
   hls_codec_ctx_t  *codec_ctx =  s->codec_ctx;
   hls_stream_ctx_t *ctx =  s->stream_ctx;
   AVCodecContext    *pCodecCtxOrig;
   AVCodec          *pCodec = NULL;// set NULL to prevent segfaults!
   hls_buf_t        *aac_header;

   pCodecCtxOrig=ctx->pFormatCtx->streams[audioStream]->codec;
   codec_ctx->audio_codec_id = 0;
   if ( pCodecCtxOrig->codec_id != AV_CODEC_ID_AAC ) {
      ERROR("unsupported audio codec, id=%d, AV_CODEC_ID_AAC=%d", 
            pCodecCtxOrig->codec_id,AV_CODEC_ID_AAC);
      return -1;
   }
   codec_ctx->audio_codec_id = AV_CODEC_ID_AAC;

/*
   pCodec=avcodec_find_decoder(pCodecCtxOrig->codec_id);
   if(pCodec==NULL) {
       DEBUG("Unsupported codec!\n");
       return 0;
   }

   ctx->pACodecCtx = avcodec_alloc_context3(pCodec);
   if(avcodec_copy_context(ctx->pACodecCtx, pCodecCtxOrig) != 0) {
       ERROR("Couldn't copy codec context");
       return 0;
   }

   if(avcodec_open2(ctx->pACodecCtx, pCodec, NULL)<0)
      return 0;
*/

   // TODO: get codec info.
   codec_ctx->aac_profile = 0;
   codec_ctx->sample_rate = 0;
   codec_ctx->aac_chan_conf = 0;
   codec_ctx->aac_ps = 0;
   codec_ctx->aac_sbr = 0;

   if (pCodecCtxOrig->extradata) {

      hls_codec_parse_aac_header(s,pCodecCtxOrig->extradata,
                     pCodecCtxOrig->extradata_size);

      aac_header = (hls_buf_t*)malloc(sizeof(*aac_header)); 
      if ( aac_header == NULL )
         return -1;
      aac_header->pos = malloc(pCodecCtxOrig->extradata_size);
      if ( aac_header->pos == NULL )
         return -2;
      aac_header->start = aac_header->pos;
      aac_header->last = aac_header->pos + pCodecCtxOrig->extradata_size;
      memcpy(aac_header->pos,pCodecCtxOrig->extradata, pCodecCtxOrig->extradata_size);

      codec_ctx->aac_header = aac_header;

   } else {

   }

   DEBUG("aac_profile=%u,sample_rate=%u,"
           "aac_chan_conf=%u,aac_ps=%u,aac_sbr=%u",
           codec_ctx->aac_profile,
           codec_ctx->sample_rate,
           codec_ctx->aac_chan_conf,
           codec_ctx->aac_ps,
           codec_ctx->aac_sbr);

   return 0;
}

hls_session_t*
hls_segment_file(hls_context_t *hls_ctx, const char* file)
{
   hls_session_t        *session;
   hls_stream_ctx_t     *ctx;
   int                   frameFinished;
   int                   i = 0;

   session = hls_create_session(hls_ctx);
   if (session == NULL ) {
      ERROR("creating hls session failed\n");
      return 0;
   }
   ctx = session->stream_ctx;
   ctx->session = session;

   av_register_all();
   if(avformat_open_input(&ctx->pFormatCtx, file, NULL, NULL)!=0)
      return 0;
  
   if(avformat_find_stream_info(ctx->pFormatCtx, NULL)<0)
       return 0;

   // Dump information about file onto standard error
   //av_dump_format(ctx->pFormatCtx, 0, file, 0);
  
   // Find the first audio&video stream
   ctx->audio_idx = -1;
   ctx->video_idx = -1;
   for(i=0; i<ctx->pFormatCtx->nb_streams; i++) {
      if(ctx->pFormatCtx->streams[i]->codec->codec_type==AVMEDIA_TYPE_VIDEO)
          ctx->video_idx=i;
      
      if(ctx->pFormatCtx->streams[i]->codec->codec_type==AVMEDIA_TYPE_AUDIO)
          ctx->audio_idx=i;

      if(ctx->video_idx!=-1 && ctx->audio_idx!=-1)
         break;
   }

   if(ctx->video_idx==-1 || ctx->audio_idx==-1)
      return 0;

   hls_write_mpegts(session);

   return 0;
}

int
hls_copy(hls_session_t *s, void *dst, char **src, size_t n, hls_buf_t *buf)
{
    char    *last;
    size_t   pn;

    if (buf == NULL) {
        return -1;
    }

    last = buf->last;

    //fprintf(stderr, "hls: read %u byte(s), len=%u, last=%p, *src=%p\n", 
    //                 n, last - *src, last, *src);
    if ((size_t)(last - *src) >= n) {
        if (dst) {
           memcpy(dst, *src, n);
        }

        *src += n;
        return 0;
    }

    ERROR("error reading buffer of %u byte(s)", n);

    return -1;
}


int
hls_parse_aac_header(hls_session_t *s, uint64_t *objtype,
    uint64_t *srindex, uint64_t *chconf)
{
    hls_codec_ctx_t   *codec_ctx;
    hls_buf_t            *cl;
    char                 *p, b0, b1;

    codec_ctx = s->codec_ctx;

    if ( codec_ctx->aac_header == NULL ) {
       return -1;
    }

    cl = codec_ctx->aac_header;

    //hexdump(cl->pos,cl->last-cl->pos,"aac");
    p = cl->pos;

    //if (hls_copy(s, NULL, &p, 2, cl) != 0) {
    //    return -1;
    //}

    if (hls_copy(s, &b0, &p, 1, cl) != 0) {
        return -1;
    }

    if (hls_copy(s, &b1, &p, 1, cl) != 0) {
        return -1;
    }

    *objtype = b0 >> 3;
    if (*objtype == 0 || *objtype == 0x1f) {
        fprintf(stderr, "hls: unsupported adts object type:%ui\n", *objtype);
        return -1;
    }

    if (*objtype > 4) {

        /*
         * Mark all extended profiles as LC
         * to make Android as happy as possible.
         */

        *objtype = 2;
    }

    *srindex = ((b0 << 1) & 0x0f) | ((b1 & 0x80) >> 7);
    if (*srindex == 0x0f) {
        fprintf(stderr, "hls: unsupported adts sample rate:%ui", *srindex);
        return -1;
    }

    *chconf = (b1 >> 3) & 0x0f;

    //fprintf(stderr, "hls: aac object_type:%u, sample_rate_index:%u, "
    //               "channel_config:%u\n", *objtype, *srindex, *chconf);

    return 0;
}



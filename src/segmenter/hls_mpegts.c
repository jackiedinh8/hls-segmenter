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
 * @(#)hls_mpegts.c
 */

#include <fcntl.h>
#include <stdio.h>
#include <errno.h>

#include "hls_mpegts.h"
#include "hls_stream.h"
#include "hls_utils.h"


static char hls_mpegts_header[] = {

    /* TS */
    0x47, 0x40, 0x00, 0x10, 0x00,
    /* PSI */
    0x00, 0xb0, 0x0d, 0x00, 0x01, 0xc1, 0x00, 0x00,
    /* PAT */
    0x00, 0x01, 0xf0, 0x01,
    /* CRC */
    0x2e, 0x70, 0x19, 0x05,
    /* stuffing 167 bytes */
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,

    /* TS */
    0x47, 0x50, 0x01, 0x10, 0x00,
    /* PSI */
    0x02, 0xb0, 0x17, 0x00, 0x01, 0xc1, 0x00, 0x00,
    /* PMT */
    0xe1, 0x00,
    0xf0, 0x00,
    0x1b, 0xe1, 0x00, 0xf0, 0x00, /* h264 */
    0x0f, 0xe1, 0x01, 0xf0, 0x00, /* aac */
    /*0x03, 0xe1, 0x01, 0xf0, 0x00,*/ /* mp3 */
    /* CRC */
    0x2f, 0x44, 0xb9, 0x9b, /* crc for aac */
    /*0x4e, 0x59, 0x3d, 0x1e,*/ /* crc for mp3 */
    /* stuffing 157 bytes */
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff
};

/* 700 ms PCR delay */
#define HLS_DELAY  63000


int
hls_flush_audio(hls_session_t *s)
{
    hls_mpegts_ctx_t             *ctx;
    hls_mpegts_frame_t         frame;
    int64_t                       rc;  
    hls_buf_t                      *b;  

    ctx = s->mpegts_ctx;

    //DEBUG( "hls: flush audio, opened=%u\n", ctx->opened);
    if (ctx == NULL || !ctx->opened) {
        return 0;
    }    

    b = ctx->aframe;
    
    if (b == NULL || b->pos == b->last) {
        return 0;
    }    

    //hexdump(b->pos,b->last-b->pos,"hls4");

    memset(&frame, 0,sizeof(frame));

    frame.dts = ctx->aframe_pts;
    frame.pts = frame.dts;
    frame.cc = ctx->audio_cc;
    frame.pid = 0x101;
    frame.sid = 0xc0;

    //DEBUG( "hls: flush audio, pts=%u", frame.pts);

    rc = hls_mpegts_write_frame(&ctx->file, &frame, b);

    if (rc != 0) {
        DEBUG("hls: audio flush failed");
    }

    ctx->audio_cc = frame.cc;
    b->pos = b->last = b->start;

    return rc;
}


hls_frag_t*
hls_get_frag(hls_session_t *s, uint32_t n)
{
   hls_context_t    *hctx = (hls_context_t*)s->ctx;
   hls_mpegts_ctx_t *ctx = s->mpegts_ctx;

   return  &ctx->frags[(ctx->frag + n) % (hctx->winfrags * 2 + 1)];
}

int
hls_mpegts_close_file(hls_mpegts_file_t *file)
{
   close(file->fd);

   return 0;
}

int
hls_next_frag(hls_session_t *s)
{
   hls_context_t *ctx = s->ctx;
   hls_mpegts_ctx_t *hctx = s->mpegts_ctx;

   if ( hctx->nfrags == ctx->winfrags )
      hctx->frag++;
   else 
      hctx->nfrags++;

   return 0;
}

int
hls_write_record(hls_session_t *s)
{
    static char       buffer[1024];
    hls_str_t         name_part, key_name_part;
    const char       *sep, *key_sep;
    uint64_t          i, max_frag;
    uint64_t          prev_key_id;
    char             *p, *end;
    hls_mpegts_ctx_t *ctx;
    hls_context_t    *hctx;
    hls_frag_t       *f;  
    int               fd;  
    ssize_t           n;   


    ctx = s->mpegts_ctx;
    hctx = s->ctx;

    fd = hls_open_file(ctx->playlist.data, 
               O_WRONLY, O_CREAT|O_APPEND, 0644);

    if (fd == -1) {
        ERROR("hls: open failed: %s", &ctx->playlist.data);
        return -1;
    }    

    max_frag = hctx->fraglen / 1000;

    p = buffer;
    end = p + sizeof(buffer);

    INFO("record: nfrags=%u,p=%p,end=%p\n",ctx->nfrags,p,end);
    if ( ctx->record && !ctx->on_record ) {
       DEBUG("record: writing m3u8 header\n"); 
       n = snprintf(p, end - p,
                     "#EXTM3U\n"
                     "#EXT-X-VERSION:3\n"
                     "#EXT-X-MEDIA-SEQUENCE:%u\n"
                     "#EXT-X-TARGETDURATION:%u\n",
                     ctx->frag, max_frag);

       if ( n >= sizeof(buffer) ) {
          DEBUG("snprintf failed.");
       }
       p += n;

       if (hctx->type == HLS_TYPE_EVENT) {
           n = snprintf(p, end - p, "#EXT-X-PLAYLIST-TYPE: EVENT\n");
           if ( n < end - p) {
              p += n;
           } else {
              ERROR("hls: writing failed: file=%s,ret=%d",
                         &ctx->playlist.data,n);
              return -1;
           }
       }

       n = write(fd, buffer, p - buffer);

       if (n < 0) {
          ERROR("hls:writing failed: file=%s,ret=%d",
                         &ctx->playlist.data,n);
          close(fd);
          return -2;
       }

       ctx->on_record = 1;

       // reset buffer; 
       p = buffer;
       end = p + sizeof(buffer);
    }


    sep = hctx->nested ? (hctx->base_url.len ? "/" : "") : "-";
    key_sep = hctx->nested ? (hctx->key_url.len ? "/" : "") : "-";

    name_part.len = 0;
    if (!hctx->nested || hctx->base_url.len) {
        name_part = ctx->name;
    }

    key_name_part.len = 0;
    if (!hctx->nested || hctx->key_url.len) {
        key_name_part = ctx->name;
    }

    // current frag: nfrags + frag - 1;
    f = hls_get_frag(s, ctx->nfrags-1);

    DEBUG("frag=%u,duration=%f,base_url=%s,name=%s,sep=%s,id=%u,i=%u",
          ctx->nfrags-1,f->duration, hctx->base_url.data, 
          name_part.data, sep, f->id,i);

    n = snprintf(p, end-p,
                     "#EXTINF:%.3f,\n"
                     "%s%s%s%u.ts\n",
                     f->duration, hctx->base_url.data, 
                     name_part.data, sep, f->id);

    if ( n < end - p) {
       p += n;
    } else {
       ERROR("hls:writing failed: file=%s,ret=%d,test=%u",
                  ctx->playlist.data,n,end-p);
       return -1;
    }

    n = write(fd, buffer, p - buffer);
    if (n < 0) {
        ERROR("hls:writing failed, file=%s, ret=%d",
                      ctx->playlist.data,n);
        close(fd);
        return -3;
    }

    close(fd);
    return 0;
}

int
hls_write_playlist(hls_session_t *s)
{
    static char       buffer[1024];
    hls_str_t         name_part, key_name_part;
    const char       *sep, *key_sep;
    uint64_t          i, max_frag;
    uint64_t          prev_key_id;
    char             *p, *end;
    hls_mpegts_ctx_t *ctx;
    hls_context_t    *hctx;
    hls_frag_t       *f;  
    int               fd;  
    ssize_t           n;   


    ctx = s->mpegts_ctx;
    hctx = s->ctx;

    fd = hls_open_file(ctx->playlist_bak.data, O_WRONLY,
                       O_CREAT|O_TRUNC, 0644);

    if (fd == -1) {
        DEBUG( "hls: open failed: %s\n", &ctx->playlist_bak.data);
        return -1;
    }    

    max_frag = hctx->fraglen / 10000;

    for (i = 0; i < ctx->nfrags; i++) {
        f = hls_get_frag(s, i);
        if (f->duration > max_frag) {
            max_frag = (uint64_t) (f->duration + .5); 
        }    
    }    

    p = buffer;
    end = p + sizeof(buffer);

    DEBUG("live: writing m3u8 header\n"); 

    n = snprintf(p, end - p,
                     "#EXTM3U\n"
                     "#EXT-X-VERSION:3\n"
                     "#EXT-X-MEDIA-SEQUENCE:%u\n"
                     "#EXT-X-TARGETDURATION:%u\n",
                     ctx->frag, max_frag);

    if ( n >= sizeof(buffer) ) {
       DEBUG("snprintf failed.\n");
    }
    p += n;

    if (hctx->type == HLS_TYPE_EVENT) {
        n = snprintf(p, end - p, "#EXT-X-PLAYLIST-TYPE: EVENT\n");
        if ( n < end - p) {
           p += n;
        } else {
           DEBUG( "hls: writing failed: file=%s,ret=%d",
                      &ctx->playlist_bak.data,n);
           return -1;
        }
    }

    n = write(fd, buffer, p - buffer);

    if (n < 0) {
        DEBUG( "hls: writing failed: file=%s,ret=%d",
                      &ctx->playlist_bak.data,n);

        close(fd);
        return -2;
    }

    sep = hctx->nested ? (hctx->base_url.len ? "/" : "") : "-";
    key_sep = hctx->nested ? (hctx->key_url.len ? "/" : "") : "-";

    name_part.len = 0;
    if (!hctx->nested || hctx->base_url.len) {
        name_part = ctx->name;
    }

    DEBUG("nested=%u,base_url.len=%u, name.data=%s, name_part.data=%s\n", 
        hctx->nested, hctx->base_url.len, ctx->name.data,name_part.data);

    key_name_part.len = 0;
    if (!hctx->nested || hctx->key_url.len) {
        key_name_part = ctx->name;
    }

    prev_key_id = 0;

    for (i = 0; i < ctx->nfrags; i++) {
        f = hls_get_frag(s, i);

        p = buffer;
        end = p + sizeof(buffer);

        if (f->discont) {

            n = snprintf(p, end-p, "#EXT-X-DISCONTINUITY\n");
            if ( n < end - p) {
               p += n;
            } else {
               DEBUG( "hls: writing failed: file=%s,ret=%d",
                          &ctx->playlist_bak.data,n);
               return -1;
            }
        }
        DEBUG("keys=%u\n",hctx->keys);
        if (hctx->keys && (i == 0 || f->key_id != prev_key_id)) {

            n = snprintf(p, end-p, "#EXT-X-KEY:METHOD=AES-128,"
                             "URI=\"%s%s%s%u.key\",IV=0x%032X\n",
                             &hctx->key_url.data, &key_name_part.data,
                             key_sep, f->key_id, f->key_id);

            if ( n < end - p) {
               p += n;
            } else {
               DEBUG( "hls: writing failed: file=%s,ret=%d",
                          &ctx->playlist_bak.data,n);
               return -1;
            }

        }

        prev_key_id = f->key_id;

        DEBUG("duration=%f,base_url=%s,name=%s,sep=%s,id=%u\n",
           f->duration, hctx->base_url.data, name_part.data, sep, f->id);

        n = snprintf(p, end-p,
                         "#EXTINF:%.3f,\n"
                         "%s%s%s%u.ts\n",
                         f->duration, hctx->base_url.data, name_part.data, sep, f->id);
        if ( n < end - p) {
           p += n;
        } else {
           DEBUG( "hls: writing failed: file=%s,ret=%d",
                      ctx->playlist_bak.data,n);
           return -1;
        }

        DEBUG( "hls: fragment frag=%uL, n=%ui/%ui, duration=%.3f, discont=%i",
                       ctx->frag, i + 1, ctx->nfrags, f->duration, f->discont);

        n = write(fd, buffer, p - buffer);
        if (n < 0) {
            DEBUG( "hls: writing failed, file=%s, ret=%d",
                          ctx->playlist_bak.data,n);
            close(fd);
            return -3;
        }
    }

    close(fd);

    if (rename(ctx->playlist_bak.data, ctx->playlist.data) == -1) {

        DEBUG( "hls: rename failed: '%s'->'%s'",
                      ctx->playlist_bak.data, ctx->playlist.data);
        return -4;
    }

    return 0;
}

int
hls_close_fragment(hls_session_t *s)
{
   hls_mpegts_ctx_t *ctx = s->mpegts_ctx;

   if ( ctx == NULL || !ctx->opened )
      return 0;

   hls_mpegts_close_file(&ctx->file);
   
   ctx->opened = 0; 

   hls_next_frag(s);

   if ( ctx->record )
      hls_write_record(s);
   else 
      hls_write_playlist(s);

   return 0;
}

static uint64_t
hls_get_fragment_id(hls_session_t *s, uint64_t ts)
{
    hls_mpegts_ctx_t        *ctx;
    hls_context_t           *hctx;

    ctx = s->mpegts_ctx;

    hctx = s->ctx;

    switch (hctx->naming) {

    case HLS_NAMING_TIMESTAMP:
        return ts;

    case HLS_NAMING_SYSTEM:
        return ts;
        //return (uint64_t) ngx_cached_time->sec * 1000 + ngx_cached_time->msec;

    default: // HLS_NAMING_SEQUENTIAL
        return ctx->frag + ctx->nfrags;
    }
}

int
hls_mpegts_write_file(hls_mpegts_file_t *file, char *in, size_t in_size)
{
    char     *out;
    size_t    out_size, n;
    ssize_t   rc;

    static char  buf[1024];

    if (!file->encrypt) {

        rc = write(file->fd, in, in_size);

        if (rc < 0) {
            return -1;
        }

    }
    return 0;
}

int
hls_mpegts_write_header(hls_mpegts_file_t *file)
{
    return hls_mpegts_write_file(file, hls_mpegts_header,
                                      sizeof(hls_mpegts_header));
}

int
hls_mpegts_open_file(hls_mpegts_file_t *file, char *path)
{
    //file->fd = hls_open_file(path, O_WRONLY, O_TRUNC, 0644);
    file->fd = open(path, O_CREAT|O_WRONLY|O_TRUNC, 0644);

    if (file->fd < 0) {
        DEBUG( "hls: error creating fragment file, path=%s, ret=%d\n",
                   path, file->fd);
        return -1;
    }   

    file->size = 0;
    file->encrypt = 0;

    if (hls_mpegts_write_header(file) != 0) {
        DEBUG( "hls: error writing fragment header\n");
        close(file->fd);
        return -1;
    }   

    return 0;
}


int
hls_open_fragment(hls_session_t *s, uint64_t ts, uint64_t discont)
{
    hls_mpegts_ctx_t  *ctx;
    hls_frag_t        *f;  
    hls_context_t     *hctx;
    uint64_t           id;  
    int                fd;  
    uint64_t           g;   

    ctx = s->mpegts_ctx;
    hctx = s->ctx;

    if (ctx->opened) {
        return 0;
    }    

    INFO("hls: open fragment, opened=%u, key=%u, granularity=%u",
                  ctx->opened,hctx->keys,hctx->granularity);

    id = hls_get_fragment_id(s, ts); 

    if (hctx->granularity) {
        g = (uint64_t) hctx->granularity;
        id = (uint64_t) (id / g) * g; 
    }    

    sprintf(ctx->stream.data + ctx->stream.len, "%u.ts", id); 

    DEBUG("hls: open fragment file='%s', keyfile='%s', "
                "frag=%u, nfrags=%u, time=%u, discont=%u",
                ctx->stream.data,
                ctx->keyfile.data ? ctx->keyfile.data : (char *) "",
                ctx->frag, ctx->nfrags, ts, discont);

    if (hls_mpegts_open_file(&ctx->file, ctx->stream.data) != 0) {
        ERROR("hls: failed to open mpegts file");
        return -1;
    }

    ctx->opened = 1;

    f = hls_get_frag(s, ctx->nfrags);

    memset(f, 0,sizeof(*f));

    f->active = 1;
    f->discont = discont;
    f->id = id;
    f->key_id = ctx->key_id;

    ctx->frag_ts = ts;

    /* start fragment with audio to make iPhone happy */

    hls_flush_audio(s);

    return 0;
}

int
hls_update_fragments(hls_session_t *s, uint64_t ts, int boundary, int flush_rate)
{
   hls_context_t    *hctx = (hls_context_t*)s->ctx;
   hls_mpegts_ctx_t *ctx = s->mpegts_ctx;
   hls_frag_t       *f;
   uint64_t          ts_frag_len;
   uint64_t          same_frag,force,discont;
   hls_buf_t        *b;
   int64_t           d;

   f = NULL;
   force = 0;
   discont = 1;

   //DEBUG("boundary=%u,nfrags=%u,frag_ts=%lu,opened=%u,slicing=%u\n", 
   //            boundary,ctx->nfrags, ctx->frag_ts, ctx->opened,hctx->slicing);
   
   if ( ctx->opened ) {
      f = hls_get_frag(s,ctx->nfrags);
      d = (int64_t)(ts - ctx->frag_ts);
      if (d > (int64_t) hctx->max_fraglen*90 ||  d < -90000) {
         ERROR("hls: force fragment split: %.3f sec\n ", d / 90000.);
         force = 1;
      } else {
         f->duration = (ts - ctx->frag_ts) / 90000.;
         discont = 0;
      }
   }

   switch(hctx->slicing) {
      case 1: // slicing plain.
            if ( f && f->duration < hctx->fraglen/10000.) {
               //DEBUG("reset boundary, boundary=%u, duration=%lf, test=%lf\n",
               //            boundary,f->duration, hctx->fraglen/10000.);
               boundary = 0;
            }
            break;

      case 2:  //slicing aligned.

            ts_frag_len = hctx->fraglen * 90;
            same_frag = ctx->frag_ts / ts_frag_len == ts / ts_frag_len;

            if (f && same_frag) {
                boundary = 0;
            }

            if (f == NULL && (ctx->frag_ts == 0 || same_frag)) {
                ctx->frag_ts = ts;
                boundary = 0;
            }

        break;
   }

   if ( boundary || force ) {
      hls_close_fragment(s);
      hls_open_fragment(s,ts,discont);
   }

   b = ctx->aframe;

   if ( ctx->opened && b && b->last > b->pos && 
        ctx->aframe_pts + (uint64_t)hctx->max_audio_delay*90/flush_rate < ts ) {

      hls_flush_audio(s);
   }

   return 0;
}



int
hls_audio_handler(hls_session_t *s, AVPacket *packet)
{
   hls_context_t    *hctx = (hls_context_t*)s->ctx;
   hls_mpegts_ctx_t *ctx = s->mpegts_ctx;
   hls_codec_ctx_t  *codec_ctx = s->codec_ctx;
   hls_buf_t        *b;
   size_t            bsize;
   uint64_t          pts,est_pts;
   int64_t           dpts;
   char             *p = packet->data;
   uint64_t          objtype, srindex, chconf, size;

   b = ctx->aframe;

   if ( b == NULL ) {

      b = (hls_buf_t*)hls_alloc(sizeof(hls_buf_t));
      if ( b == NULL )
         return -1;

      b->start = hls_alloc(hctx->audio_buffer_size);
      if ( b->start == NULL )
         return -2;

      b->end = b->start + hctx->audio_buffer_size;
      b->pos = b->last = b->start;

      ctx->aframe = b;
   }

   size = packet->size + 7; // reserve 7 bytes for headers.
   pts = packet->pts * 90; //XXX: meaning of 90?

   if ( b->start + size > b->end ) {
      ERROR("too big audio frame, size=%u",size);
      return 0;
   }

   // TODO: boundary.
   hls_update_fragments(s,pts,0,2);

   if (b->last + size > b->end) {
        hls_flush_audio(s);
   }    

   //hexdump(b->pos, b->last-b->pos, "af0");
   //DEBUG( "hls: audio pts=%uL\n", pts);


   if (b->last + 7 > b->end) {
       ERROR("not enough buffer for audio header, len=%u", b->end-b->last);
       return 0;
   }    

   p = b->last;
   b->last += 7;

   /* copy payload */
   bsize = packet->size;
   if (b->last + bsize > b->end) {
       bsize = b->end - b->last;
   }    
   b->last = cpymem(b->last, packet->data, bsize);

   /* make up ADTS header */
   if (hls_parse_aac_header(s, &objtype, &srindex, &chconf) != 0) {
       ERROR("hls: aac header error");
       return 0;
   }

   //hexdump(b->pos, b->last-b->pos, "af2");

   /* we have 5 free bytes + 2 bytes of RTMP frame header */
   /* ADTS header, see ISO-13818-7, section 6.2 */

   p[0] = 0xff;
   p[1] = 0xf1;
   p[2] = (char) (((objtype - 1) << 6) | (srindex << 2) |
                    ((chconf & 0x04) >> 2));
   p[3] = (char) (((chconf & 0x03) << 6) | ((size >> 11) & 0x03));
   p[4] = (char) (size >> 3);
   p[5] = (char) ((size << 5) | 0x1f);
   p[6] = 0xfc;

   //hexdump(b->pos, b->last-b->pos, "af3");

   if (p != b->start) {
       ctx->aframe_num++;
       //DEBUG( "hls: aframe_num=%u\n",
       //           ctx->aframe_num);
       return 0;
   }

   ctx->aframe_pts = pts;

   //DEBUG( "hls: sync=%u, sample_rate=%u\n",
   //               hctx->sync, codec_ctx->sample_rate);
   if (!hctx->sync || codec_ctx->sample_rate == 0) {
       return 0;
   }

   /* align audio frames */

   /* TODO: We assume here AAC frame size is 1024
    *       Need to handle AAC frames with frame size of 960 */

   est_pts = ctx->aframe_base + ctx->aframe_num * 90000 * 1024 /
                                 codec_ctx->sample_rate;
   dpts = (int64_t) (est_pts - pts);

   //DEBUG("hls: audio sync dpts=%L (%.5fs)",
   //               dpts, dpts / 90000.);

   if (dpts <= (int64_t) hctx->sync * 90 &&
       dpts >= (int64_t) hctx->sync * -90)
   {
       ctx->aframe_num++;
       ctx->aframe_pts = est_pts;
       return 0;
   }

   ctx->aframe_base = pts;
   ctx->aframe_num  = 1;

   DEBUG("hls: audio sync gap dpts=%L (%.5fs)",
                  dpts, dpts / 90000.);

   return 0;
}

int
hls_append_sps_pps(hls_session_t *s, hls_buf_t *out)
{
    hls_codec_ctx_t           *codec_ctx;
    char                         *p;
    hls_buf_t                    *in;
    hls_mpegts_ctx_t             *ctx;
    int8_t                          nnals;
    uint16_t                        len, rlen;
    int64_t                       n;

    ctx = s->mpegts_ctx;

    codec_ctx = s->codec_ctx;

    if (ctx == NULL || codec_ctx == NULL) {
        return -1;
    }

    in = codec_ctx->avc_header;

    if (in == NULL) {
        return -1;
    }

    p = in->pos;

    /*
     * Skip bytes:
     * - flv fmt
     * - H264 CONF/PICT (0x00)
     * - 0
     * - 0
     * - 0
     * - version
     * - profile
     * - compatibility
     * - level
     * - nal bytes
     */

    //if (hls_copy(s, NULL, &p, 10, &in) != 0) {
    //    return -1;
    //}

    // ignore the first 5 bytes.
    if (hls_copy(s, NULL, &p, 5, in) != 0) {
        return -1;
    }
    
    /* number of SPS NALs */
    if (hls_copy(s, &nnals, &p, 1, in) != 0) {
        return -1;
    }

    nnals &= 0x1f; /* 5lsb */

    //DEBUG( "hls: SPS number: %u\n", nnals);
    /* SPS */
    for (n = 0; ; ++n) {
        for (; nnals; --nnals) {

            /* NAL length */
            if (hls_copy(s, &rlen, &p, 2, in) != 0) {
                return -1;
            }

            rmemcpy(&len, &rlen, 2);

            //DEBUG( "hls: header NAL length: %u\n", (size_t) len);

            /* AnnexB prefix */
            if (out->end - out->last < 4) {
                DEBUG( "hls: too small buffer for header NAL size\n");
                return -1;
            }

            *out->last++ = 0;
            *out->last++ = 0;
            *out->last++ = 0;
            *out->last++ = 1;

            /* NAL body */
            if (out->end - out->last < len) {
                DEBUG( "hls: too small buffer for header NAL\n");
                return -1;
            }

            if (hls_copy(s, out->last, &p, len, in) != 0) {
                return -1;
            }

            out->last += len;
        }
        if (n == 1) {
            break;
        }

        /* number of PPS NALs */
        if (hls_copy(s, &nnals, &p, 1, in) != 0) {
            return -1;
        }

        //DEBUG( "hls: PPS number: %u\n", nnals);
    }

    return 0;
}


int
hls_append_aud(hls_session_t *s, hls_buf_t *out)
{
    /* access unit delimiter, see ISO 14496-10, section 7.4.1 */
    static char   aud_nal[] = { 0x00, 0x00, 0x00, 0x01, 0x09, 0xf0 };

    if (out->last + sizeof(aud_nal) > out->end) {
        return -1;
    }

    out->last = cpymem(out->last, aud_nal, sizeof(aud_nal));

    return 0;
}

char *
hls_mpegts_write_pcr(char *p, uint64_t pcr)
{
    *p++ = (u_char) (pcr >> 25);
    *p++ = (u_char) (pcr >> 17);
    *p++ = (u_char) (pcr >> 9);
    *p++ = (u_char) (pcr >> 1);
    *p++ = (u_char) (pcr << 7 | 0x7e);
    *p++ = 0;

    return p;
}

char *
hls_mpegts_write_pts(char *p, uint64_t fb, uint64_t pts)
{
    uint64_t val;

    val = fb << 4 | (((pts >> 30) & 0x07) << 1) | 1;
    *p++ = (char) val;

    val = (((pts >> 15) & 0x7fff) << 1) | 1;
    *p++ = (char) (val >> 8);
    *p++ = (char) val;

    val = (((pts) & 0x7fff) << 1) | 1;
    *p++ = (char) (val >> 8);
    *p++ = (char) val;

    return p;
}

int
hls_mpegts_write_frame(hls_mpegts_file_t *file,
    hls_mpegts_frame_t *f, hls_buf_t *b)
{   
    uint64_t  pes_size, header_size, body_size, in_size, stuff_size, flags;
    char      packet[188], *p, *base;
    int64_t   first, rc;
    
   
    //DEBUG( "mpegts: pid=%lu, sid=%lu, pts=%lu, "
    //               "dts=%lu, key=%lu, size=%lu\n",
    //               f->pid, f->sid, f->pts, f->dts,
    //               f->key, (size_t) (b->last - b->pos));

    first = 1;

    /* MPEGTS format */
    while (b->pos < b->last) {
        p = packet;

        f->cc++;

        /* TS header has following format: 
           - first byte: sync byte of pattern 0x47.
           - second & third byte: 3 most sigfi. bits: error, start indicator, 
                          transport priority and 13 lower bits for packet ID.
           - forth byte: 2-bits scrambling control, 
                         2-bits adpation field control, 
                         4-bits continuity counter.
           - optional 4-bytes: adaption field.  */

        *p++ = 0x47;
        *p++ = (char) (f->pid >> 8);

        //DEBUG("mpegts: f->pid >> 8 =%u\n", f->pid >> 8);

        if (first) {
            p[-1] |= 0x40;
        }

        *p++ = (char) f->pid;
        *p++ = 0x10 | (f->cc & 0x0f); /* no adaptation & payload only */

        if (first) {

            if (f->key) {
                packet[3] |= 0x20; /* adaptation */

                *p++ = 7;    /* size */
                *p++ = 0x50; /* random access + PCR */

                //DEBUG("mpegts: dts=%lu, pcr=%ld\n", 
                //        f->dts, (uint64_t)(f->dts - HLS_DELAY));

                p = hls_mpegts_write_pcr(p, f->dts - HLS_DELAY);
            }

            /* PES header */
            *p++ = 0x00;
            *p++ = 0x00;
            *p++ = 0x01;
            *p++ = (char) f->sid;

            //DEBUG("mpegts: sid=%lu\n", f->sid);
            header_size = 5;
            flags = 0x80; /* PTS */

            if (f->dts != f->pts) {
                header_size += 5;
                flags |= 0x40; /* DTS */
            }

            pes_size = (b->last - b->pos) + header_size + 3;

            //DEBUG("mpegts: pes_size=%u, header_size=%u, flags=%u\n",
            //       pes_size, header_size, flags);

            if (pes_size > 0xffff) {
                pes_size = 0;
            }


            *p++ = (char) (pes_size >> 8);
            *p++ = (char) pes_size;
            *p++ = 0x80; /* H222 */
            *p++ = (char) flags;
            *p++ = (char) header_size;

            p = hls_mpegts_write_pts(p, flags >> 6, f->pts + HLS_DELAY);

            if (f->dts != f->pts) {
                p = hls_mpegts_write_pts(p, 1, f->dts + HLS_DELAY);
            }

            first = 0;
        }
        body_size = (uint64_t) (packet + sizeof(packet) - p);
        in_size = (uint64_t) (b->last - b->pos);

        if (body_size <= in_size) {
            memcpy(p, b->pos, body_size);
            b->pos += body_size;

        } else {
            stuff_size = (body_size - in_size);

            if (packet[3] & 0x20) {

                /* has adaptation */
                base = &packet[5] + packet[4];
                p = hls_movemem(base + stuff_size, base, p - base);
                memset(base, 0xff, stuff_size);
                packet[4] += (u_char) stuff_size;

            } else {

                /* no adaptation */
                packet[3] |= 0x20;
                p = hls_movemem(&packet[4] + stuff_size, &packet[4],
                                p - &packet[4]);

                packet[4] = (char) (stuff_size - 1);
                if (stuff_size >= 2) {
                    packet[5] = 0;
                    memset(&packet[6], 0xff, stuff_size - 2);
                }
            }

            memcpy(p, b->pos, in_size);
            b->pos = b->last;
        }

        rc = hls_mpegts_write_file(file, packet, sizeof(packet));
        if (rc != 0) {
            return rc;
        }
    }

    return 0;
}

int
hls_video_handler(hls_session_t *s, AVPacket *packet)
{
    hls_context_t        *hctx;
    hls_mpegts_ctx_t     *ctx;
    hls_codec_ctx_t      *codec_ctx;
    char                 *p;
    uint8_t              fmt, ftype, htype, nal_type, src_nal_type;
    uint32_t             len, rlen;
    hls_buf_t            out, tmp_in, *b,*in;
    uint32_t             cts;
    hls_mpegts_frame_t   frame;
    uint64_t             nal_bytes;
    int64_t              aud_sent, sps_pps_sent, boundary;
    static char          buffer[HLS_BUFSIZE];

    hctx = s->ctx;
    ctx = s->mpegts_ctx; 
    codec_ctx = s->codec_ctx;


    if (hctx == NULL || ctx == NULL || codec_ctx == NULL 
           || codec_ctx->avc_header == NULL) {
        ERROR("not processed");
        return 0;
    }

    /* Only H264 is supported */
    if (codec_ctx->video_codec_id != AV_CODEC_ID_H264) {
        ERROR("codec not supported");
        return 0;
    }

    in = &tmp_in;
    memset(in,0,sizeof(*in));
    in->pos = packet->data;
    in->start = in->pos;
    in->last = in->end = in->pos + packet->size;
    p = in->pos;


#ifdef _FROM_HLS_STREAM_
    if (hls_copy(s, &fmt, &p, 1, in) != 0) {
        return 0;
    }

    /* 1: keyframe (IDR)
     * 2: inter frame
     * 3: disposable inter frame 
       see FLV file format spec 10.1, section E.4.3.1 */

    ftype = (fmt & 0xf0) >> 4;

    /* H264 HDR/PICT */
    if (hls_copy(s, &htype, &p, 1, in) != 0) {
        return -1;
    }

    /* proceed only with PICT */
    if (htype != 1) {
        return 0;
    }

    /* 3 bytes: decoder delay */
    if (hls_copy(s, &cts, &p, 3, in) != 0) {
        return -1;
    }

    cts = ((cts & 0x00FF0000) >> 16) | ((cts & 0x000000FF) << 16) |
          (cts & 0x0000FF00);

#endif //_FROM_HLS_STREAM_

    ftype = packet->flags & AV_PKT_FLAG_KEY;
    cts = 0;

    memset(&out, 0,sizeof(out));

    out.start = buffer;
    out.end = buffer + sizeof(buffer);
    out.pos = out.start;
    out.last = out.pos;

    nal_bytes = codec_ctx->avc_nal_bytes;
    aud_sent = 0;
    sps_pps_sent = 0;

    INFO("h264 NAL ftype=%lu, htype=%lu, nal_bytes=%lu",
         ftype, htype, nal_bytes);

    while (p < in->last) {

        /* TODO: which docs defines this NAL unit length? */
        if (hls_copy(s, &rlen, &p, nal_bytes, in) != 0) {
            return 0;
        }

        len = 0;
        rmemcpy(&len, &rlen, nal_bytes);
       
        INFO("len=%u,rlen=%u",len,rlen);

        if (len == 0) {
            continue;
        }

        if (hls_copy(s, &src_nal_type, &p, 1, in) != 0) {
            return 0;
        }

        nal_type = src_nal_type & 0x1f;

        if (nal_type >= 7 && nal_type <= 9) {
            if (hls_copy(s, NULL, &p, len - 1, in) != 0) {
                return -1;
            }
            continue;
        }

        if (!aud_sent) {
            switch (nal_type) {
                case 1:
                case 5:
                case 6:

                    if (hls_append_aud(s, &out) != 0) {
                        ERROR("error appending AUD NAL");
                    }
                case 9:
                    aud_sent = 1;
                    break;
            }
        }

        switch (nal_type) {
            case 1:
                sps_pps_sent = 0;
                break;
            case 5:
                if (sps_pps_sent) {
                    break;
                }

                if (hls_append_sps_pps(s, &out) != 0) {
                    ERROR("error appenging SPS/PPS NALs");
                }
                sps_pps_sent = 1;
                break;
        }

        /* AnnexB prefix */
        if (out.end - out.last < 5) {
            ERROR("not enough buffer");
            return 0;
        }

        /* first AnnexB prefix is long (4 bytes) */
        if (out.last == out.pos) {
            *out.last++ = 0;
        }

        /* XXX: insert start prefix? */
        *out.last++ = 0;
        *out.last++ = 0;
        *out.last++ = 1;
        *out.last++ = src_nal_type;

        /* NAL body */

        if (out.end - out.last < (int64_t) len) {
            ERROR("not enough buffer for NAL,len=%u", len);
            //hexdump(packet->data,packet->size,"avpacket");
            return 0;
        }

        if (hls_copy(s, out.last, &p, len - 1, in) != 0) {
            return 0;
        }

        out.last += (len - 1);

        //DEBUG( "hls: copy %ui bytes\n", len-1);
        if ( in->pos == in->last )
           break;
    }

    memset(&frame,0,sizeof(frame));

    frame.cc = ctx->video_cc;
    frame.dts = (uint64_t) packet->pts * 90; //h->timestamp * 90;
    frame.pts = frame.dts + cts * 90;
    frame.pid = 0x100;
    frame.sid = 0xe0;
    frame.key = (ftype == 1);

    //DEBUG( "hls: frame info, cc=%u,dts=%u,pts=%u,pid=%u,sid=%u,key=%u\n",
    //              frame.cc,frame.dts,frame.pts,frame.pid,frame.sid,frame.key);

    /*
     * start new fragment if
     * - we have video key frame AND
     * - we have audio buffered or have no audio at all or stream is closed
     */

    b = ctx->aframe;
    boundary = frame.key && (codec_ctx->aac_header == NULL || !ctx->opened ||
                             (b && b->last > b->pos));

    hls_update_fragments(s, frame.dts, boundary, 1);

    if (!ctx->opened) {
        return 0;
    }

    if (hls_mpegts_write_frame(&ctx->file, &frame, &out) != 0) {
        ERROR("hls: video frame failed");
    }

    ctx->video_cc = frame.cc;

    return 0;
}


void
hls_handle_stream(hls_session_t *s)
{
   static char       buffer[128];
   static int cnt = 0;
   hls_stream_ctx_t     *hctx = s->stream_ctx;
   hls_mpegts_ctx_t     *ctx = s->mpegts_ctx;
   AVPacket              packet;
   char                 *p, *end;
   int                   fd, n;
   int                   i = 0;

   while(av_read_frame(hctx->pFormatCtx, &packet)>=0) {

      if(packet.stream_index==hctx->audio_idx) {

         hls_audio_handler(s,&packet);

      } else if(packet.stream_index==hctx->video_idx) {

         hls_video_handler(s,&packet);

      } else {
         DEBUG("unknown packet, stream_index=%u",
                packet.stream_index);
         print_avpacket(&packet);
      }

      av_free_packet(&packet);

      //if ( i++ > 10 )
      //   break;
   }
  
   hls_close_fragment(s);
        
   if ( ctx->record ) {
      DEBUG("finalizing playlist %s ...",ctx->playlist.data);

      fd = hls_open_file(ctx->playlist.data, 
                  O_WRONLY, O_CREAT|O_APPEND, 0644);

      if (fd == -1) {
          ERROR("hls: open failed: %s", &ctx->playlist.data);
          return;
      }    

      p = buffer;
      end = p + sizeof(buffer);

      n = snprintf(p, end - p,"#EXT-X-ENDLIST\n");
      p += n;
      n = write(fd, buffer, p - buffer);
   }
}

void
hls_write_mpegts(hls_session_t *s)
{
   hls_stream_ctx_t     *ctx = s->stream_ctx;

   if ( hls_handle_h264_header(s,ctx->video_idx) != 0 ||
        hls_handle_aac_header(s,ctx->audio_idx) !=0 ) {
      // codecs not supported.
      return;
   }
   hls_handle_stream(s);

   return;
}



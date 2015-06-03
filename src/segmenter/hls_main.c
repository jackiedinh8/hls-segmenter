#include <stdio.h>

#include "hls_core.h"
#include "hls_utils.h"
#include "hls_stream.h"

int main(int argc, char *argv[]) {
   hls_context_t *ctx;

   ctx = hls_create_context();
   if ( ctx == NULL )
      return -1;
     
   // parse program options
   hls_parse_args(ctx,argc,argv);
      
   //hls_segment_file(ctx,argv[1]);
   hls_segment_file(ctx,ctx->infile.data);

   return 0;
}

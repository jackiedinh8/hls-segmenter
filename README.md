# hls-segmenter
A tool to split video files into HSL segments (support H264 and AAC formats only).

Installation instructions:
```
./configure
make
make install
```

Usage:
```
Usage hlsegmneter [options] ...

HTTP Live Stream - Create hls segments from video files (h264 & aac).

     -i, --input FILE          Video file to segment.
     -p, --output-dir DIR      Directory to store the HLS segments.
     -n, --m3u8-file NAME      Name of the HLS stream.
     -u, --url-prefix URL      Prefix for web address of the HLS segments.
```

TODO-list:
 - support other audio & video formats.
 - support fixed duration.

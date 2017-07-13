Introduction
============

NVPipe is a simple and lightweight library for low-latency video compression.  It
provides access to NVIDIA's hardware-accelerated codecs as well as a fallback
to libx264 (via FFMpeg).

It is a great choice to drastically lower the bandwidth required for your
networked interactive client/server application.  It would be a poor choice for
streaming passive video content.

Sample usage
============

The library is specifically designed to be easily integratable into existing
low-latency streaming applications.  NVPipe does not take over any of the
network communication aspects, allowing your application to dictate the
client/server scenario it is used in.

A sample encoding scenario:

```c
  const size_t width=..., height=...; // input image size.
  const uint8_t* rgba = ...; // your image data.
  size_t osize = ...; // number of bytes in 'output'.
  void* output = malloc(max_osize); // place for the compressed stream.

  // First create our encoder.
  const size_t f_m = 4; // "quality".  Generally in [1,5].
  const size_t fps = 30;
  const uint64_t bitrate = width*height * fps*f_m*0.07;
  nvpipe* enc = nvpipe_create_encoder(NVPIPE_H264_NV, bitrate);

  while(you_have_more_frames) {
    nvpipe_encode(enc, rgba, width*height*4, output,&osize, width, height,
                  NVPIPE_RGBA);

    // Send the frame size and compressed stream to the consuming side.
    uint32_t size = to_network_byte_order_zu(osize);
    send(socket, &size, sizeof(uint32_t), ...);
    send(socket, output, osize, ...);
  }

  // Be sure to destroy the encoder when done.
  nvpipe_destroy(enc);
```

`osize` on the encode side will become `N` in the sample decode scenario:

```c
  nvpipe* dec = nvpipe_create_decoder(NVPIPE_H264_NV);

  size_t width=1920, height=1080;
  size_t imgsz = width * height * 3;
  uint8_t* rgb = malloc(imgsz);

  void* strm = malloc(max_N);

  while(you_have_more_frames) {
    uint32_t N = 0;
    recv(socket, &N, sizeof(uin32_t), ...);
    recv(socket, strm, from_network_byte_order(N), ...);
    nvpipe_decode(dec, strm,N, rgb, width, height);
    use_frame(rgb, width, height); // blit, save; whatever.
  }

  nvpipe_destroy(dec); // destroy the decoder when finished
```

Note that the underlying H.264 stream does not support alpha channels.  Such
information would be lost in the scenario above.

Build
=====

This library can optionally make use of the NVIDIA Video Codec SDK headers
available from:

	https://developer.nvidia.com/nvidia-video-codec-sdk

These headers enable additional features that can improve the quality of the
generated stream.

To utilize a custom SDK, set the `NVIDIA_VIDEO_CODEC_SDK` CMake option
to the directory where you have unzipped the SDK.  Then build using
this library using the standard CMake process.

The only special CMake variable is the boolean

  USE_FFMPEG

that controls whether or not the (optional) ffmpeg-based backend is built.

Only shared libraries are supported.

(Optional) FFMpeg-based backend
===============================

To use the `NVPIPE_H264_NVFFMPEG` or `NVPIPE_H264_FFMPEG` backends, the library
must be compiled with FFMpeg support.  Furthermore a small modification is
required in the FFMpeg source tree: in libavcodec/cuvid.c, change the
`ulMaxDisplayDelay` from 4 to 0.

Then build FFMpeg with the 'nvenc' and 'cuvid' backends enabled, e.g.:

```sh
	#!/bin/sh
	export CFLAGS="-ggdb -fPIC"
	export CXXFLAGS="-ggdb -fPIC"
	cu="/usr/local/cuda"
	sdk="${HOME}/sw/nv-video-sdk-7.0.1/"
	ldf="-fPIC"
	ldf="${ldf} -L${cu}/lib64 -Wl,-rpath=${cu}/lib64"
	ldf="${ldf} -L${cu}/lib64/stubs -Wl,-rpath=${cu}/lib64/stubs"
	ldf="${ldf} -L${cu}/lib -Wl,-rpath=${cu}/lib"

	rm -f config.fate config.h config.log config.mak Makefile
	rm -fr libav* libsw*
	../configure \
		--prefix="${HOME}/sw/ffmpeg3.1" \
		--extra-cflags="-I${cu}/include -I${sdk}/include ${CFLAGS}" \
		--extra-cxxflags="-I${cu}/include -I${sdk}/include ${CFLAGS}" \
		--extra-ldflags="${ldf}" \
		--extra-ldexeflags="${ldf}" \
		--extra-ldlibflags="${ldf}" \
		--disable-stripping \
		--assert-level=2 \
		--enable-shared \
		--disable-static \
		--enable-nonfree \
		--enable-nvenc \
		--enable-cuda \
		--enable-cuvid \
		--disable-yasm \
		--disable-gpl \
		--disable-doc \
		--disable-htmlpages \
		--disable-podpages \
		--disable-txtpages \
		--disable-vaapi \
		--disable-everything \
		--enable-decoder=h264 \
		--enable-decoder=h264_cuvid \
		--enable-encoder=nvenc \
		--enable-encoder=nvenc_h264 \
		--enable-encoder=h264_nvenc \
		--enable-encoder=png \
		--enable-hwaccel=h264_cuvid \
		--enable-parser=png \
		--enable-parser=h264 \
		--enable-demuxer=h264 \
		--enable-muxer=mjpeg \
		--enable-muxer=h264 \
		--enable-protocol=pipe \
		--enable-protocol=file \
		--enable-protocol=tcp \
		--enable-protocol=udp \
		--enable-filter=scale \
		--enable-bsf=h264_mp4toannexb
	make -j install
```

Finally build this library while setting the USE_FFMPEG CMake variable to true.

Supported platforms
===================

NVPipe is supported on both Linux and Windows.  Kepler-class NVIDIA
hardware is required.  The NVIDIA Video Codec SDK is an optional dependency to
enable extra features of the generated stream.

OS X support is not plausible in the short term.  NVPipe uses the
[NVIDIA Video Codec SDK](https://developer.nvidia.com/nvidia-video-codec-sdk)
under the hood, and the Video Codec SDK is not available on OS X at this time.

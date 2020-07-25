#ifndef _VIDEO_DECODER_CORE_H_
#define _VIDEO_DECODER_CORE_H_

#include "VideoReader.h"
#include "VideoDecoderBuffer.h"

#include <iostream>
#include <mutex>
#include <atomic>
#include <vector>
#include <functional>

#ifndef VIDEO_NULL_PTR
#define VIDEO_NULL_PTR 0x02
#endif
#ifndef AV_CODEC_FLAG_GLOBAL_HEADER
#define AV_CODEC_FLAG_GLOBAL_HEADER (1 << 22)
#endif
#ifndef CODEC_FLAG_GLOBAL_HEADER
#define CODEC_FLAG_GLOBAL_HEADER AV_CODEC_FLAG_GLOBAL_HEADER
#endif

// configuration
#ifndef HW_TYPE
#define HW_TYPE "CPU_API"
#endif

class VideoDecoderCore
{
private:
  // libav's contexts
  AVFormatContext *mInputFormatContext;
  AVCodecContext  *mVideoContext;
  AVCodecContext  *mAudioContext;
  SwsContext      *mSwsYUVtoRGBAContext;
  AVCodec         *mVideoCodec;
  AVCodec         *mAudioCodec;
  AVStream        *mVideoStream;
  AVStream        *mAudioStream;
  AVPixelFormat    mPixelFormat;
  int              mVideoStreamIDX;
  int              mAudioStreamIDX;
  unsigned int     mVideoWidth;
  unsigned int     mVideoHeight;
  uint64_t         mFrameInterval;
  // for decoding with hw
  AVBufferRef     *mHWDeviceContext;
  AVHWDeviceType   mHWDeviceType;

  // input configurations
  const char*      mInputUrl;
  const char*      mHWType;
  bool             mFlagReadFromBuffer;

  // checked codec contexts and playback information
  AVCodecID        mVideoCodecID;
  AVCodecID        mAudioCodecID;
  std::atomic<int> mVideoFrameCount;
  std::atomic<int> mAudioFrameCount;
  int              mReferenceCount;

  bool init_input_context_from_file(const char* url, const char* hwType = NULL);
  bool init_pool_buffer();
  // If the HWtype exists.
  bool init_hw_context(AVCodecContext *ctx, const enum AVHWDeviceType type);
  int open_codec_context(int *stream_idx, AVCodec *dec,
    AVCodecContext **dec_ctx, AVFormatContext *formatContext, enum AVMediaType type,
    const char* filename, int *referenceCount, int hwinit = 0);
  int decode_video_packet(AVPacket *packet, int *eofframe);
  int decode_audio_packet(AVPacket *packet, int *eofframe);
  int flush_video_frame();
  int flush_audio_frame();


public:
  VideoDecoderCore();
  ~VideoDecoderCore();

  std::shared_ptr<VideoDecoderBuffer> mDecoderBuffer;

  bool initialize();
  bool read();
  int  decode();
  int  convert();

  uint64_t  getVideoInterval() { return mFrameInterval; }
  uint8_t   getVideoWidth()    { return mVideoWidth;    }
  uint8_t   getVideoHeight()   { return mVideoHeight;   }
  AVCodecID getVideoCodecID()  { return mVideoCodecID;  }

  void              setVideoInputUrl(const char* url) { mInputUrl = url; }
  const std::string getVideoInputUrl() { return std::string(mInputUrl); }
  void              setVideoHWType(const char* hwtype) { mHWType = hwtype; }
  const std::string getVideoHwType() { return std::string(mHWType); }
  void              setFlagReadFromBuffer(const bool flag) { mFlagReadFromBuffer = (flag == true); }
  const bool        getFlagReadFromBuffer() { return mFlagReadFromBuffer == true; }
  std::shared_ptr<VideoDecoderBuffer> getBufferPointer() { return mDecoderBuffer; }
};


#endif
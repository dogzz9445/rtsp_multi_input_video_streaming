#ifndef _H_VIDEO_READER_MCSL_
#define _H_VIDEO_READER_MCSL_

extern "C"
{
#include "libavcodec/avcodec.h"
#include "libavutil/imgutils.h"
#include "libavutil/samplefmt.h"
#include "libavutil/timestamp.h"
#include "libavutil/pixdesc.h"
#include "libavutil/hwcontext.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
}

#include <iostream>

// This is stream reader for istream to buffer
// It includes the overrided reading function of libav 
template <class T>
class VideoReader
{
public:
  std::shared_ptr<AVIOContext> mAVIOContext;
  std::shared_ptr<AVFormatContext> mAVFormatContext;
  std::shared_ptr<T> mInputBuffer;
  int mBufferSize;

public:
  VideoReader() :
    mBufferSize(8192)
  {}
  ~VideoReader() {}

  // TODO:
  static int decoderReadFromBuffer(void* opaque, uint8_t* buffer, int buffer_size)
  {
    auto& me = *reinterpret_cast<std::istream*>(opaque);
    me.read(reinterpret_cast<char*>(buffer), buffer_size);
    return me.gcount();
  }

  AVFormatContext* getAVFormatContextFromVideoStreamReader()
  {
    std::ifstream stream("file.avi", std::ios::binary);

    mInputBuffer = std::shared_ptr<T>(reinterpret_cast<T*>(av_malloc(mBufferSize)));
    mAVIOContext = std::shared_ptr<AVIOContext>
    (
      avio_alloc_context
      (
        mInputBuffer.get(),   // Buffer
        mBufferSize,           // Buffer_size
        false,              // Write_flag
        reinterpret_cast<void*>(static_cast<std::istream*>(&stream)), // Opaque
        &VideoReader::decoderReadFromBuffer, // Read function
        nullptr,        // Write function
        nullptr         // Seek pointer
      ),
      &av_free
    );

    mAVFormatContext = std::shared_ptr<AVFormatContext>(avformat_alloc_context(), &avformat_free_context);
    mAVFormatContext->pb = mAVIOContext.get();
    return mAVFormatContext.get();
  }
};

#endif // _H_VIDEO_READER_MCSL_
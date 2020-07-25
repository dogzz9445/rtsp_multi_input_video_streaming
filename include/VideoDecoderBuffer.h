#ifndef _VIDEO_QUEUE_H_
#define _VIDEO_QUEUE_H_

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

#include "ThreadSafeQueue.h"

class AVPacketQueue : public ThreadSafeQueue<AVPacket*>
{
public:
  AVPacketQueue(size_t size) :
    ThreadSafeQueue(size)
  {}
  void reset()
  {
    std::unique_lock<std::mutex> lock(mSafetyMutex);
    while (!mQueue.empty())
    {
      AVPacket* data = mQueue.front();
      mQueue.pop();
      av_packet_unref(data);
      delete data;
      data = nullptr;
    }
    std::queue<AVPacket*> queue;
    mQueue.swap(queue);
    mClosed.store(true);
    mConditionEmpty.notify_all();
    mConditionFull.notify_all();
  }
};

class AVFrameQueue : public ThreadSafeQueue<AVFrame*>
{
public:
  AVFrameQueue(size_t size) :
    ThreadSafeQueue(size)
  {}
  void reset()
  {
    std::unique_lock<std::mutex> lock(mSafetyMutex);
    while (!mQueue.empty())
    {
      AVFrame* data = mQueue.front();
      mQueue.pop();
      av_frame_free(&data);
      delete data;
      data = nullptr;
    }
    std::queue<AVFrame*> queue;
    mQueue.swap(queue);
    mConditionEmpty.notify_all();
    mConditionFull.notify_all();
    mClosed.store(true);
  }
};

class VideoDecoderBuffer
{
public:
  std::atomic_bool mUpdatedVideoFlag{ false };
  std::atomic_bool mUpdatedAudioFlag{ false };
  std::atomic_bool mResizedFlag{ false };
  std::atomic_bool mClosedFlag{ false };
  std::atomic<unsigned int> mWidth;
  std::atomic<unsigned int> mHeight;
  void setResolution(unsigned int width, unsigned int height)
  {
    mWidth = width; mHeight = height; mResizedFlag = true;
  }
  bool isUpdated() { return (!OutVideoQueue.isEmpty() && mUpdatedVideoFlag); }
  bool isResized() { return mResizedFlag; }
  bool isClosed() { return mClosedFlag; }


  // PacketQueue and Pool receive from files and streams
  AVPacketQueue PacketQueue{ 24 };
  AVPacketQueue PacketPool{ 24 };
  // Video Queue has analysed video frame
  AVFrameQueue VideoQueue{ 8 };
  AVFrameQueue VideoPool{ 8 };
  // Out Video Queue has decoded video frame
  AVFrameQueue OutVideoQueue{ 8 };
  AVFrameQueue OutVideoPool{ 8 };
  // Audio Queue has analysed audio frame
  AVFrameQueue AudioQueue{ 16 };
  AVFrameQueue AudioPool{ 16 };
  // Out Audio Queue has decoded video frame
  AVFrameQueue OutAudioQueue{ 16 };
  AVFrameQueue OutAudioPool{ 16 };

  // Stream Queue has In video frames to send
  AVPacketQueue InStreamQueue{ 24 };
  // Stream Queue has out video frames to send
  AVPacketQueue OutStreamQueue{ 24 };

public:
  void      PushPoolPacket(AVPacket* packet) { PacketPool.push(packet); }
  AVPacket* PopPoolPacket(AVPacket* packet = NULL) { return PacketPool.pop(packet); }
  void      PushPacket(AVPacket* packet) { PacketQueue.push(packet); }
  AVPacket* PopPacket(AVPacket* packet = NULL) { return PacketQueue.pop(packet); }
  void      PushPoolVideo(AVFrame* frame) { VideoPool.push(frame); }
  AVFrame*  PopPoolVideo(AVFrame* frame = NULL) { return VideoPool.pop(frame); }
  void      PushVideo(AVFrame* frame) { VideoQueue.push(frame); }
  AVFrame*  PopVideo(AVFrame* frame = NULL) { return VideoQueue.pop(frame); }
  void      PushPoolPicture(AVFrame* frame) { OutVideoPool.push(frame); }
  AVFrame*  PopPoolPicture(AVFrame* frame = NULL) { return OutVideoPool.pop(frame); }
  void      PushPicture(AVFrame* frame) { OutVideoQueue.push(frame); mUpdatedVideoFlag = true; }
  AVFrame*  PopPicture(AVFrame* frame = NULL) { return OutVideoQueue.pop(frame);  }
  void      PushPoolAudio(AVFrame* frame) { AudioPool.push(frame); }
  AVFrame*  PopPoolAudio(AVFrame* frame = NULL) { return AudioPool.pop(frame); }
  void      PushAudio(AVFrame* frame) { AudioQueue.push(frame); }
  AVFrame*  PopAudio(AVFrame* frame = NULL) { return AudioQueue.pop(frame); }
  void      PushPoolAudioOut(AVFrame* frame) { OutAudioPool.push(frame); }
  AVFrame*  PopPoolAudioOut(AVFrame* frame) { return OutAudioPool.pop(); }
  void      PushAudioOut(AVFrame* frame) { OutAudioQueue.push(frame); mUpdatedAudioFlag = true; }
  AVFrame*  PopAudioOut(AVFrame* frame) { return OutAudioQueue.pop(); }

  void      PushInStreamQueue(AVPacket* packet) { InStreamQueue.push(packet); }
  AVPacket* PopInStreamQueue(AVPacket* packet = NULL) { return InStreamQueue.pop(packet); }
  void      PushOutStreamQueue(AVPacket* packet) { OutStreamQueue.push(packet); }
  AVPacket* PopOutStreamQueue(AVPacket* packet = NULL) { return OutStreamQueue.pop(packet); }

  void reset() 
  {
    mUpdatedVideoFlag = false;
    mUpdatedAudioFlag = false;
    mResizedFlag = false;
    mClosedFlag = false;
    PacketQueue.reset();
    PacketPool.reset();
    VideoQueue.reset();
    VideoPool.reset();
    OutVideoQueue.reset();
    OutVideoPool.reset();
    AudioQueue.reset();
    AudioPool.reset();
    OutAudioPool.reset();
    OutAudioQueue.reset();
    InStreamQueue.reset();
    OutStreamQueue.reset();
  }

  void close()
  {
    mClosedFlag = true;
    PacketQueue.close();
    PacketPool.close();
    VideoQueue.close();
    VideoPool.close();
    OutVideoQueue.close();
    OutVideoPool.close();
    AudioQueue.close();
    AudioPool.close();
    OutAudioPool.close();
    OutAudioQueue.close();
    InStreamQueue.close();
    OutStreamQueue.close();
  }

  void open()
  {
    PacketQueue.close(false);
    PacketPool.close(false);
    VideoQueue.close(false);
    VideoPool.close(false);
    OutVideoQueue.close(false);
    OutVideoPool.close(false);
    AudioQueue.close(false);
    AudioPool.close(false);
    OutAudioPool.close(false);
    OutAudioQueue.close(false);
    InStreamQueue.close(false);
    OutStreamQueue.close(false);
  }

  VideoDecoderBuffer() {};
  ~VideoDecoderBuffer() {};
};

#endif
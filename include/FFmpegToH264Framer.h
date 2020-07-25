#ifndef _H_FFMPEG_TO_H264_FRAMER_MCSL_
#define _H_FFMPEG_TO_H264_FRAMER_MCSL_

#include <string>

#include "VideoQueue.h"
#include "VideoBasicLive555.h"

class FFmpegToH264Framer : public FramedSource
{
protected:
  std::shared_ptr<VideoBuffer> pBuffer;
  unsigned int referenceCount;
  //H264or5VideoStreamParser mParser;

public:
  FFmpegToH264Framer(UsageEnvironment& env, std::shared_ptr<VideoBuffer> buffer) :
    FramedSource(env)
  {
    //bool createParser = false;
    //mParser = createParser
    //  ? new H264or5VideoStreamParser(hNumber, this, inputSource, includeStartCodeInOutput)
    //  : NULL;

      pBuffer = buffer;
    fPresentationTime.tv_sec = 0;
    fPresentationTime.tv_usec = 0;
    referenceCount = 1;

    //if (eventTriggerId == 0)
    //{
    //  env.taskScheduler().createEventTrigger(/*updateOutputBufferOneFrame*/);
    //}
  };
  virtual ~FFmpegToH264Framer() {}

  void doGetNextFrame()
  {
    // handleClosure(this);
    bool handleClosed = false;
    if (pBuffer->isClosed())
    {
      handleClosure(this);
      return;
    }

    // update information
    updateOutputBufferOneFrame();
  }

  void doStopGettingFrames()
  {
    envir().taskScheduler().unscheduleDelayedTask(nextTask());
  }

  void packetReadableHandler(FFmpegToH264Framer* source, int /*mask*/)
  {
    if (!source->isCurrentlyAwaitingData())
    {
      source->doStopGettingFrames();
      return;
    }
    source->updateOutputBufferOneFrame();
  }

  void updateOutputBufferOneFrame()
  {
    AVPacket* packet;
    // Pop method include mutex lock by the buffer.
    //pBuffer->PopStreamQueue(packet);

    fFrameSize = packet->size;
    if (fFrameSize == 0)
    {
      handleClosure(this);
    }
    //fMaxSize = packet->size;
    fNumTruncatedBytes = 0;

    fDurationInMicroseconds = static_cast<unsigned int>(packet->duration);

    fPresentationTime.tv_sec = static_cast<long>(packet->pts / 1000000);
    fPresentationTime.tv_usec = static_cast<long>(packet->pts % 1000000);

    memmove(fTo, packet->data, fFrameSize);
    // parse Frame
    //fparser = create H264or5VideoStreamParser();
    //fparser->registerFrameInterest(fTo, fMaxsize);

    //fNumTruncatedBytes = 0;
    //fDurationInMicroseconds = 0;

    FramedSource::afterGetting(this);
  }
};

#endif
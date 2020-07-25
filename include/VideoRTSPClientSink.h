#ifndef _H_VIDEO_RTSP_CLIENT_SINK_MCSL_
#define _H_VIDEO_RTSP_CLIENT_SINK_MCSL_

#include "VideoBasicLive555.h"

class VideoRTSPClientSink : public MediaSink
{
public:
  static VideoRTSPClientSink* createNew(UsageEnvironment& env,
    MediaSubsession& subsession, const char* streamId = NULL);

protected:
  // called only by createNew()
  VideoRTSPClientSink(UsageEnvironment& env, MediaSubsession& subsession, const char* streamId);
  virtual ~VideoRTSPClientSink();

  static void afterGettingFrame(void* clientData, unsigned frameSize,
    unsigned numTruncatedBytes, struct timeval presentationTime, unsigned durationInMicroseconds);
  void afterGettingFrame(unsigned frameSize, unsigned numTruncatedBytes,
    struct timeval presentationTime, unsigned durationInMicroseconds);

protected:
  virtual Boolean continuePlaying();

protected:
  u_int8_t* fReceiveBuffer;
  MediaSubsession& fSubsession;
  char* fStreamId;
};
#endif //_H_VIDEO_RTSP_CLIENT_SINK_MCSL_
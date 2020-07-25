#ifndef _H_VIDEO_RTSP_CLIENT_MCSL_
#define _H_VIDEO_RTSP_CLIENT_MCSL_

#include <iostream>

#include "VideoBasicLive555.h"
#include "StreamClientState.h"

ostream& operator<<(ostream& os, const RTSPClient& rtspClient) 
{
  return os << "[URL:\"" << rtspClient.url() << "\"]: ";
}

ostream& operator<<(ostream& os, const MediaSubsession& subsession)
{
  return os << subsession.mediumName() << "/" << subsession.codecName();
}

class VideoRTSPClient : public RTSPClient
{
public:
  static VideoRTSPClient* createNew(UsageEnvironment& env, const char* rtspURL,
    int verbosityLevel = 0,
    const char* applicationName = NULL,
    portNumBits tunnelOverHTTPPortNum = 0);

protected:
  // called only by creatNew();
  VideoRTSPClient(UsageEnvironment& env, const char* rtspURL,
    int verbosityLevel, const char* applicationName, portNumBits tunnelOverHTTPPortNum);
  virtual ~VideoRTSPClient();

public:
  StreamClientState mStreamClientState;
};

#endif // _H_VIDEO_RTSP_CLIENT_MCSL_
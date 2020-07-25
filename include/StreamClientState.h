#ifndef _H_STREAM_CLIENT_STATE_MCSL_
#define _H_STREAM_CLIENT_STATE_MCSL_

#include "VideoBasicLive555.h"

class StreamClientState 
{
public:
  StreamClientState();
  virtual ~StreamClientState();

public:
  MediaSubsessionIterator* iter;
  MediaSession* session;
  MediaSubsession* subsession;
  TaskToken streamTimerTask;
  double duration;
};

#endif //_H_STREAM_CLIENT_STATE_MCSL_
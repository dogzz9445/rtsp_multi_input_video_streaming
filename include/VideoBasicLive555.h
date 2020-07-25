#ifndef _VIDEO_MEDIA_H_
#define _VIDEO_MEDIA_H_

#include <string>
#include <vector>
#include <functional>

#include "live/ServerMediaSession.hh"
#include "live/RTSPServer.hh"
#include "live/RTSPClient.hh"
#include "live/RTSPCommon.hh"
#include "live/liveMedia.hh"
#include "live/BasicUsageEnvironment.hh"
#include "live/GroupsockHelper.hh"
#include "live/H264or5VideoStreamFramer.hh"

#ifndef BASE_RTSP_PORT
#define BASE_RTSP_PORT 554;
#endif
#ifndef BASE_RTP_PORT
#define BASE_RTP_PORT 18888;
#endif
const std::string BASE_STR_MULTICAST_IP("224.0.67.67");

typedef enum STREAMING_EVENT
{
  EVENT_STOP_ALL = 0,
  EVENT_AFTER_PLAYING = 1
};

struct RTSPAddress
{
public:
  in_addr Address;
  uint16_t RTSPPort;
  uint16_t RTPPort;
  uint16_t RTCPPort;

  RTSPAddress() : RTPPort(0), RTCPPort(0) {}
  ~RTSPAddress() {}

  void writeAddress()
  {
    std::cout <<
      "Address  : " << Address.S_un.S_addr << std::endl <<
      "RTSP port: " << RTSPPort << std::endl <<
      "RTP port : " << RTPPort << std::endl <<
      "RTCP port: " << RTCPPort << std::endl;
  }
};

struct FramedStreamState
{
public:
  ServerMediaSubsession* session;
  FramedSource* source;
  RTPSink* sink;
  RTCPInstance* state;

  //typedef MediaSink::afterPlayingFunc* _after_playing;
  //void registerAfterPlaying(const _after_playing func) { afterPlaying = func; }
  //_after_playing afterPlaying;

  //void play()
  //{
  //  registerAfterPlaying(std::bind(&FramedStreamState::close, this, _1));
  //  sink->startPlaying(*source, afterPlaying, sink);
  //}

  void close(/*void**/)
  {
    sink->stopPlaying();
    Medium::close(source);
    Medium::close(state);
    Medium::close(sink);
  }
};

class VideoBasicLive555
{
protected:
  TaskScheduler           *mTaskScheduler;
  UsageEnvironment        *mEnv;
  EventTriggerId           mEvents[10];
  std::vector<RTSPAddress> mAddresses;

public:
  VideoBasicLive555() 
  {
    mTaskScheduler = BasicTaskScheduler::createNew();
    mEnv = BasicUsageEnvironment::createNew(*mTaskScheduler);
  }
  ~VideoBasicLive555() {}

  VideoBasicLive555* createNew()
  {
    return new VideoBasicLive555();
  }

  void networkLoop()
  {
    mEnv->taskScheduler().doEventLoop();
  }

  void startStreaming()
  {
    networkLoop();
  }

  void stopStreaming()
  {
    mEnv->taskScheduler().triggerEvent(mEvents[EVENT_STOP_ALL]);
  }

  void showStreamingInformation()
  {
    std::cout << "Total of Streamings: " << std::endl;
    for (auto it : mAddresses)
    {
      it.writeAddress();
    }
  }

  virtual void stopPlayingAll(void*);

  // Events
  typedef std::function<void(void*)> _stop_playing_all;
  typedef std::function<void(void*)> _check_session_out_broken_server;

  void registerStopPlayingAll(_stop_playing_all func) { StopPlayingAll = func; }
  void registerCheckSessionOutBrokenServer(_check_session_out_broken_server func) { CheckSessionOutBrokenServer = func; }

  _stop_playing_all StopPlayingAll;
  _check_session_out_broken_server CheckSessionOutBrokenServer;
};

#endif //_VIDEO_MEDIA_H_
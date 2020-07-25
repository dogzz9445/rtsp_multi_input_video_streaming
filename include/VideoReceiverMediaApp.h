#ifndef _VIDEO_RECEIVER_MEDIA_APP_H_
#define _VIDEO_RECEIVER_MEDIA_APP_H_

#include "VideoDecoderBuffer.h"
#include "VideoDecoderApp.h"

#include "VideoBasicLive555.h"
#include "VideoRTSPClient.h"
#include "VideoRTSPClientSink.h"

#include <iostream>
#include <vector>

#ifndef RTSP_CLIENT_VERBOSITY_LEVEL
#define RTSP_CLIENT_VERBOSITY_LEVEL 1
#endif
#ifndef REQUEST_STREAMING_OVER_TCP
#define REQUEST_STREAMING_OVER_TCP False
#endif

char eventLoopWatchVariable = 0;

// Usage:
// VideoApp* app = new VideoReceiverMediaApp;
//
class VideoReceiverMediaApp : public VideoBasicLive555, public VideoDecoderApp
{
private:
  unsigned mRTSPClientCount;
  in_addr mSrcAddress;
  uint16_t mBaseRTSPPort;
  uint16_t mBaseRTPPort;
  std::vector<RTSPClient*> mRTSPClients;

private:
  VideoReceiverMediaApp();
  ~VideoReceiverMediaApp();

  // call back functions
  void setupNextSubsession(RTSPClient* rtspClient);
  void continueAfterSETUP(RTSPClient*, int resultCode, char* resultString);
  void continueAfterDESCRIBE(RTSPClient*, int, char*);
  void continueAfterPLAY(RTSPClient*, int, char*);
  void shutdownStream(RTSPClient*, int exitCode = 1);
  void subsessionAfterPlaying(void* clientData);
  void subsessionByeHandler(void*, const char* reason);
  void streamTimerHandler(void*);

  typedef std::function<void(RTSPClient*, int, char*)> _continue_after_setup;
  typedef std::function<void(RTSPClient*, int, char*)> _continue_after_describe;
  typedef std::function<void(RTSPClient*, int, char*)> _continue_after_play;
  typedef std::function<void(void*)> _subsession_after_playing;
  typedef std::function<void(void*, const char*)> _subsession_after_byehandler;
  typedef std::function<void(void*)> _stream_timer_handler;

  void registerContinueAfterSetup(_continue_after_setup func) { funcContinueAfterDescribe = func; }
  void registerContinueAfterDescribe(_continue_after_describe func) { funcContinueAfterDescribe = func; }
  void registerContinueAfterPlay(_continue_after_play func) { funcContinueAfterDescribe = func; }
  void registerSubsessionAfterPlaying(_subsession_after_playing func) { funcSubsessionAfterPlaying = func; }
  void registerSubsessionAfterByeHandler(_subsession_after_byehandler func) { funcSubsessionAfterByeHandler = func; }
  void registerStreamTimerHandler(_stream_timer_handler func) { funcStreamTimerHandler = func; }

  _continue_after_setup funcContinueAfterSetup;
  _continue_after_describe funcContinueAfterDescribe;
  _continue_after_play funcContinueAfterPlay;
  _subsession_after_playing funcSubsessionAfterPlaying;
  _subsession_after_byehandler funcSubsessionAfterByeHandler;
  _stream_timer_handler funcStreamTimerHandler;

public:
  VideoReceiverMediaApp(std::string srcAddress, uint16_t rtspPort, uint16_t baseRtpPort);
  bool setupSourceAddress(std::string srcAddress, uint16_t rtspPort, uint16_t baseRtpPort);

  // FIXME:
  void startStreaming();
  void stopStreaming();
  void showStreamingInformation();

  // Add video
  int  addReceiveStream();
  int  openURL(const char* progName, const char* rtspURL);

public:
  // procedure
  void receivePacket();

  virtual void init();
  virtual void playLoop();
  virtual void addVideo(const char* inputUrl, const char* outputUrl = NULL, int portNum = NULL);
  virtual void stopPlayingAll(void* clientdata);
};

#endif //_VIDEO_RECEIVER_MEDIA_APP_H_
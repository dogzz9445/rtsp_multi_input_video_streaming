#include "VideoSenderMediaApp.h"

#include <iostream>
#include <functional>

using namespace std::placeholders;

VideoSenderMediaApp::VideoSenderMediaApp() : 
  VideoBasicLive555(), 
  VideoDecoderApp()
{
  mEstimatedSessionBandwidth = 1000;
  mCNAMElen = 100;
  mCNAME = new unsigned char[mCNAMElen];
  gethostname((char*)mCNAME, mCNAMElen-1);
  mCNAME[mCNAMElen-1] = '\0';
}

VideoSenderMediaApp::VideoSenderMediaApp(std::string dstAddress, uint16_t rtspPort, uint16_t baseRtpPort) :
  VideoSenderMediaApp()
{
  setupDestinationAddress(dstAddress, rtspPort, baseRtpPort);
}

VideoSenderMediaApp::~VideoSenderMediaApp()
{
  for (auto it = mStreams.begin(); it != mStreams.end; it++)
  {
    (*it).close();
  }
  if (mCNAME)
  {
    delete[] mCNAME;
    mCNAME = NULL;
  }

  mRTSPServer->closeAllClientSessionsForServerMediaSession(mServerMediaSession);
  mServerMediaSession->deleteAllSubsessions();
}

bool VideoSenderMediaApp::setupRTSPServerSession()
{
  // RTSPServer is needed only one for making connection
  mRTSPServer = RTSPServer::createNew(*mEnv, mBaseRTSPPort, NULL);
  if (!mRTSPServer)
  {
    std::cerr << "Failed to create RTSP Server\n";
    return false;
  }
  mServerMediaSession = ServerMediaSession::createNew(*mEnv, "teststream", "Sender sendstream", "AlloVideo", True);
  if (!mServerMediaSession)
  {
    std::cerr << "Failed to create Server Media Session\n";
    return false;
  }
  mRTSPServer->addServerMediaSession(mServerMediaSession);
}

void VideoSenderMediaApp::addServerMediaSubsession(AVCodecID codec, std::shared_ptr<VideoDecoderBuffer> buffer, int portNum)
{
  FramedStreamState streamstate;

  const Port rtpPort(portNum);
  const Port rtcpPort(portNum + 1);
  
  Groupsock rtpGroupsock(*mEnv, mDstAddress, rtpPort, /*ttl*/255);
  Groupsock rtcpGroupsock(*mEnv, mDstAddress, rtcpPort, /*ttl*/255);
  rtpGroupsock.multicastSendOnly();
  rtcpGroupsock.multicastSendOnly();

  switch (codec)
  {
  case AV_CODEC_ID_H264:
    streamstate.source = new FFmpegToH264Framer(*mEnv, buffer);
    streamstate.sink = H264VideoRTPSink::createNew(*mEnv, &rtpGroupsock, 96);
    //streamstate.session = BufferToH264ServerMediaSubsession::createNew(mEnv);
    break;
  case AV_CODEC_ID_H265:
    // FIXME:
    // have to implement H264 and H265 separatly.
    //streamstate.source = new FFmpegToH265Framer(*mEnv, buffer);
    streamstate.source = new FFmpegToH264Framer(*mEnv, buffer);
    streamstate.sink = H265VideoRTPSink::createNew(*mEnv, &rtpGroupsock, 96);
    //streamstate.session = BufferToH265ServerMediaSubsession::createNew(mEnv);
    break; 
    
  default:
    std::cerr << "Substream could not find input codec\n";
    streamstate.close();
    break;
  }

  streamstate.state = RTCPInstance::createNew(*mEnv, &rtcpGroupsock, mEstimatedSessionBandwidth,
    mCNAME, streamstate.sink, /*For client*/NULL, /*SSM*/True);
  streamstate.session = PassiveServerMediaSubsession::createNew(*streamstate.sink, streamstate.state);
  mServerMediaSession->addSubsession(streamstate.session);
  streamstate.sink->startPlaying(*(streamstate.source), NULL, NULL);
  mStreams.push_back(streamstate);

  RTSPAddress address;
  address.Address = mDstAddress;
  address.RTSPPort = mBaseRTSPPort;
  address.RTPPort = rtpPort.num();
  address.RTCPPort = rtcpPort.num();
  mAddresses.push_back(address);
}

void VideoSenderMediaApp::checkSessionoutBrokenServer(void*)
{
//  if (!sendKeepAlivesToBrokenServers) return; // we're not checking
//
//// Send an "OPTIONS" request, starting with the second call
//  if (sessionTimeoutBrokenServerTask != NULL) {
//    getOptions(NULL);
//  }
//
//  unsigned sessionTimeout = sessionTimeoutParameter == 0 ? 60/*default*/ : sessionTimeoutParameter;
//  unsigned secondsUntilNextKeepAlive = sessionTimeout <= 5 ? 1 : sessionTimeout - 5;
//  // Reduce the interval a little, to be on the safe side
//
//  sessionTimeoutBrokenServerTask
//    = env->taskScheduler().scheduleDelayedTask(secondsUntilNextKeepAlive * 1000000,
//    (TaskFunc*)checkSessionTimeoutBrokenServer, NULL);
}

void VideoSenderMediaApp::stopPlayingAll(void* clientdata)
{
  mRTSPServer->closeAllClientSessionsForServerMediaSession(mServerMediaSession);
  mServerMediaSession->deleteAllSubsessions();

  for (auto stream = mStreams.begin(); stream != mStreams.end(); stream++)
  {
    stream->close();
  }
  mStreams.clear();
}

void VideoSenderMediaApp::sendPacket()
{
  while (checkStateOnPlaying())
  {
    startStreaming();
  }
  Stop();
}

//---------------------------------------------------------------------------------------
// inherit functions (Overriding)
//---------------------------------------------------------------------------------------
void VideoSenderMediaApp::init()
{
  setupRTSPServerSession();

  VideoDecoderApp::init();
  registerSendPacket(std::bind(&VideoSenderMediaApp::sendPacket, this));
  registerPlay(std::bind(&VideoSenderMediaApp::playLoop, this));

  // Set event triggers for sessions
  int clientdata;
  registerStopPlayingAll(std::bind(&VideoSenderMediaApp::stopPlayingAll, this, _1));
  mEvents[EVENT_STOP_ALL] = mEnv->taskScheduler().createEventTrigger((TaskFunc*)(StopPlayingAll, clientdata));
}

bool VideoSenderMediaApp::addSendStream(VideoDecoderCore* core, const char* outputUrl)
{
  static int rtpPortNum = mBaseRTPPort;
  addServerMediaSubsession(core->getVideoCodecID(), core->mDecoderBuffer, rtpPortNum);
  rtpPortNum += 2;
  return true;
}

void VideoSenderMediaApp::addVideo(const char* inputUrl, const char* outputUrl = NULL, int portNum = NULL)
{
  int coreId   = addVideoDecoderCore(inputUrl);
  int streamId = addSendStream(&mDecoderCores[coreId], outputUrl);
}

void VideoSenderMediaApp::playLoop()
{
  std::thread readingPacket(funcReadPacket, &mDecoderCores);
  std::thread decodingPacket(funcDecodePacket, &mDecoderCores);
  std::thread scalingFrame(funcScaleFrame, &mDecoderCores);
  std::thread sendingPacket(funcSendPacket, &mDecoderCores, &mStreams);

  mMainTask.start();
  funcSinkVideo();

  sendingPacket.join();
  readingPacket.join();
  decodingPacket.join();
  scalingFrame.join();
}


bool VideoSenderMediaApp::setupDestinationAddress(std::string dstAddress, uint16_t rtspPort, uint16_t baseRtpPort)
{
  // FIXME: this is allowed just one address for multi-cast
  NetAddressList addresses(dstAddress.c_str());
  auto addr = *(const char**)addresses.firstAddress()->data();
  inet_pton(AF_INET, addr, &(mDstAddress.s_addr));
  mBaseRTSPPort = rtspPort;
  mBaseRTPPort = baseRtpPort;
}
#ifndef _VIDEO_SENDER_MEDIA_APP_H_
#define _VIDEO_SENDER_MEDIA_APP_H_

#include "FFmpegToH264Framer.h"

#include "VideoBasicLive555.h"
#include "VideoDecoderBuffer.h"
#include "VideoDecoderApp.h"

// Usage:
// VideoApp *app = new VideoSenderMediaApp(BASE_STR_MULTICAST_IP, BASE_RTSP_PORT);
class VideoSenderMediaApp : public VideoBasicLive555, public VideoDecoderApp
{
private:
  RTSPServer            *mRTSPServer;
  ServerMediaSession    *mServerMediaSession;
  in_addr                mDstAddress;
  uint16_t               mBaseRTSPPort;
  uint16_t               mBaseRTPPort;
  std::vector<FramedStreamState> mStreams;

  // For RTCP
  unsigned int           mEstimatedSessionBandwidth;
  unsigned int           mCNAMElen;
  unsigned char*         mCNAME;
private:
  VideoSenderMediaApp();
  ~VideoSenderMediaApp();
public:
  VideoSenderMediaApp(std::string dstAddress, uint16_t port, uint16_t baseRtpPort);
  bool setupDestinationAddress(std::string dstAddress, uint16_t rtspPort, uint16_t baseRtpPort);
  bool setupRTSPServerSession();
  void addServerMediaSubsession(AVCodecID codec, std::shared_ptr<VideoDecoderBuffer> buffer, int portNum);
  bool addSendStream(VideoDecoderCore* core, const char* outputUrl);

  void showStreamingInformation();

  // Callback functions
  void checkSessionoutBrokenServer(void*);

  // Procedure
  // reading->decoding->converting->sending
  void sendPacket();

  // Main Events
  void startStreaming();
  void stopStreaming();

  // inherit functions (Overrriding)
  virtual void init();
  virtual void playLoop();
  virtual void addVideo(const char* inputUrl, const char* outputUrl = NULL, int portNum = NULL);
  virtual void stopPlayingAll(void* clientdata);
};

#endif //_VIDEO_SENDER_MEDIA_APP_H_
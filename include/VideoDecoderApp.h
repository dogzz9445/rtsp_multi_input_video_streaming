#ifndef _H_VIDEO_DECODER_APP_
#define _H_VIDEO_DECODER_APP_

#include "VideoDecoderCore.h"
#include "VideoApp.h"
#include "VideoDecoderBuffer.h"

/*
VideoDecoderApp uses video decoder as default
*/
class VideoDecoderApp : public VideoApp
{
protected:
  std::vector<VideoDecoderCore>       mDecoderCores;
  std::vector<VideoReader<uint8_t>>   mVideoReaders;
    
  // Decoder core functions
  void read(std::vector<VideoDecoderCore>&);
  void decodePacket(std::vector<VideoDecoderCore>&);
  void scaleFrame(std::vector<VideoDecoderCore>&);

  // Sender and Receiver functions
  int addVideoDecoderCore(const char* inputUrl);

  // Procedure
  typedef std::function<void()>                                _receive_packet;
  typedef std::function<void(std::vector<VideoDecoderCore>&)>  _read_packet;
  typedef std::function<void()>                                _send_packet;
  typedef std::function<void(std::vector<VideoDecoderCore>&)>  _decode_packet;
  typedef std::function<void(std::vector<VideoDecoderCore>&)>  _scale_frame;
  typedef std::function<void()>                                _sink_video;

  void registerReceivePacket(const _receive_packet func) { funcReceivePacket = func; }
  void registerReadPacket(const _read_packet func) { funcReadPacket = func; }
  void registerSendPacket(const _send_packet func) { funcSendPacket = func; }
  void registerDecodePacket(const _decode_packet func) { funcDecodePacket = func; }
  void registerScaleFrame(const _scale_frame func) { funcScaleFrame = func; }
  void registerSinkVideo(const _sink_video func) { funcSinkVideo = func; }

  _receive_packet funcReceivePacket;
  _read_packet    funcReadPacket;
  _send_packet    funcSendPacket;
  _decode_packet  funcDecodePacket;
  _scale_frame    funcScaleFrame;
  _sink_video     funcSinkVideo;

public:
  VideoDecoderApp();
  ~VideoDecoderApp();

  // Main loop
  virtual void init();
  virtual void playLoop();
  virtual void addVideo(const char* inputUrl, const char* outputUrl = NULL, int portNum = NULL);

  // Implemented interface that communicates with AlloLib
  virtual bool KeyDown(const al::Keyboard& k);
  virtual void GetTexture(int decoderId, Texture* texture);
};


#endif // _H_VIDEO_DECODER_APP_
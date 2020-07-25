#include "VideoDecoderApp.h"

using namespace std::placeholders;
VideoDecoderApp::VideoDecoderApp() : VideoApp()
{
}

VideoDecoderApp::~VideoDecoderApp()
{
  Stop();
  mDecoderCores.clear();
}

void VideoDecoderApp::init()
{
  registerReadPacket(std::bind(&VideoDecoderApp::readFromStream, this, _1));
  registerReadPacket(std::bind(&VideoDecoderApp::readFromFile, this, _1));
  registerDecodePacket(std::bind(&VideoDecoderApp::decodePacket, this, _1));
  registerScaleFrame(std::bind(&VideoDecoderApp::scaleFrame, this, _1));
  registerSinkVideo(std::bind(&VideoApp::sinkVideo, this));
  registerPlay(std::bind(&VideoDecoderApp::playLoop, this));
}

void VideoDecoderApp::playLoop()
{
  std::thread readingPacket(funcReadPacket, &mDecoderCores);
  std::thread decodingPacket(funcDecodePacket, &mDecoderCores);
  std::thread scalingFrame(funcScaleFrame, &mDecoderCores);

  mMainTask.start();
  funcSinkVideo();
  
  readingPacket.join();
  decodingPacket.join();
  scalingFrame.join();
}

void VideoDecoderApp::addVideo(const char* inputUrl, const char* outputUrl = NULL, int portNum = NULL)
{
  addVideoDecoderCore(inputUrl);
}

int VideoDecoderApp::addVideoDecoderCore(const char* inputUrl)
{
  VideoDecoderCore core;
  core.setVideoInputUrl(inputUrl);
  core.setVideoHWType(HW_TYPE);

  if (!core.initialize())
  {
    return false;
  }
  mMainTask.setInterval(core.getVideoInterval());

  mDecoderCores.push_back(core);
  return mDecoderCores.size() - 1;
}

void VideoDecoderApp::read(std::vector<VideoDecoderCore>& cores)
{
  while (checkStateOnPlaying())
  {
    for (auto core = cores.begin(); core != cores.end(); core++)
    {
      core->read();
    }
  }
  Stop();
}

void VideoDecoderApp::decodePacket(std::vector<VideoDecoderCore>& cores)
{
  while (checkStateOnPlaying())
  {
    for (auto core = cores.begin(); core != cores.end(); core++)
    {
      core->decode_packet_video_audio();
    }
  }
  Stop();
}

void VideoDecoderApp::scaleFrame(std::vector<VideoDecoderCore>& cores)
{
  while (checkStateOnPlaying())
  {
    for (auto core = cores.begin(); core != cores.end(); core++)
    {
      core->convert_yuv2rgba_frame();
    }
  }
  Stop();
}


bool VideoDecoderApp::KeyDown(const al::Keyboard& k)
{
  switch (k.key())
  {
  // Basic playback
  // If choose 'pause'
  case 'p': 
    switch (State())
    {
    case VIDEO_STATE_STOP:
      mPlayingThread = std::thread(std::bind(&VideoDecoderApp::Start, this));
      break;
    case VIDEO_STATE_PLAYING:
      Pause();
      break;
    case VIDEO_STATE_PAUSED:
      Resume();
      break;
    }
    break;

  // Stop playing
  case 's':
    if (State() != VIDEO_STATE_STOP)
    {
      Stop();
      mPlayingThread.join();
      mDecoderCores.clear();
    }
    break;
  }
  return false;
}

void VideoDecoderApp::GetTexture(int decoderId, Texture* texture)
{
  if ((mVideoAppState != VIDEO_STATE_STOP))
  {
    if (mMainTask.isUpdated())
    {
      if (mDecoderCores[decoderId].buffer->isResized())
      {
        texture->resize(mDecoderCores[decoderId].buffer->mWidth,
          mDecoderCores[decoderId].buffer->mHeight);
      }
      AVFrame *out = mDecoderCores[decoderId].buffer->PopPicture();
      texture->submit(out->data[0]);
      delete[] out->data[0];
      av_frame_free(&out);
    }
  }
}
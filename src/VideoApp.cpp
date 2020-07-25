#include "VideoApp.h"

VideoApp::VideoApp()
{
  mVideoAppState = VIDEO_STATE_STOP;
  mVideoEventMutex = std::make_shared<std::mutex>();
  mConditionVar = std::make_unique<std::condition_variable>();
}

VideoApp::~VideoApp()
{}

VideoAppState VideoApp::State()
{
  return mVideoAppState;
}

bool VideoApp::isPlaying()
{
  return mVideoAppState == VIDEO_STATE_PLAYING;
}

bool VideoApp::isStopping()
{
  return mVideoAppState == VIDEO_STATE_STOP;
}

bool VideoApp::unlockPause()
{
  return (isPlaying() || isStopping());
}

void VideoApp::changeState(VideoAppState state)
{
  mVideoAppState.store(state, std::memory_order_release);
  mConditionVar->notify_all();
}

bool VideoApp::checkStateOnPlaying()
{
  if (mVideoAppState == VIDEO_STATE_PAUSED)
  {
    std::unique_lock<std::mutex> lock(*mVideoEventMutex);
    mConditionVar->wait(lock, std::bind(&VideoApp::unlockPause, this));
  }
  if (mVideoAppState == VIDEO_STATE_STOP)
  {
    return false;
  }
  return true;
}

void VideoApp::Start()
{
  if (mVideoAppState == VIDEO_STATE_STOP)
  {
    changeState(VIDEO_STATE_PLAYING);
    Play();
  }
}

void VideoApp::Stop()
{
  if (mVideoAppState != VIDEO_STATE_STOP)
  {
    changeState(VIDEO_STATE_STOP);
  }
}

void VideoApp::Pause()
{
  if (mVideoAppState != VIDEO_STATE_PAUSED)
  {
    changeState(VIDEO_STATE_PAUSED);
    mMainTask.pause();
  }
}

void VideoApp::Resume()
{
  if (mVideoAppState == VIDEO_STATE_PAUSED)
  {
    changeState(VIDEO_STATE_PLAYING);
    mMainTask.resume();
  }
}

void VideoApp::sinkVideo()
{
  while (checkStateOnPlaying())
  {
    mMainTask.sink();
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
  }
}
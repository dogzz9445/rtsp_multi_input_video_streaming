#ifndef _VIDEO_INTERFACE_H_
#define _VIDEO_INTERFACE_H_

#include <functional>
#include <thread>
#include <atomic>
#include <mutex>

#include "al/app/al_App.hpp"
#include "TaskController.h"

typedef enum APP_PLAYING_STATE
{
  VIDEO_STATE_STOP,
  VIDEO_STATE_PLAYING,
  VIDEO_STATE_PAUSED,
  VIDEO_STATE_EVENT_PAUSE,
  VIDEO_ERROR_RET,
  VIDEO_ERROR_NONE
} VideoAppState;

// Usage:
// VideoApp *interface = new VideoSenderMeidaApp;
// VideoApp *interface = new VideoReceiverMeidaApp;
// VideoApp *interface = new VideoMediaApp;
class VideoApp
{
protected:
  std::thread                         mPlayingThread;
  TaskController                      mMainTask;
  std::atomic<VideoAppState>          mVideoAppState;
  std::shared_ptr<mutex>              mVideoEventMutex;
  std::unique_ptr<condition_variable> mConditionVar;
  
  void sinkVideo();
  bool isPlaying();
  bool isStopping();
  bool unlockPause();
  void changeState(VideoAppState state);
  bool checkStateOnPlaying();

  typedef std::function<void()> _play;
  void registerPlay(const _play func) { Play = func; }
  _play Play;

public:
  VideoApp();
  ~VideoApp();

  VideoAppState State();
  void Start();
  void Stop();
  void Pause();
  void Resume();

  virtual bool KeyDown(const al::Keyboard& k) {}
  virtual void GetTexture(int decoderId, Texture* texture) {}
};

#endif


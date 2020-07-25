#ifndef _TASK_CONTROLLER_H_
#define _TASK_CONTROLLER_H_

#include <atomic>

#include "VideoTime.h"

class TaskController
{
private:
  VideoTime mVideoTime;
public:
  std::atomic<unsigned int> framecount;
  bool flagUpdated;
  unsigned int countUpdated;

  TaskController() :
    flagUpdated(false),
    countUpdated(0)
  {
    framecount.store(0);
  }

  void reset()
  {
    framecount.store(0);
    mVideoTime.reset();
  }

  void stop()
  {
    mVideoTime.end(framecount);
    mVideoTime.summary();
  }

  void pause()
  {
    mVideoTime.check("Pause");
  }

  void resume()
  {
    mVideoTime.check("Resume");
  }

  void start()
  {
    mVideoTime.start();
  }

  void setInterval(uint64_t interval)
  {
    mVideoTime.setInterval(interval);
  }

  void operator ++()
  {
    countUpdated++;
  }

  void operator --()
  {
    countUpdated--;
  }

  void sink()
  {
    flagUpdated = mVideoTime.sinkWithFrame(framecount);
  }

  bool isUpdated()
  {
    return flagUpdated;
  }

};

#endif //_TASK_CONTROLLER_H_
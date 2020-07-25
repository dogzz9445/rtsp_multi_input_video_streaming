#ifndef _VIDEO_TIME_H_
#define _VIDEO_TIME_H_


#include <chrono>
#include <iostream>
#include <thread>

using namespace std;

//#define DM_UTIL
#ifdef DM_UTIL
typedef uint64_t dm_nsec;
typedef dm_nsec USING_AL_NSEC;
#else
#include "al/system/al_Time.hpp"
#include "al/math/al_Interval.hpp"
using namespace al;
typedef al_nsec USING_AL_NSEC;
#endif

class VideoTime
{
  // Overload to use steady_clock
  auto alSteadyTime(bool reset = false)
  {
    static auto t = std::chrono::steady_clock::now();
    if (reset)
    {
      t = std::chrono::steady_clock::now();
    }
    return t;
  }

private:
  USING_AL_NSEC mStartTime;
  USING_AL_NSEC mCurrentTime;
  USING_AL_NSEC mTimePerFrame;
  USING_AL_NSEC mReadingTime;
  USING_AL_NSEC mDecodingTime;
  USING_AL_NSEC mThrashedTime;
  USING_AL_NSEC mPausedTime;
  USING_AL_NSEC mFrameInterval;
  USING_AL_NSEC mSumIntervaled;
  int mPreviousFrameCount;
  bool mPrintInformation;

public:
  VideoTime() :
    mReadingTime(0),
    mDecodingTime(0),
    mPausedTime(0),
    mThrashedTime(0),
    mSumIntervaled(0),
    mPreviousFrameCount(0),
    mPrintInformation(false)
  {}
  ~VideoTime() {}

  void summary()
  {
    if (mPrintInformation)
    {
#ifndef DM_UTIL
      std::cout << "Expected time: " << toTimecode(mFrameInterval * mPreviousFrameCount, "M:S:m\n") <<
                   "Total time   : " << toTimecode(mSumIntervaled, "M:S:m\n") <<
                   "Paused time  : " << toTimecode(mPausedTime, "M:S:m\n");
#endif
    }
  }

  USING_AL_NSEC now(bool _now = false)
  {
    if (_now)
    {
      mCurrentTime = steady_time_nsec();
    }
    return mCurrentTime;
  }

  USING_AL_NSEC reset()
  {
    mReadingTime = 0;
    mDecodingTime = 0;
    mPausedTime = 0;
    mThrashedTime = 0;
    mSumIntervaled = 0;
    mPreviousFrameCount = 0;
    mPrintInformation = false;
    reset_steady_clock();
    mCurrentTime = steady_time_nsec();
    return mCurrentTime;
  }

  USING_AL_NSEC start()
  {
    mStartTime = mCurrentTime = now();
    return now();
  }

  USING_AL_NSEC end(int framecount)
  {
    mPreviousFrameCount = framecount;
    return now(true);
  }

  USING_AL_NSEC check(std::string in = "")
  {
    USING_AL_NSEC time = steady_time_nsec() - now(true);
    if (in.compare("Reading") == 0)
    {
      mReadingTime += time;
    }
    else if (in.compare("Decoding") == 0)
    {
      mDecodingTime += time;
    }
    else if (in.compare("Pause") == 0)
    {
      mThrashedTime += time;
    }
    else if (in.compare("Resume") == 0)
    {
      mPausedTime += time;
    }

    return now();
  }

  bool sinkWithFrame(int framecount)
  {
    USING_AL_NSEC targetTime = mFrameInterval * framecount;
    USING_AL_NSEC flowTime = now(true) - mStartTime;
    if (framecount != mPreviousFrameCount)
    {
      if (mPrintInformation)
      {
#ifndef DM_UTIL
        std::cout << "Expected fps: " << 1 / (mFrameInterval * al_time_ns2s) << std::endl <<
          "Current  fps: " << framecount / ((flowTime - mPausedTime) * al_time_ns2s) << std::endl;
#endif
      }
      mPreviousFrameCount = framecount;
    }
    if (targetTime + mPausedTime < flowTime)
    {
      return true;
    }
    else
    {
      return false;
    }
  }

  USING_AL_NSEC sleep_for_test(int framecount)
  {
    if (framecount > mPreviousFrameCount)
    {
      mSumIntervaled += mReadingTime + mDecodingTime + mThrashedTime;
      USING_AL_NSEC targetTime = mFrameInterval * framecount;
      if (targetTime > mSumIntervaled)
      {
        USING_AL_NSEC diffTime = targetTime - mSumIntervaled;
        std::this_thread::sleep_for(std::chrono::nanoseconds(diffTime));
        mSumIntervaled += diffTime;
        if (mPrintInformation)
        {
#ifndef DM_UTIL
          std::cout << "Frame" << framecount << ": " << toTimecode(diffTime, "M:S:m") << std::endl;
#endif
        }
      }
      mReadingTime = mDecodingTime = mThrashedTime = 0;
      mPreviousFrameCount = framecount;
    }
    return now(true);
  }

  USING_AL_NSEC setInterval(uint64_t interval)
  {
    mFrameInterval = interval;
    return mFrameInterval;
  }

  USING_AL_NSEC interval()
  {
    return mFrameInterval;
  }

  void start_steady_clock() { alSteadyTime(); }
  void reset_steady_clock() { alSteadyTime(true); }

  USING_AL_NSEC steady_time_nsec()
  {
    return (std::chrono::duration_cast<std::chrono::nanoseconds>(
      std::chrono::steady_clock::now() - alSteadyTime()) 
      .count());
  }
};


#endif
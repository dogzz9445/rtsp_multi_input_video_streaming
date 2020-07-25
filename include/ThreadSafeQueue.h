#ifndef _THREAD_SAFE_QUEUE_H_
#define _THREAD_SAFE_QUEUE_H_

#include <mutex>
#include <condition_variable>
#include <queue>
#include <atomic>

template <typename Data> 
class ThreadSafeQueue
{
public:
  std::queue<Data>        mQueue;
  size_t                  mSize;
  std::mutex              mSafetyMutex;
  std::condition_variable mConditionFull;
  std::condition_variable mConditionEmpty;
  std::atomic_bool        mClosed;

public:
  ThreadSafeQueue(size_t size) :
    mSize(size),
    mClosed(false)
  {}
  virtual void reset();
  bool isEmpty()
  {
    return mQueue.empty();
  }
  void push(Data data)
  {
    std::unique_lock<std::mutex> lock(mSafetyMutex);
    while (!mClosed)
    {
      if (mQueue.size() < mSize)
      {
        mQueue.push(data);
        mConditionEmpty.notify_all();
        return;
      }
      {
        mConditionFull.wait(lock);
      }
    }
  }
  // Data : AVPacket*, AVFrame*
  Data pop(Data data = NULL)
  {
    std::unique_lock<std::mutex> lock(mSafetyMutex);
    while (!mClosed)
    {
      if (!mQueue.empty())
      {
        Data redata = NULL;
        data = mQueue.front();
        redata = mQueue.front();
        mQueue.pop();
        mConditionFull.notify_all();
        return redata;
      }
      else
      {
        mConditionEmpty.wait(lock);
      }
    }
    return NULL;
  }
  void resize(size_t size)
  {
    mSize = size;
  }
  void close(bool closed = true)
  {
    mClosed = closed;
    mConditionEmpty.notify_all();
    mConditionFull.notify_all();
  }
  bool empty()
  {
    return mQueue.empty();
  }

};

#endif
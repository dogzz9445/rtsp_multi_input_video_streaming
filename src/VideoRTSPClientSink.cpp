#include "VideoRTSPClientSink.h"

#define DUMMY_SINK_RECEIVE_BUFFER_SIZE 100000

VideoRTSPClientSink* VideoRTSPClientSink::createNew(UsageEnvironment& env, MediaSubsession& subsession, char const* streamId) {
  return new VideoRTSPClientSink(env, subsession, streamId);
}

VideoRTSPClientSink::VideoRTSPClientSink(UsageEnvironment& env, MediaSubsession& subsession, char const* streamId)
  : MediaSink(env),
  fSubsession(subsession)
{
  fStreamId = strDup(streamId);
  fReceiveBuffer = new u_int8_t[DUMMY_SINK_RECEIVE_BUFFER_SIZE];
}

VideoRTSPClientSink::~VideoRTSPClientSink()
{
  delete[] fReceiveBuffer;
  delete[] fStreamId;
}

void VideoRTSPClientSink::afterGettingFrame(void* clientData, unsigned frameSize, unsigned numTruncatedBytes,
  struct timeval presentationTime, unsigned durationInMicroseconds)
{
  VideoRTSPClientSink* sink = (VideoRTSPClientSink*)clientData;
  sink->afterGettingFrame(frameSize, numTruncatedBytes, presentationTime, durationInMicroseconds);
}

// If you don't want to see debugging output for each received frame, then comment out the following line:
#define DEBUG_PRINT_EACH_RECEIVED_FRAME 1

void VideoRTSPClientSink::afterGettingFrame(unsigned frameSize, unsigned numTruncatedBytes,
  struct timeval presentationTime, unsigned /*durationInMicroseconds*/)
{
  // We've just received a frame of data.  (Optionally) print out information about it:
#ifdef DEBUG_PRINT_EACH_RECEIVED_FRAME
  if (fStreamId != NULL) envir() << "Stream \"" << fStreamId << "\"; ";
  envir() << fSubsession.mediumName() << "/" << fSubsession.codecName() << ":\tReceived " << frameSize << " bytes";
  if (numTruncatedBytes > 0) envir() << " (with " << numTruncatedBytes << " bytes truncated)";
  char uSecsStr[6 + 1]; // used to output the 'microseconds' part of the presentation time
  sprintf(uSecsStr, "%06u", (unsigned)presentationTime.tv_usec);
  envir() << ".\tPresentation time: " << (int)presentationTime.tv_sec << "." << uSecsStr;
  if (fSubsession.rtpSource() != NULL && !fSubsession.rtpSource()->hasBeenSynchronizedUsingRTCP())
  {
    envir() << "!"; // mark the debugging output to indicate that this presentation time is not RTCP-synchronized
  }
#ifdef DEBUG_PRINT_NPT
  envir() << "\tNPT: " << fSubsession.getNormalPlayTime(presentationTime);
#endif
  envir() << "\n";
#endif

  // Then continue, to request the next frame of data:
  continuePlaying();
}

Boolean VideoRTSPClientSink::continuePlaying()
{
  if (fSource == NULL) return False; // sanity check (should not happen)

  // Request the next frame of data from our input source.  "afterGettingFrame()" will get called later, when it arrives:
  fSource->getNextFrame(fReceiveBuffer, DUMMY_SINK_RECEIVE_BUFFER_SIZE,
    afterGettingFrame, this, onSourceClosure, this);
  return True;
}

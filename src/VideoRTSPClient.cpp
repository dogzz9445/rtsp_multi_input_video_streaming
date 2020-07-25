#include "VideoRTSPClient.h"

VideoRTSPClient* VideoRTSPClient::createNew(UsageEnvironment& env, char const* rtspURL,
  int verbosityLevel, char const* applicationName, portNumBits tunnelOverHTTPPortNum) 
{
  return new VideoRTSPClient(env, rtspURL, verbosityLevel, applicationName, tunnelOverHTTPPortNum);
}

VideoRTSPClient::VideoRTSPClient(UsageEnvironment& env, char const* rtspURL,
  int verbosityLevel, char const* applicationName, portNumBits tunnelOverHTTPPortNum)
  : RTSPClient(env, rtspURL, verbosityLevel, applicationName, tunnelOverHTTPPortNum, -1) 
{
}

VideoRTSPClient::~VideoRTSPClient() 
{
}
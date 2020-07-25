#include "VideoReceiverMediaApp.h"

using namespace std::placeholders;

VideoReceiverMediaApp::VideoReceiverMediaApp() :
  VideoBasicLive555(),
  VideoDecoderApp(),
  mRTSPClientCount(0)
{
}
VideoReceiverMediaApp::VideoReceiverMediaApp(std::string srcAddress, uint16_t rtspPort, uint16_t baseRtpPort) :
  VideoReceiverMediaApp()
{
  setupSourceAddress(srcAddress, rtspPort, baseRtpPort);
}

VideoReceiverMediaApp::~VideoReceiverMediaApp()
{
}

void VideoReceiverMediaApp::init()
{
  VideoDecoderApp::init();
  registerReceivePacket(std::bind(&VideoReceiverMediaApp::receivePacket, this));
  registerContinueAfterSetup(std::bind(&VideoReceiverMediaApp::continueAfterSETUP, this, _1, _2, _3));
  registerContinueAfterDescribe(std::bind(&VideoReceiverMediaApp::continueAfterDESCRIBE, this, _1, _2, _3));
  registerContinueAfterPlay(std::bind(&VideoReceiverMediaApp::continueAfterPLAY, this, _1, _2, _3));
  registerSubsessionAfterPlaying(std::bind(&VideoReceiverMediaApp::subsessionAfterPlaying, this, _1));
  registerSubsessionAfterByeHandler(std::bind(&VideoReceiverMediaApp::subsessionByeHandler, this, _1, _2));
  registerStreamTimerHandler(std::bind(&VideoReceiverMediaApp::streamTimerHandler, this, _1));

  int clientdata;
  registerStopPlayingAll(std::bind(&VideoReceiverMediaApp::stopPlayingAll, this, _1));
  mEvents[EVENT_STOP_ALL] = mEnv->taskScheduler().createEventTrigger((TaskFunc*)(StopPlayingAll, clientdata));
}

void VideoReceiverMediaApp::addVideo(const char* inputUrl, const char* outputUrl, int portNum)
{
  int streamId = addReceiveStream(reader);
  int coreId = addVideoDecoderCore(inputUrl);

  // TODO:
  openURL("RTSP streaming", "rtsp://~~~~");
}

int VideoReceiverMediaApp::openURL(const char* progName, const char* rtspURL)
{
  RTSPClient* rtspClient = VideoRTSPClient::createNew(*mEnv, rtspURL, RTSP_CLIENT_VERBOSITY_LEVEL, progName);

  if (rtspClient == NULL)
  {
    std::cerr << "Failed to create a RTSP client for URL " << rtspURL << mEnv->getResultMsg();
    return;
  }

  mRTSPClients.push_back(rtspClient);
  rtspClient->sendDescribeCommand((RTSPClient::responseHandler*)(&funcContinueAfterDescribe));
  return mRTSPClientCount++;
}

void VideoReceiverMediaApp::stopPlayingAll(void* clientdata)
{
  mRTSPServer->closeAllClientSessionsForServerMediaSession(mServerMediaSession);
  mServerMediaSession->deleteAllSubsessions();

  for (auto stream = mStreams.begin(); stream != mStreams.end(); stream++)
  {
    stream->close();
  }
  mStreams.clear();
}

int VideoReceiverMediaApp::addReceiveStream()
{
  static int rtpPortNum = mBaseRTPPort;
  RTSPAddress address;
  address.Address = mSrcAddress;
  address.RTSPPort = mBaseRTSPPort;
  mAddresses.push_back(address);

  std::string rtspUrl = (std::string("rtsp://") + std::to_string(mSrcAddress.S_un.S_addr) + std::to_string(mBaseRTSPPort));
  return openURL("Test Program", rtspUrl.c_str());
}

void VideoReceiverMediaApp::playLoop()
{
  std::thread receivingPacket(funcReceivePacket);
  std::thread readingPacket(funcReadPacket, &mDecoderCores);
  std::thread decodingPacket(funcDecodePacket, &mDecoderCores);
  std::thread scalingFrame(funcScaleFrame, &mDecoderCores);

  mMainTask.start();
  funcSinkVideo();

  readingPacket.join();
  decodingPacket.join();
  scalingFrame.join();
  receivingPacket.join();
}


void VideoReceiverMediaApp::receivePacket()
{
  while (checkStateOnPlaying())
  {
  }
  Stop();
}

void VideoReceiverMediaApp::setupNextSubsession(RTSPClient* rtspClient)
{
  UsageEnvironment& env = rtspClient->envir(); // alias
  StreamClientState& scs = ((VideoRTSPClient*)rtspClient)->mStreamClientState; // alias

  scs.subsession = scs.iter->next();
  if (scs.subsession != NULL) 
  {
    if (!scs.subsession->initiate()) 
    {
      std::cerr << *rtspClient << "Failed to initiate the \"" << *scs.subsession << "\" subsession: " << env.getResultMsg() << "\n";
      setupNextSubsession(rtspClient); // give up on this subsession; go to the next one
    }
    else 
    {
      std::cout << *rtspClient << "Initiated the \"" << *scs.subsession << "\" subsession (";
      if (scs.subsession->rtcpIsMuxed()) 
      {
        std::cout << "client port " << scs.subsession->clientPortNum();
      }
      else 
      {
        std::cout << "client ports " << scs.subsession->clientPortNum() << "-" << scs.subsession->clientPortNum() + 1;
      }
      std::cout << ")\n";

      // Continue setting up this subsession, by sending a RTSP "SETUP" command:
      rtspClient->sendSetupCommand(*scs.subsession, (RTSPClient::responseHandler*)(&funcContinueAfterSetup), False, REQUEST_STREAMING_OVER_TCP);
    }
    return;
  }

  // We've finished setting up all of the subsessions.  Now, send a RTSP "PLAY" command to start the streaming:
  if (scs.session->absStartTime() != NULL) 
  {
    // Special case: The stream is indexed by 'absolute' time, so send an appropriate "PLAY" command:
    rtspClient->sendPlayCommand(*scs.session, (RTSPClient::responseHandler*)(&funcContinueAfterPlay), scs.session->absStartTime(), scs.session->absEndTime());
  }
  else 
  {
    scs.duration = scs.session->playEndTime() - scs.session->playStartTime();
    rtspClient->sendPlayCommand(*scs.session, (RTSPClient::responseHandler*)(&funcContinueAfterPlay));
  }
}

void VideoReceiverMediaApp::continueAfterSETUP(RTSPClient* rtspClient, int resultCode, char* resultString) 
{
  do 
  {
    UsageEnvironment& env = rtspClient->envir(); // alias
    StreamClientState& scs = ((VideoRTSPClient*)rtspClient)->mStreamClientState; // alias

    if (resultCode != 0) 
    {
      std::cerr << *rtspClient << "Failed to set up the \"" << *scs.subsession << "\" subsession: " << resultString << "\n";
      break;
    }

    std::cout << *rtspClient << "Set up the \"" << *scs.subsession << "\" subsession (";
    if (scs.subsession->rtcpIsMuxed()) 
    {
      std::cout << "client port " << scs.subsession->clientPortNum();
    }
    else 
    {
      std::cout << "client ports " << scs.subsession->clientPortNum() << "-" << scs.subsession->clientPortNum() + 1;
    }
    std::cout << ")\n";

    // Having successfully setup the subsession, create a data sink for it, and call "startPlaying()" on it.
    // (This will prepare the data sink to receive data; the actual flow of data from the client won't start happening until later,
    // after we've sent a RTSP "PLAY" command.)

    scs.subsession->sink = VideoRTSPClientSink::createNew(env, *scs.subsession, rtspClient->url());
    // perhaps use your own custom "MediaSink" subclass instead
    if (scs.subsession->sink == NULL) 
    {
      std::cerr << *rtspClient << "Failed to create a data sink for the \"" << *scs.subsession
        << "\" subsession: " << env.getResultMsg() << "\n";
      break;
    }

    std::cout << *rtspClient << "Created a data sink for the \"" << *scs.subsession << "\" subsession\n";
    scs.subsession->miscPtr = rtspClient; // a hack to let subsession handler functions get the "RTSPClient" from the subsession 
    scs.subsession->sink->startPlaying(*(scs.subsession->readSource()),
      (MediaSink::afterPlayingFunc*)(&funcSubsessionAfterPlaying), scs.subsession);
    // Also set a handler to be called if a RTCP "BYE" arrives for this subsession:
    if (scs.subsession->rtcpInstance() != NULL) 
    {
      scs.subsession->rtcpInstance()->setByeWithReasonHandler((ByeWithReasonHandlerFunc*)(&funcSubsessionAfterByeHandler), scs.subsession);
    }
  } while (0);
  delete[] resultString;

  // Set up the next subsession, if any:
  setupNextSubsession(rtspClient);
}

void VideoReceiverMediaApp::continueAfterDESCRIBE(RTSPClient* rtspClient, int resultCode, char* resultString) 
{
  do 
  {
    UsageEnvironment& env = rtspClient->envir(); // alias
    StreamClientState& scs = ((VideoRTSPClient*)rtspClient)->mStreamClientState; // alias

    if (resultCode != 0) 
    {
      std::cerr << *rtspClient << "Failed to get a SDP description: " << resultString << "\n";
      delete[] resultString;
      break;
    }

    char* const sdpDescription = resultString;
    std::cout << *rtspClient << "Got a SDP description:\n" << sdpDescription << "\n";

    // Create a media session object from this SDP description:
    scs.session = MediaSession::createNew(env, sdpDescription);
    delete[] sdpDescription; // because we don't need it anymore
    if (scs.session == NULL) 
    {
      std::cout << *rtspClient << "Failed to create a MediaSession object from the SDP description: " << env.getResultMsg() << "\n";
      break;
    }
    else if (!scs.session->hasSubsessions()) 
    {
      std::cout << *rtspClient << "This session has no media subsessions (i.e., no \"m=\" lines)\n";
      break;
    }

    // Then, create and set up our data source objects for the session.  We do this by iterating over the session's 'subsessions',
    // calling "MediaSubsession::initiate()", and then sending a RTSP "SETUP" command, on each one.
    // (Each 'subsession' will have its own data source.)
    scs.iter = new MediaSubsessionIterator(*scs.session);
    setupNextSubsession(rtspClient);
    return;
  } while (0);

  // An unrecoverable error occurred with this stream.
  shutdownStream(rtspClient);
}

void VideoReceiverMediaApp::continueAfterPLAY(RTSPClient* rtspClient, int resultCode, char* resultString) 
{
  Boolean success = False;

  do 
  {
    UsageEnvironment& env = rtspClient->envir(); // alias
    StreamClientState& scs = ((VideoRTSPClient*)rtspClient)->mStreamClientState; // alias

    if (resultCode != 0) 
    {
      std::cerr << *rtspClient << "Failed to start playing session: " << resultString << "\n";
      break;
    }

    // Set a timer to be handled at the end of the stream's expected duration (if the stream does not already signal its end
    // using a RTCP "BYE").  This is optional.  If, instead, you want to keep the stream active - e.g., so you can later
    // 'seek' back within it and do another RTSP "PLAY" - then you can omit this code.
    // (Alternatively, if you don't want to receive the entire stream, you could set this timer for some shorter value.)
    if (scs.duration > 0) 
    {
      unsigned const delaySlop = 2; // number of seconds extra to delay, after the stream's expected duration.  (This is optional.)
      scs.duration += delaySlop;
      unsigned uSecsToDelay = (unsigned)(scs.duration * 1000000);
      scs.streamTimerTask = env.taskScheduler().scheduleDelayedTask(uSecsToDelay, (TaskFunc*)(&funcStreamTimerHandler), rtspClient);
    }

    std::cout << *rtspClient << "Started playing session";
    if (scs.duration > 0) 
    {
      std::cout << " (for up to " << scs.duration << " seconds)";
    }
    std::cout << "...\n";

    success = True;
  } while (0);
  delete[] resultString;

  if (!success) 
  {
    // An unrecoverable error occurred with this stream.
    shutdownStream(rtspClient);
  }
}

void VideoReceiverMediaApp::subsessionAfterPlaying(void* clientData) 
{
  MediaSubsession* subsession = (MediaSubsession*)clientData;
  RTSPClient* rtspClient = (RTSPClient*)(subsession->miscPtr);

  // Begin by closing this subsession's stream:
  Medium::close(subsession->sink);
  subsession->sink = NULL;

  // Next, check whether *all* subsessions' streams have now been closed:
  MediaSession& session = subsession->parentSession();
  MediaSubsessionIterator iter(session);
  while ((subsession = iter.next()) != NULL) 
  {
    if (subsession->sink != NULL) return; // this subsession is still active
  }

  // All subsessions' streams have now been closed, so shutdown the client:
  shutdownStream(rtspClient);
}

void VideoReceiverMediaApp::subsessionByeHandler(void* clientData, char const* reason) 
{
  MediaSubsession* subsession = (MediaSubsession*)clientData;
  RTSPClient* rtspClient = (RTSPClient*)subsession->miscPtr;
  UsageEnvironment& env = rtspClient->envir(); // alias

  std::cout << *rtspClient << "Received RTCP \"BYE\"";
  if (reason != NULL) 
  {
    std::cerr << " (reason:\"" << reason << "\")";
    delete[](char*)reason;
  }
  std::cout << " on \"" << *subsession << "\" subsession\n";

  // Now act as if the subsession had closed:
  subsessionAfterPlaying(subsession);
}

void VideoReceiverMediaApp::streamTimerHandler(void* clientData) 
{
  VideoRTSPClient* rtspClient = (VideoRTSPClient*)clientData;
  StreamClientState& scs = rtspClient->mStreamClientState; // alias

  scs.streamTimerTask = NULL;

  // Shut down the stream:
  shutdownStream(rtspClient);
}

void VideoReceiverMediaApp::shutdownStream(RTSPClient* rtspClient, int exitCode) 
{
  UsageEnvironment& env = rtspClient->envir(); // alias
  StreamClientState& scs = ((VideoRTSPClient*)rtspClient)->mStreamClientState; // alias

  // First, check whether any subsessions have still to be closed:
  if (scs.session != NULL) 
  {
    Boolean someSubsessionsWereActive = False;
    MediaSubsessionIterator iter(*scs.session);
    MediaSubsession* subsession;

    while ((subsession = iter.next()) != NULL) 
    {
      if (subsession->sink != NULL) 
      {
        Medium::close(subsession->sink);
        subsession->sink = NULL;

        if (subsession->rtcpInstance() != NULL) 
        {
          subsession->rtcpInstance()->setByeHandler(NULL, NULL); // in case the server sends a RTCP "BYE" while handling "TEARDOWN"
        }

        someSubsessionsWereActive = True;
      }
    }

    if (someSubsessionsWereActive) 
    {
      // Send a RTSP "TEARDOWN" command, to tell the server to shutdown the stream.
      // Don't bother handling the response to the "TEARDOWN".
      rtspClient->sendTeardownCommand(*scs.session, NULL);
    }
  }

  std::cout << *rtspClient << "Closing the stream.\n";
  Medium::close(rtspClient);
  // Note that this will also cause this stream's "StreamClientState" structure to get reclaimed.

  if (--mRTSPClientCount == 0) 
  {
    // The final stream has ended, so exit the application now.
    // (Of course, if you're embedding this code into your own application, you might want to comment this out,
    // and replace it with "eventLoopWatchVariable = 1;", so that we leave the LIVE555 event loop, and continue running "main()".)
    exit(exitCode);
  }
}

bool VideoReceiverMediaApp::setupSourceAddress(std::string srcAddress, uint16_t rtspPort, uint16_t baseRtpPort)
{
  NetAddressList addresses(srcAddress.c_str());
  auto addr = *(const char**)addresses.firstAddress()->data();
  inet_pton(AF_INET, addr, &(mSrcAddress.s_addr));
  mBaseRTSPPort = rtspPort;
  mBaseRTPPort = baseRtpPort;
}

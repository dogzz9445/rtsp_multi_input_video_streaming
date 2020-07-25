#include "VideoDecoderCore.h"

VideoDecoderCore::VideoDecoderCore() :
  mInputFormatContext(NULL),
  mVideoContext(NULL),
  mAudioContext(NULL),
  mVideoCodec(NULL),
  mAudioCodec(NULL),
  mVideoStream(NULL),
  mAudioStream(NULL),
  mVideoStreamIDX(-1),
  mAudioStreamIDX(NULL),
  mSwsYUVtoRGBAContext(NULL),
  mVideoFrameCount(0),
  mAudioFrameCount(0),
  mReferenceCount(0),
  mHWDeviceContext(NULL),
  mInputUrl(NULL),
  mHWType(NULL),
  mFlagReadFromBuffer(false)
{}

VideoDecoderCore::~VideoDecoderCore()
{
  avcodec_free_context(&mVideoContext);
  avcodec_free_context(&mAudioContext);
  avformat_close_input(&mInputFormatContext);

  if (mHWType)
  {
    av_buffer_unref(&mHWDeviceContext);
  }
  sws_freeContext(mSwsYUVtoRGBAContext);

  mDecoderBuffer->close();
  mDecoderBuffer->reset();
  mDecoderBuffer.reset();
}

/*-----------------------------------------------------------
Functions for callback
-----------------------------------------------------------*/
bool VideoDecoderCore::read()
{
  AVPacket* packet = mDecoderBuffer->PopPoolPacket();
  if (packet == nullptr)
  {
    return VIDEO_NULL_PTR;
  }
  if (av_read_frame(mInputFormatContext, packet) < 0)
  {
    mDecoderBuffer->close();
    return true;
  }
  mDecoderBuffer->PushPacket(packet);

  AVPacket *newPacket = new AVPacket;
  av_init_packet(newPacket);
  newPacket->data = NULL;
  newPacket->size = 0;
  mDecoderBuffer->PushPoolPacket(newPacket);
  return false;
}

int VideoDecoderCore::convert()
{
  AVFrame *inframe = mDecoderBuffer->PopVideo();
  AVFrame *outframe = mDecoderBuffer->PopPoolPicture();
  if (inframe == nullptr || outframe == nullptr)
  {
    return VIDEO_NULL_PTR;
  }
  sws_scale(mSwsYUVtoRGBAContext, inframe->data, inframe->linesize,
    0, inframe->height, outframe->data, outframe->linesize);
  mDecoderBuffer->PushPicture(outframe);
  av_frame_free(&inframe);
  AVFrame *newoutframe = NULL;
  newoutframe = av_frame_alloc();
  uint8_t* data = new uint8_t[mVideoWidth * mVideoHeight * 4];
  avpicture_fill((AVPicture*)newoutframe, &data[0], AV_PIX_FMT_RGBA, mVideoWidth, mVideoHeight);
  mDecoderBuffer->PushPoolPicture(newoutframe);
  return 1;
}

int VideoDecoderCore::decode_video_packet(AVPacket *packet, int *eofframe)
{
  AVFrame *frame = mDecoderBuffer->PopPoolVideo();
  if (frame == nullptr)
  {
    return VIDEO_NULL_PTR;
  }

  int ret = 0;
  if (mHWType)
  {
    ret = avcodec_send_packet(mVideoContext, packet);
  }
  else
  {
    ret = avcodec_decode_video2(mVideoContext, frame, eofframe, packet);
    if (ret < 0)
    {
      std::cerr << "Error decoding video frame: " << ret << "\n";
      return ret;
    }
    mDecoderBuffer->PushVideo(frame);
  }

  AVFrame *newframe = NULL;
  newframe = av_frame_alloc();
  mDecoderBuffer->PushPoolVideo(newframe);
  return ret;
}

int VideoDecoderCore::decode_audio_packet(AVPacket *packet, int *eofframe)
{
  AVFrame *frame = mDecoderBuffer->PopPoolAudio();
  //AVFrame *outframe = buffer->PopPoolAudioOut();
  if (frame == nullptr /*|| outframe == nullptr*/)
  {
    return VIDEO_NULL_PTR;
  }
  int ret = 0;
  ret = avcodec_decode_audio4(mAudioContext, frame, eofframe, packet);
  if (ret < 0)
  {
    std::cerr << "Error decoding audio frame" << ret << "\n";
    return ret;
  }
  /* Some audio decoders decode only part of the packet, and have to be
   * called again with the remainder of the packet data.
   * Sample: fate-suite/lossless-audio/luckynight-partial.shn
   * Also, some decoders might over-read the packet. */
   //decoded = FFMIN(ret, mPacket.size);

   //if (*eofframe) {
   //  size_t unpadded_linesize = frame->nb_samples * av_get_bytes_per_sample(frame->format);
   //  printf("audio_frame%s n:%d nb_samples:%d pts:%s\n",
   //    cached ? "(cached)" : "",
   //    mAudioFrameCount++, mFrame->nb_samples,
   //    av_ts2timestr(mFrame->pts, &mAudioContext->time_base));

   //  //fwrite(frame->extended_data[0], 1, unpadded_linesize, audio_dst_file);
   //}
  av_frame_free(&frame);

  AVFrame *newframe = NULL;
  newframe = av_frame_alloc();
  //AVFrame *newoutframe = NULL;
  mDecoderBuffer->PushPoolAudio(newframe);
  //buffer->PushPoolAudioOut(newoutframe);
  return ret;
}

int VideoDecoderCore::decode()
{
  int eofframe = 0;
  AVPacket* packet = mDecoderBuffer->PopPacket();
  if (packet == nullptr)
  {
    return VIDEO_NULL_PTR;
  }

  do {
    int ret = packet->size;
    eofframe = 0;
    if (packet->stream_index == mVideoStreamIDX)
    {
      decode_video_packet(packet, &eofframe);
    }
    else if (packet->stream_index == mAudioStreamIDX)
    {
      decode_audio_packet(packet, &eofframe);
    }

    if (ret < 0) break;
    packet->data += ret;
    packet->size -= ret;
  } while (packet->size > 0);

  av_packet_unref(packet);
  av_free_packet(packet);
  return 1;
}

// TODO:
int VideoDecoderCore::flush_video_frame()
{

}

int VideoDecoderCore::flush_audio_frame()
{

}

bool VideoDecoderCore::initialize()
{
  // TODO:
  // check configurations
  if (mFlagReadFromBuffer)
  {
    VideoReader<uint8_t> reader;
    mInputFormatContext = reader.getAVFormatContextFromVideoStreamReader();
  }

  // Initialize the decoder's contexts
  if (!init_input_context_from_file(mInputUrl, mHWType))
  {
    std::cerr << "Failed to initialize input context\n";
    return false;
  }

  // Set yuv to rgba contexts
  mSwsYUVtoRGBAContext = sws_getContext(mVideoWidth, mVideoHeight, AV_PIX_FMT_YUV420P,
                                mVideoWidth, mVideoHeight, AV_PIX_FMT_RGBA, 0, 0, 0, 0);
  
  // Initialize the buffer
  if (!init_pool_buffer())
  {
    std::cerr << "Failed to initialize pool_buffer\n";
    return false;
  }

  return true;
}

bool VideoDecoderCore::init_input_context_from_file(const char* url, const char* hwType = NULL)
{
  // Initialize input contexts
  if (hwType)
  {
    mHWDeviceType = av_hwdevice_find_type_by_name(hwType);
    if (mHWDeviceType == AV_HWDEVICE_TYPE_NONE) {
      std::cerr << ("Device type %s is not supported.\n", hwType);
      std::cerr << "Available device types:";
      while ((mHWDeviceType = av_hwdevice_iterate_types(mHWDeviceType)) != AV_HWDEVICE_TYPE_NONE)
        std::cerr << (" %s", av_hwdevice_get_type_name(mHWDeviceType));
      std::cerr << "\n";
      return false;
    }
  }
  if (avformat_open_input(&mInputFormatContext, url, NULL, NULL) < 0)
  {
    std::cerr << "Could not open source file\n";
    return false;
  }

  if (avformat_find_stream_info(mInputFormatContext, NULL) < 0)
  {
    std::cerr << "Could not find stream information\n";
    return false;
  }

  // Open video codec
  if (open_codec_context(&mVideoStreamIDX, mVideoCodec, &mVideoContext, mInputFormatContext,
    AVMEDIA_TYPE_VIDEO, url, &mReferenceCount, mHWType != NULL) >= 0)
  {
    mVideoStream = mInputFormatContext->streams[mVideoStreamIDX];

    mFrameInterval = 1e9 * mVideoStream->r_frame_rate.den;
    mFrameInterval /= mVideoStream->r_frame_rate.num;
    mVideoWidth = mVideoContext->width;
    mVideoHeight = mVideoContext->height;
    mPixelFormat = mVideoContext->pix_fmt;
    mVideoCodecID = mVideoContext->codec_id;
  }

  // Open audio codec
  if (open_codec_context(&mAudioStreamIDX, mAudioCodec, &mAudioContext, mInputFormatContext,
    AVMEDIA_TYPE_AUDIO, url, &mReferenceCount) >= 0)
  {
    mAudioStream = mInputFormatContext->streams[mAudioStreamIDX];
    mAudioCodecID = mAudioContext->codec_id;
  }

  av_dump_format(mInputFormatContext, 0, url, 0);

  if (!mAudioStream && !mVideoStream)
  {
    std::cerr << "Could not find audio or video stream in the input, aborting\n";
    return false;
  }
}

bool VideoDecoderCore::init_pool_buffer()
{
  mDecoderBuffer = std::make_shared<VideoDecoderBuffer>();
  mDecoderBuffer->setResolution(mVideoWidth, mVideoHeight);
  for (int i = 0; i < 12; i++)
  {
    AVPacket* packet = new AVPacket;
    av_init_packet(packet);
    packet->data = NULL;
    packet->size = 0;
    mDecoderBuffer->PushPoolPacket(packet);
  }
  for (int i = 0; i < 4; i++)
  {
    AVFrame* frame = NULL;
    frame = av_frame_alloc();
    mDecoderBuffer->PushPoolVideo(frame);
  }
  for (int i = 0; i < 4; i++)
  {
    AVFrame* frame = NULL;
    frame = av_frame_alloc();
    uint8_t* data = new uint8_t[mVideoWidth * mVideoHeight * 4];
    avpicture_fill((AVPicture*)frame, &data[0], AV_PIX_FMT_RGBA, mVideoWidth, mVideoHeight);
    mDecoderBuffer->PushPoolPicture(frame);
  }
  for (int i = 0; i < 8; i++)
  {
    AVFrame* frame = NULL;
    frame = av_frame_alloc();
    mDecoderBuffer->PushPoolAudio(frame);
  }
  for (int i = 0; i < 12; i++)
  {
    AVFrame* frame = NULL;
    frame = av_frame_alloc();
    mDecoderBuffer->PushPoolAudioOut(frame);
  }
  return true;
}

bool VideoDecoderCore::init_hw_context(AVCodecContext *ctx, const enum AVHWDeviceType type)
{
  int ret = av_hwdevice_ctx_create(&mHWDeviceContext, type, NULL, NULL, 0);
  if (ret < 0)
  {
    std::cerr << "Failed to create specified HW device\n";
    return false;
  }
  ctx->hw_device_ctx = av_buffer_ref(mHWDeviceContext);
  return true;
}

int VideoDecoderCore::open_codec_context(int *stream_idx, AVCodec *dec,
  AVCodecContext **dec_ctx, AVFormatContext *formatContext, enum AVMediaType type,
  const char* filename, int *referenceCount, int hwinit)
{
  int ret, stream_index;
  AVStream *st;
  AVDictionary *opts = NULL;

  ret = av_find_best_stream(formatContext, type, -1, -1, NULL, 0);
  if (ret < 0)
  {
    fprintf(stderr, "Could not find %s stream in input file '%s'\n",
      av_get_media_type_string(type), filename);
    return ret;
  }
  stream_index = ret;
  st = formatContext->streams[stream_index];

  /* find decoder for the stream */
  if (hwinit)
  {
    for (int i = 0;; i++) {
      const AVCodecHWConfig *config = avcodec_get_hw_config(dec, i);
      if (!config) {
        fprintf(stderr, "Decoder %s does not support device type %s.\n",
          dec->name, av_hwdevice_get_type_name(mHWDeviceType));
        return -1;
      }
      if (config->methods & AV_CODEC_HW_CONFIG_METHOD_HW_DEVICE_CTX &&
        config->device_type == mHWDeviceType) {
        //mHWPixelFormat = config->pix_fmt;
        break;
      }
    }
  }
  else
  {
    dec = avcodec_find_decoder(st->codecpar->codec_id);
    if (!dec)
    {
      fprintf(stderr, "Failed to find %s codec\n",
        av_get_media_type_string(type));
      return AVERROR(EINVAL);
    }
  }

  /* Allocate a codec context for the decoder */
  *dec_ctx = avcodec_alloc_context3(dec);
  if (!*dec_ctx)
  {
    fprintf(stderr, "Failed to allocate the %s codec context\n", av_get_media_type_string(type));
    return AVERROR(ENOMEM);
  }

  /* Copy codec parameters from input stream to output codec context */
  if (avcodec_parameters_to_context(*dec_ctx, st->codecpar) < 0)
  {
    fprintf(stderr, "Failed to copy %s codec parameters to decoder context\n",
      av_get_media_type_string(type));
    return -1;
  }

  if (hwinit)
  {  
    using namespace std::placeholders;
    //(*dec_ctx)->get_format = &VideoDecoderCore::get_hw_format;

    if (init_hw_context(*dec_ctx, mHWDeviceType) < 0)
      return -1;
  }

  /* Init the decoders, with or without reference counting */
  av_dict_set(&opts, "refcounted_frames", *referenceCount ? "1" : "0", 0);
  if (avcodec_open2(*dec_ctx, dec, &opts) < 0)
  {
    fprintf(stderr, "Failed to open %s codec\n",
      av_get_media_type_string(type));
    return -1;
  }
  *stream_idx = stream_index;

  return 0;
}

//bool VideoDecoderCore::init_output_context()
//{
//  //if (avformat_alloc_output_context2(&mOutputFormatContext, NULL, "mov", mOutputUrl) < 0)
//  //if (avformat_alloc_output_context2(&mOutputFormatContext, NULL, "rtsp", mOutputUrl) < 0)
//  //if (avformat_alloc_output_context2(&mOutputFormatContext, NULL, "mov", mOutputUrl) < 0)
//  //if (avformat_alloc_output_context2(&mOutputFormatContext, NULL, "rtmp", mOutputUrl) < 0)
//  if (avformat_alloc_output_context2(&mOutputFormatContext, NULL, "mpegts", mOutputUrl) < 0)
//  //if (avformat_alloc_output_context2(&mOutputFormatContext, NULL, "mov", mOutputUrl) < 0)
//  //if (avformat_alloc_output_context2(&mOutputFormatContext, NULL, "mov", mOutputUrl) < 0)
//  {
//    std::cerr << "Could not allocate output context\n";
//    if (avformat_alloc_output_context2(&mOutputFormatContext, NULL, "rtsp", mOutputUrl) < 0)
//    {
//      std::cerr << "Failed to initialize codec and context\n";
//    }
//    //avformat_alloc_output_context2(&mOutputFormatContext, NULL, "mpeg", mOutputUrl);
//  }
//
//  if (!mOutputFormatContext)
//    return false;
//
//  mOutputFormat = mOutputFormatContext->oformat;
//  if (!mOutputFormat)
//  {
//    std::cerr << "Error creating outformat\n";
//    return false;
//  }
//
//  // FIXME:
//  // For not defined the contexts of codec, should be define contexts (timebase, pixfmt...)
//  for (int i = 0; i < mInputFormatContext->nb_streams; i++)
//  {
//    AVStream *inStream = mInputFormatContext->streams[i];
//    auto pCodec = avcodec_find_decoder(inStream->codecpar->codec_id);
//    AVStream *outStream = avformat_new_stream(mOutputFormatContext, pCodec);
//    //AVStream *outStream = avformat_new_stream(mOutputFormatContext, inStream->codec->codec);
//
//    if (!outStream)
//    {
//      std::cerr << "Failed to allocate output streams\n";
//      return false;
//    }
//
//    // case 1:
//    if (avcodec_copy_context(outStream->codec, inStream->codec) < 0)
//    {
//      std::cerr << "Failed to copy codec context\n";
//      return false;
//    }
//    outStream->sample_aspect_ratio.den = outStream->codec->sample_aspect_ratio.den;
//    outStream->sample_aspect_ratio.num =
//      inStream->codec->sample_aspect_ratio.num;
//    outStream->codec->codec_id = inStream->codec->codec_id;
//    //outStream->codec->time_base.num = 1;
//    //outStream->codec->time_base.den =
//    //  m_fps * (inStream->codec->ticks_per_frame);
//    //outStream->time_base.num = 1;
//    //outStream->time_base.den = 1000;
//    //outStream->r_frame_rate.num = m_fps;
//    //outStream->r_frame_rate.den = 1;
//    //outStream->avg_frame_rate.den = 1;
//    //outStream->avg_frame_rate.num = m_fps;
//
//    // case 2:
//    if (avcodec_parameters_to_context(outStream->codec, inStream->codecpar) < 0)
//    {
//      std::cerr << "Failed to copy codec context from input stream\n";
//      return false;
//    }
//
//    outStream->codec->codec_tag = 0;
//    if (mOutputFormatContext->oformat->flags & AVFMT_GLOBALHEADER)
//    {
//      outStream->codec->flags |= CODEC_FLAG_GLOBAL_HEADER;
//    }
//  }
//
//  av_dump_format(mOutputFormatContext, 0, mOutputUrl, 1);
//
//  if (mOutputFormat->flags & AVFMT_NOFILE)
//  {
//    std::cerr << "Failed to initialize output format\n";
//    return false;
//  }
//  else
//  {
//    int ret = avio_open(&mOutputFormatContext->pb, mOutputUrl, AVIO_FLAG_WRITE);
//    if (ret < 0)
//    {
//      char errorbuf[80];
//      std::cerr << "Could not open output URL\n";
//      std::cerr << ("Error: %s", av_make_error_string(errorbuf, 80, ret)) << std::endl;
//      return false;
//    }
//  }
//  //Write file header
//  if (avformat_write_header(mOutputFormatContext, NULL) < 0)
//  {
//    std::cerr << "Error occurred when opening output URL\n";
//    return false;
//  }
//  std::cout << "Success to init output contexts\n";
//  return true;
//}
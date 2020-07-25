
#include "al/math/al_Functions.hpp"
#include "al/graphics/al_Shapes.hpp"
#include "al/scene/al_PolySynth.hpp"
#include "al/app/al_OmniRendererDomain.hpp"
#include "al/scene/al_DynamicScene.hpp"

#include "al/app/al_App.hpp"
#include "al/app/al_DistributedApp.hpp"

#include "VideoSenderMediaApp.h"

#pragma comment (lib, "Ws2_32.lib" )
#pragma comment (lib, "winmm.lib")

#include <iostream>
#include <mutex>
#include <atomic>
#include <vector>
#include <functional>

using namespace std;
using namespace al;

// Sender Apop
class MyOmniRendererApp : public DistributedApp {
protected:
public:
  Mesh          mMesh;
  Texture*      mTexture;
  VideoSenderMediaApp*     mVideoApp;

  ~MyOmniRendererApp()
  {}

  virtual void onDraw(Graphics& g) override
  {
    g.clear(0.f, 0.f, 0.4f);

    if (mVideoApp)
    {
      mVideoApp->GetTexture(0, mTexture);
    }

    mTexture->bind();
    g.texture();
    g.draw(mMesh);
    mTexture->unbind();
  }

  bool onKeyDown(const Keyboard& k) override
  {
    if (k.key() == 'o')
    {
      if (!mVideoApp)
      {
        mVideoApp = new VideoSenderMediaApp;
        mVideoApp->Open
        (
         "C:\\allolib\\allolib-master\\x64\\x64\\Release\\360vr2_libx264.mp4", 
         //"*********************************", 
         "<address>", 
         SENDER_RTSP_WITH_DECODE, 
         NULL
        );
      }
    }
    if (mVideoApp)
    {
      mVideoApp->KeyDown(k);
    }
    return false;
  }

  void onCreate() override
  {
    av_register_all();
    avformat_network_init();

    al::addSphereWithTexcoords(mMesh, 2.f, 40);

    unsigned int width = 960;
    unsigned int height = 480;

    int stride = 4;
    int Nx = width;
    int Ny = height;

    std::vector<uint8_t> pixels;
    pixels.resize(stride * Nx * Ny);

    for (int j = 0; j < Ny; ++j) {
      float y = float(j) / (Ny - 1) * 2 - 1;
      for (int i = 0; i < Nx; ++i) {
        float x = float(i) / (Nx - 1) * 2 - 1;

        float m = 1 - al::clip(hypot(x, y));
        float a = al::wrap(atan2(y, x) / M_2PI);

        al::Color col = al::HSV(a, 1, m);

        int idx = j * Nx + i;
        pixels[idx * stride + 0] = col.r * 255.;
        pixels[idx * stride + 1] = col.g * 255.;
        pixels[idx * stride + 2] = col.b * 255.;
        pixels[idx * stride + 3] = col.a;
      }
    }

    if (!mTexture)
    {
      mTexture = new al::Texture;
    }
    mTexture->create2D(width, height, al::Texture::RGBA8, al::Texture::RGBA, al::Texture::UBYTE);
    mTexture->filterMag(al::Texture::LINEAR);
    mTexture->submit((pixels.data()));
  }

};

int main(int argc, const char** argv)
{
  MyOmniRendererApp app;
  app.configureAudio(44100., 256, 2, 0);

  app.start();
}
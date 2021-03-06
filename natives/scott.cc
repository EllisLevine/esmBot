#include <napi.h>
#include <list>
#include <Magick++.h>

using namespace std;
using namespace Magick;

class ScottWorker : public Napi::AsyncWorker {
 public:
  ScottWorker(Napi::Function& callback, string in_path, string type, int delay)
      : Napi::AsyncWorker(callback), in_path(in_path), type(type), delay(delay) {}
  ~ScottWorker() {}

  void Execute() {
    list <Image> frames;
    list <Image> coalesced;
    list <Image> mid;
    Image watermark;
    readImages(&frames, in_path);
    watermark.read("./assets/images/scott.png");
    coalesceImages(&coalesced, frames.begin(), frames.end());

    for (Image &image : coalesced) {
      Image watermark_new = watermark;
      image.virtualPixelMethod(Magick::TransparentVirtualPixelMethod);
      image.backgroundColor("none");
      image.scale(Geometry("415x234!"));
      double arguments[16] = {0, 0, 129, 187, 415, 0, 517, 182, 415, 234, 517, 465, 0, 234, 132, 418};
      image.distort(Magick::PerspectiveDistortion, 16, arguments, true);
      image.extent(Geometry("864x481"), Magick::CenterGravity);
      watermark_new.composite(image, Geometry("-110+83"), Magick::OverCompositeOp);
      watermark_new.magick(type);
      watermark_new.animationDelay(delay == 0 ? image.animationDelay() : delay);
      mid.push_back(watermark_new);
    }

    optimizeTransparency(mid.begin(), mid.end());

    if (type == "gif") {
      for (Image &image : mid) {
        image.quantizeDitherMethod(FloydSteinbergDitherMethod);
        image.quantize();
      }
    }

    writeImages(mid.begin(), mid.end(), &blob);
  }

  void OnOK() {
    Callback().Call({Env().Undefined(), Napi::Buffer<char>::Copy(Env(), (char *)blob.data(), blob.length())});
  }

 private:
  string in_path, type;
  int delay;
  Blob blob;
};

Napi::Value Scott(const Napi::CallbackInfo &info)
{
  Napi::Env env = info.Env();

  Napi::Object obj = info[0].As<Napi::Object>();
  Napi::Function cb = info[1].As<Napi::Function>();
  string path = obj.Get("path").As<Napi::String>().Utf8Value();
  string type = obj.Get("type").As<Napi::String>().Utf8Value();
  int delay = obj.Has("delay") ? obj.Get("delay").As<Napi::Number>().Int32Value() : 0;

  ScottWorker* blurWorker = new ScottWorker(cb, path, type, delay);
  blurWorker->Queue();
  return env.Undefined();
}
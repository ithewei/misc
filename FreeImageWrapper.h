#ifndef FREE_IMAGE_WRAPPER_H_
#define FREE_IMAGE_WRAPPER_H_

#include "FreeImage.h"

class FreeImageWrapper {
 public:
    FreeImageWrapper();
    ~FreeImageWrapper();

    static void Init();
    static void DeInit();

    bool LoadFromFile(const char* filepath);
    bool LoadFromMemory(const void* data, size_t size);

    bool Save(const char* filepath);

    int GetWidth() {
        return FreeImage_GetWidth(_bmp);
    }
    int GetHeight() {
        return FreeImage_GetHeight(_bmp);
    }
    int GetPitch() {
        return FreeImage_GetPitch(_bmp);
    }
    int GetBPP() {
        return FreeImage_GetBPP(_bmp);
    }
    BYTE* GetData() {
        return FreeImage_GetBits(_bmp);
    }
    size_t GetSize() {
        return GetPitch()*GetHeight();
    }

 protected:
    void unload();

 public:
    FIBITMAP* _bmp;
    FIMEMORY* _mem;

 private:
    static bool s_bInit;
};

#endif  // FREE_IMAGE_WRAPPER_H_


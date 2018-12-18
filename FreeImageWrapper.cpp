#include "FreeImageWrapper.h"

bool FreeImageWrapper::s_bInit = false;

FreeImageWrapper::FreeImageWrapper() {
    Init();

    _bmp = NULL;
    _mem = NULL;
}

FreeImageWrapper::~FreeImageWrapper() {
    unload();
}

void FreeImageWrapper::Init() {
    if (s_bInit == false) {
        s_bInit = true;
        FreeImage_Initialise();
    }
}

void FreeImageWrapper::DeInit() {
    if (s_bInit) {
        s_bInit = false;
        FreeImage_DeInitialise();
    }
}

void FreeImageWrapper::unload() {
    if (_bmp) {
        FreeImage_Unload(_bmp);
        _bmp = NULL;
    }

    if (_mem) {
        FreeImage_CloseMemory(_mem);
        _mem = NULL;
    }
}

bool FreeImageWrapper::LoadFromFile(const char* filepath) {
    unload();

    FREE_IMAGE_FORMAT fmt = FreeImage_GetFileType(filepath);
    if (fmt == FIF_UNKNOWN) {
        return false;
    }

    _bmp = FreeImage_Load(fmt, filepath, JPEG_EXIFROTATE);
    return _bmp != NULL;
}

bool FreeImageWrapper::LoadFromMemory(const void* data, size_t size) {
    unload();

    _mem = FreeImage_OpenMemory((BYTE*)data, size);
    if (_mem == NULL) {
        return false;
    }

    FREE_IMAGE_FORMAT fmt = FreeImage_GetFileTypeFromMemory(_mem);
    if (fmt == FIF_UNKNOWN) {
        return false;
    }

    _bmp = FreeImage_LoadFromMemory(fmt, _mem, JPEG_EXIFROTATE);
    return _bmp != NULL;
}

#include "hw/hlog.h"
#include "hw/hstring.h"
#include "hw/hplatform.h"  // for stricmp in linux
bool FreeImageWrapper::Save(const char* filepath) {
    if (_bmp == NULL) {
        return false;
    }

    // At present, we just provide bmp/jpg/png
    string strFile(filepath);
    const char* suffix = suffixname(strFile).c_str();
    FREE_IMAGE_FORMAT fmt = FIF_UNKNOWN;
    if (stricmp(suffix, "bmp") == 0) {
        fmt = FIF_BMP;
    } else if (stricmp(suffix, "jpg") == 0) {
        fmt = FIF_JPEG;
    } else if (stricmp(suffix, "png") == 0) {
        fmt = FIF_PNG;
    } else {
        hloge("Unsupported format %s", suffix);
        return false;
    }

    return FreeImage_Save(fmt, _bmp, filepath);
}


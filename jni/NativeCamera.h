// TAGRELEASE: PUBLIC
#ifndef NativeCamera_H
#define NativeCamera_H

#include "NvAppBase/NvSampleApp.h"
#include "NvGLUtils/NvImage.h"

#include "native_camera2/native_camera2.h"

#include <memory>

class NvStopWatch;
class NvFramerateCounter;

class NativeCamera : public NvSampleApp
{
public:
    NativeCamera(NvPlatformContext* platform);
    ~NativeCamera();

    void focusChanged(bool focused);
    void initUI(void);
    void initRendering(void);
    void draw(void);
    void reshape(int32_t width, int32_t height);

    void configurationCallback(NvEGLConfiguration& config);

protected:
    void drawStreamImage(const nv::camera2::CameraStream *stream,
            GLuint *textures);
    void setupStreamTextures( const nv::camera2::CameraStream *stream,
            GLuint *textures);
    void uploadImage( nv::camera2::CameraBuffer &img,
            GLuint *textures);

    std::unique_ptr<NvFramerateCounter> mFrameRate;
    std::unique_ptr<NvGLSLProgram> mProgYUV;
    GLuint mPreviewTextures[4];
    float mViewAspectRatio;

    // Camera setup and destruction
    void startCamera();
    void stopCamera();

    // Camera objects
    std::unique_ptr<nv::camera2::CameraManager> mCameraManager;
    std::unique_ptr<nv::camera2::CameraDevice>  mCameraDevice;
    std::unique_ptr<nv::camera2::CameraStream>  mPreviewStream;
    nv::camera2::StaticProperties mStaticProperties;
    nv::camera2::CaptureRequest mRequest;

};

#endif

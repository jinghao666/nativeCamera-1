// TAGRELEASE: PUBLIC
#include "NativeCamera.h"
#include "NvAppBase/NvFramerateCounter.h"
#include "NV/NvStopWatch.h"
#include "NvAssetLoader/NvAssetLoader.h"
#include "NvUI/NvTweakBar.h"
#include "NV/NvLogs.h"
#include "NvGLUtils/NvGLSLProgram.h"
#include "ImageSave.h"

NativeCamera::NativeCamera(NvPlatformContext* platform) :
    NvSampleApp(platform, "NativeCamera")
{
    mFrameRate.reset( new NvFramerateCounter(this) );
    mViewAspectRatio = 1.0f;

    // Required in all subclasses to avoid silent link issues
    forceLinkHack();
}

NativeCamera::~NativeCamera()
{
    LOGI("NativeCamera: destroyed\n");
}

void NativeCamera::configurationCallback(NvEGLConfiguration& config)
{ 
    config.depthBits = 24; 
    config.stencilBits = 0; 
    config.apiVer = NvGfxAPIVersionGL4();
}

void NativeCamera::focusChanged( bool focused ) {

    if ( focused )
    {
        startCamera();
    }
    else
    {
        stopCamera();
    }
}

void NativeCamera::initUI() {
    // sample apps automatically have a tweakbar they can use.
    if (mTweakBar) { // create our tweak ui
    }
}

void NativeCamera::initRendering(void) {

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    NvAssetLoaderAddSearchPath("gl4-kepler/NativeCamera");

    if (!requireMinAPIVersion(NvGfxAPIVersionGL4()))
        return;

    mProgYUV.reset(
            NvGLSLProgram::createFromFiles("shaders/plain.vert",
            "shaders/yuv.frag"));

    setupStreamTextures( mPreviewStream.get(), mPreviewTextures);
}

void NativeCamera::reshape(int32_t width, int32_t height)
{
    glViewport( 0, 0, (GLint) width, (GLint) height );
    mViewAspectRatio = (float)width/(float)height;

    CHECK_GL_ERROR();
}

void NativeCamera::startCamera()
{

    // Create an instance of the camera manager.
    mCameraManager = nv::camera2::CameraManager::createCameraManager();
    if ( !mCameraManager ) return;

    // Get the camera properties for the first camera (0)
    mCameraManager->queryStaticProperties( 0, mStaticProperties );

    // Create a camera device to refer to camera (0)
    mCameraDevice = mCameraManager->createCameraDevice( 0, NULL);
    if ( !mCameraDevice ) return;

    // Create a 1080p preview stream
    // 1080p is a supported size. You can query for other supported sizes
    // from mStaticProperties.availableYUVSizes
    mPreviewStream = mCameraDevice->createStream( nv::camera2::YCbCr_420_888,
            nv::camera2::Size(1920, 1080) );
    if ( !mPreviewStream ) return;

    // Initialize a request - the PREVIEW intent will initialize the request
    // with the default settings for viewfinder requests.
    mCameraDevice->initializeDefaultSettings(
            nv::camera2::CAPTURE_INTENT::PREVIEW, mRequest );

    // Set the preview stream as output.
    mRequest.outputs.clear();
    mRequest.outputs.push_back( mPreviewStream.get() );

    mCameraDevice->capture(mRequest);
}

void NativeCamera::stopCamera()
{
    // Cancel the streaming request
    mCameraDevice->cancelRequest(mRequest.requestId);

    // Destroy the camera objects in reverse order
    mPreviewStream = nullptr;
    mCameraDevice = nullptr;
    mCameraManager = nullptr;
}

void NativeCamera::setupStreamTextures( const nv::camera2::CameraStream *stream,
    GLuint *textures)
{
    uint width, height;
    width  = stream->size().width;
    height = stream->size().height;

    glActiveTexture( GL_TEXTURE0 );

    if ( stream->format() == nv::camera2::RAW16 )
    {
        glGenTextures(1, textures);

        // Setup texture for Y plane
        glBindTexture( GL_TEXTURE_2D, textures[0] );
        glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, width, height,
                     0, GL_LUMINANCE, GL_UNSIGNED_SHORT, (GLvoid*) NULL);

        CHECK_GL_ERROR();

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    }
    else if ( stream->format() == nv::camera2::YCbCr_420_888 )
    {
        glGenTextures(3, textures);

        // Setup texture for Y plane
        glBindTexture( GL_TEXTURE_2D, textures[0] );
        glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, width, height,
                     0, GL_LUMINANCE, GL_UNSIGNED_BYTE, (GLvoid*) NULL);

        CHECK_GL_ERROR();

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        // Setup texture for U plane
        glBindTexture( GL_TEXTURE_2D, textures[1] );
        glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, width/2, height/2,
                         0, GL_LUMINANCE, GL_UNSIGNED_BYTE, (GLvoid*) NULL);
        CHECK_GL_ERROR();

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        // Setup texture for V plane
        glBindTexture( GL_TEXTURE_2D, textures[2] );
        glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, width/2, height/2,
                         0, GL_LUMINANCE, GL_UNSIGNED_BYTE, NULL);
        CHECK_GL_ERROR();

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    }
}

void NativeCamera::drawStreamImage(const nv::camera2::CameraStream *stream,
        GLuint *textures)
{
    float edgeX, edgeY;

    // TODO!
    float aspectRatio = (float) stream->size().width /
                        (float) stream->size().height;
    if ( aspectRatio < mViewAspectRatio )
    {
        edgeX = aspectRatio / mViewAspectRatio;
        edgeY = 1.0f;
    }
    // View is taller than image - use entire width, center height
    else
    {
        edgeX = 1.0f;
        edgeY = mViewAspectRatio / aspectRatio;
    }

    float const vertexPosition[] = {
         edgeX, -edgeY,
        -edgeX, -edgeY,
         edgeX,  edgeY,
        -edgeX,  edgeY};

    float const textureCoord[] = {
        1.0f, 1.0f,
        0.0f, 1.0f,
        1.0f, 0.0f,
        0.0f, 0.0f };

    if ( stream->format() == nv::camera2::YCbCr_420_888)
    {
        glUseProgram(mProgYUV->getProgram());

        for ( uint i = 0; i < 3; ++i )
        {
            glActiveTexture( GL_TEXTURE0 + i );
            glBindTexture( GL_TEXTURE_2D, textures[i]);
        }

        glUniform1i(mProgYUV->getUniformLocation("uYTex"), 0);
        glUniform1i(mProgYUV->getUniformLocation("uUTex"), 1);
        glUniform1i(mProgYUV->getUniformLocation("uVTex"), 2);

        int aPosCoord = mProgYUV->getAttribLocation("aPosition");
        int aTexCoord = mProgYUV->getAttribLocation("aTexCoord");

        glVertexAttribPointer(aPosCoord, 2, GL_FLOAT, GL_FALSE, 0, vertexPosition);
        glVertexAttribPointer(aTexCoord, 2, GL_FLOAT, GL_FALSE, 0, textureCoord);
        glEnableVertexAttribArray(aPosCoord);
        glEnableVertexAttribArray(aTexCoord);


    }

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    for ( uint i = 0; i < 3; ++i )
    {
        glActiveTexture( GL_TEXTURE0 + i );
        glBindTexture( GL_TEXTURE_2D, 0 );
    }

    CHECK_GL_ERROR();
}

void NativeCamera::draw(void)
{
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Nothing to draw if the preview stream is no longer running.
    if ( !mPreviewStream ) return;

    // Dequeue the next available frame - do not wait, if a frame
    // is not available we will render the previous frame.
    std::unique_ptr<nv::camera2::CameraFrame> frame;
    frame = mPreviewStream->dequeue( 0 );

    if ( frame )
    {
        // Update the textures with the image content.
        uploadImage( *(frame->imageBuffer.get()), mPreviewTextures );
    }

    // Draw the image
    drawStreamImage(mPreviewStream.get(), mPreviewTextures);

    // print fps
    if (mFrameRate->nextFrame())
    {
        LOGI("fps: %.2f", mFrameRate->getMeanFramerate());
    }
}

void NativeCamera::uploadImage( nv::camera2::CameraBuffer &img, GLuint *textures )
{

    nv::camera2::CameraBuffer::Data imgPlane;
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    if ( img.format() == nv::camera2::YCbCr_420_888 )
    {
        for ( uint i = 0; i < 3; i++ )
        {
            imgPlane = img.data(i);
            glPixelStorei(GL_UNPACK_ROW_LENGTH, imgPlane.stride);

            glActiveTexture( GL_TEXTURE0 + i);
            glBindTexture( GL_TEXTURE_2D, textures[i] );
            glTexSubImage2D( GL_TEXTURE_2D, 0, 0, 0,
                imgPlane.width, imgPlane.height,
                GL_LUMINANCE, GL_UNSIGNED_BYTE, (GLvoid*) imgPlane.ptr);

            glBindTexture( GL_TEXTURE_2D, 0 );
            CHECK_GL_ERROR();
        }
    }
    else if ( img.format() == nv::camera2::RAW16 )
    {
        imgPlane = img.data(0);
        glPixelStorei(GL_UNPACK_ROW_LENGTH, imgPlane.stride);

        glActiveTexture( GL_TEXTURE0 );
        glBindTexture( GL_TEXTURE_2D, textures[0] );
        glTexSubImage2D( GL_TEXTURE_2D, 0, 0, 0,
            imgPlane.width, imgPlane.height,
            GL_LUMINANCE, GL_UNSIGNED_SHORT, (GLvoid*) imgPlane.ptr);

        glBindTexture( GL_TEXTURE_2D, 0 );
        CHECK_GL_ERROR();
    }
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
}

NvAppBase* NvAppFactory(NvPlatformContext* platform) {
    return new NativeCamera(platform);
}

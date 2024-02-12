/**
 * Copyright(c) Live2D Inc. All rights reserved.
 *
 * Use of this source code is governed by the Live2D Open Software license
 * that can be found at https://www.live2d.com/eula/live2d-open-software-license-agreement_en.html.
 */

#include "CubismOffscreenSurface_OpenGLES2.hpp"
#include "rlgl.h"

//------------ LIVE2D NAMESPACE ------------
namespace Live2D { namespace Cubism { namespace Framework { namespace Rendering {

CubismOffscreenFrame_OpenGLES2::CubismOffscreenFrame_OpenGLES2()
    : _renderTexture(0)
    , _colorBuffer(0)
    , _oldFBO(0)
    , _bufferWidth(0)
    , _bufferHeight(0)
    , _isColorBufferInherited(false)
{
}


void CubismOffscreenFrame_OpenGLES2::BeginDraw(int restoreFBO)
{
    if (_renderTexture == 0)
    {
        return;
    }

    // バックバッファのサーフェイスを記憶しておく
    if (restoreFBO < 0)
    {
        //glGetIntegerv(GL_FRAMEBUFFER_BINDING, &_oldFBO); // TODO!!!
        _oldFBO = 0;
    }
    else
    {
        _oldFBO = restoreFBO;
    }

    // マスク用RenderTextureをactiveにセット
    rlEnableFramebuffer(_renderTexture);
}

void CubismOffscreenFrame_OpenGLES2::EndDraw()
{
    if (_renderTexture == 0)
    {
        return;
    }

    // 描画対象を戻す
    if (_oldFBO > -1) {
        rlEnableFramebuffer(_oldFBO);
    }
}

void CubismOffscreenFrame_OpenGLES2::Clear(float r, float g, float b, float a)
{
    // マスクをクリアする
    rlClearColor(r * 255,g * 255,b* 255,a * 255);
    rlClearScreenBuffers();
}

csmBool CubismOffscreenFrame_OpenGLES2::CreateOffscreenFrame(csmUint32 displayBufferWidth, csmUint32 displayBufferHeight, unsigned int colorBuffer)
{
    DestroyOffscreenFrame();
    unsigned int fbo = 0;

    if (colorBuffer == 0)
    {
        _colorBuffer = rlLoadTexture(NULL, displayBufferWidth, displayBufferHeight, RL_PIXELFORMAT_UNCOMPRESSED_R8G8B8A8, 1);
        _isColorBufferInherited = false;
    }
    else
    {
        _colorBuffer = colorBuffer;
        _isColorBufferInherited = true;
    }

    fbo = rlLoadFramebuffer();
    rlFramebufferAttach(fbo, _colorBuffer, RL_ATTACHMENT_COLOR_CHANNEL0, RL_ATTACHMENT_TEXTURE2D, 0);
    rlDisableFramebuffer();

    _renderTexture = fbo;

    _bufferWidth = displayBufferWidth;
    _bufferHeight = displayBufferHeight;

    // 成功
    return true;
}

void CubismOffscreenFrame_OpenGLES2::DestroyOffscreenFrame()
{
    if (!_isColorBufferInherited && (_colorBuffer != 0))
    {
        rlUnloadTexture(_colorBuffer);
        _colorBuffer = 0;
    }

    if (_renderTexture!=0)
    {
        rlUnloadTexture(_renderTexture);
        _renderTexture = 0;
    }
}

unsigned int CubismOffscreenFrame_OpenGLES2::GetColorBuffer() const
{
    return _colorBuffer;
}

csmUint32 CubismOffscreenFrame_OpenGLES2::GetBufferWidth() const
{
    return _bufferWidth;
}

csmUint32 CubismOffscreenFrame_OpenGLES2::GetBufferHeight() const
{
    return _bufferHeight;
}

csmBool CubismOffscreenFrame_OpenGLES2::IsValid() const
{
    return _renderTexture != 0;
}

}}}}

//------------ LIVE2D NAMESPACE ------------

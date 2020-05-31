/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QRHIIMGUI_P_H
#define QRHIIMGUI_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "qrhiimgui.h"

#include "imgui.h"

QT_BEGIN_NAMESPACE

#define FIRSTSPECKEY (0x01000000)
#define LASTSPECKEY (0x01000017)
#define MAPSPECKEY(k) ((k) - FIRSTSPECKEY + 256)

class QRhiImGuiInputEventFilter : public QObject
{
public:
    QRhiImGuiInputEventFilter()
    {
        memset(keyDown, 0, sizeof(keyDown));
    }

    bool eventFilter(QObject *watched, QEvent *event) override;

    bool eatEvents = false;
    QPointF mousePos;
    Qt::MouseButtons mouseButtonsDown = Qt::NoButton;
    float mouseWheel = 0;
    Qt::KeyboardModifiers modifiers = Qt::NoModifier;
    bool keyDown[256 + (LASTSPECKEY - FIRSTSPECKEY + 1)];
    QString keyText;
};

class QRhiImguiPrivate
{
public:
    QRhiImguiPrivate();
    ~QRhiImguiPrivate();

    void updateInput();

    QRhiImgui::FrameFunc frame = nullptr;
    bool showDemoWindow = true;

    QRhi *rhi = nullptr;

    struct Texture {
        QImage image;
        QRhiTexture *tex = nullptr;
        QRhiShaderResourceBindings *srb = nullptr;
    };
    QVector<Texture> textures;

    QRhiBuffer *vbuf = nullptr;
    QRhiBuffer *ibuf = nullptr;
    QRhiBuffer *ubuf = nullptr;
    QRhiGraphicsPipeline *ps = nullptr;
    QRhiSampler *sampler = nullptr;
    QVector<QRhiResource *> releasePool;
    QSizeF lastDisplaySize;
    QSizeF lastOutputSize;
    QVarLengthArray<quint32, 4> vbufOffsets;
    QVarLengthArray<quint32, 4> ibufOffsets;

    QRhiImGuiInputEventFilter *inputEventFilter = nullptr;
    QObject *inputEventSource = nullptr;
    bool inputInitialized = false;

    bool depthTest = true;
};

QT_END_NAMESPACE

#endif

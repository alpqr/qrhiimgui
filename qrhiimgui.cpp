/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the Qt RHI module
**
** $QT_BEGIN_LICENSE:LGPL3$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPLv3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or later as published by the Free
** Software Foundation and appearing in the file LICENSE.GPL included in
** the packaging of this file. Please review the following information to
** ensure the GNU General Public License version 2.0 requirements will be
** met: http://www.gnu.org/licenses/gpl-2.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qrhiimgui_p.h"
#include <QFile>
#include <QMouseEvent>
#include <QKeyEvent>

QT_BEGIN_NAMESPACE

QRhiImgui::QRhiImgui()
    : d(new QRhiImguiPrivate)
{
}

QRhiImgui::~QRhiImgui()
{
    releaseResources();
    delete d;
}

void QRhiImgui::setFrameFunc(FrameFunc f)
{
    d->frame = f;
}

void QRhiImgui::demoWindow()
{
    ImGui::ShowDemoWindow(&d->showDemoWindow);
}

static QRhiShader getShader(const QString &name)
{
    QFile f(name);
    if (f.open(QIODevice::ReadOnly))
        return QRhiShader::fromSerialized(f.readAll());

    return QRhiShader();
}

bool QRhiImgui::prepareFrame(QRhiRenderTarget *rt, QRhiRenderPassDescriptor *rp,
                             QRhiResourceUpdateBatch *dstResourceUpdates)
{
    ImGuiIO &io(ImGui::GetIO());

    if (d->textures.isEmpty()) {
        unsigned char *pixels;
        int w, h;
        io.Fonts->GetTexDataAsRGBA32(&pixels, &w, &h);
        const QImage wrapperImg((const uchar *) pixels, w, h, QImage::Format_RGBA8888);
        QRhiImguiPrivate::Texture t;
        t.image = wrapperImg.copy();
        d->textures.append(t);
        io.Fonts->SetTexID(reinterpret_cast<ImTextureID>(quintptr(d->textures.count() - 1)));
    }

    const QSize outputSize = rt->pixelSize();
    const float dpr = rt->devicePixelRatio();
    io.DisplaySize.x = outputSize.width() / dpr;
    io.DisplaySize.y = outputSize.height() / dpr;
    io.DisplayFramebufferScale = ImVec2(dpr, dpr);

    d->updateInput();

    ImGui::NewFrame();
    if (d->frame)
        d->frame();
    ImGui::Render();

    ImDrawData *draw = ImGui::GetDrawData();
    draw->ScaleClipRects(ImVec2(dpr, dpr));

    if (!d->ubuf) {
        d->ubuf = d->rhi->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, 64 + 4);
        d->ubuf->setName(QByteArrayLiteral("imgui uniform buffer"));
        d->releasePool << d->ubuf;
        if (!d->ubuf->build())
            return false;

        float opacity = 1.0f;
        dstResourceUpdates->updateDynamicBuffer(d->ubuf, 64, 4, &opacity);
    }

    if (d->lastDisplaySize.width() != io.DisplaySize.x || d->lastDisplaySize.height() != io.DisplaySize.y) {
        QMatrix4x4 mvp = d->rhi->clipSpaceCorrMatrix();
        mvp.ortho(0, io.DisplaySize.x, io.DisplaySize.y, 0, 1, -1);
        dstResourceUpdates->updateDynamicBuffer(d->ubuf, 0, 64, mvp.constData());
        d->lastDisplaySize = QSizeF(io.DisplaySize.x, io.DisplaySize.y); // without dpr
        d->lastOutputSize = outputSize; // with dpr
    }

    if (!d->sampler) {
        d->sampler = d->rhi->newSampler(QRhiSampler::Linear, QRhiSampler::Linear, QRhiSampler::None,
                                        QRhiSampler::Repeat, QRhiSampler::Repeat);
        d->sampler->setName(QByteArrayLiteral("imgui sampler"));
        d->releasePool << d->sampler;
        if (!d->sampler->build())
            return false;
    }

    for (int i = 0; i < d->textures.count(); ++i) {
        QRhiImguiPrivate::Texture &t(d->textures[i]);
        if (!t.tex) {
            t.tex = d->rhi->newTexture(QRhiTexture::RGBA8, t.image.size());
            t.tex->setName(QByteArrayLiteral("imgui texture ") + QByteArray::number(i));
            if (!t.tex->build())
                return false;
            dstResourceUpdates->uploadTexture(t.tex, t.image);
        }
        if (!t.srb) {
            t.srb = d->rhi->newShaderResourceBindings();
            t.srb->setBindings({
                QRhiShaderResourceBinding::uniformBuffer(0, QRhiShaderResourceBinding::VertexStage | QRhiShaderResourceBinding::FragmentStage, d->ubuf),
                QRhiShaderResourceBinding::sampledTexture(1, QRhiShaderResourceBinding::FragmentStage, t.tex, d->sampler)
            });
            if (!t.srb->build())
                return false;
        }
    }

    if (!d->ps) {
        d->ps = d->rhi->newGraphicsPipeline();
        d->releasePool << d->ps;
        QRhiGraphicsPipeline::TargetBlend blend;
        blend.enable = true;
        blend.srcColor = QRhiGraphicsPipeline::SrcAlpha;
        blend.dstColor = QRhiGraphicsPipeline::OneMinusSrcAlpha;
        blend.srcAlpha = QRhiGraphicsPipeline::One;
        blend.dstAlpha = QRhiGraphicsPipeline::Zero;
        blend.colorWrite = QRhiGraphicsPipeline::R | QRhiGraphicsPipeline::G | QRhiGraphicsPipeline::B;
        d->ps->setTargetBlends({ blend });
        d->ps->setCullMode(QRhiGraphicsPipeline::None);
        d->ps->setDepthTest(true);
        d->ps->setDepthOp(QRhiGraphicsPipeline::LessOrEqual);
        d->ps->setDepthWrite(false);
        d->ps->setFlags(QRhiGraphicsPipeline::UsesScissor);

        QRhiShader vs = getShader(QLatin1String(":/imgui.vert.qsb"));
        Q_ASSERT(vs.isValid());
        QRhiShader fs = getShader(QLatin1String(":/imgui.frag.qsb"));
        Q_ASSERT(fs.isValid());
        d->ps->setShaderStages({
            { QRhiGraphicsShaderStage::Vertex, vs },
            { QRhiGraphicsShaderStage::Fragment, fs }
        });

        QRhiVertexInputLayout inputLayout;
        inputLayout.setBindings({
            { 4 * sizeof(float) + sizeof(quint32) }
        });
        inputLayout.setAttributes({
            { 0, 0, QRhiVertexInputAttribute::Float2, 0 },
            { 0, 1, QRhiVertexInputAttribute::Float2, 2 * sizeof(float) },
            { 0, 2, QRhiVertexInputAttribute::UNormByte4, 4 * sizeof(float) }
        });

        d->ps->setVertexInputLayout(inputLayout);
        d->ps->setShaderResourceBindings(d->textures[0].srb);
        d->ps->setRenderPassDescriptor(rp);

        if (!d->ps->build())
            return false;
    }

    // the imgui default
    Q_ASSERT(sizeof(ImDrawVert) == 20);
    // switched to uint in imconfig.h to avoid trouble with 4 byte offset alignment reqs
    Q_ASSERT(sizeof(ImDrawIdx) == 4);

    d->vbufOffsets.resize(draw->CmdListsCount);
    d->ibufOffsets.resize(draw->CmdListsCount);
    int totalVbufSize = 0;
    int totalIbufSize = 0;
    for (int n = 0; n < draw->CmdListsCount; ++n) {
        const ImDrawList *cmdList = draw->CmdLists[n];
        const int vbufSize = cmdList->VtxBuffer.Size * sizeof(ImDrawVert);
        d->vbufOffsets[n] = totalVbufSize;
        totalVbufSize += vbufSize;
        const int ibufSize = cmdList->IdxBuffer.Size * sizeof(ImDrawIdx);
        d->ibufOffsets[n] = totalIbufSize;
        totalIbufSize += ibufSize;
    }

    if (!d->vbuf) {
        d->vbuf = d->rhi->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::VertexBuffer, totalVbufSize);
        d->vbuf->setName(QByteArrayLiteral("imgui vertex buffer"));
        d->releasePool << d->vbuf;
        if (!d->vbuf->build())
            return false;
    } else {
        if (totalVbufSize > d->vbuf->size()) {
            d->vbuf->setSize(totalVbufSize);
            if (!d->vbuf->build())
                return false;
        }
    }
    if (!d->ibuf) {
        d->ibuf = d->rhi->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::IndexBuffer, totalIbufSize);
        d->ibuf->setName(QByteArrayLiteral("imgui index buffer"));
        d->releasePool << d->ibuf;
        if (!d->ibuf->build())
            return false;
    } else {
        if (totalIbufSize > d->ibuf->size()) {
            d->ibuf->setSize(totalIbufSize);
            if (!d->ibuf->build())
                return false;
        }
    }

    for (int n = 0; n < draw->CmdListsCount; ++n) {
        const ImDrawList *cmdList = draw->CmdLists[n];
        const int vbufSize = cmdList->VtxBuffer.Size * sizeof(ImDrawVert);
        dstResourceUpdates->updateDynamicBuffer(d->vbuf, d->vbufOffsets[n], vbufSize, cmdList->VtxBuffer.Data);
        const int ibufSize = cmdList->IdxBuffer.Size * sizeof(ImDrawIdx);
        dstResourceUpdates->updateDynamicBuffer(d->ibuf, d->ibufOffsets[n], ibufSize, cmdList->IdxBuffer.Data);
    }

    return true;
}

void QRhiImgui::queueFrame(QRhiCommandBuffer *cb)
{
    QRhiCommandBuffer::VertexInput vbufBinding(d->vbuf, 0);
    cb->setViewport({ 0, 0, float(d->lastOutputSize.width()), float(d->lastOutputSize.height()) });

    ImDrawData *draw = ImGui::GetDrawData();
    for (int n = 0; n < draw->CmdListsCount; ++n) {
        const ImDrawList *cmdList = draw->CmdLists[n];
        const ImDrawIdx *indexBufOffset = nullptr;
        vbufBinding.second = d->vbufOffsets[n];

        for (int i = 0; i < cmdList->CmdBuffer.Size; ++i) {
            const ImDrawCmd *cmd = &cmdList->CmdBuffer[i];
            const quint32 indexOffset = d->ibufOffsets[n] + quintptr(indexBufOffset);

            if (!cmd->UserCallback) {
                const QPointF scissorPixelBottomLeft = QPointF(cmd->ClipRect.x, d->lastOutputSize.height() - cmd->ClipRect.w);
                const QSizeF scissorPixelSize = QSizeF(cmd->ClipRect.z - cmd->ClipRect.x, cmd->ClipRect.w - cmd->ClipRect.y);
                const int textureIndex = int(reinterpret_cast<qintptr>(cmd->TextureId));
                cb->setGraphicsPipeline(d->ps);
                cb->setShaderResources(d->textures[textureIndex].srb);
                cb->setScissor({ int(scissorPixelBottomLeft.x()), int(scissorPixelBottomLeft.y()),
                                 int(scissorPixelSize.width()), int(scissorPixelSize.height()) });
                cb->setVertexInput(0, 1, &vbufBinding, d->ibuf, indexOffset, QRhiCommandBuffer::IndexUInt32);
                cb->drawIndexed(cmd->ElemCount);
            } else {
                cmd->UserCallback(cmdList, cmd);
            }

            indexBufOffset += cmd->ElemCount;
        }
    }
}

void QRhiImgui::initialize(QRhi *rhi)
{
    d->rhi = rhi;
    d->lastOutputSize = QSizeF();
}

void QRhiImgui::releaseResources()
{
    for (QRhiImguiPrivate::Texture &t : d->textures) {
        if (t.tex)
            t.tex->releaseAndDestroy();
        if (t.srb)
            t.srb->releaseAndDestroy();
    }
    d->textures.clear();

    for (QRhiResource *r : d->releasePool)
        r->releaseAndDestroy();
    d->releasePool.clear();

    d->vbuf = d->ibuf = d->ubuf = nullptr;
    d->ps = nullptr;
    d->sampler = nullptr;

    delete d->inputEventFilter;
    d->inputEventFilter = nullptr;
}

QRhiImgui::FrameFunc QRhiImgui::frameFunc() const
{
    return d->frame;
}

void QRhiImgui::setInputEventSource(QObject *src)
{
    if (d->inputEventSource && d->inputEventFilter)
        d->inputEventSource->removeEventFilter(d->inputEventFilter);

    d->inputEventSource = src;

    if (!d->inputEventFilter) {
        d->inputEventFilter = new QRhiImGuiInputEventFilter;
        d->inputInitialized = false;
    }

    d->inputEventSource->installEventFilter(d->inputEventFilter);
}

QRhiImguiPrivate::QRhiImguiPrivate()
{
    ImGui::CreateContext();
}

QRhiImguiPrivate::~QRhiImguiPrivate()
{
    ImGui::DestroyContext();
}

bool QRhiImGuiInputEventFilter::eventFilter(QObject *, QEvent *event)
{
    switch (event->type()) {
    case QEvent::MouseButtonPress:
    case QEvent::MouseMove:
    case QEvent::MouseButtonRelease:
    {
        QMouseEvent *me = static_cast<QMouseEvent *>(event);
        mousePos = me->pos();
        mouseButtonsDown = me->buttons();
        modifiers = me->modifiers();
    }
        break;

    case QEvent::Wheel:
    {
        QWheelEvent *we = static_cast<QWheelEvent *>(event);
        mouseWheel += we->angleDelta().y() / 120.0f;
    }
        break;

    case QEvent::KeyPress:
    case QEvent::KeyRelease:
    {
        const bool down = event->type() == QEvent::KeyPress;
        QKeyEvent *ke = static_cast<QKeyEvent *>(event);
        modifiers = ke->modifiers();
        if (down)
            keyText.append(ke->text());
        int k = ke->key();
        if (k <= 0xFF)
            keyDown[k] = down;
        else if (k >= FIRSTSPECKEY && k <= LASTSPECKEY)
            keyDown[MAPSPECKEY(k)] = down;
    }
        break;

    default:
        break;
    }

    return false;
}

void QRhiImguiPrivate::updateInput()
{
    if (!inputEventFilter)
        return;

    ImGuiIO &io = ImGui::GetIO();

    if (!inputInitialized) {
        inputInitialized = true;

        io.KeyMap[ImGuiKey_Tab] = MAPSPECKEY(Qt::Key_Tab);
        io.KeyMap[ImGuiKey_LeftArrow] = MAPSPECKEY(Qt::Key_Left);
        io.KeyMap[ImGuiKey_RightArrow] = MAPSPECKEY(Qt::Key_Right);
        io.KeyMap[ImGuiKey_UpArrow] = MAPSPECKEY(Qt::Key_Up);
        io.KeyMap[ImGuiKey_DownArrow] = MAPSPECKEY(Qt::Key_Down);
        io.KeyMap[ImGuiKey_PageUp] = MAPSPECKEY(Qt::Key_PageUp);
        io.KeyMap[ImGuiKey_PageDown] = MAPSPECKEY(Qt::Key_PageDown);
        io.KeyMap[ImGuiKey_Home] = MAPSPECKEY(Qt::Key_Home);
        io.KeyMap[ImGuiKey_End] = MAPSPECKEY(Qt::Key_End);
        io.KeyMap[ImGuiKey_Delete] = MAPSPECKEY(Qt::Key_Delete);
        io.KeyMap[ImGuiKey_Backspace] = MAPSPECKEY(Qt::Key_Backspace);
        io.KeyMap[ImGuiKey_Enter] = MAPSPECKEY(Qt::Key_Return);
        io.KeyMap[ImGuiKey_Escape] = MAPSPECKEY(Qt::Key_Escape);

        io.KeyMap[ImGuiKey_A] = Qt::Key_A;
        io.KeyMap[ImGuiKey_C] = Qt::Key_C;
        io.KeyMap[ImGuiKey_V] = Qt::Key_V;
        io.KeyMap[ImGuiKey_X] = Qt::Key_X;
        io.KeyMap[ImGuiKey_Y] = Qt::Key_Y;
        io.KeyMap[ImGuiKey_Z] = Qt::Key_Z;
    }

    QRhiImGuiInputEventFilter *w = inputEventFilter;

    io.MousePos = ImVec2(w->mousePos.x(), w->mousePos.y());

    io.MouseDown[0] = w->mouseButtonsDown.testFlag(Qt::LeftButton);
    io.MouseDown[1] = w->mouseButtonsDown.testFlag(Qt::RightButton);
    io.MouseDown[2] = w->mouseButtonsDown.testFlag(Qt::MiddleButton);

    io.MouseWheel = w->mouseWheel;
    w->mouseWheel = 0;

    io.KeyCtrl = w->modifiers.testFlag(Qt::ControlModifier);
    io.KeyShift = w->modifiers.testFlag(Qt::ShiftModifier);
    io.KeyAlt = w->modifiers.testFlag(Qt::AltModifier);
    io.KeySuper = w->modifiers.testFlag(Qt::MetaModifier);

    memcpy(io.KeysDown, w->keyDown, sizeof(w->keyDown));

    if (!w->keyText.isEmpty()) {
        for (const QChar &c : w->keyText) {
            ImWchar u = c.unicode();
            if (u)
                io.AddInputCharacter(u);
        }
        w->keyText.clear();
    }
}

QT_END_NAMESPACE

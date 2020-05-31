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

#include <QGuiApplication>
#include <QQuickView>
#include "qrhiimgui.h"

struct ImGuiQuick
{
    ~ImGuiQuick();
    void init(QQuickWindow *window);
    void release();
    void prepare();
    void render();
    QRhiImgui d;
    QQuickWindow *w = nullptr;
    QRhi *rhi = nullptr;
    QRhiSwapChain *swapchain = nullptr;
};

ImGuiQuick::~ImGuiQuick()
{
    release();
}

void ImGuiQuick::init(QQuickWindow *window)
{
    QSGRendererInterface *rif = window->rendererInterface();
    rhi = static_cast<QRhi *>(rif->getResource(window, QSGRendererInterface::RhiResource));
    if (!rhi)
        qFatal("Failed to retrieve QRhi from QQuickWindow");
    swapchain = static_cast<QRhiSwapChain *>(rif->getResource(window, QSGRendererInterface::RhiSwapchainResource));
    if (!swapchain)
        qFatal("Failed to retrieve QRhiSwapChain from QQuickWindow");

    d.initialize(rhi);
    w = window;
}

void ImGuiQuick::release()
{
    d.releaseResources();
    w = nullptr;
}

void ImGuiQuick::prepare()
{
    if (!w)
        return;

    QRhiResourceUpdateBatch *u = rhi->nextResourceUpdateBatch();
    d.prepareFrame(swapchain->currentFrameRenderTarget(), swapchain->renderPassDescriptor(), u);
    swapchain->currentFrameCommandBuffer()->resourceUpdate(u);
}

void ImGuiQuick::render()
{
    if (!w)
        return;

    d.queueFrame(swapchain->currentFrameCommandBuffer());
}

int main(int argc, char *argv[])
{
    qputenv("QSG_INFO", "1");
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QGuiApplication app(argc, argv);

    // Either ig should outlive view, or should not rely on sceneGraphInvalidated. Chose the former here.
    ImGuiQuick ig;
    QQuickView view;

    QObject::connect(&view, &QQuickWindow::sceneGraphInitialized, &view, [&ig, &view] { ig.init(&view); }, Qt::DirectConnection);
    QObject::connect(&view, &QQuickWindow::sceneGraphInvalidated, &view, [&ig] { ig.release(); }, Qt::DirectConnection);
    QObject::connect(&view, &QQuickWindow::beforeRendering, &view, [&ig] { ig.prepare(); }, Qt::DirectConnection);
    QObject::connect(&view, &QQuickWindow::afterRenderPassRecording, &view, [&ig] { ig.render(); }, Qt::DirectConnection);

    ig.d.setDepthTest(false);
    ig.d.setInputEventSource(&view, true); // do not pass mouse and key input to Qt Quick

    ig.d.setFrameFunc([&ig] { ig.d.demoWindow(); });

    view.setColor(Qt::black);
    view.setResizeMode(QQuickView::SizeRootObjectToView);
    view.resize(1280, 720);
    view.setSource(QUrl("qrc:/main.qml"));
    view.show();

    return app.exec();
}

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
#include "globalstate.h"
#include "qrhiimgui.h"
#include "gui.h"
#include "imnodes.h"

struct ImGuiQuick
{
    ImGuiQuick();
    ~ImGuiQuick();

    void setWindow(QQuickWindow *window);
    void setVisible(bool b);

    void init();
    void release();
    void prepare();
    void render();

    QRhiImgui d;
    QQuickWindow *w = nullptr;
    QRhi *rhi = nullptr;
    QRhiSwapChain *swapchain = nullptr;
    bool visible = true;
};

ImGuiQuick::ImGuiQuick()
{
    d.setDepthTest(false);
}

ImGuiQuick::~ImGuiQuick()
{
    release();
}

void ImGuiQuick::setWindow(QQuickWindow *window)
{
    d.setInputEventSource(window);
    d.setEatInputEvents(true);
    w = window;
}

void ImGuiQuick::setVisible(bool b)
{
    if (visible == b)
        return;

    visible = b;
    d.setEatInputEvents(visible);
}

void ImGuiQuick::init()
{ // render thread
    QSGRendererInterface *rif = w->rendererInterface();
    rhi = static_cast<QRhi *>(rif->getResource(w, QSGRendererInterface::RhiResource));
    if (!rhi)
        qFatal("Failed to retrieve QRhi from QQuickWindow");
    swapchain = static_cast<QRhiSwapChain *>(rif->getResource(w, QSGRendererInterface::RhiSwapchainResource));
    if (!swapchain)
        qFatal("Failed to retrieve QRhiSwapChain from QQuickWindow");

    d.initialize(rhi);
}

void ImGuiQuick::release()
{ // render thread
    d.releaseResources();
    w = nullptr;
}

void ImGuiQuick::prepare()
{ // render thread
    if (!w)
        return;

    QRhiResourceUpdateBatch *u = rhi->nextResourceUpdateBatch();
    d.prepareFrame(swapchain->currentFrameRenderTarget(), swapchain->renderPassDescriptor(), u);
    swapchain->currentFrameCommandBuffer()->resourceUpdate(u);
}

void ImGuiQuick::render()
{ // render thread
    if (!w || !visible)
        return;

    d.queueFrame(swapchain->currentFrameCommandBuffer());
}

GlobalState globalState;

int main(int argc, char *argv[])
{
    qputenv("QSG_INFO", "1");
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QGuiApplication app(argc, argv);

    qmlRegisterSingletonInstance("imguidemo", 1, 0, "GlobalState", &globalState);

    Gui gui;

    // Either ig should outlive view, or should not rely on sceneGraphInvalidated. Chose the former here.
    ImGuiQuick ig;
    QQuickView view;

    QObject::connect(&view, &QQuickWindow::sceneGraphInitialized, &view, [&ig] { ig.init(); }, Qt::DirectConnection);
    QObject::connect(&view, &QQuickWindow::sceneGraphInvalidated, &view, [&ig] { ig.release(); }, Qt::DirectConnection);
    QObject::connect(&view, &QQuickWindow::beforeRendering, &view, [&ig] { ig.prepare(); }, Qt::DirectConnection);
    QObject::connect(&view, &QQuickWindow::afterRenderPassRecording, &view, [&ig] { ig.render(); }, Qt::DirectConnection);

    ig.setWindow(&view);
    ig.d.setFrameFunc(
        [&ig, &gui] {
            gui.frame();
            ig.d.demoWindow();
        });

    ImGui::GetIO().IniFilename = nullptr; // don't save and restore layout

    // start out as hidden
    ig.setVisible(false);
    globalState.setVisible(false);
    QObject::connect(&globalState, &GlobalState::visibleChanged, &view, [&ig] {
        ig.setVisible(globalState.isVisible());
    });

    gui.init();

    view.setColor(Qt::black);
    view.setResizeMode(QQuickView::SizeRootObjectToView);
    view.resize(1280, 720);
    view.setSource(QUrl("qrc:/main.qml"));
    view.show();

    int r = app.exec();

    gui.cleanup();
    return r;
}

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

#include "imgui.h"
#include "imnodes.h"
#include "TextEditor.h"

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

struct Editor : public TextEditor
{
    Editor() {
        SetText(R"(#version 440

layout(location = 0) in vec2 v_texcoord;
layout(location = 1) in vec4 v_color;

layout(location = 0) out vec4 fragColor;

layout(std140, binding = 0) uniform buf {
    mat4 mvp;
    float opacity;
} ubuf;

layout(binding = 1) uniform sampler2D tex;

void main()
{
    vec4 c = v_color * texture(tex, v_texcoord);
    fragColor = vec4(c.rgb, c.a * ubuf.opacity);
})");
        SetLanguageDefinition(LanguageDefinition::GLSL());
        SetPalette(GetDarkPalette());

        ErrorMarkers markers;
        markers.insert({ 8, "Blah blah blah\nblah" });
        SetErrorMarkers(markers);

//        TextEditor::Breakpoints b;
//        b.insert(6);
//        SetBreakpoints(b);
    }
};

static GlobalState globalState;

void frame(ImGuiQuick *ig)
{
    {
        ImGui::Begin("Control");
        if (ImGui::Button("Close ImGui overlay"))
            globalState.setVisible(false);
        ImGui::End();
    }

    {
        ImGui::SetNextWindowPos(ImVec2(50, 300), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(400, 400), ImGuiCond_FirstUseEver);
        ImGui::Begin("imnodes");

        imnodes::PushAttributeFlag(imnodes::AttributeFlags_EnableLinkDetachWithDragClick);
        imnodes::BeginNodeEditor();

        imnodes::BeginNode(1);
        imnodes::BeginNodeTitleBar();
        ImGui::TextUnformatted("In node");
        imnodes::EndNodeTitleBar();
        static float x = 0, y = 0;
        imnodes::BeginStaticAttribute(100);
        ImGui::PushItemWidth(50);
        ImGui::InputFloat("X", &x);
        ImGui::PopItemWidth();
        imnodes::EndStaticAttribute();
        imnodes::BeginStaticAttribute(101);
        ImGui::PushItemWidth(50);
        ImGui::InputFloat("Y", &y);
        ImGui::PopItemWidth();
        imnodes::EndStaticAttribute();
        imnodes::BeginOutputAttribute(2);
        ImGui::Indent(40);
        ImGui::Text("output");
        imnodes::EndOutputAttribute();
        imnodes::EndNode();

        imnodes::BeginNode(3);
        imnodes::BeginNodeTitleBar();
        ImGui::TextUnformatted("In node 2");
        imnodes::EndNodeTitleBar();
        static float r = 0, g = 0, b = 0;
        imnodes::BeginStaticAttribute(200);
        ImGui::PushItemWidth(50);
        ImGui::SliderFloat("R", &r, 0.0f, 1.0f);
        ImGui::PopItemWidth();
        imnodes::EndStaticAttribute();
        imnodes::BeginStaticAttribute(201);
        ImGui::PushItemWidth(50);
        ImGui::SliderFloat("G", &g, 0.0f, 1.0f);
        ImGui::PopItemWidth();
        imnodes::EndStaticAttribute();
        imnodes::BeginStaticAttribute(202);
        ImGui::PushItemWidth(50);
        ImGui::SliderFloat("B", &b, 0.0f, 1.0f);
        ImGui::PopItemWidth();
        imnodes::EndStaticAttribute();
        ImGui::SameLine();
        imnodes::BeginOutputAttribute(4);
        ImGui::Indent(40);
        ImGui::Text("output");
        imnodes::EndOutputAttribute();
        imnodes::EndNode();

        imnodes::BeginNode(50);
        imnodes::BeginNodeTitleBar();
        ImGui::TextUnformatted("In node 3");
        imnodes::EndNodeTitleBar();
        imnodes::BeginStaticAttribute(300);
        ImGui::PushItemWidth(50);
        static int uvItem = 0;
        const char *uvItems[] = { "UV0", "UV1", "UV2" };
        ImGui::Combo("", &uvItem, uvItems, 3);
        ImGui::PopItemWidth();
        imnodes::EndStaticAttribute();
        ImGui::SameLine();
        imnodes::BeginOutputAttribute(51);
        ImGui::Indent(40);
        ImGui::Text("output");
        imnodes::EndOutputAttribute();
        imnodes::EndNode();

        imnodes::BeginNode(5);
        imnodes::BeginNodeTitleBar();
        ImGui::TextUnformatted("Some node");
        imnodes::EndNodeTitleBar();
        imnodes::BeginInputAttribute(6);
        ImGui::Text("input");
        imnodes::EndInputAttribute();
        imnodes::BeginOutputAttribute(7);
        ImGui::Indent(40);
        ImGui::Text("output");
        imnodes::EndOutputAttribute();
        imnodes::EndNode();

        imnodes::BeginNode(8);
        imnodes::BeginNodeTitleBar();
        ImGui::TextUnformatted("Some other node");
        imnodes::EndNodeTitleBar();
        imnodes::BeginInputAttribute(9);
        ImGui::Text("input");
        imnodes::EndInputAttribute();
        imnodes::BeginInputAttribute(10);
        ImGui::Text("input2");
        imnodes::EndInputAttribute();
        imnodes::BeginInputAttribute(11);
        ImGui::Text("input3");
        imnodes::EndInputAttribute();
        imnodes::BeginOutputAttribute(12);
        ImGui::Indent(40);
        ImGui::Text("output");
        imnodes::EndOutputAttribute();
        imnodes::EndNode();

        imnodes::BeginNode(13);
        imnodes::BeginNodeTitleBar();
        ImGui::TextUnformatted("Out node");
        imnodes::EndNodeTitleBar();
        imnodes::BeginInputAttribute(14);
        ImGui::Text("input");
        imnodes::EndInputAttribute();
        imnodes::EndNode();

        for (const auto &link : globalState.links)
            imnodes::Link(link.id, link.fromAttrId, link.toAttrId);

        imnodes::EndNodeEditor();
        imnodes::PopAttributeFlag();

        static int nextLinkId = 7;
        int id, idFrom, idTo;
        if (imnodes::IsLinkCreated(&idFrom, &idTo))
            globalState.links.append({ nextLinkId++, idFrom, idTo });

        if (imnodes::IsLinkDestroyed(&id)) {
            globalState.links.erase(std::remove_if(globalState.links.begin(), globalState.links.end(),
                                                   [id](const GlobalState::Link &link) { return link.id == id; }),
                                    globalState.links.end());
        }

        ImGui::End();
    }

    {
        ImGui::SetNextWindowPos(ImVec2(300, 10), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(400, 300), ImGuiCond_FirstUseEver);
        ImGui::Begin("ImGuiColorTextEdit");
        static Editor e;
        e.Render("Hello world");
        ImGui::End();
    }

    {
        ig->d.demoWindow();
    }
}

int main(int argc, char *argv[])
{
    qputenv("QSG_INFO", "1");
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QGuiApplication app(argc, argv);

    qmlRegisterSingletonInstance("imguidemo", 1, 0, "GlobalState", &globalState);

    // Either ig should outlive view, or should not rely on sceneGraphInvalidated. Chose the former here.
    ImGuiQuick ig;
    QQuickView view;

    QObject::connect(&view, &QQuickWindow::sceneGraphInitialized, &view, [&ig] { ig.init(); }, Qt::DirectConnection);
    QObject::connect(&view, &QQuickWindow::sceneGraphInvalidated, &view, [&ig] { ig.release(); }, Qt::DirectConnection);
    QObject::connect(&view, &QQuickWindow::beforeRendering, &view, [&ig] { ig.prepare(); }, Qt::DirectConnection);
    QObject::connect(&view, &QQuickWindow::afterRenderPassRecording, &view, [&ig] { ig.render(); }, Qt::DirectConnection);

    ig.setWindow(&view);
    ig.d.setFrameFunc([&ig] { frame(&ig); });

    ImGui::GetIO().IniFilename = nullptr; // don't save and restore layout

    // start out as hidden
    ig.setVisible(false);
    globalState.setVisible(false);
    QObject::connect(&globalState, &GlobalState::visibleChanged, &view, [&ig] {
        ig.setVisible(globalState.isVisible());
    });

    imnodes::Initialize();
    imnodes::SetNodeGridSpacePos(1, ImVec2(10.0f, 10.0f));
    imnodes::SetNodeGridSpacePos(3, ImVec2(10.0f, 120.0f));
    imnodes::SetNodeGridSpacePos(50, ImVec2(10.0f, 240.0f));
    imnodes::SetNodeGridSpacePos(5, ImVec2(200.0f, 10.0f));
    imnodes::SetNodeGridSpacePos(8, ImVec2(200.0f, 100.0f));
    imnodes::SetNodeGridSpacePos(13, ImVec2(300.0f, 300.0f));

    view.setColor(Qt::black);
    view.setResizeMode(QQuickView::SizeRootObjectToView);
    view.resize(1280, 720);
    view.setSource(QUrl("qrc:/main.qml"));
    view.show();

    int r = app.exec();

    imnodes::Shutdown();
    return r;
}

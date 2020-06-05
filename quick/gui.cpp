#include "gui.h"
#include "globalstate.h"
#include "imnodes.h"
#include "TextEditor.h"

extern GlobalState globalState;

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

static Editor e;

void Gui::init()
{
    imnodes::Initialize();
    imnodes::SetNodeGridSpacePos(1, ImVec2(10.0f, 10.0f));
    imnodes::SetNodeGridSpacePos(3, ImVec2(10.0f, 120.0f));
    imnodes::SetNodeGridSpacePos(50, ImVec2(10.0f, 240.0f));
    imnodes::SetNodeGridSpacePos(5, ImVec2(200.0f, 10.0f));
    imnodes::SetNodeGridSpacePos(8, ImVec2(200.0f, 100.0f));
    imnodes::SetNodeGridSpacePos(13, ImVec2(300.0f, 300.0f));
}

void Gui::cleanup()
{
    imnodes::Shutdown();
}

void Gui::frame()
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
        e.Render("Hello world");
        ImGui::End();
    }
}

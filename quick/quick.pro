TEMPLATE = app

QT += quick gui-private

SOURCES = \
    main.cpp \
    gui.cpp \
    ../qrhiimgui.cpp \
    ../imgui/imgui.cpp \
    ../imgui/imgui_draw.cpp \
    ../imgui/imgui_widgets.cpp \
    ../imgui/imgui_demo.cpp \
    ../imnodes/imnodes.cpp \
    ../ImGuiColorTextEdit/TextEditor.cpp

HEADERS = \
    globalstate.h \
    gui.h \
    ../qrhiimgui.h \
    ../qrhiimgui_p.h

INCLUDEPATH += .. ../imgui ../imnodes ../ImGuiColorTextEdit

RESOURCES = \
    imguidemo.qrc

TEMPLATE = app

QT += quick gui-private

SOURCES = \
    main.cpp \
    ../qrhiimgui.cpp \
    ../imgui/imgui.cpp \
    ../imgui/imgui_draw.cpp \
    ../imgui/imgui_widgets.cpp \
    ../imgui/imgui_demo.cpp \
    ../imnodes/imnodes.cpp

HEADERS = \
    globalstate.h \
    ../qrhiimgui.h \
    ../qrhiimgui_p.h

INCLUDEPATH += .. ../imgui ../imnodes

RESOURCES = \
    imguidemo.qrc

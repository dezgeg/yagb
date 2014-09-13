CONFIG += qt debug silent
QT += core gui widgets multimedia opengl
QMAKE_CXXFLAGS += -std=c++0x -O0 -gdwarf-2 -Wall -Wextra -Woverloaded-virtual -Werror -Wno-unused-parameter -Wno-unknown-pragmas

HEADERS += gui/*.hpp emu/*.hpp
SOURCES += gui/*.cpp emu/*.cpp
FORMS += gui/*.ui

release:OBJECTS_DIR = out/opt/
release:MOC_DIR = out/opt/.moc
release:RCC_DIR = out/opt/.rcc
release:UI_DIR = out/opt/.ui

debug:OBJECTS_DIR = out/debug/
debug:MOC_DIR = out/debug/.moc
debug:RCC_DIR = out/debug/.rcc
debug:UI_DIR = out/debug/.ui

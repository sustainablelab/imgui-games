#
# Cross Platform Makefile
# Compatible with MSYS2/MINGW, Ubuntu 14.04.1 and Mac OS X
#
# You will need GLFW (http://www.glfw.org):
# Linux:
#   apt-get install libglfw-dev
# Mac OS X:
#   brew install glfw
# MSYS2:
#   pacman -S --noconfirm --needed mingw-w64-x86_64-toolchain mingw-w64-x86_64-glfw
#

#CXX = g++
#CXX = clang++

EXE = bob
IMGUI_DIR = ./imgui
SOURCES = main.cpp
SOURCES += $(IMGUI_DIR)/imgui.cpp $(IMGUI_DIR)/imgui_demo.cpp $(IMGUI_DIR)/imgui_draw.cpp $(IMGUI_DIR)/imgui_tables.cpp $(IMGUI_DIR)/imgui_widgets.cpp
SOURCES += $(IMGUI_DIR)/backends/imgui_impl_glfw.cpp $(IMGUI_DIR)/backends/imgui_impl_opengl3.cpp
OBJS = $(addsuffix .o, $(basename $(notdir $(SOURCES))))
UNAME_S := $(shell uname -s)

LINUX_AL_LIBS = -lopenal -laudio
LINUX_GL_LIBS = -lGL

CXXFLAGS = -I$(IMGUI_DIR) -I$(IMGUI_DIR)/backends -I./utility
CXXFLAGS += -g -Wall -Wformat
LIBS =

# Disable build optimizations
ifeq ($(DEBUG),yes)
  CXXFLAGS += -O0
else
  CXXFLAGS += -O3
  CXXFLAGS += -DNDEBUG=1
endif

# Enable memory tracking/sanitization instrumentation (leaks, bad points, etc.)
ifeq ($(SANITIZE),yes)
  CXXFLAGS += -fsanitize=address -fsanitize-address-use-after-scope -DADDRESS_SANITIZER -g -fno-omit-frame-pointer
  LIBS += -fsanitize=address, -static-libasan
endif

##---------------------------------------------------------------------
## OPENGL ES
##---------------------------------------------------------------------

## This assumes a GL ES library available in the system, e.g. libGLESv2.so
# CXXFLAGS += -DIMGUI_IMPL_OPENGL_ES2
# LINUX_GL_LIBS = -lGLESv2

##---------------------------------------------------------------------
## BUILD FLAGS PER PLATFORM
##---------------------------------------------------------------------

ifeq ($(UNAME_S), Linux) #LINUX
	ECHO_MESSAGE = "Linux"
	LIBS += $(LINUX_AL_LIBS)
	LIBS += $(LINUX_GL_LIBS) `pkg-config --static --libs glfw3`

	CXXFLAGS += `pkg-config --cflags glfw3`
	CXXFLAGS += "-DPLATFORM_SUPPORTS_AUDIO"
	CFLAGS = $(CXXFLAGS)
endif

ifeq ($(UNAME_S), Darwin) #APPLE
	ECHO_MESSAGE = "Mac OS X"
	LIBS += -framework OpenGL -framework Cocoa -framework IOKit -framework CoreVideo
	LIBS += -L/usr/local/lib -L/opt/local/lib -L/opt/homebrew/lib
	#LIBS += -lglfw3
	LIBS += -lglfw

	CXXFLAGS += -I/usr/local/include -I/opt/local/include -I/opt/homebrew/include
	CFLAGS = $(CXXFLAGS)
endif

ifeq ($(OS), Windows_NT)
	ECHO_MESSAGE = "MinGW"
	LIBS += -lglfw3 -lgdi32 -lopengl32 -limm32 -lglew32 -lglu32
	LIBS += `pkg-config --libs freealut`
	CXXFLAGS += `pkg-config --cflags glfw3`
	CXXFLAGS += -DPLATFORM_WINDOWS
	CXXFLAGS += -DPLATFORM_SUPPORTS_AUDIO
	CFLAGS = $(CXXFLAGS)

	## TODO : [ADD OPENAL FLAGS FOR WINDOWS]
endif

##---------------------------------------------------------------------
## BUILD RULES
##---------------------------------------------------------------------

%.o:%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

%.o:$(IMGUI_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

%.o:$(IMGUI_DIR)/backends/%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

all: $(EXE)
	@echo Build complete for $(ECHO_MESSAGE)

$(EXE): $(OBJS)
	$(CXX) -o $@ $^ $(CXXFLAGS) $(LIBS)

clean:
	rm -f $(EXE) $(OBJS)
	rm -f libs.txt

.PHONY: what-compiler
what-compiler:
	@echo $(CXX)
.PHONY: print-libs
print-libs: main.cpp
	$(CXX) $(CXXFLAGS) $< -M > libs.txt
.PHONY: tags
tags: main.cpp
	ctags --c-kinds=+l --exclude=Makefile -R .
.PHONY: lib-tags
lib-tags: main.cpp
	$(CXX) $(CXXFLAGS) $< -M > headers-windows.txt
	python.exe parse-lib-tags.py
	rm -f headers-windows.txt
	ctags -f lib-tags --c-kinds=+p -L headers-posix.txt
	rm -f headers-posix.txt

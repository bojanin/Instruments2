# Cross Platform Makefile
# Compatible with MSYS2/MINGW, Ubuntu 14.04.1 and Mac OS X
#
# You will need SDL2 (http://www.libsdl.org):
# Linux:
#   apt-get install libsdl2-dev
# Mac OS X:
#   brew install sdl2
# MSYS2:
#   pacman -S mingw-w64-i686-SDL2
#

# CXX = g++
# CXX = clang++

BUILD_DIR = build
EXE = $(BUILD_DIR)/example_sdl2_opengl3  # Place executable in the build directory
IMGUI_DIR = imgui
SOURCES = main.cc
SOURCES += $(IMGUI_DIR)/imgui.cpp \
           $(IMGUI_DIR)/imgui_demo.cpp \
           $(IMGUI_DIR)/imgui_draw.cpp \
           $(IMGUI_DIR)/imgui_tables.cpp \
           $(IMGUI_DIR)/imgui_widgets.cpp 
SOURCES += $(IMGUI_DIR)/backends/imgui_impl_sdl2.cpp \
           $(IMGUI_DIR)/backends/imgui_impl_opengl3.cpp

OBJS = $(addprefix $(BUILD_DIR)/, $(addsuffix .o, $(basename $(notdir $(SOURCES)))))
UNAME_S := $(shell uname -s)
LINUX_GL_LIBS = -lGL

CXXFLAGS = -std=c++11 -I$(IMGUI_DIR) -I$(IMGUI_DIR)/backends
CXXFLAGS += -g -Wall -Wformat
LIBS =

##---------------------------------------------------------------------
## OPENGL ES
##---------------------------------------------------------------------

# CXXFLAGS += -DIMGUI_IMPL_OPENGL_ES2
# LINUX_GL_LIBS = -lGLESv2
# LINUX_GL_LIBS = -L/opt/vc/lib -lbrcmGLESv2

##---------------------------------------------------------------------
## BUILD FLAGS PER PLATFORM
##---------------------------------------------------------------------

ifeq ($(UNAME_S), Linux) # LINUX
    ECHO_MESSAGE = "Linux"
    LIBS += $(LINUX_GL_LIBS) -ldl `sdl2-config --libs`
    CXXFLAGS += `sdl2-config --cflags`
    CFLAGS = $(CXXFLAGS)
endif

ifeq ($(UNAME_S), Darwin) # APPLE
    ECHO_MESSAGE = "Mac OS X"
    LIBS += -framework OpenGL -framework Cocoa -framework IOKit -framework CoreVideo `sdl2-config --libs`
    LIBS += -L/usr/local/lib -L/opt/local/lib
    CXXFLAGS += `sdl2-config --cflags`
    CXXFLAGS += -I/usr/local/include -I/opt/local/include
    CFLAGS = $(CXXFLAGS)
endif

ifeq ($(OS), Windows_NT)
    ECHO_MESSAGE = "MinGW"
    LIBS += -lgdi32 -lopengl32 -limm32 `pkg-config --static --libs sdl2`
    CXXFLAGS += `pkg-config --cflags sdl2`
    CFLAGS = $(CXXFLAGS)
endif

##---------------------------------------------------------------------
## BUILD RULES
##---------------------------------------------------------------------

# Create build directory if it doesn't exist
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Pattern rule to compile .cpp files to .o files in the build directory
$(BUILD_DIR)/%.o: %.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

$(BUILD_DIR)/%.o: $(IMGUI_DIR)/%.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

$(BUILD_DIR)/%.o: $(IMGUI_DIR)/backends/%.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

all: $(BUILD_DIR) $(EXE)
	@echo Build complete for $(ECHO_MESSAGE)

$(EXE): $(OBJS)
	$(CXX) -o $@ $^ $(CXXFLAGS) $(LIBS)

# New run target
run: all
	@echo Running the executable...
	@$(EXE)

clean:
	rm -rf $(EXE) $(BUILD_DIR)

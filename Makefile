default-target: bob.exe

.PHONY:
what-compiler: 
	echo $(CXX)

OBJ :=  imgui/build/backends/imgui_impl_opengl3.o \
		imgui/build/backends/imgui_impl_glfw.o \
		imgui/build/imgui.o \
		imgui/build/libs/gl3w.o \
		imgui/build/imgui_demo.o \
		imgui/build/imgui_draw.o \
		imgui/build/imgui_tables.o \
		imgui/build/imgui_widgets.o

HEADER_PATHS := -Iimgui \
				-Iimgui/libs/gl3w \
				-Iimgui/libs/glfw/include/GLFW \
				-Iimgui/backends

CFLAGS := $(HEADER_PATHS) \
		-g -Wall -Wformat
CFLAGS += `pkg-config --cflags glfw3`
LFLAGS := -lglfw3 -lgdi32 -lopengl32 -limm32

imgui/build/%.o: imgui/%.cpp
	$(CXX) -c $< -o $@ $(CFLAGS)

imgui/build/backends/%.o: imgui/backends/%.cpp
	$(CXX) -c $< -o $@ $(CFLAGS)

imgui/build/libs/%.o: imgui/libs/gl3w/GL/%.c
	$(CXX) -c $< -o $@ $(CFLAGS)

bob.exe: main.cpp $(OBJ)
	$(CXX) -o $@ $^ $(CFLAGS) $(LFLAGS)

.PHONY: clean
clean:
	rm -f libs.txt
	rm -f bob.exe
	rm -f $(OBJ)

.PHONY: print-libs
print-libs: find-headers.c
	$(CXX) $(CFLAGS) $< -M > libs.txt

.PHONY: tags
tags: main.cpp
	ctags --c-kinds=+l --exclude=Makefile -R .

.PHONY: lib-tags
lib-tags: main.cpp
	$(CXX) $(CFLAGS) $< -M > headers-windows.txt
	python.exe parse-lib-tags.py
	rm -f headers-windows.txt
	ctags -f lib-tags --c-kinds=+p -L headers-posix.txt
	rm -f headers-posix.txt


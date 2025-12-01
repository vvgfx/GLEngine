# Original objects plus ImGui
OBJS = RenderingEngine.o View.o Controller.o Model.o GUIView.o GUIController.o

INCLUDES = -I../include -I../include/imgui -I../include/imgui/backends
LIBS = -L../lib
LDFLAGS = -lglad -lglfw3 -lstb_image -limgui
CFLAGS = -g -std=c++11
PROGRAM = Rendering_Engine

ifeq ($(OS),Windows_NT)     # is Windows_NT on XP, 2000, 7, Vista, 10...
    LDFLAGS += -lopengl32 -lgdi32
    PROGRAM :=$(addsuffix .exe,$(PROGRAM))
	COMPILER = g++
else ifeq ($(shell uname -s),Darwin)     # is MACOSX
    LDFLAGS += -framework Cocoa -framework OpenGL -framework IOKit
	COMPILER = clang++
else
    LIBS = -L../lib_linux
    LDFLAGS += -lGL
	COMPILER = g++
endif

Rendering_Engine: $(OBJS)
	$(COMPILER) -o $(PROGRAM) $(OBJS) $(LIBS) $(LDFLAGS)

RenderingEngine.o: RenderingEngine.cpp
	$(COMPILER) $(INCLUDES) $(CFLAGS) -c RenderingEngine.cpp

View.o: View.cpp View.h
	$(COMPILER) $(INCLUDES) $(CFLAGS) -c View.cpp  

Controller.o: Controller.cpp Controller.h
	$(COMPILER) $(INCLUDES) $(CFLAGS) -c Controller.cpp

GUIController.o: GUIController.cpp GUIController.h
	$(COMPILER) $(INCLUDES) $(CFLAGS) -c GUIController.cpp  

Model.o: Model.cpp Model.h
	$(COMPILER) $(INCLUDES) $(CFLAGS) -c Model.cpp      

GUIView.o: GUIView.cpp GUIView.h
	$(COMPILER) $(INCLUDES) $(CFLAGS) -c GUIView.cpp

RM = rm -f
# ifeq ($(OS),Windows_NT)     # is Windows_NT on XP, 2000, 7, Vista, 10...
#     RM := rm -f
# endif

clean:
	$(RM) $(OBJS) $(PROGRAM)
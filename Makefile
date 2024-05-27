# Makefile

CXX = g++
SDL2FLAGS = -g $(shell sdl2-config --cflags --libs) -lSDL2_ttf -lSDL2_image
PROGRAM = main
SRC = Main.cpp App.cpp StickFigure.cpp SpriteSheet.cpp Utils.cpp TextRenderer.cpp StackCleanup.cpp
# -lSDL2_ttf
all: compile

prepare:
	sudo apt-get install libsdl2-dev libsdl2-ttf-dev libsdl2-image-dev

compile: $(SRC)
	$(CXX) -o $(PROGRAM) $(SRC) $(SDL2FLAGS)

run: $(PROGRAM)
	./$(PROGRAM)

clean:
	rm -f $(PROGRAM)

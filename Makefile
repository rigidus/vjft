CXX = g++
SDL2FLAGS = $(shell sdl2-config --cflags --libs) -lSDL2_ttf
PROGRAM = main
SRC = main.cpp App.cpp StickFigure.cpp SpriteSheet.cpp utilities.cpp TextRenderer.cpp
# -lSDL2_ttf
all: compile

prepare:
	sudo apt-get install libsdl2-dev libsdl2-ttf-dev

compile: $(SRC)
	$(CXX) -o $(PROGRAM) $(SRC) $(SDL2FLAGS)

run: $(PROGRAM)
	./$(PROGRAM)

clean:
	rm -f $(PROGRAM)

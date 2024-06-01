# Makefile

CXX = g++
SDL2FLAGS = -std=c++17 -g $(shell sdl2-config --cflags --libs) -lSDL2_ttf -lSDL2_image
PROGRAM = main
SRC = Main.cpp App.cpp Figure.cpp SpriteSheet.cpp Utils.cpp TextRenderer.cpp StackCleanup.cpp Scene.cpp Viewport.cpp Dialog.cpp TextField.cpp
# -lSDL2_ttf
all: compile

prepare:
	sudo apt-get install libsdl2-dev libsdl2-ttf-dev libsdl2-image-dev

compile: $(SRC)
	$(CXX) -o $(PROGRAM) $(SRC) $(SDL2FLAGS)

run: $(PROGRAM)
	./$(PROGRAM)

emcc:
	~/src/emsdk/upstream/emscripten/emcc emcc_test.cpp -s USE_SDL=2 -o index.html

clean:
	rm -f $(PROGRAM)

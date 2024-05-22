CXX = g++
SDL2FLAGS = $(shell sdl2-config --cflags --libs) -lSDL2_ttf
PROGRAM = main
SRC = main.cpp app.cpp stick_figure.cpp spritesheet.cpp utilities.cpp
# -lSDL2_ttf
all: compile

compile: $(SRC)
	$(CXX) -o $(PROGRAM) $(SRC) $(SDL2FLAGS)

run: $(PROGRAM)
	./$(PROGRAM)

clean:
	rm -f $(PROGRAM)

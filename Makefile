CXX = g++
SDL2FLAGS = $(shell sdl2-config --cflags --libs)
PROGRAM = main
SRC = main.cpp application.cpp stick_figure.cpp

all: compile

compile: $(SRC)
	$(CXX) -o $(PROGRAM) $(SRC) $(SDL2FLAGS)

run: $(PROGRAM)
	./$(PROGRAM)

clean:
	rm -f $(PROGRAM)

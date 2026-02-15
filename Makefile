CC      = clang
CFLAGS  = -Wall -Wextra -O2
TARGET  = dice
SRC     = main.c

UNAME_S := $(shell uname -s)

ifeq ($(UNAME_S),Darwin)
  LDLIBS = -framework OpenGL -framework GLUT
else
  LDLIBS = -lGL -lGLU -lglut -lm
endif

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET) $(LDLIBS)

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET)

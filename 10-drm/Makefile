CXX      := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -g $(shell pkg-config --cflags libdrm gbm)
LIBS     := $(shell pkg-config --libs libdrm gbm)
ASANFLAGS:= -fsanitize=address

SRC      ?= main.cpp
BIN      := $(basename $(SRC))

.PHONY: run asan clean

$(BIN): $(SRC)
	$(CXX) $(CXXFLAGS) $(SRC) -o $(BIN) $(LIBS)

run: $(BIN)
	./$(BIN)

# Build with AddressSanitizer: `make asan SRC=main.cpp`
asan: $(SRC)
	$(CXX) $(CXXFLAGS) $(ASANFLAGS) $(SRC) -o $(BIN)_asan $(LIBS)
	./$(BIN)_asan

clean:
	rm -f $(BIN) $(BIN)_asan

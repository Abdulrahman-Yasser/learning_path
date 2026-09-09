CXX      := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -g -pthread
TSANFLAGS:= -fsanitize=thread

SRC      ?= main.cpp
BIN      := $(basename $(SRC))

.PHONY: run tsan clean

$(BIN): $(SRC)
	$(CXX) $(CXXFLAGS) $(SRC) -o $(BIN)

run: $(BIN)
	./$(BIN)

# Build with ThreadSanitizer: `make tsan SRC=race_unguarded.cpp`
# Wrapped in `setarch -R` (disable ASLR for this run only) because TSan's
# fixed shadow-memory layout can hit "FATAL: unexpected memory mapping" on
# newer kernels with high mmap ASLR entropy -- not a bug in your code, a
# known TSan/kernel interaction. This is a per-process workaround, not a
# system-wide change (no sudo, nothing persists after the run).
tsan: $(SRC)
	$(CXX) $(CXXFLAGS) $(TSANFLAGS) $(SRC) -o $(BIN)_tsan
	setarch $$(uname -m) -R ./$(BIN)_tsan

clean:
	rm -f $(BIN) $(BIN)_tsan a.out

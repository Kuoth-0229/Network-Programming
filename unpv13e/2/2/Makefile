# Compiler and flags
CXX = g++
CXXFLAGS = -std=c++11 -Wall -O2
LDFLAGS = `pkg-config --cflags --libs opencv4` -I/opt/homebrew/include -L/opt/homebrew/lib -lportaudio

# Targets
SENDER = send2
RECEIVER = recv2

# Source files
SENDER_SRC = send2.cpp
RECEIVER_SRC = recv2.cpp

# Default target
all: $(SENDER) $(RECEIVER)

# Build sender
$(SENDER): $(SENDER_SRC)
	$(CXX) $(CXXFLAGS) $(SENDER_SRC) -o $(SENDER) $(LDFLAGS)

# Build receiver
$(RECEIVER): $(RECEIVER_SRC)
	$(CXX) $(CXXFLAGS) $(RECEIVER_SRC) -o $(RECEIVER) $(LDFLAGS)

# Clean
clean:
	rm -f $(SENDER) $(RECEIVER)

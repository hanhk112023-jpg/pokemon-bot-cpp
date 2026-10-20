CXX = g++
CXXFLAGS = -std=c++17 -O2 -Wall -Wextra -pthread
LIBS = -lcurl

TARGET = pokemon_bot
SRCS = src/main.cpp

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SRCS) $(LIBS)

clean:
	rm -f $(TARGET)

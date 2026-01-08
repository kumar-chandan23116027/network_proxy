CXX = g++
CXXFLAGS = -std=c++11 -pthread -Wall

# Output executable name
TARGET = proxy_server

# Source directories
SRCDIR = src
INCDIR = include

# Source files
SOURCES = $(SRCDIR)/main.cpp $(SRCDIR)/proxy_server.cpp $(SRCDIR)/cache.cpp

# Object files
OBJECTS = $(SOURCES:.cpp=.o)

# Build rule
all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJECTS)

# Compile .cpp to .o
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean rule
clean:
	rm -f $(SRCDIR)/*.o $(TARGET)
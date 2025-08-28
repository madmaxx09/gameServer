CXX = g++
CXXFLAGS = -Wall -Wextra -std=c++17 #-O2
LDFLAGS = -lssl -lcrypto
OBJS = main.o GameServer.o Match.o utils.o
TARGET = server

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJS) $(LDFLAGS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

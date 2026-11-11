CXX ?= g++
CXXFLAGS += -std=c++17 -Wall -Wextra -Wpedantic
LDLIBS += -lcrypto

TRACKER := build/tracker/tracker
PEER := build/peer/peer
COMMON_SOURCES := src/socket_helper.cpp src/utils.cpp
TRACKER_SOURCES := src/tracker.cpp src/trackerfile.cpp $(COMMON_SOURCES)
PEER_SOURCES := src/peer.cpp $(COMMON_SOURCES)
HEADERS := src/*.hpp

.PHONY: all tracker peer clean

all: tracker peer

tracker: $(TRACKER)

peer: $(PEER)

$(TRACKER): $(TRACKER_SOURCES) $(HEADERS)
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) $(TRACKER_SOURCES) $(LDLIBS) -o $@

$(PEER): $(PEER_SOURCES) $(HEADERS)
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) $(PEER_SOURCES) $(LDLIBS) -o $@

clean:
	rm -f $(TRACKER) $(PEER)
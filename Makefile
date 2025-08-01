CFLAGS   = -Wall -O2 -I.
CXXFLAGS = -Wall -O2 -I.

all: agent sample sample2 sample3

agent: agent.c
	$(CC) $(CFLAGS) agent.c -o agent

sample: sample.cpp NetInt.h 
	$(CXX) $(CXXFLAGS) sample.cpp -o sample

sample2: sample2.cpp NetInt.h
	$(CXX) $(CXXFLAGS) sample2.cpp -o sample2

sample3: sample3.cpp NetInt.h
	$(CXX) $(CXXFLAGS) sample3.cpp -o sample3

clean:
	rm -f agent sample sample2 sample3
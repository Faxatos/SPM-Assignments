CXX                = g++ -std=c++17
OPTFLAGS          = -O3 -march=native
CXXFLAGS          += -Wall 
INCLUDES          = -I. -I./utils
LIBS              = -pthread
SOURCES           = collatz_par.cpp collatz_seq.cpp
TARGET            = $(SOURCES:.cpp=)

.PHONY: all clean cleanall 

%: %.cpp
	$(CXX) $(INCLUDES) $(CXXFLAGS) $(OPTFLAGS) -o $@ $< $(LIBS)

all: $(TARGET)

clean:
	-rm -fr *.o *~

cleanall: clean
	-rm -fr $(TARGET)
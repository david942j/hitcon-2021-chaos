CXXFLAGS :=-std=c++17 -O2 -Wall -Wno-pointer-arith
DEPS = $(wildcard *.h)
OBJ = sandbox.o inferior.o crypto.o

all: sandbox firmware
sandbox: $(OBJ)
	$(CXX) -o $@ $^ $(CXXFLAGS) -lcrypto

%.o: %.cpp $(DEPS)
	$(CXX) -c -o $@ $< $(CXXFLAGS)

firmware: .PHONY
	$(MAKE) -C firmware

.PHONY:

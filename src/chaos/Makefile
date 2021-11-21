CXXFLAGS :=-std=c++17 -O2 -Wall -Wno-pointer-arith
DEPS = $(wildcard *.h)
OBJ = sandbox.o inferior.o crypto.o cipher/cipher.o

all: sandbox firmware
sandbox: $(OBJ)
	$(CXX) -o $@ $^ $(CXXFLAGS) -lcrypto -lgmp

cipher/cipher.o: .PHONY
	$(MAKE) CXXFLAGS="$(CXXFLAGS)" -C cipher cipher.o

%.o: %.cpp $(DEPS)
	$(CXX) -c -o $@ $< $(CXXFLAGS)

firmware: .PHONY
	$(MAKE) -C firmware

.PHONY:

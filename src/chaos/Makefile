CXXFLAGS :=-std=c++17 -O2 -Wall -Wno-pointer-arith
DEPS = $(wildcard *.h)
OBJ = sandbox.o inferior.o crypto.o
CIPHER_DIR = cipher
CIPHER_SRC = $(wildcard $(CIPHER_DIR)/*.cpp)
CIPHER_OBJ = $(patsubst $(CIPHER_DIR)/%.cpp,$(CIPHER_DIR)/%.o,$(CIPHER_SRC))

all: sandbox firmware
sandbox: $(OBJ) $(CIPHER_OBJ)
	$(CXX) -o $@ $^ $(CXXFLAGS) -lcrypto -lgmp

%.o: %.cpp $(DEPS)
	$(CXX) -c -o $@ $< $(CXXFLAGS)

firmware: .PHONY
	$(MAKE) -C firmware

.PHONY:

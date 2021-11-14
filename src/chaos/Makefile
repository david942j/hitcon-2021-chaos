CXXFLAGS :=-std=c++17 -O2
DEPS = $(wildcard *.h)
OBJ = sandbox.o inferior.o crypto.o

all: sandbox
sandbox: $(OBJ)
	$(CXX) -o $@ $^ $(CXXFLAGS) -lcrypto # remove lcrypto if openssl is not required

%.o: %.cpp $(DEPS)
	$(CXX) -c -o $@ $< $(CXXFLAGS)

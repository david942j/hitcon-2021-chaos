CXXFLAGS :=-std=c++17 -O2
DEPS = $(wildcard *.h)
OBJ = sandbox.o inferior.o crypto.o

all: sandbox firmware
sandbox: $(OBJ)
	$(CXX) -o $@ $^ $(CXXFLAGS) -lcrypto # remove lcrypto if openssl is not required

%.o: %.cpp $(DEPS)
	$(CXX) -c -o $@ $< $(CXXFLAGS)

firmware: .PHONY
	python3 -c "open('firmware','wb').write(__import__('struct').pack('I',1)+b'a'*128+b'A')"
.PHONY:

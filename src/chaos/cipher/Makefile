SRC = $(wildcard *.cpp)
OBJ = $(patsubst %.cpp,%.o,$(SRC))

cipher.o: $(OBJ)
	$(LD) -r -o cipher.o $^

%.o: %.cpp %.h ../check.h
	$(CXX) -c -o $@ $< $(CXXFLAGS)

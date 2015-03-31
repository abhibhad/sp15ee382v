all: iccad2014_evaluate_solution
CXX = g++ -std=c++0x

OFLAGS = -pedantic -Wall -O3
LFLAGS = -static

#iccad2014_evaluate_solution: main.cpp evaluate.h evaluate.cpp flute.o
iccad2014_evaluate_solution: main.cpp parser_helper.cpp evaluate.cpp evaluate.h check_legality.cpp flute.o
	/bin/rm -f iccad2014_evaluation_solution
	$(CXX) $(OFLAGS) main.cpp parser_helper.cpp evaluate.cpp check_legality.cpp flute.o -o iccad2014_evaluate_solution $(LFLAGS) 

flute.o: Flute/flute.h Flute/flute.cpp
	/bin/rm -f flute.o
	$(CXX) $(OFLAGS) Flute/flute.cpp -c

clean:
	/bin/rm -f iccad2014_evaluate_solution evaluate.h.gch flute.o

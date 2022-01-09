reformat: main.cpp
	@g++ -g main.cpp -o reformat

run: reformat
	@./reformat ./test


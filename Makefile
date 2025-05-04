build:gen
	cd build && ninja

gen:
	cmake -G Ninja -B build -S .

clean:
	rm -rf build
	rm -f CMakeCache.txt
	rm -f CMakeFiles
	rm -f cmake_install.cmake
	rm -f CMakeOutput.log
	rm -f CMakeError.log
	
run:
	cd build && ./iot
	@echo "Run complete"
	
test_code:
	mkdir -p build
	g++ -c test/test_code.cpp -o build/test_code.o
	g++ -c src/Src/coder.cpp -o build/coder.o
	g++ build/coder.o build/test_code.o -o build/test_code
	cd build && ./test_code
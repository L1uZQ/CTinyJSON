[ ! -d "build" ] && mkdir build
cd build
rm -r *
cmake ..
make 
./ctinyjson_test
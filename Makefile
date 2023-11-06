ht:
	@echo " Compile ht_runtests ...";
	gcc -I ./include/ -L ./lib/ -Wl,-rpath,./lib/ ./examples/ht_runtests.c ./src/hash_file.c -lbf -o ./build/ht_main -O2

bf:
	@echo " Compile bf_main ...";
	gcc -I ./include/ -L ./lib/ -Wl,-rpath,./lib/ ./examples/bf_main.c -lbf -o ./build/runner -O2


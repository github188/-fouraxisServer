all:
	gcc -o camera_server.bin camera_server.c camera_config.c -lpthread
clean:
	rm -rf *.bin

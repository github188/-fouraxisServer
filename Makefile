all:
	gcc camera_server.c -o camera_server.bin
clean:
	rm -rf *.bin

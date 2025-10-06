.PHONY:gcc clean  x1 x2
file1 = serve_process
file2 = client_process

gcc:
	gcc -o $(file1) $(file1).c -lpthread
	gcc -o $(file2) $(file2).c -lpthread
	
clean:
	rm $(file1) $(file2)

x1:
	./$(file1)

x2:
	./$(file2)
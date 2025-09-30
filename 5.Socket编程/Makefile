.PHONY:gcc clean  x1 x2
file1 = serve
file2 = client

gcc:
	gcc -o $(file1) $(file1).c
	gcc -o $(file2) $(file2).c
	
clean:
	rm $(file1) $(file2)

x1:
	./$(file1)

x2:
	./$(file2)
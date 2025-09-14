.PHONY:gcc clean x
file1 =mq_send
file2 =mq_receive
gcc:
	gcc -o $(file1) $(file1).c;
	gcc -o $(file2) $(file2).c
x1:
	./$(file1)
x2:
	./$(file2)
clean:
	rm $(file2) $(file1)

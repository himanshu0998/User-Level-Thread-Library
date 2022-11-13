CC = gcc
CFLAGS = -g -w

all:: parallel_cal vector_multiply external_cal factorial transactions fibonacci_sum matrix_addition matrix_mul test

parallel_cal:
	$(CC) $(CFLAGS) -pthread -o parallel_cal parallel_cal.c -L../ -lmypthread

vector_multiply:
	$(CC) $(CFLAGS) -pthread -o vector_multiply vector_multiply.c -L../ -lmypthread

external_cal:
	$(CC) $(CFLAGS) -pthread -o external_cal external_cal.c -L../ -lmypthread

factorial:
	$(CC) $(CFLAGS) -pthread -o factorial factorial.c -L../ -lmypthread

transactions:
	$(CC) $(CFLAGS) -pthread -o transactions transactions.c -L../ -lmypthread

fibonacci_sum:
	$(CC) $(CFLAGS) -pthread -o fibonacci_sum fibonacci_sum.c -L../ -lmypthread

matrix_addition:
	$(CC) $(CFLAGS) -pthread -o matrix_addition matrix_addition.c -L../ -lmypthread

matrix_mul:
	$(CC) $(CFLAGS) -pthread -o matrix_mul matrix_mul.c -L../ -lmypthread

test:
	$(CC) $(CFLAGS) -pthread -o test test.c -L../ -lmypthread

clean:
	rm -rf testcase test parallel_cal vector_multiply factorial transactions fibonacci_sum matrix_addition matrix_mul external_cal *.o ./record/

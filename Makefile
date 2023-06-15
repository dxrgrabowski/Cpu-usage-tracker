#burn: cpuBurner.o
#	gcc cpuBurner.o -o burn

#cpuBurner.o: cpuBurner.c
#	gcc -c cpuBurner.c

output: main.o
	gcc main.o -o output

main.o: main.c
	gcc -c main.c


clean:
	rm *.o output
	rm *.o burn
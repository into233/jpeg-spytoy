cc = gcc
cflag = -g

all: decoder

jpegerror.o:
	$(cc) $(cflag) -c jpegerror.c

reader.o: jpegerror.o
	$(cc) $(cflag) -c reader.c

decoder: reader.o jpegerror.o
	$(cc) $(cflag) -o decoder decoder.c

parse_matadafile:
	$(cc) $(cflag) -o parse_metafile parse_matafile.c 

build:
	mkdir -p build/

clean:
	rm -rf decode parse_metafile test
	rm -rf *.o
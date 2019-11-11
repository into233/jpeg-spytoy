cc = gcc
cflag = -g -lm -Wall

all: main

jpegerror.o:jpegerror.c jpegerror.h
	$(cc) $(cflag) -c jpegerror.c

reader.o: reader.c reader.h jpegerror.o spywriter.o spyreader.o
	$(cc) $(cflag) -c reader.c

parse_metafile.o: parse_metafile.c parse_metafile.h jpegerror.o
	$(cc) $(cflag) -c parse_metafile.c 

decoder.o: decoder.c decoder.h reader.o jpegerror.o
	$(cc) $(cflag) -c decoder.c

spywriter.o: spywriter.c spywriter.h reader.o jpegerror.o
	$(cc) $(cflag) -c spywriter.c

spyreader.o: spyreader.c spyreader.h reader.o jpegerror.o
	$(cc) $(cflag) -c spyreader.c
	
main: decoder.o reader.o jpegerror.o parse_metafile.o spywriter.o spyreader.o
	$(cc) $(cflag) -o main  parse_metafile.o reader.o decoder.o jpegerror.o spywriter.o spyreader.o

build:
	mkdir -p build/

clean:
	rm -rf decode parse_metafile test main
	rm -rf *.o
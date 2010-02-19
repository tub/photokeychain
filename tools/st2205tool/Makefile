SRC	=	main.c
OBJ	=	main.o
LIBS	=	-lgd	
CFLAGS = -W

all:	phack splice bgrep

phack:	$(OBJ) $(SRC)
	gcc -o $(@) $(OBJ) $(LIBS) $(FLAGS)

splice:	splice.o splice.c
	gcc -o splice splice.o

bgrep:	bgrep.o bgrep.c
	gcc -o bgrep bgrep.o

clean:	
	rm -f $(OBJ) phack splice splice.o bgrep bgrep.o

distclean: clean
	rm -f fwimage.bak memimage.bak fwimage.bin

all: build

build: tema2.obj
	link /dll /out:so_stdio.dll tema2.obj
tema2.obj: tema2.c
	cl /c tema2.c
clean:
	del *.obj
	del so_stdio.dll
 

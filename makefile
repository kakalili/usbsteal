CC=cl
DLLNAME=\"usbstro.dll\"
DLLNAME_HACK="usbstro.dll"
CCFLAGS=/EHsc /nologo /Os /MT /DDLLNAME=$(DLLNAME) 
LIBS=user32.lib shell32.lib
OBJS=inject.obj Remotelib.obj RemotelibNT.obj misc.obj usbsteal.obj

all: $(DLLNAME_HACK) ���ص�C��.exe ���ص�D��.exe ���ص�E��.exe
zip: all
	zip usbstro.zip usbstro.dll *.exe *.dll
$(DLLNAME_HACK): $(OBJS)
	$(CC) $(CCFLAGS) $(LIBS) /LD /o $@ /DEF usbstro.def  $(OBJS)
remotelib.obj: RemoteLib.h Kernel32Funcs.h remotelib.cpp
remotelibnt.obj: RemoteLib.h Kernel32Funcs.h remotelibnt.cpp
inject.obj: config.h inject.cpp Kernel32Funcs.h remotelib.h folderutils.h
.cpp.obj:
	$(CC) $(CCFLAGS) /c $*.cpp
.c.obj:
	$(CC) $(CCFLAGS) /c $*.c
clean:
	del *.obj usbstro.dll *.lib *.exp usbstro.zip *.exe
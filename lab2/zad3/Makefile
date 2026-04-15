CC = gcc
CFLAGS = -Wall
LDFLAGS = -ldl # Wymagane dla dlopen()
OBJS = sig_default.o sig_ignore.o sig_mask.o sig_handle.o

# Domyślny cel, wywoływany wpisując po prostu "make"
all: static shared dynamic_prog

# Tworzenie plików obiektowych z flagą -fPIC (wymagana dla bibliotek .so)
%.o: %.c
	$(CC) $(CFLAGS) -fPIC -c $< -o $@

# Biblioteka statyczna (.a)
static_lib: $(OBJS)
	ar rcs libsignals.a $(OBJS)

# Biblioteka współdzielona (.so)
shared_lib: $(OBJS)
	$(CC) -shared -o libsignals.so $(OBJS)

# Kompilacja z biblioteką statyczną
static: static_lib main.c
	$(CC) $(CFLAGS) main.c -L. -lsignals -o main_static

# Kompilacja z biblioteką współdzieloną (linkowana podczas uruchamiania)
shared: shared_lib main.c
	$(CC) $(CFLAGS) main.c -L. -lsignals -o main_shared -Wl,-rpath=.

# Kompilacja programu ładującego funkcje przez dlopen()
# Definiujemy flagę preprocesora DYNAMIC (-DDYNAMIC)
dynamic_prog: shared_lib main.c
	$(CC) $(CFLAGS) -DDYNAMIC main.c -o main_dynamic $(LDFLAGS)

clean:
	rm -f *.o *.a *.so main_static main_shared main_dynamic

OBJS =	main.o	\
	stm32ld.o		\
	serial_platform.o

all: stm32ld

serial_platform.o: serial_posix.c serial_win32.c

stm32ld: $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS)

clean:
	rm -f $(OBJS)

.PHONY: all clean


# sources = 'main,stm32ld'

# if WINDOWS then
#   sources = sources..",serial_win32"
# else
#   sources = sources..",serial_posix"
# end

# c.program{'stm32ld', src=sources}

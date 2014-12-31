
all:
	gcc -Wall -o stm32ld main.c stm32ld.c serial_posix.c

# sources = 'main,stm32ld'

# if WINDOWS then
#   sources = sources..",serial_win32"
# else
#   sources = sources..",serial_posix"
# end

# c.program{'stm32ld', src=sources}

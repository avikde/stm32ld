#if defined(__WIN32__) || defined(__CYGWIN__)
#   include "serial_win32.c"
#else
#   include "serial_posix.c"
#endif

// Loader driver

#include "stm32ld.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <limits.h>

static FILE *fp;
static u32 fpsize;

#define BL_VERSION_MAJOR  2
#define BL_VERSION_MINOR  1
#define BL_MKVER( major, minor )    ( ( major ) * 256 + ( minor ) ) 
#define BL_MINVERSION               BL_MKVER( BL_VERSION_MAJOR, BL_VERSION_MINOR )

// Add other chip IDs here? Is there any reason to actually check this?
#define CHIP_ID           0x0414
#define CHIP_ID_ALT       0x0413

// ****************************************************************************
// Helper functions and macros

// Get data function
static u32 writeh_read_data( u8 *dst, u32 len )
{
  size_t readbytes = 0;

  if( !feof( fp ) )
    readbytes = fread( dst, 1, len, fp );
  return ( u32 )readbytes;
}

// Progress function
static void writeh_progress( u32 wrote )
{
  unsigned pwrite = ( wrote * 100 ) / fpsize;
  // static int expected_next = 10;

  // if( pwrite >= expected_next )
  // {
  //   printf( "%d%% ", expected_next );
  //   expected_next += 10;
  // }
  static int expected_next = 2;

  if( pwrite >= expected_next )
  {
    printf("=");
    expected_next += 2;
  }
}

// ****************************************************************************
// Entry point

int main( int argc, const char **argv )
{
  u8 not_flashing=0;
  u8 send_go_command=0;
  u8 minor, major;
  u16 version;
  long baud;
 
  // Argument validation
  if( argc < 5 )
  {
    fprintf( stderr, "Usage: stm32ld <port> <baud> <entry> <binary image name|0 to not flash> [<0|1 to send Go>]\n" );
    fprintf( stderr, "       <entry> one of 'dtr_rts' (Mainboard v1) or 'mblc'\n" );
    // fprintf( stderr, "Note: Thanks to Go command you don't need to change status of BOOT0 after flashing,\n\t\ttake care after power cycle ...\n\n\n" );
    exit( 1 );
  }
  errno = 0;
  baud = strtol( argv[ 2 ], NULL, 10 );
  if( ( errno == ERANGE && ( baud == LONG_MAX || baud == LONG_MIN ) ) || ( errno != 0 && baud == 0 ) || ( baud < 0 ) ) 
  {
    fprintf( stderr, "Invalid baud '%s'\n", argv[ 2 ] );
    exit( 1 );
  }
  
  if( argc >= 6 && strlen(argv[ 5 ])==1 && strncmp(argv[ 5 ], "1", 1)==0 )
  {
    send_go_command=1;
  }

  if( strlen(argv[ 4 ])==1 && strncmp(argv[ 4 ], "0", 1)==0 )
  {
    not_flashing=1;
  }
  else
  {
    if( ( fp = fopen( argv[ 4 ], "rb" ) ) == NULL )
    {
      fprintf( stderr, "Unable to open %s\n", argv[ 4 ] );
      exit( 1 );
    }
    else
    {
      fseek( fp, 0, SEEK_END );
      fpsize = ftell( fp );
      fseek( fp, 0, SEEK_SET );
    }
  }

  // new entry type
  entry_type_t ent = MBLC;
  if (strncmp(argv[3], "mblc", 4) == 0) {
    ent = MBLC;
  } else if (strncmp(argv[3], "dtr_rts", 7) == 0) {
    ent = MAINBOARD_V1;
  } else if (strncmp(argv[3], "rts_trpl_inv", 12) == 0) {
    ent = MAINBOARD_V2;
  } else {
    fprintf(stderr, "Unrecognized entry type: %s\n", argv[3]);
    exit( 1 );
  }
  
  // Connect to bootloader
  if( stm32_init( argv[ 1 ], baud, ent ) != STM32_OK )
  {
    fprintf( stderr, "Unable to connect to bootloader\n" );
    exit( 1 );
  }
  
  // Get version
  if( stm32_get_version( &major, &minor ) != STM32_OK )
  {
    fprintf( stderr, "Unable to get bootloader version\n" );
    exit( 1 );
  }
  else
  {
    printf( "Bootloader version: %d.%d; ", major, minor );
    if( BL_MKVER( major, minor ) < BL_MINVERSION )
    {
      fprintf( stderr, "Unsupported bootloader version" );
      exit( 1 );
    }
  }
  
  // Get chip ID
  if( stm32_get_chip_id( &version ) != STM32_OK )
  {
    fprintf( stderr, "Unable to get chip ID\n" );
    exit( 1 );
  }
  else
  {
    printf( "chip ID: %04X\r\n", version );
    // if( version != CHIP_ID && version != CHIP_ID_ALT )
    // {
    //   fprintf( stderr, "Unsupported chip ID\n" );
    //   exit( 1 );
    // }
  }
  fflush(stdout);
  fflush(stderr);
  
  if( not_flashing == 0 )
  {
    // Write unprotect
    if( stm32_write_unprotect() != STM32_OK )
    {
      fprintf( stderr, "Unable to execute write unprotect\n" );
      exit( 1 );
    }
    else {
      // printf( "Cleared write protection\n" );
    }

    // Erase flash
    if( major == 3 )
    {
      printf("Extended erase...\r\n");
      fflush(stdout);
      if( stm32_extended_erase_flash() != STM32_OK )
      {
        fprintf( stderr, "Unable to extended erase chip\n" );
        exit( 1 );
      }
    }
    else
    {
      if( stm32_erase_flash() != STM32_OK )
      {
        fprintf( stderr, "Unable to erase chip\n" );
        exit( 1 );
      }
      else
        printf( "Erased FLASH memory\n" );
    }

    // Program flash
    setbuf( stdout, NULL );
    printf( "Programming flash: ");
    if( stm32_write_flash( writeh_read_data, writeh_progress ) != STM32_OK )
    {
      fprintf( stderr, "Unable to program FLASH memory.\n" );
      exit( 1 );
    }
    else
      printf( "\n" );

    fclose( fp );
  }
  else
    printf( "Skipping flashing\n" );

  if( send_go_command == 1 )
  {
    // Run GO
    // printf( "Sending go command\n" );
    if( stm32_go_command( ) != STM32_OK )
    {
      fprintf( stderr, "Unable to run Go command.\n" );
      exit( 1 );
    }
  }

  return 0;
}
           

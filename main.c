
/*********************************************************************************************

    This is public domain software that was developed by or for the U.S. Naval Oceanographic
    Office and/or the U.S. Army Corps of Engineers.

    This is a work of the U.S. Government. In accordance with 17 USC 105, copyright protection
    is not available for any work of the U.S. Government.

    Neither the United States Government, nor any employees of the United States Government,
    nor the author, makes any warranty, express or implied, without even the implied warranty
    of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE, or assumes any liability or
    responsibility for the accuracy, completeness, or usefulness of any information,
    apparatus, product, or process disclosed, or represents that its use would not infringe
    privately-owned rights. Reference herein to any specific commercial products, process,
    or service by trade name, trademark, manufacturer, or otherwise, does not necessarily
    constitute or imply its endorsement, recommendation, or favoring by the United States
    Government. The views and opinions of authors expressed herein do not necessarily state
    or reflect those of the United States Government, and shall not be used for advertising
    or product endorsement purposes.
*********************************************************************************************/


/****************************************  IMPORTANT NOTE  **********************************

    Comments in this file that start with / * ! or / / ! are being used by Doxygen to
    document the software.  Dashes in these comment blocks are used to create bullet lists.
    The lack of blank lines after a block of dash preceeded comments means that the next
    block of dash preceeded comments is a new, indented bullet list.  I've tried to keep the
    Doxygen formatting to a minimum but there are some other items (like <br> and <pre>)
    that need to be left alone.  If you see a comment that starts with / * ! or / / ! and
    there is something that looks a bit weird it is probably due to some arcane Doxygen
    syntax.  Be very careful modifying blocks of Doxygen comments.

*****************************************  IMPORTANT NOTE  **********************************/



#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <errno.h>
#include <time.h>
#include <getopt.h>

#include "nvutility.h"

#include "misp.h"
#include "chrtr2.h"

#include "version.h"


#define         FILTER 9


typedef struct
{
  CHRTR2_RECORD      ch2;
  int32_t            rank;
} CH2_GRID;



/*

    Programmer : Stacy Johnson
    Date : 01/19/2012

    See usage (below) for an explanation of the purpose of this program.

*/


void usage ()
{
  fprintf (stderr, "\n\nUsage: chrtr2_replace INPUT_CHRTR2_FILE OUTPUT_CHRTR2_FILE REPLACEMENT [VALUE]\n\n");
  fprintf (stderr, "This program will replace values within a CHRTR2 file.\n");
  fprintf (stderr, "Where:\n");
  fprintf (stderr, "\tINPUT_CHRTR2_FILE = input CHRTR2 file name\n");
  fprintf (stderr, "\tOUTPUT_CHRTR2_FILE = output CHRTR2 file name\n");
  fprintf (stderr, "\tREPLACEMENT = replace negative values with REPLACEMENT\n");
  fprintf (stderr, "\tRVALUE = replace all instances of VALUE with REPLACEMENT\n\n");  
  fprintf (stderr, "You must specify either -l VALUE or VALUE and REPLACEMENT on the command line.\n\n");
  fflush (stderr);
  exit (-1);
}



int32_t main (int32_t argc, char *argv[])
{
  extern char        *optarg;
  int32_t            j, k, chrtr2_handle, chrtr2_outhandle;
  int32_t            percent = 0 , old_percent = -1;
  char               input_file[512], output_file[512];
  uint8_t            land = NVFalse;
  CHRTR2_HEADER      chrtr2_header;
  CHRTR2_RECORD      chrtr2_record;
  float              min_z, max_z;
  NV_I32_COORD2      coord;
  float              old_val, new_val;
  
  

  fprintf(stderr, "\n\n %s \n\n", VERSION);
  fflush (stderr);


  if (argc < 4) usage ();


  min_z = 9999999999.0;
  max_z = -9999999999.0;


  strcpy (input_file, argv[1]);
  fprintf (stderr, "Input file : %s\n", input_file);
  fflush (stderr);


  /*  Open the input file  */

  chrtr2_handle = chrtr2_open_file (input_file, &chrtr2_header, CHRTR2_READONLY);  
  if (chrtr2_handle < 0)
    {
      chrtr2_perror ();
      exit (-1);
    }


  /*  open output file for writing  */	

  strcpy (output_file, argv[2]);
  fprintf (stderr, "Output file : %s\n\n", output_file);
  fflush (stderr);

  chrtr2_outhandle = chrtr2_create_file (output_file, &chrtr2_header);
  if (chrtr2_outhandle < 0)
    {
      chrtr2_perror ();
      exit (-1);
    }


  /*  Get the replacement value and, optionally, the value to be replaced.  */

  sscanf (argv[3], "%f", &new_val);

  if (argc == 5) sscanf (argv[4], "%f", &old_val);


  for (j = 0 ; j < chrtr2_header.height ; j++)
    {
      coord.y = j;


      /*  Loop for width of input file.  */

      for (k = 0 ; k < chrtr2_header.width ; k++)
        {
          coord.x = k;


          /*  Read the input record.  */

          chrtr2_read_record (chrtr2_handle, coord, &chrtr2_record);


          /*  Leave CHRTR2_NULL data alone.  */

          if (chrtr2_record.status)
            {
              /*  Replace the values and recompute min/max  */

              if (land)
                {
                  if (chrtr2_record.z < 0.0) chrtr2_record.z = new_val;
                }
              else
                {
                  if (fabs (chrtr2_record.z - old_val) < 0.01) chrtr2_record.z = new_val;
                }


              min_z = MIN (chrtr2_record.z, min_z);
              max_z = MAX (chrtr2_record.z, max_z);
            }


          /*  Write the modified record out to the output file.  */

          chrtr2_write_record (chrtr2_outhandle, coord, chrtr2_record);


          /*  show percents  */

          percent = NINT (((float) j / (float) chrtr2_header.height) * 100.0);
          if (percent != old_percent)
            {
              fprintf (stderr, "Processing CHRTR2 file - %03d%% complete\r", percent);
              fflush (stderr);
              old_percent = percent;
            }
   	}
    }


  chrtr2_close_file (chrtr2_handle);


  /*  Update the header with the observed min and max values.  */

  chrtr2_header.min_observed_z = min_z;
  chrtr2_header.max_observed_z = max_z;

  chrtr2_update_header (chrtr2_outhandle, chrtr2_header);

  chrtr2_close_file (chrtr2_outhandle);

  fprintf (stderr, "\n\n%s complete\n\n\n", argv[0]);
  fflush (stderr);


  /*  Please ignore the following line.  It is useless.  Except...

      On some versions of Ubuntu, if I compile a program that doesn't use the math
      library but it calls a shared library that does use the math library I get undefined
      references to some of the math library functions even though I have -lm as the last
      library listed on the link line.  This happens whether I use qmake to build the
      Makefile or I have a pre-built Makefile.  Including math.h doesn't fix it either.
      The following line forces the linker to bring in the math library.  If there is a
      better solution please let me know at area.based.editor AT gmail DOT com.  */

  float ubuntu; ubuntu = 4.50 ; ubuntu = fmod (ubuntu, 1.0);


  return (0);
}

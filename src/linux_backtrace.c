/* 
 * Ardesia -- a program for painting on the screen
 * with this program you can play, draw, learn and teach
 * This program has been written such as a freedom sonet
 * We believe in the freedom and in the freedom of education
 *
 * Copyright (C) 2009 Pilolli Pietro <pilolli@fbk.eu>
 *
 * Ardesia is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * Ardesia is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */


#ifdef linux

#include <linux_backtrace.h>
#include <utils.h>

#ifdef HAVE_LIBSIGSEGV
#include <sigsegv.h>
#endif

#ifdef HAVE_LIBBFD
#include <bfd.h>

/* globals retained across calls to resolve. */
static bfd* abfd = 0;
static asymbol **syms = 0;
static asection *text = 0;


/* Put a trace line in the file giving the address */
static void create_trace_line(char *address, FILE *file) 
{
  char ename[1024];
  int l = readlink("/proc/self/exe",ename,sizeof(ename));
  
  if (l == -1) 
    {
      fprintf(stderr, "failed to find executable\n");
      return;
    }
 	
  ename[l] = 0;

  bfd_init();

  abfd = bfd_openr(ename, 0);
   
  if (!abfd)
    {
      fprintf(stderr, "bfd_openr failed: ");
      return;
    }
   
  /* oddly, this is required for it to work... */
  bfd_check_format(abfd,bfd_object);

  unsigned storage_needed = bfd_get_symtab_upper_bound(abfd);
  syms = (asymbol **) malloc(storage_needed);

  text = bfd_get_section_by_name(abfd, ".text");

  long offset = ((long)address) - text->vma;
   
  if (offset > 0) 
    {
      const char *filen;
      const char *func;
      unsigned line;
      if (bfd_find_nearest_line(abfd, text, syms, offset, &filen, &func, &line) && file)
	{
	  fprintf(stderr, "file: %s, line: %u, func %s\n",filen,line,func);
          fprintf(file, "file: %s, line: %u, func %s\n",filen,line,func);
	}
    }
}
#else
static void create_trace_line(char *address, FILE *file) 
{
  fprintf(stderr, "Unable to create the stacktrace line bfd library is not build or supported on your system\n");
}
#endif


#ifdef HAVE_BACKTRACE
static void create_trace() 
{

  void *array[MAX_FRAMES];
  size_t size;
  size_t i;
  void *approx_text_end = (void*) ((128+100) * 2<<20);

  /* 
   * the glibc functions backtrace is missing on all non-glibc platforms
   */
  
  size = backtrace (array, MAX_FRAMES);
  gchar* default_filename = get_default_name();
  const gchar* tmpdir = g_get_tmp_dir();
  gchar* filename  = g_strdup_printf("%s%s%s_stacktrace.txt",tmpdir, G_DIR_SEPARATOR_S, default_filename);
  g_free(default_filename);
  FILE *file = fopen(filename, "w");
  for (i = 0; i < size; i++)
    {
      if (array[i] < approx_text_end)
	{
	  create_trace_line(array[i], file);      
	}
    }
  fclose(file);
  send_trace_with_email(filename);
  g_free(filename);
}
#else
static void create_trace() 
{
  fprintf(stderr, "Unable to create the stacktrace the glibc backtrace function is not supported by your system\n");
  fprintf(stderr, "Please use the gdb and the bt command to create the trace\n");
}
#endif


/* Is called when occurs a sigsegv */
static int sigsegv_handler(void *addr, int bad)
{
  create_trace(); 
  exit(2);
}


void linux_backtrace_register()
{
#ifdef HAVE_LIBSIGSEGV
  /* Install the SIGSEGV handler */
  if (sigsegv_install_handler(sigsegv_handler)<0)
    {
      exit(2);
    }
#endif
}

#endif


// --------------------------------------------------------------------------
// This file is part of the shmem software.
//
//    shmem software is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.
//
//    pmpd firmware is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with shmem software. If not, see <http://www.gnu.org/licenses/>.
// -------------------------------------------------------------------------- 
//
// pmpd = physical modeling for pure data
// ch@chnry.net

#ifdef _WIN32
  #define NOMINMAX // avoid interference between macro min and the function defined in this file...
  #include <windows.h>
  #include <stdio.h>
  #include <conio.h>
  #include <tchar.h>
#else
  #include <sys/shm.h>
  #include <sys/stat.h>
#endif // _WIN32

#include "m_pd.h"
#include <stdio.h>

#include "shmem_export.h"
#include "shmem_version.h"

typedef struct shmem
{
  t_object x_obj;
#ifdef _WIN32
  HANDLE m_MapFile;
  char m_fileMappingName[128];
#else
  int segment_id;
#endif // _WIN32
  t_outlet *m_info_out;
  t_float *share_memory;
  int segment_size;
} t_shmem;

static inline t_float shmem_min(t_float x1,t_float x2)
{
	if (x1 <= x2) {
		return x1;
	} else {
		return x2;
	}
}

/**
 * \fn void shmem_set_tab(t_shmem *x, t_symbol *unused, int argc, t_atom *argv)
 * \brief Set the table to bind to and copy its data to internal buffer.
 *
 */
int shmem_set_tab(t_shmem *x, t_symbol *table, int src_offset, int dest_offset, int size)
{
	int npoints, i, index_max;
	t_garray *a;
    t_word *vec;

	i=0;
	t_symbol *s = table;
	if (!(a = (t_garray *)pd_findbyclass(s, garray_class)))
		pd_error(x, "%s: no such array", s->s_name);
	else if (!garray_getfloatwords(a, &npoints, &vec))
		pd_error(x, "%s: bad template for tabread", s->s_name);
	else {
		index_max = shmem_min(x->segment_size-dest_offset, npoints-src_offset);
		index_max = shmem_min(index_max, size);
		for (i=0; i < index_max; i++)
			x->share_memory[i+dest_offset] = vec[i+src_offset].w_float;
	}
	// post("copied=%d", i);
	return(i);
}

/**
 * \fn void shmem_set(t_shmem *x, t_symbol *unused, int argc, t_atom *argv)
 * \brief Handle the [memset ...( message
 *
 */
void shmem_set(t_shmem *x, t_symbol *unused, int argc, t_atom *argv)
{
    if ( !x->share_memory ){
        pd_error(x,"Create a valid shared memory before setting data !");
        return;
    }

    int i, index_max, shmem_offset, array_offset, size;

	if (argc < 1) {
		pd_error(x, "shmem usage : [memset table_name [table_name []]< or [memset src_offset data [dest_offset]< (data can be a float, a list, or a table name)");
		return;
	}

  for ( int i=0; i<argc; ++i){
    int j = 0;
    if ( argv[i].a_type == A_SYMBOL ){
      shmem_set_tab(x, atom_getsymbolarg(i,argc,argv), 0, j, x->segment_size);
    } else {
      break;
    }
  }

 if ((argc > 1) && (argv[0].a_type == A_FLOAT) ) {
		shmem_offset = atom_getfloatarg(0,argc,argv);
		if (shmem_offset < 0) shmem_offset=0;

		if (argv[1].a_type == A_SYMBOL) { // argument is an arrayname
			array_offset = 0;
			size = x->segment_size;
			if ((argc > 2) && (argv[2].a_type == A_FLOAT))
				array_offset = atom_getfloatarg(2,argc,argv);
			if (array_offset < 0) array_offset=0;
			if ((argc > 3) && (argv[2].a_type == A_FLOAT))
				size = atom_getfloatarg(3,argc,argv);
			shmem_set_tab(x, atom_getsymbolarg(1,argc,argv), array_offset, shmem_offset, size);
		}
		else if (argv[1].a_type == A_FLOAT) { // argument is a float
			index_max = x->segment_size - shmem_offset;
			if (index_max > argc-1) index_max = argc-1;
			for (i=0; i<index_max ; i++)
				x->share_memory[i+shmem_offset]=atom_getfloatarg(i+1, argc, argv);
		}
	}
}

int shmem_dump_tab(t_shmem *x, t_symbol *table, int src_offset, int dest_offset, int size)
{
	t_garray *a;
    t_word *vec;
    int i, index_max, vecsize;

    t_symbol *s = table;
    i=0;

    if (!(a = (t_garray *)pd_findbyclass(s, garray_class)))
        pd_error(x, "%s: no such array", s->s_name);
    else if (!garray_getfloatwords(a, &vecsize, &vec))
        pd_error(x, "%s: bad template for tabwrite", s->s_name);
	else {

		index_max = shmem_min(x->segment_size-src_offset, vecsize-dest_offset);
		index_max= shmem_min(index_max, size);
		for (i=0; i<index_max; i++)
			vec[i+dest_offset].w_float = x->share_memory[i+src_offset];
	}
	garray_redraw(a);

	// post("copied=%d", i);
	return(i);
}

/**
 * void shmem_dump(t_shmem *x, t_symbol *unused, int argc, t_atom *argv)
 * \brief Handle the [memdump ...( message
 *
 */
void shmem_dump(t_shmem *x, t_symbol *unused, int argc, t_atom *argv)
{
    if ( !x->share_memory ){
        pd_error(x,"Create a valid shared memory before dumping data !");
        return;
    }

    int i, j, shmem_offset, dest_offset, size;

	if (argc < 1) {
		pd_error(x, "shmem usage : [memdump table_name [table_name []]< or [memset src_offset table_name [dest_offset [size]]<");
		return;
	}

	if (argv[0].a_type == A_SYMBOL) {
		i = 0;
		j = 0;
		while (argv[i].a_type == A_SYMBOL) {
			j += shmem_dump_tab(x, atom_getsymbolarg(i,argc,argv), j, 0, x->segment_size);
			i++;
		}
	}
	else if ((argc > 1) && (argv[0].a_type == A_FLOAT) ) {
	   	shmem_offset = atom_getfloatarg(0,argc,argv);
		if (shmem_offset < 0) shmem_offset = 0;
		// if (shmem_offset > x->segment_size-1) i = x->segment_size-1;
		// post("i=%d", offset);

		t_symbol *s = atom_getsymbolarg(1,argc,argv);
		dest_offset = 0;
		size = x->segment_size;
		if ((argc >= 3) && ( argv[2].a_type == A_FLOAT )){
			dest_offset = atom_getfloatarg(2,argc,argv);
		}
		if ((argc >= 4) && ( argv[3].a_type == A_FLOAT )){
			size = atom_getfloatarg(3,argc,argv);
		}
		shmem_dump_tab(x, s, shmem_offset, dest_offset, size);
    }
}

void shmem_read(t_shmem *x, t_float index)
{
    if ( !x->share_memory ) return;
    int shmem_offset;

	shmem_offset = index;
	if (shmem_offset < 0) shmem_offset = 0;
	if (shmem_offset > x->segment_size-1) shmem_offset = x->segment_size-1;
	// post("i=%d", offset);
	outlet_float(x->x_obj.ob_outlet, x->share_memory[shmem_offset]);
}

/**
 * \fn void shmem_allocate(t_shmem *x, int id, int size)
 * \brief Allocate a new shared memory segment.
 *
 */

void shmem_allocate(t_shmem *x, t_float fId, t_float fSize)
{
  int id = (int) fId;
  if ( id < 1 ){
    pd_error(x,"id should be > 0"); // shmid 0 doesn't seem to work
    return;
  }

  x->segment_size = (int) fSize;
#ifdef  _WIN32
  int size = sizeof(t_float)*x->segment_size;
  if ( x->share_memory ) UnmapViewOfFile( x->share_memory );
  if ( x->m_MapFile ) CloseHandle( x->m_MapFile );

  sprintf(x->m_fileMappingName, "puredata-FileMappingObject_%d", id);

  x->m_MapFile = CreateFileMapping(
               INVALID_HANDLE_VALUE,    // use paging file
               NULL,                    // default security
               PAGE_READWRITE,          // read/write access
               (size & 0xFFFFFFFF00000000) >> 32,         // maximum object size (high-order DWORD)
               size & 0xFFFFFFFF,         // maximum object size (low-order DWORD)
               x->m_fileMappingName);      // name of mapping object

  if (x->m_MapFile == NULL)
  {
    pd_error(x,"Could not create file mapping object %s - error %ld.",x->m_fileMappingName, GetLastError());
    outlet_float(x->m_info_out, -1);
    return;
  }

  x->share_memory = (t_float*) MapViewOfFile(x->m_MapFile,   // handle to map object
                      FILE_MAP_ALL_ACCESS, // read/write permission
                      0,
                      0,
                      size);

  if ( !x->share_memory ){
    pd_error(x,"Could not get a view of file %s - error %ld",x->m_fileMappingName, GetLastError());
    outlet_float(x->m_info_out, -1);
    return;
  } else {
    verbose(0,"File mapping object %s successfully created.",x->m_fileMappingName);
  }

#else // for LINUX / UNIX

  struct shmid_ds shmbuffer;
  if (x->share_memory) {
    shmdt (x->share_memory);
    x->share_memory=NULL;
  }
  if (x->segment_id != -1) {
    shmctl (x->segment_id, IPC_RMID, 0);
    x->segment_id = -1;
  }

  x->segment_id  =  shmget  (id,  sizeof(t_float)*x->segment_size,
        IPC_CREAT | S_IRUSR | S_IWUSR | S_IROTH | S_IWOTH); // AV it should safer to use ftok() to generate an id

  if(x->segment_id != -1) { // 0 could be a valid value, -1 means error
    x->share_memory = (t_float*) shmat (x->segment_id, 0, 0);
    // post("shmem memory attached at address %p\n", x->share_memory);

    shmctl (x->segment_id, IPC_STAT, &shmbuffer);
    // post ("segment size: %d\n", shmbuffer.shm_segsz);

    if ((int)shmbuffer.shm_segsz < (int)sizeof(t_float)*x->segment_size) {
      // there was a problem, set object meme size to 0
      pd_error(x,"could not allocate shmem memory Id : %d, size %d", (int)id,  x->segment_size );
      x->segment_size = 0;
      x->share_memory=NULL;
      outlet_float(x->m_info_out, -1);
    }
  }
  else {
    pd_error(x,"could not allocate shmem memory Id : %d, size %d", (int)id,  x->segment_size );
    x->segment_size = 0;
    outlet_float(x->m_info_out, -1);
  }
#endif // _WIN32
    outlet_float(x->m_info_out, id);
}

t_class *shmem_class;

/**
 * \fn void shmem_new(t_floatarg id, t_floatarg size)
 * \brief Create a new instance of object.
 *
 * \param t_floatarg id : a unique id defining the shared memory segment
 * \param t_floatarg size : size of the memory segment
 *
 */
void *shmem_new( t_floatarg id,  t_floatarg size)
{
  // post ("id, size: %f, %f", id, size);
  t_shmem *x = (t_shmem *)pd_new(shmem_class);

#ifndef _WIN32
  x->segment_id=-1;
#endif
  x->share_memory = NULL;

  outlet_new(&x->x_obj, 0);
  x->m_info_out = outlet_new(&x->x_obj, 0);
  shmem_allocate(x, id, size);
  return (void *)x;
}

void shmem_free(t_shmem *x)
{
#ifdef  _WIN32
  if ( x->share_memory ) UnmapViewOfFile( x->share_memory );
  if ( x->m_MapFile ) CloseHandle( x->m_MapFile );
#else
  if ( x->share_memory ){
    if ( shmdt (x->share_memory) == -1) pd_error(x,"shmdt failed at %f", *x->share_memory);
  }
  x->share_memory=NULL;

  int shm_id = x->segment_id;
  struct shmid_ds shm_desc;
  if(shm_id>0){
    if (shmctl(shm_id,IPC_STAT, &shm_desc) != -1){
      if(shm_desc.shm_nattch<=0){
        if (shmctl(shm_id,IPC_RMID, &shm_desc) == -1) pd_error(x,"shmctl remove failed for %d", shm_id);
      }
    }
  }
  shm_id=0;
	//~shmctl (x->segment_id, IPC_RMID, 0);
#endif // _WIN32
}

void shmem_clear(t_shmem *x)
{
	int i;
	for (i=0; i<x->segment_size; i++)
		x->share_memory[i] = 0;
}

SHMEM_EXPORT void shmem_setup(void)
{

    shmem_class = class_new(gensym("shmem"), (t_newmethod)shmem_new, (t_method)shmem_free,
        sizeof(t_shmem), 0, A_DEFFLOAT, A_DEFFLOAT, 0);

    if(!shmem_class)
      return;

    verbose(4, "shmem version %s (%s)", shmem_tag(), shmem_sha());

    class_addmethod(shmem_class, (t_method)shmem_set, gensym("memset"), A_GIMME, 0);
    class_addmethod(shmem_class, (t_method)shmem_dump, gensym("memdump"), A_GIMME, 0);
    class_addmethod(shmem_class, (t_method)shmem_clear, gensym("memclear"), 0);
    class_addmethod(shmem_class, (t_method)shmem_read, gensym("memread"),A_DEFFLOAT, 0);
    class_addmethod(shmem_class, (t_method)shmem_allocate, gensym("allocate"),A_FLOAT, A_FLOAT, 0);
}


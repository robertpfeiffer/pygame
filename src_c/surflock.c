/*
  pygame - Python Game Library
  Copyright (C) 2000-2001  Pete Shinners
  Copyright (C) 2008 Marcus von Appen

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Library General Public
  License as published by the Free Software Foundation; either
  version 2 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Library General Public License for more details.

  You should have received a copy of the GNU Library General Public
  License along with this library; if not, write to the Free
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

  Pete Shinners
  pete@shinners.org
*/

/*
 *  internal surface locking support for python objects
 */
#define PYGAMEAPI_SURFLOCK_INTERNAL

#include "pygame.h"

#include "pgcompat.h"

static int
pgSurface_Lock(PyObject *);
static int
pgSurface_Unlock(PyObject *);
static int
pgSurface_LockBy(PyObject *, PyObject *);
static int
pgSurface_UnlockBy(PyObject *, PyObject *);
static SDL_Surface *
pg_get_surf_root(pgSurfaceObject *surf);

/* static void */
/* _lifelock_dealloc(PyObject *); */

static SDL_Surface *
pg_get_surf_root(pgSurfaceObject *surf)
{
    while (surf->subsurface!=NULL) {
        surf = surf->subsurface->owner;
    }

    return pgSurface_AsSurface(surf);
}

static void
pgSurface_Prep(PyObject *surfobj)
{
    struct pgSubSurface_Data *data = ((pgSurfaceObject *)surfobj)->subsurface;
    SDL_Surface *surf = pgSurface_AsSurface(surfobj);

    if (data != NULL) {
        SDL_Surface *owner =  pg_get_surf_root((pgSurfaceObject *) surfobj);
        SDL_LockSurface(owner);
        surf->pixels = ((char *)owner->pixels) + data->pixeloffset;
    }
}

static void
pgSurface_Unprep(PyObject *surfobj)
{
    struct pgSubSurface_Data *data = ((pgSurfaceObject *)surfobj)->subsurface;
    SDL_Surface *surf = pgSurface_AsSurface(surfobj);
    if (data != NULL) {
        SDL_Surface *owner =  pg_get_surf_root((pgSurfaceObject *) surfobj);
        SDL_UnlockSurface(owner);
    }
}

static int
pgSurface_Lock(PyObject *surfobj)
{
    SDL_Surface *owner =  pg_get_surf_root((pgSurfaceObject *) surfobj);
    return (SDL_LockSurface(owner) == 0);
}

static int
pgSurface_Unlock(PyObject *surfobj)
{
    SDL_Surface *owner =  pg_get_surf_root((pgSurfaceObject *) surfobj);
    SDL_UnlockSurface(owner);
    return SDL_TRUE;
}

static int
pgSurface_LockBy(PyObject *surfobj, PyObject *lockobj)
{
    SDL_Surface *owner =  pg_get_surf_root((pgSurfaceObject *) surfobj);
    return (SDL_LockSurface(owner) == 0);
}

static int
pgSurface_UnlockBy(PyObject *surfobj, PyObject *lockobj)
{
    SDL_Surface *owner =  pg_get_surf_root((pgSurfaceObject *) surfobj);
    SDL_UnlockSurface(owner);
    return SDL_TRUE;
}

static PyMethodDef _surflock_methods[] = {{NULL, NULL, 0, NULL}};

/*DOC*/ static char _surflock_doc[] =
    /*DOC*/ "Surface locking support";

MODINIT_DEFINE(surflock)
{
    PyObject *module, *dict, *apiobj;
    int ecode;
    static void *c_api[PYGAMEAPI_SURFLOCK_NUMSLOTS];

#if PY3
    static struct PyModuleDef _module = {PyModuleDef_HEAD_INIT,
                                         "surflock",
                                         _surflock_doc,
                                         -1,
                                         _surflock_methods,
                                         NULL,
                                         NULL,
                                         NULL,
                                         NULL};
#endif

    /* Create the module and add the functions */
#if PY3
    module = PyModule_Create(&_module);
#else
    module =
        Py_InitModule3(MODPREFIX "surflock", _surflock_methods, _surflock_doc);
#endif
    if (module == NULL) {
        MODINIT_ERROR;
    }
    dict = PyModule_GetDict(module);

    /* export the c api */
    //c_api[0] = &pgLifetimeLock_Type;
    c_api[1] = pgSurface_Prep;
    c_api[2] = pgSurface_Unprep;
    c_api[3] = pgSurface_Lock;
    c_api[4] = pgSurface_Unlock;
    c_api[5] = pgSurface_LockBy;
    c_api[6] = pgSurface_UnlockBy;
    //c_api[7] = pgSurface_LockLifetime;
    apiobj = encapsulate_api(c_api, "surflock");
    if (apiobj == NULL) {
        DECREF_MOD(module);
        MODINIT_ERROR;
    }
    ecode = PyDict_SetItemString(dict, PYGAMEAPI_LOCAL_ENTRY, apiobj);
    Py_DECREF(apiobj);
    if (ecode) {
        DECREF_MOD(module);
        MODINIT_ERROR;
    }
    MODINIT_RETURN(module);
}

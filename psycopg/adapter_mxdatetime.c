/* adapter_mxdatetime.c - mx date/time objects
 *
 * Copyright (C) 2003-2004 Federico Di Gregorio <fog@debian.org>
 *
 * This file is part of psycopg.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2,
 * or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include <Python.h>
#include <structmember.h>
#include <stringobject.h>
#include <mxDateTime.h>
#include <string.h>

#define PSYCOPG_MODULE
#include "psycopg/config.h"
#include "psycopg/python.h"
#include "psycopg/psycopg.h"
#include "psycopg/adapter_mxdatetime.h"
#include "psycopg/microprotocols_proto.h"

/* the pointer to the mxDateTime API is initialized by the module init code,
   we just need to grab it */
extern mxDateTimeModule_APIObject *mxDateTimeP;


/* mxdatetime_str, mxdatetime_getquoted - return result of quoting */

static PyObject *
mxdatetime_str(mxdatetimeObject *self)
{
    PyObject *res = NULL;
    char *buffer = NULL;
    
    mxDateTimeObject *obj = (mxDateTimeObject*)self->wrapped;

    switch (self->type) {
        
    case 0:
        asprintf(&buffer, "'%02d:%02d:%.6f'",
                 (int)obj->hour, (int)obj->minute, (float)obj->second);
        if (buffer) res = PyString_FromString(buffer);
        break;

    case 1:
        asprintf(&buffer, "'%ld-%02d-%02d'",
                 obj->year, (int)obj->month, (int)obj->day);
        if (buffer) res = PyString_FromString(buffer);
        break;

    case 2:
        asprintf(&buffer, "'%ld-%02d-%02d %02d:%02d:%.6f'",
                 obj->year, (int)obj->month, (int)obj->day,
                 (int)obj->hour, (int)obj->minute, (float)obj->second);
        if (buffer) res = PyString_FromString(buffer);
        break;

    case 3:
        res = PyObject_CallMethod(self->wrapped, "strftime", "s",
                                  "'%d:%H:%M:%S'");
        break;
    }

    if (buffer) free(buffer);
    return res;
}

PyObject *
mxdatetime_getquoted(mxdatetimeObject *self, PyObject *args)
{
    if (!PyArg_ParseTuple(args, "")) return NULL;
    return mxdatetime_str(self);
}

PyObject *
mxdatetime_conform(mxdatetimeObject *self, PyObject *args)
{
    PyObject *res, *proto;
    
    if (!PyArg_ParseTuple(args, "O", &proto)) return NULL;

    if (proto == (PyObject*)&isqlquoteType)
        res = (PyObject*)self;
    else
        res = Py_None;
    
    Py_INCREF(res);
    return res;
}

/** the MxDateTime object **/

/* object member list */

static struct PyMemberDef mxdatetimeObject_members[] = {
    {"adapted", T_OBJECT, offsetof(mxdatetimeObject, wrapped), RO},
    {"type", T_INT, offsetof(mxdatetimeObject, type), RO},
    {NULL}
};

/* object method table */

static PyMethodDef mxdatetimeObject_methods[] = {
    {"getquoted", (PyCFunction)mxdatetime_getquoted, METH_VARARGS,
     "getquoted() -> wrapped object value as SQL date/time"},
    {"__conform__", (PyCFunction)mxdatetime_conform, METH_VARARGS, NULL},
    {NULL}  /* Sentinel */
};

/* initialization and finalization methods */

static int
mxdatetime_setup(mxdatetimeObject *self, PyObject *obj, int type)
{
    Dprintf("mxdatetime_setup: init mxdatetime object at %p, refcnt = %d",
            self, ((PyObject *)self)->ob_refcnt);

    self->type = type;
    self->wrapped = obj;
    Py_INCREF(self->wrapped);
    
    Dprintf("mxdatetime_setup: good mxdatetime object at %p, refcnt = %d",
            self, ((PyObject *)self)->ob_refcnt);
    return 0;
}

static void
mxdatetime_dealloc(PyObject* obj)
{
    mxdatetimeObject *self = (mxdatetimeObject *)obj;

    Py_XDECREF(self->wrapped);
    
    Dprintf("mxdatetime_dealloc: deleted mxdatetime object at %p, refcnt = %d",
            obj, obj->ob_refcnt);
    
    obj->ob_type->tp_free(obj);
}

static int
mxdatetime_init(PyObject *obj, PyObject *args, PyObject *kwds)
{
    PyObject *mx;
    int type = -1; /* raise an error if type was not passed! */
    
    if (!PyArg_ParseTuple(args, "O|i", &mx, &type))
        return -1;

    return mxdatetime_setup((mxdatetimeObject *)obj, mx, type);
}

static PyObject *
mxdatetime_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{    
    return type->tp_alloc(type, 0);
}

static void
mxdatetime_del(PyObject* self)
{
    PyObject_Del(self);
}

static PyObject *
mxdatetime_repr(mxdatetimeObject *self)
{
    return PyString_FromFormat("<psycopg2._psycopg.MxDateTime object at %p>",
                                self);
}

/* object type */

#define mxdatetimeType_doc \
"MxDateTime(mx, type) -> new mx.DateTime wrapper object"

PyTypeObject mxdatetimeType = {
    PyObject_HEAD_INIT(NULL)
    0,
    "psycopg2._psycopg.MxDateTime",
    sizeof(mxdatetimeObject),
    0,
    mxdatetime_dealloc, /*tp_dealloc*/
    0,          /*tp_print*/
    0,          /*tp_getattr*/
    0,          /*tp_setattr*/   

    0,          /*tp_compare*/
    (reprfunc)mxdatetime_repr, /*tp_repr*/
    0,          /*tp_as_number*/
    0,          /*tp_as_sequence*/
    0,          /*tp_as_mapping*/
    0,          /*tp_hash */

    0,          /*tp_call*/
    (reprfunc)mxdatetime_str, /*tp_str*/
    0,          /*tp_getattro*/
    0,          /*tp_setattro*/
    0,          /*tp_as_buffer*/

    Py_TPFLAGS_DEFAULT|Py_TPFLAGS_BASETYPE, /*tp_flags*/

    mxdatetimeType_doc, /*tp_doc*/
    
    0,          /*tp_traverse*/
    0,          /*tp_clear*/

    0,          /*tp_richcompare*/
    0,          /*tp_weaklistoffset*/

    0,          /*tp_iter*/
    0,          /*tp_iternext*/

    /* Attribute descriptor and subclassing stuff */

    mxdatetimeObject_methods, /*tp_methods*/
    mxdatetimeObject_members, /*tp_members*/
    0,          /*tp_getset*/
    0,          /*tp_base*/
    0,          /*tp_dict*/
    
    0,          /*tp_descr_get*/
    0,          /*tp_descr_set*/
    0,          /*tp_dictoffset*/
    
    mxdatetime_init, /*tp_init*/
    PyType_GenericAlloc, /*tp_alloc*/
    mxdatetime_new, /*tp_new*/
    (freefunc)mxdatetime_del, /*tp_free  Low-level free-memory routine */
    0,          /*tp_is_gc For PyObject_IS_GC */
    0,          /*tp_bases*/
    0,          /*tp_mro method resolution order */
    0,          /*tp_cache*/
    0,          /*tp_subclasses*/
    0           /*tp_weaklist*/
};


/** module-level functions **/

#ifdef PSYCOPG_DEFAULT_MXDATETIME

PyObject *
psyco_Date(PyObject *self, PyObject *args) 
{
	PyObject *res, *mx;
	int year, month, day;
	
	if (!PyArg_ParseTuple(args, "iii", &year, &month, &day))
	    return NULL;
					 
	mx = mxDateTimeP->DateTime_FromDateAndTime(year, month, day, 0, 0, 0.0);
    if (mx == NULL) return NULL;

    res = PyObject_CallFunction((PyObject *)&mxdatetimeType, "Oi", mx,
                                PSYCO_MXDATETIME_DATE);
    Py_DECREF(mx);
    return res;
}

PyObject *
psyco_Time(PyObject *self, PyObject *args) 
{
	PyObject *res, *mx;
    int hours, minutes=0;
	double seconds=0.0;

    if (!PyArg_ParseTuple(args, "iid", &hours, &minutes, &seconds))
	    return NULL;
					 
	mx = mxDateTimeP->DateTimeDelta_FromTime(hours, minutes, seconds);
    if (mx == NULL) return NULL;

    res = PyObject_CallFunction((PyObject *)&mxdatetimeType, "Oi", mx,
                                PSYCO_MXDATETIME_TIME);
    Py_DECREF(mx);
    return res;    
}

PyObject *
psyco_Timestamp(PyObject *self, PyObject *args) 
{
	PyObject *res, *mx;
    int year, month, day;
	int hour=0, minute=0; /* default to midnight */
	double second=0.0;
    
    if (!PyArg_ParseTuple(args, "lii|iid", &year, &month, &day,
                          &hour, &minute, &second))
	    return NULL;

    mx = mxDateTimeP->DateTime_FromDateAndTime(year, month, day,
                                               hour, minute, second);
    if (mx == NULL) return NULL;

    res = PyObject_CallFunction((PyObject *)&mxdatetimeType, "Oi", mx,
                                PSYCO_MXDATETIME_TIMESTAMP);
    Py_DECREF(mx);
    return res;     
}

PyObject *
psyco_DateFromTicks(PyObject *self, PyObject *args)
{
    PyObject *res, *mx;
    double ticks;

    if (!PyArg_ParseTuple(args,"d", &ticks))
        return NULL;
					 
    if (!(mx = mxDateTimeP->DateTime_FromTicks(ticks)))
        return NULL;
    
    res = PyObject_CallFunction((PyObject *)&mxdatetimeType, "Oi", mx,
                                PSYCO_MXDATETIME_DATE);
    Py_DECREF(mx);
    return res; 
}

PyObject *
psyco_TimeFromTicks(PyObject *self, PyObject *args)
{
    PyObject *res, *mx, *dt;
    double ticks;

    if (!PyArg_ParseTuple(args,"d", &ticks))
        return NULL;
					 
    if (!(dt = mxDateTimeP->DateTime_FromTicks(ticks)))
        return NULL;

    if (!(mx = mxDateTimeP->DateTimeDelta_FromDaysAndSeconds(
            0, ((mxDateTimeObject*)dt)->abstime)))
    {
        Py_DECREF(dt);
        return NULL;
    }

    Py_DECREF(dt);
    res = PyObject_CallFunction((PyObject *)&mxdatetimeType, "Oi", mx,
                                PSYCO_MXDATETIME_TIME);
    Py_DECREF(mx);
    return res; 
}

PyObject *
psyco_TimestampFromTicks(PyObject *self, PyObject *args)
{
    PyObject *mx, *res;
    double ticks;

    if (!PyArg_ParseTuple(args, "d", &ticks))
        return NULL;
					 
    if (!(mx = mxDateTimeP->DateTime_FromTicks(ticks)))
        return NULL;

    res = PyObject_CallFunction((PyObject *)&mxdatetimeType, "Oi", mx,
                                 PSYCO_MXDATETIME_TIMESTAMP);
    Py_DECREF(mx);
    return res; 
}

#endif

PyObject *
psyco_DateFromMx(PyObject *self, PyObject *args)
{
    PyObject *mx;

    if (!PyArg_ParseTuple(args, "O!", mxDateTimeP->DateTime_Type, &mx))
        return NULL;
    
    return PyObject_CallFunction((PyObject *)&mxdatetimeType, "Oi", mx,
                                 PSYCO_MXDATETIME_DATE);
}

PyObject *
psyco_TimeFromMx(PyObject *self, PyObject *args)
{
    PyObject *mx;

    if (!PyArg_ParseTuple(args, "O!", mxDateTimeP->DateTimeDelta_Type, &mx))
        return NULL;
    
    return PyObject_CallFunction((PyObject *)&mxdatetimeType, "Oi", mx,
                                 PSYCO_MXDATETIME_TIME);
}

PyObject *
psyco_TimestampFromMx(PyObject *self, PyObject *args)
{
    PyObject *mx;

    if (!PyArg_ParseTuple(args, "O!", mxDateTimeP->DateTime_Type, &mx))
        return NULL;
    
    return PyObject_CallFunction((PyObject *)&mxdatetimeType, "Oi", mx,
                                 PSYCO_MXDATETIME_TIMESTAMP);
}

PyObject *
psyco_IntervalFromMx(PyObject *self, PyObject *args)
{
    PyObject *mx;

    if (!PyArg_ParseTuple(args, "O!", mxDateTimeP->DateTime_Type, &mx))
        return NULL;
    
    return PyObject_CallFunction((PyObject *)&mxdatetimeType, "Oi", mx,
                                 PSYCO_MXDATETIME_INTERVAL);
}

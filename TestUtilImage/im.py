import ctypes as C
from ctypes.util import find_library as _find_library
import numpy as N

_cim = C.CDLL(_find_library("im"))

#templates
_alias_c = { N.dtype('uint8')  : "u8",
             N.dtype('uint16') : "u16",
             N.dtype('uint32') : "u32",
             N.dtype('uint64') : "u64",
              N.dtype('int8')  : "i8",
              N.dtype('int16') : "i16",
              N.dtype('int32') : "i32",
              N.dtype('int64') : "i64",
              N.dtype('float') : "f32",
            N.dtype('float32') : "f32",
             N.dtype('double') : "f64",
            N.dtype('float64') : "f64" }
_alias_ctypes = { N.dtype('uint8')  : C.c_uint8,
                  N.dtype('uint16') : C.c_uint16,
                  N.dtype('uint32') : C.c_uint32,
                  N.dtype('uint64') : C.c_uint64,
                   N.dtype('int8')  : C.c_int8, 
                   N.dtype('int16') : C.c_int16,
                   N.dtype('int32') : C.c_int32,
                   N.dtype('int64') : C.c_int64,
                   N.dtype('float') : C.c_float,
                 N.dtype('float32') : C.c_float,
                 N.dtype( 'double') : C.c_double,
                 N.dtype('float64') : C.c_double }

def _cim_template_fun(name,t0,t1):
  return _cim.__getattr__( "%s_%s_%s"%( name, _alias_c[t0], _alias_c[t1] ) )

def _im_copy(t0,t1):
  return _cim_template_fun("imCopy",t0,t1)
def _im_cast_copy(t0,t1):
  return _cim_template_fun("imCastCopy",t0,t1)
def _im_copy_transpose(t0,t1):
  return _cim_template_fun("imCopyTranspose",t0,t1)

def _arg_helper(source, shape = None, dest = None, source_ori = None, dest_ori = None, source_pitches = None, dest_pitches = None): 
  # source origin
  if source_ori is None:
    source_ori = N.array((0,0,0))
  if dest_ori is None:
    dest_ori = N.array((0,0,0))

  # compute shape of copied area
  def clipped(a,ori):
    return N.array(a.shape) - ori

  if shape is None:
    shape = clipped(source,source_ori)
    if not (dest is None):
      shape = N.minimum( shape, clipped(dest,dest_ori) )
  if not isinstance(shape,N.ndarray):
    shape = N.array(shape)

  if dest is None:
    dest = N.zeros(shape,dtype=source.dtype)

  # pitches
  def pitches(a):
    return list(reversed( N.cumprod( list(reversed( a.shape )) ) ))
  ptype = N.uint32 if C.sizeof(C.c_size_t(0))==4 else N.uint64
  shape = shape.astype(ptype)

  if source_pitches is None:
    source_pitches = N.ones(4,dtype = ptype)
    source_pitches[:-1] = pitches(source) # leave source_pitches[-1] = 1
  if dest_pitches is None:
    dest_pitches = N.ones(4,dtype = ptype)
    dest_pitches[:-1] = pitches(dest)
  return source, shape, dest, source_ori, dest_ori, source_pitches, dest_pitches

def copy(source, shape = None, dest = None, source_ori = None, dest_ori = None, source_pitches = None, dest_pitches = None):
  # handle the different ways of calling
  source, shape, dest, source_ori, dest_ori, source_pitches, dest_pitches = _arg_helper(source, shape, dest, source_ori, dest_ori, source_pitches, dest_pitches )
  # call the c function
  _im_copy( dest.dtype, source.dtype )( dest.ctypes.data_as(          C.POINTER( _alias_ctypes[ dest.dtype ] )),
                                        dest_pitches.ctypes.data_as(  C.POINTER( C.c_size_t )),
                                        source.ctypes.data_as(        C.POINTER( _alias_ctypes[ source.dtype ] )),
                                        source_pitches.ctypes.data_as(C.POINTER( C.c_size_t )),
                                        shape.ctypes.data_as(         C.POINTER( C.c_size_t )) )
  return dest

def castcopy(source, shape = None, dest = None, source_ori = None, dest_ori = None, source_pitches = None, dest_pitches = None):
  # handle the different ways of calling
  source, shape, dest, source_ori, dest_ori, source_pitches, dest_pitches = _arg_helper(source, shape, dest, source_ori, dest_ori, source_pitches, dest_pitches )
  # call the c function
  _im_cast_copy( dest.dtype, source.dtype )(dest.ctypes.data_as(          C.POINTER( _alias_ctypes[ dest.dtype ] )),
                                            dest_pitches.ctypes.data_as(  C.POINTER( C.c_size_t )),
                                            source.ctypes.data_as(        C.POINTER( _alias_ctypes[ source.dtype ] )),
                                            source_pitches.ctypes.data_as(C.POINTER( C.c_size_t )),
                                            shape.ctypes.data_as(         C.POINTER( C.c_size_t )) )
  return dest

def swapaxes(source, axis1, axis2, shape = None, dest = None, source_ori = None, dest_ori = None, source_pitches = None, dest_pitches = None):
  # handle the different ways of calling
  source, shape, dest, source_ori, dest_ori, source_pitches, dest_pitches = _arg_helper(source, shape, dest, source_ori, dest_ori, source_pitches, dest_pitches )
  # call the c function
  _im_copy_transpose( dest.dtype, source.dtype )( dest.ctypes.data_as(          C.POINTER( _alias_ctypes[ dest.dtype ] )),
                                                  dest_pitches.ctypes.data_as(  C.POINTER( C.c_size_t )),
                                                  source.ctypes.data_as(        C.POINTER( _alias_ctypes[ source.dtype ] )),
                                                  source_pitches.ctypes.data_as(C.POINTER( C.c_size_t )),
                                                  shape.ctypes.data_as(         C.POINTER( C.c_size_t )),
                                                  C.c_int(axis1), C.c_int(axis2) )
  return dest

###
###
###     TESTING 
###
###

def testimage(w,h,dtype=N.uint8):
  top  = { N.uint8 : (1<<8)-1,
           N.uint16: (1<<16)-1,
           N.uint32: (1<<32)-1,
           N.uint64: (1<<64)-1,
            N.int8 : (1<<7)-1,
            N.int16: (1<<15)-1,
            N.int32: (1<<31)-1,
            N.int64: (1<<63)-1,
          N.float32: 1.0,
          N.float  : 1.0,
          N.double : 1.0,
          N.float64: 1.0 }[dtype]
  rgb = N.zeros((h,w,3),dtype)
  i = N.indices( rgb.shape )
  x = i[1][:,:,0]
  y = i[0][:,:,0]
  f = lambda x: top * (1+x)/2.0
  rgb[:,:,0] = f( N.imag( N.exp(-1j*2*N.pi*0.1*N.sqrt(( (x-w/2.0)**2 + (y-h/2.0)**2 ))) ) )
  rgb[:,:,1] = f( N.real( N.exp(-1j*2*N.pi*0.1*N.sqrt(( (x-w/2.0)**2 + (y-h/2.0)**2 ))) ) )
  rgb[:,:,2] = f( N.real( N.exp(-1j*2*N.pi*0.1*N.sqrt(( (x-w/2.0)**2  ))) ) )
  return rgb

import unittest

class TestCase(unittest.TestCase):
  def setUp(self):
    self.rgb = testimage(320,240)

  def test_copy_0(self):
    out = copy(self.rgb)
    self.assertTrue( (out-self.rgb).sum() == 0 )

  def test_castcopy_0(self):
    out = N.zeros((240,320,3),dtype=float)
    castcopy(self.rgb,dest=out)
    self.assertTrue( (out-self.rgb).sum() == 0 )

  def test_copy_1(self):
    out = N.zeros((240,320,3),dtype=float)
    copy(self.rgb,dest=out)
    self.assertTrue( (out-self.rgb).sum() == 0 )

  def test_swap_axes_0(self):
    out = N.zeros((480,640,3),dtype=self.rgb.dtype)
    swapaxes(self.rgb,0,1,dest=out)
    self.assertTrue( (N.swapaxes(out,0,1)[:240,:320]-self.rgb).sum() == 0 )

  def test_swap_axes_1(self):
    out = N.zeros((320,240,3),dtype=self.rgb.dtype)
    out = swapaxes(self.rgb,0,1,dest=out,shape=(240,320))

if __name__=='__main__':
  suite = unittest.TestLoader().loadTestsFromTestCase(TestCase)
  unittest.TextTestRunner(verbosity=2).run(suite)

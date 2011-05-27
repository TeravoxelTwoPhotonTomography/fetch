/*
 * util-wrap.h
 *
 *  Created on: May 17, 2010
 *      Author: Nathan Clack <clackn@janelia.hhmi.org>
 */
/*
 * Copyright 2010 Howard Hughes Medical Institute.
 * All rights reserved.
 * Use is subject to Janelia Farm Research Campus Software Copyright 1.1
 * license terms (http://license.janelia.org/license/jfrc_copyright_1_1.html).
 */
/*
 * TODO
 * ----
 * [ ] Convert while loops to for loops
 */

#pragma once

#pragma warning(push)
#pragma warning(disable:4244)

#include <math.h>

namespace resonant_correction
{
  namespace wrap
  {
  
    /* 
    INTERFACE
    ---------
    
    get_dim    
      Computes dimensions of output image given the turning point.      
      Returns: 0 if turn is out-of-bounds, 1 otherwise.
      
    transform
      Performs the wrapping at the specified turning point.            
      Returns: 0 if turn is out-of-bounds, 1 otherwise
      
    optimize_turn
      Finds the turning point that minimizes the difference between
      odd and even lines in the output image.      
      Returns: the estimated turning point.      
    */

    bool
    get_dim(int w, int h, float turn, int *ow, int *oh);

    template<typename T>
      bool
      transform(T *out, T *im, int iw, int ih, float turn);

    template<typename T>
      float
      optimize_turn(T* im, int w, int h, int low, int high, float step,
                           int nright);

  } // namespace wrap
} // namespace resonant_correction


/*
 *
 * (most) IMPLEMENTATION
 *
 */
namespace resonant_correction
{
  namespace wrap
  {
    // low inclusive, high exclusive
    template<typename T>
      void
      colcopy(T *dst, int dstw, int dsth, int dstlow, int dsthigh, T *src,
              int srcw, int srch, int srclow, int srchigh)
      {
        T *dbeg = dst + dstlow, *dcur = dbeg + dstw * dsth, *sbeg = src
            + srclow, *scur = sbeg + srcw * srch;
        int dn = dsthigh - dstlow, sn = srchigh - srclow, dstep = (dstlow
            < dsthigh) ? 1 : -1, sstep = (srclow < srchigh) ? 1 : -1;
        assert(abs(dn) == abs(sn));
        while (dcur > dbeg)
        {
          T *d, *s;
          dcur -= dstw;
          scur -= srcw;
          d = dcur;
          s = scur;
          while (d != dcur + dn)
          {
            *d = *s;
            d += dstep;
            s += sstep;
          }
        }
      }

    // low inclusive, high exclusive
    template<typename T>
      void
      colcopyadd(T *dst, int dstw, int dsth, int dstlow, int dsthigh, T *src,
                 int srcw, int srch, int srclow, int srchigh, float alpha,
                 float beta)
      {
        T *dbeg = dst + dstlow, *dcur = dbeg + dstw * dsth, *sbeg = src
            + srclow, *scur = sbeg + srcw * srch;
        int dn = dsthigh - dstlow, sn = srchigh - srclow, dstep = (dstlow
            < dsthigh) ? 1 : -1, sstep = (srclow < srchigh) ? 1 : -1;
        assert(dn == sn);
        while (dcur > dbeg)
        {
          T *d, *s;
          dcur -= dstw;
          scur -= srcw;
          d = dcur;
          s = scur;
          while (d != dcur + dsthigh)
          {
            *d = (T) (*d * alpha + *s * beta);
            d += dstep;
            s += sstep;
          }
        }
      }

    namespace internal
    {
      template<typename T>
      inline T min(const T& a, const T& b) {return (a<b)?a:b;}
      template<typename T>
      inline T max(const T& a, const T& b) {return (a<b)?b:a;}
    }

    template<typename T>
      bool
      transform(T *out, T *im, int iw, int ih, float turn)
      {
        int w = lroundf(turn), t, outw, outh = ih;
        float s = floorf(turn), delta = turn - s, L, R;

        if (turn >= iw)
          return 0;

        w = internal::min<T>(w, iw - w);
        outw = 2 * w;
        t = internal::max<T>(0, s - w);

        if (delta >= 0.5)
        {
          delta = 1 - delta;
          R = 1 - 2 * delta;
          L = 2 * delta;
        } else
        {
          L = 1 - 2 * delta;
          R = 2 * delta;
        }

        memset(out, 0, outw * outh * sizeof(T));
        colcopy(out, outw, outh, outw - 1, w - 1, // right side flipped
                im,
                iw,
                ih,
                s,
                s + w);
        colcopy(out, outw, outh, 0, w, // left side interpolated
                im,
                iw,
                ih,
                t,
                t + w);
        colcopyadd(out, outw, outh, 0, w, im, iw, ih, t + 1, t + w + 1, L, R);
        return 1; // success
      }

    template<typename T>
      float
      rmsd(T *a, int apitch, T *b, int bpitch, int nrows, int ncols)
      {
        float acc = 0, d, norm = nrows * ncols;

        while (nrows--)
        {
          T *ac = a + nrows * apitch, *bc = b + nrows * bpitch, *ab = ac
              - ncols;
          while (ac > ab)
          {
            d = (float) (*(--ac)) - (float) (*(--bc));
            acc += d * d;
          }
        }
        return sqrt(acc / norm);
      }

    template<typename T>
      float
      err(T* im, int w, int h, int nright)
      {
        int ww = 2 * w, hh = h / 2;
        nright = min(nright, w);
        return unwrap_im_rmsd<T> (im + w - nright,
                                  ww,
                                  im + 2 * w - nright,
                                  ww,
                                  hh,
                                  nright);
      }

    template<typename T>
      T *
      im_alloc(int w, int h)
      {
        return malloc(w * h * sizeof(T));
      }

#include <float.h>
    template<typename T>
      int
      optimize_turn_pixel_precision(T* im, int w, int h, int low,
                                           int high, int nright)
      {
        T *out = im_alloc<T> (w, h);
        int turn, ow, oh, arg = -1;
        float e, argmin = FLT_MAX;
        for (turn = low; turn < high; turn++)
        {
          unwrap<T> (out, im, w, h, turn);
          get_dim(w, h, turn, &ow, &oh);
          e = unwrap_err(out, ow, oh, nright);
          if (e < argmin)
          {
            argmin = e;
            arg = turn;
          }
        }
        free(out);
        return arg;
      }

    template<typename T>
      float
      optimize_turn_subpixel_precision(T* im, int w, int h, float low,
                                              float high, float step,
                                              int nright)
      {
        T *out = im_alloc<T> (w, h);
        int ow, oh;
        float turn, arg, e, argmin = FLT_MAX;
        for (turn = low; turn < high; turn += step)
        {
          unwrap<T> (out, im, w, h, turn);
          get_dim(w, h, turn, &ow, &oh);
          e = unwrap_err(out, ow, oh, nright);
          if (e < argmin)
          {
            argmin = e;
            arg = turn;
          }
        }
        free(out);
        return arg;
      }

    template<typename T>
      float
      optimize_turn(T* im, int w, int h, int low, int high, float step,
                           int nright)
      {
        int px = unwrap_optimize_turn_pixel_precision<T> (im,
                                                          w,
                                                          h,
                                                          low,
                                                          high,
                                                          nright);
        return unwrap_optimize_turn_subpixel_precision<T> (im, w, h, px - 1, px
            + 1, step, nright);
      }
  } // namespace wrap
} // namespace resonant_correction

#pragma warning(pop) // disable C4244 - argument conversion, potential loss of data
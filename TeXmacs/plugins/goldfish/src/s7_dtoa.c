/* s7_dtoa.c - double-to-ASCII conversion (Grisu2 algorithm)
 *
 * derived from fpconv (MIT License)
 * SPDX-License-Identifier: MIT
 *
 * The MIT License

Copyright (c) 2013 Andreas Samoljuk

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be
included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#ifndef WITH_DTOA
  #define WITH_DTOA 1
#endif

#if WITH_DTOA

#include "s7_dtoa.h"

#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <inttypes.h>

#define dtoa_npowers     87
#define dtoa_steppowers  8
#define dtoa_firstpower -348 /* 10 ^ -348 */
#define dtoa_expmax     -32
#define dtoa_expmin     -60

typedef struct dtoa_np {uint64_t frac; int32_t exp;} dtoa_np;

static const dtoa_np dtoa_powers_ten[] = {
    { 18054884314459144840U, -1220 }, { 13451937075301367670U, -1193 }, { 10022474136428063862U, -1166 }, { 14934650266808366570U, -1140 },
    { 11127181549972568877U, -1113 }, { 16580792590934885855U, -1087 }, { 12353653155963782858U, -1060 }, { 18408377700990114895U, -1034 },
    { 13715310171984221708U, -1007 }, { 10218702384817765436U, -980 }, { 15227053142812498563U, -954 },  { 11345038669416679861U, -927 },
    { 16905424996341287883U, -901 },  { 12595523146049147757U, -874 }, { 9384396036005875287U,  -847 },  { 13983839803942852151U, -821 },
    { 10418772551374772303U, -794 },  { 15525180923007089351U, -768 }, { 11567161174868858868U, -741 },  { 17236413322193710309U, -715 },
    { 12842128665889583758U, -688 },  { 9568131466127621947U,  -661 }, { 14257626930069360058U, -635 },  { 10622759856335341974U, -608 },
    { 15829145694278690180U, -582 },  { 11793632577567316726U, -555 }, { 17573882009934360870U, -529 },  { 13093562431584567480U, -502 },
    { 9755464219737475723U,  -475 },  { 14536774485912137811U, -449 }, { 10830740992659433045U, -422 },  { 16139061738043178685U, -396 },
    { 12024538023802026127U, -369 },  { 17917957937422433684U, -343 }, { 13349918974505688015U, -316 },  { 9946464728195732843U,  -289 },
    { 14821387422376473014U, -263 },  { 11042794154864902060U, -236 }, { 16455045573212060422U, -210 },  { 12259964326927110867U, -183 },
    { 18268770466636286478U, -157 },  { 13611294676837538539U, -130 }, { 10141204801825835212U, -103 },  { 15111572745182864684U, -77 },
    { 11258999068426240000U, -50 },   { 16777216000000000000U, -24 }, { 12500000000000000000U,   3 },   { 9313225746154785156U,   30 },
    { 13877787807814456755U,  56 },   { 10339757656912845936U,  83 }, { 15407439555097886824U, 109 },   { 11479437019748901445U, 136 },
    { 17105694144590052135U, 162 },   { 12744735289059618216U, 189 }, { 9495567745759798747U,  216 },   { 14149498560666738074U, 242 },
    { 10542197943230523224U, 269 },   { 15709099088952724970U, 295 }, { 11704190886730495818U, 322 },   { 17440603504673385349U, 348 },
    { 12994262207056124023U, 375 },   { 9681479787123295682U,  402 }, { 14426529090290212157U, 428 },   { 10748601772107342003U, 455 },
    { 16016664761464807395U, 481 },   { 11933345169920330789U, 508 }, { 17782069995880619868U, 534 },   { 13248674568444952270U, 561 },
    { 9871031767461413346U,  588 },   { 14708983551653345445U, 614 }, { 10959046745042015199U, 641 },   { 16330252207878254650U, 667 },
    { 12166986024289022870U, 694 },   { 18130221999122236476U, 720 }, { 13508068024458167312U, 747 },   { 10064294952495520794U, 774 },
    { 14996968138956309548U, 800 },   { 11173611982879273257U, 827 }, { 16649979327439178909U, 853 },   { 12405201291620119593U, 880 },
    { 9242595204427927429U,  907 },   { 13772540099066387757U, 933 }, { 10261342003245940623U, 960 },   { 15290591125556738113U, 986 },
    { 11392378155556871081U, 1013 },  { 16975966327722178521U, 1039 },
    { 12648080533535911531U, 1066 }};

static dtoa_np dtoa_find_cachedpow10(int exp, int *k)
{
  const double one_log_ten = 0.30102999566398114;
  int32_t approx = -(exp + dtoa_npowers) * one_log_ten;
  int32_t idx = (approx - dtoa_firstpower) / dtoa_steppowers;
  while (true)
    {
      int32_t current = exp + dtoa_powers_ten[idx].exp + 64;
      if (current < dtoa_expmin)
        {
          idx++;
          continue;
        }
      if (current > dtoa_expmax)
        {
          idx--;
          continue;
        }
      *k = (dtoa_firstpower + idx * dtoa_steppowers);
      return(dtoa_powers_ten[idx]);
    }
}

#define dtoa_fracmask  0x000FFFFFFFFFFFFFU
#define dtoa_expmask   0x7FF0000000000000U
#define dtoa_hiddenbit 0x0010000000000000U
#define dtoa_signmask  0x8000000000000000U
#define dtoa_expbias   (1023 + 52)
#define dtoa_absv(n)   ((n) < 0 ? -(n) : (n))
#define dtoa_minv(a, b) ((a) < (b) ? (a) : (b))

static uint64_t dtoa_tens[] =
  { 10000000000000000000U, 1000000000000000000U, 100000000000000000U,
    10000000000000000U, 1000000000000000U, 100000000000000U,
    10000000000000U, 1000000000000U, 100000000000U,
    10000000000U, 1000000000U, 100000000U,
    10000000U, 1000000U, 100000U,
    10000U, 1000U, 100U,
    10U, 1U};

static uint64_t dtoa_get_dbits(double d)
{
  union {double dbl; uint64_t i;} dbl_bits = {d};
  return(dbl_bits.i);
}

static dtoa_np dtoa_build_np(double d)
{
  uint64_t bits = dtoa_get_dbits(d);
  dtoa_np fp;
  fp.frac = bits & dtoa_fracmask;
  fp.exp = (bits & dtoa_expmask) >> 52;
  if (fp.exp)
    {
      fp.frac += dtoa_hiddenbit;
      fp.exp -= dtoa_expbias;
    }
  else fp.exp = -dtoa_expbias + 1;
  return(fp);
}

static void dtoa_normalize(dtoa_np *fp)
{
  int32_t shift = 64 - 52 - 1;
  while ((fp->frac & dtoa_hiddenbit) == 0)
    {
      fp->frac <<= 1;
      fp->exp--;
    }
  fp->frac <<= shift;
  fp->exp -= shift;
}

static void dtoa_get_normalized_boundaries(dtoa_np *fp, dtoa_np *lower, dtoa_np *upper)
{
  int32_t u_shift, l_shift;
  upper->frac = (fp->frac << 1) + 1;
  upper->exp  = fp->exp - 1;
  while ((upper->frac & (dtoa_hiddenbit << 1)) == 0)
    {
      upper->frac <<= 1;
      upper->exp--;
    }
  u_shift = 64 - 52 - 2;
  upper->frac <<= u_shift;
  upper->exp = upper->exp - u_shift;
  l_shift = (fp->frac == dtoa_hiddenbit) ? 2 : 1;
  lower->frac = (fp->frac << l_shift) - 1;
  lower->exp = fp->exp - l_shift;
  lower->frac <<= lower->exp - upper->exp;
  lower->exp = upper->exp;
}

static dtoa_np dtoa_multiply(dtoa_np *a, dtoa_np *b) /* const dtoa_np* here and elsewhere is slower!  perverse */
{
  dtoa_np fp;
  const uint64_t lomask = 0x00000000FFFFFFFF;
  uint64_t ah_bl = (a->frac >> 32)    * (b->frac & lomask);
  uint64_t al_bh = (a->frac & lomask) * (b->frac >> 32);
  uint64_t al_bl = (a->frac & lomask) * (b->frac & lomask);
  uint64_t ah_bh = (a->frac >> 32)    * (b->frac >> 32);
  uint64_t tmp = (ah_bl & lomask) + (al_bh & lomask) + (al_bl >> 32);
  /* round up */
  tmp += 1U << 31;
  fp.frac = ah_bh + (ah_bl >> 32) + (al_bh >> 32) + (tmp >> 32);
  fp.exp = a->exp + b->exp + 64;
  return(fp);
}

static void dtoa_round_digit(char *digits, int32_t ndigits, uint64_t delta, uint64_t rem, uint64_t kappa, uint64_t frac)
{
  while ((rem < frac) && (delta - rem >= kappa) &&
         ((rem + kappa < frac) || (frac - rem > rem + kappa - frac)))
    {
      digits[ndigits - 1]--;
      rem += kappa;
    }
}

static int32_t dtoa_generate_digits(dtoa_np *fp, dtoa_np *upper, dtoa_np *lower, char *digits, int *K)
{
  uint64_t part1, part2, wfrac = upper->frac - fp->frac, delta = upper->frac - lower->frac;
  uint64_t *unit;
  int32_t idx = 0, kappa = 10;
  dtoa_np one;

  one.frac = 1ULL << -upper->exp;
  one.exp  = upper->exp;
  part1 = upper->frac >> -one.exp;
  part2 = upper->frac & (one.frac - 1);

  /* 1000000000 */
  for (uint64_t *divp = dtoa_tens + 10; kappa > 0; divp++)
    {
      uint64_t tmp, div = *divp;
      unsigned digit = part1 / div;
      if (digit || idx)
        digits[idx++] = digit + '0';
      part1 -= digit * div;
      kappa--;
      tmp = (part1 << -one.exp) + part2;
      if (tmp <= delta)
        {
          *K += kappa;
          dtoa_round_digit(digits, idx, delta, tmp, div << -one.exp, wfrac);
          return(idx);
        }}

  /* 10 */
  unit = dtoa_tens + 18;
  while(true)
    {
      unsigned digit;
      part2 *= 10;
      delta *= 10;
      kappa--;
      digit = part2 >> -one.exp;
      if (digit || idx)
        digits[idx++] = digit + '0';
      part2 &= one.frac - 1;
      if (part2 < delta)
        {
          *K += kappa;
          dtoa_round_digit(digits, idx, delta, part2, one.frac, wfrac * *unit);
          return(idx);
        }
      unit--;
    }
}

static int32_t dtoa_grisu2(double d, char *digits, int *K)
{
  int32_t k;
  dtoa_np cp, lower, upper;
  dtoa_np w = dtoa_build_np(d);
  dtoa_get_normalized_boundaries(&w, &lower, &upper);
  dtoa_normalize(&w);
  cp = dtoa_find_cachedpow10(upper.exp, &k);
  w = dtoa_multiply(&w, &cp);
  upper = dtoa_multiply(&upper, &cp);
  lower = dtoa_multiply(&lower, &cp);
  lower.frac++;
  upper.frac--;
  *K = -k;
  return(dtoa_generate_digits(&w, &upper, &lower, digits, K));
}

static int32_t dtoa_emit_digits(char *digits, int32_t ndigits, char *dest, int32_t K, bool neg)
{
  int32_t idx, cent;
  char sign;
  int32_t exp = dtoa_absv(K + ndigits - 1);

  /* write plain integer */
  if ((K >= 0) && (exp < (ndigits + 7)))
    {
      memcpy(dest, digits, ndigits);
      memset(dest + ndigits, '0', K); /* unaligned */
      dest[ndigits + K] = '.';
      dest[ndigits + K + 1] = '0';
      return(ndigits + K + 2);
    }

  /* write decimal w/o scientific notation */
  if ((K < 0) && (K > -7 || exp < 4))
    {
      int32_t offset = ndigits - dtoa_absv(K);
      /* fp < 1.0 -> write leading zero */
      if (offset <= 0)
        {
          offset = -offset;
          dest[0] = '0';
          dest[1] = '.';
          memset(dest + 2, '0', offset); /* unaligned */
          memcpy(dest + offset + 2, digits, ndigits);
          return(ndigits + 2 + offset);
          /* fp > 1.0 */
        }
      else
        {
          memcpy(dest, digits, offset);
          dest[offset] = '.';
          memcpy(dest + offset + 1, digits + offset, ndigits - offset);
          return(ndigits + 1);
        }}

  /* write decimal w/ scientific notation */
  ndigits = dtoa_minv(ndigits, 18 - neg);
  idx = 0;
  dest[idx++] = digits[0];
  if (ndigits > 1)
    {
      dest[idx++] = '.';
      memcpy(dest + idx, digits + 1, ndigits - 1);
      idx += ndigits - 1;
    }
  dest[idx++] = 'e';
  sign = K + ndigits - 1 < 0 ? '-' : '+';
  dest[idx++] = sign;
  cent = 0;
  if (exp > 99)
    {
      cent = exp / 100;
      dest[idx++] = cent + '0';
      exp -= cent * 100;
    }
  if (exp > 9)
    {
      int32_t dec = exp / 10;
      dest[idx++] = dec + '0';
      exp -= dec * 10;
    }
  else
    if (cent)
      dest[idx++] = '0';

  dest[idx++] = exp % 10 + '0';
  return(idx);
}

static int64_t dtoa_nan_payload(double x)
{
  union {uint64_t ix; double fx;} num;
  num.fx = x;
  return(num.ix & 0xffffffffffff);
}

static int32_t dtoa_filter_special(double fp, char *dest, bool neg)
{
  uint64_t bits;
  bool nan;
  if (fp == 0.0)
    {
      dest[0] = '0'; dest[1] = '.'; dest[2] = '0';
      return(3);
    }
  bits = dtoa_get_dbits(fp);
  nan = (bits & dtoa_expmask) == dtoa_expmask;
  if (!nan) return(0);

  if (!neg)
    {
      dest[0] = '+'; /* else 1.0-nan...? */
      dest++;
    }
  if (bits & dtoa_fracmask)
    {
      int64_t payload = dtoa_nan_payload(fp);
      int32_t len;
      len = (int32_t)snprintf(dest, 22, "nan.%" PRId64, payload);
      return((neg) ? len : len + 1);
    }
  dest[0] = 'i'; dest[1] = 'n'; dest[2] = 'f'; dest[3] = '.'; dest[4] = '0';
  return((neg) ? 5 : 6);
}

int fpconv_dtoa(double d, char *buffer)
{
  char digit[23];
  int32_t str_len = 0, spec, K, ndigits;
  bool neg = false;

  if (dtoa_get_dbits(d) & dtoa_signmask)
    {
      buffer[0] = '-';
      str_len++;
      neg = true;
    }
  spec = dtoa_filter_special(d, buffer + str_len, neg);
  if (spec) return(str_len + spec);
  K = 0;
  ndigits = dtoa_grisu2(d, digit, &K);
  str_len += dtoa_emit_digits(digit, ndigits, buffer + str_len, K, neg);
  return(str_len);
}

#endif /* WITH_DTOA */

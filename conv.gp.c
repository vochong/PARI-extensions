/******************************************************************************/
/**                             Convenience                                  **/
/******************************************************************************/

void
listput_shallow(GEN L, GEN x)
{
  long l;
  GEN z = list_data(L);
  l = z ? lg(z): 1;
  ensure_nb(L, l);
  z = list_data(L); /* it may change ! */
  gel(z,l++) = x;
  z[0] = evaltyp(t_VEC) | evallg(l); /*must be after gel(z,index) is set*/
}


INLINE GEN
gtor(GEN x, const char* funcName, long prec)
{
    switch (typ(x)) {
        case t_REAL:
            return x;  // x, not a copy of x
        case t_INT:
        case t_FRAC:
            return cxcompotor(x, prec);
        default:
            pari_err_TYPE(funcName, x);
    }
    __builtin_unreachable();
    return NEVER_USED;
}


void
gToC(GEN n)
{
    pari_printf("%s\n", toC(n));
}


/* Given a PARI object, return a string containing a C representation.
 * The C representation should work regardless of whether LONG_IS_64BIT
 * is set or not.
 * Works on t_INT and t_FRAC.
 * Tries to give a simple form when possible. This includes the
 * universal objects gen_0, gen_1, gen_m1, gen_2, gen_m2, and ghalf.
 * Note: The functions mkoo(), mkmoo(), and gen_I(), though related,
 * are not produced by this function.
*/
const char*
toC(GEN n)
{
  long t = typ(n);

  /* Handle types other than t_INT */
  if (t == t_FRAC) {
    GEN num = gel(n, 1), den = gel(n, 2);
    if (cmpis(den, 2) == 0 && equali1(num)) return "ghalf";
    if (cmpis(den, 0x7FFFFFFF) <= 1 && (signe(num) > 0 ? cmpis(num, 0x7FFFFFFF) <= 1 : cmpis(num, -0x80000000) >= 1)) {
      return pari_sprintf("mkfracss(%ld, %ld)", itos(num), itos(den));
    }
    if (equali1(num)) return pari_sprintf("ginv(%s)", toC(den));
    return pari_sprintf("Qdivii(%s, %s)", toC(num), toC(den));
  }
  if (t != t_INT) {
    pari_err_TYPE("toC", n);
    __builtin_unreachable();
  }

  /* Handle special values */
  if (abscmpiu(n, 3) < 0) {
    if (cmpis(n, 2) == 0)
      return "gen_2";
    else if (equali1(n))
      return "gen_1";
    else if (signe(n) == 0)
      return "gen_0";
    else if (equalim1(n))
      return "gen_m1";
    else if (equalis(n, -2))
      return "gen_m2";
    __builtin_unreachable();
  }
  
  if (ispow2(n)) return pari_sprintf("int2u(%ld)", expi(n));
    
  // words: number of 32-bit words in n.
#ifdef LONG_IS_64BIT
  long words = (lgefint(n) - 2) << 1;
  if ((ulong)*int_MSW(n) <= 0xFFFFFFFF)
    words--;
#else
  long words = lgefint(n) - 2;
#endif

  /* 32 bits */
  if (words == 1) {
    if (signe(n) > 0)
      return pari_sprintf("utoipos(%Ps)", n);
    else
      return pari_sprintf("utoineg(%Ps)", absi(n));
  }

  /* 64 bits */
  if (words == 2) {
    if (signe(n) > 0)
      return pari_sprintf("uu32toi(%Ps, %Ps)", shifti(n, -32), remi2n(n, 32));
    n = absi(n);
    return pari_sprintf("uu32toineg(%Ps, %Ps)", shifti(n, -32), remi2n(n, 32));
  }

  /* Handle negatives */
  if (signe(n) < 0) return pari_sprintf("negi(%s)", toC(absi(n)));
    
  // Large numbers
  // N.B., this requires sprintf rather than pari_sprintf.
  size_t maxSize = 9 + countdigits(stoi(words)) + 12*words;
  char* buffer = (char*)pari_malloc(maxSize*sizeof(char));
  int index = sprintf(buffer, "mkintn(%ld", words);  // 7+len(words) characters
  long i = words - 1;
  pari_sp btop = avma, st_lim = stack_lim(btop, 1);
  for (; i >= 0; i--) {
#ifdef LONG_IS_64BIT
    ulong chunk = mod2BIL(n) & 0xffffffff;
#else
    ulong chunk = mod2BIL(n);
#endif
    index += sprintf(buffer+index, ", %lu", chunk);  // 12 characters max
    n = shifti(n, -32); // a faster approach would use int_nextW
    if (low_stack(st_lim, stack_lim(btop, 1)))
      gerepileall(btop, 1, &n);
  }
  sprintf(buffer+index, ")");  // 2 characters with \0
  return buffer;
}


long
countdigits(GEN x)
{
    pari_sp av = avma;
    long s = sizedigit(x) - 1;
    if (gcmp(x, powis(utoipos(10), s)) >= 0)
    s++;
    avma = av;
    return s;
}

GEN
eps(long prec)
{
    GEN ret = real_1(DEFAULTPREC);
    setexpo(ret, 1 - bit_accuracy(prec));
    return ret;
}

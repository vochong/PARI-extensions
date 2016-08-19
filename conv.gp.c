/******************************************************************************/
/**														 Convenience																	**/
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
			return x;	// x, not a copy of x
		case t_INT:
		case t_FRAC:
			return cxcompotor(x, prec);
		default:
			pari_err_TYPE(funcName, x);
	}
	__builtin_unreachable();
	return NEVER_USED;
}


// TODO: Binary splitting for smaller subproducts.
GEN
vecprod(GEN v)
{
	pari_sp ltop = avma;
	GEN p2 = gen_1;
	if (!is_matvec_t(typ(v)))
		pari_err_TYPE("vecprod", v);
	long l1 = lg(v);

	pari_sp btop = avma;
	long i;
	for (i = 1; i < l1; ++i)
	{
		p2 = gmul(p2, gel(v, i));
		p2 = gerepileupto(btop, p2);
	}
	p2 = gerepileupto(ltop, p2);
	return p2;
}


void
gToC(GEN n)
{
	pari_printf("%s\n", toC(n));
}


const char*
toC(GEN n)
{
	if (typ(n) != t_INT)
	{
		pari_err_TYPE("toC", n);
		__builtin_unreachable();
	}

	if (abscmpiu(n, 3) < 0)
	{
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

	if (words == 1)
	{
		if (signe(n) > 0)
			return pari_sprintf("utoipos(%Ps)", n);
		else
			return pari_sprintf("utoineg(%Ps)", absi(n));
	}
	if (words == 2)
	{
		if (signe(n) > 0) {
			return pari_sprintf("uu32toi(%Ps, %Ps)", shifti(n, -32), remi2n(n, 32));
		} else {
			n = absi(n);
			return pari_sprintf("uutoineg(%Ps, %Ps)", shifti(n, -32), remi2n(n, 32));
		}
	}

	// Handle negatives
	if (signe(n) < 0) {
		// This format is ugly, is there a good way to improve this?
		return pari_sprintf("variable_name=%s;\nsetsigne(variable_name, -1);", toC(absi(n)));
	}
	
	// Large numbers
	// N.B., this requires sprintf rather than pari_sprintf.
	size_t maxSize = 9 + countdigits(stoi(words)) + 12*words;
	char* buffer;
	buffer = (char*)pari_malloc(maxSize*sizeof(char));
	int index = sprintf(buffer, "mkintn(%ld", words);	// 7+len(words) characters
	long i = words - 1;
	pari_sp btop = avma, st_lim = stack_lim(btop, 1);
	for (; i >= 0; i--)
	{
#ifdef LONG_IS_64BIT
		ulong chunk = mod2BIL(n) & 0xffffffff;
#else
		ulong chunk = mod2BIL(n);
#endif
		index += sprintf(buffer+index, ", %lu", chunk);	// 12 characters max
		n = shifti(n, -32); // a faster approach would use int_nextW
		if (low_stack(st_lim, stack_lim(btop, 1)))
			gerepileall(btop, 1, &n);

	}
	index += sprintf(buffer+index, ")");	// 2 characters with \0
	
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

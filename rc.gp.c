/******************************************************************************/
/**													Real and complex functions											**/
/******************************************************************************/


// This uses a fairly naive method. See
// H. Cohen, High Precision Computation of Hardy-Littlewood Constants
// for a beter method.
GEN
primezeta(GEN s, long prec)
{
	switch (typ(s)) {
	case t_COMPLEX:
		return primezeta_complex(s);
	case t_REAL:
		break;
	case t_FRAC:
		s = fractor(s, prec);
		break;
	case t_INT:
		s = itor(s, prec);
		break;
	default:
		pari_err_TYPE("primezeta", s);
	}
	
	pari_sp ltop = avma;
	GEN ret = cmprs(s, 1) > 1 ? primezeta_real(s) : primezeta_complex(s);
	ret = gerepileupto(ltop, ret);
	return ret;
}


static GEN
primezeta_complex_helper(void * _cargs, GEN k)
{
	long mu = moebius(k);
	if (!mu)
		return gen_0;
	pari_sp ltop = avma;
	GEN _args = (GEN) _cargs;
	GEN s = gel(_args,1);
	long prec = gtos(gel(_args,2));
	GEN ret = gzeta(gmul(k, s), prec);
	ret = gdiv(glog(gabs(ret, prec), prec), k);
	ret = gerepileupto(ltop, ret);
	
	if (mu < 0)
		togglesign(ret);
	return ret;
}


GEN
primezeta_complex(GEN s)
{
	long prec = precision(s);
	return suminf(mkvec2(s, stoi(prec)), primezeta_complex_helper, gen_1, prec);
}


GEN
primezeta_real(GEN s)
{
	long prec = precision(s);
	pari_sp ltop = avma, st_lim = stack_lim(ltop, 2);
	
	// Determine the number of iterations needed
	// The precision here is probably overkill but the extra time is irrelevant.
	GEN t = shiftr(mulrr(rtor(s, DEFAULTPREC), dbltor(M_LN2)), 1 - precision(s));
	GEN iter = gdivent(mplambertW(shiftr(t, 1 - prec)), t);
	ulong k, mx = itou(iter);
	
	GEN accum = real_0(prec);
	for (k = 1; k <= mx; k++) {
		long mu = moebiusu(k);
		if (mu) {
			accum = addrr(accum, divrs(mplog(absr(gzeta(mulrs(s, k), prec))), k*mu));
			if (low_stack(st_lim, stack_lim(ltop, 2)))
				accum = gerepileupto(ltop, accum);
		}
	}
	return accum;
}


// Checked up to 10,000 with the 64-bit version.
GEN
Bell(long n)
{
	static const long BellMod32[] = {
		1,1,2,5,15,20,11,13,12,27,7,10,29,21,26,17,3,12,15,1,12,15,27,26,9,25,18,13,
		7,4,3,5,12,19,31,10,5,13,10,25,27,28,7,25,12,7,19,26,17,17,2,21,31,20,27,29,
		12,11,23,10,13,5,26,1,19,12,31,17,12,31,11,26,25,9,18,29,23,4,19,21,12,3,15,
		10,21,29,10,9,11,28,23,9,12,23,3,26
	};
	if (n < 2) {
		if (n < 0)
			pari_err_DOMAIN("Bell", "n", "<", gen_0, stoi(n));
		return gen_1;
	}
	pari_sp ltop = avma;
	GEN B, Br, f, t;
	
	double logn = log(n);
	double w = W_small(n);
	long sz = n * (logn - log(w) - 1 + 1/w) + logn;	// - log(w)/2 - 1;
	sz /= BITS_IN_LONG * log(2);
	sz += 5;	// Guard digits

	// This while re-does the calculation if the precision was too low.
	while (1) {
		long k = 1;
		f = real_1(sz);
		B = real_0(sz);
		pari_sp btop = avma, st_lim = stack_lim(btop, 1);
		
		// This is the hot loop where all calculations are done.
		while (1) {
			f = divrs(f, k);
			t = mulir(powuu(k, n), f);
			if (expo(t) < -25)
				break;
			B = addrr(B, t);
			++k;
			if (low_stack(st_lim, stack_lim(btop, 1)))
				gerepileall(btop, 2, &f, &B);
		}
		B = mulrr(B, gexp(gen_m1, sz));
		if (nbits2prec(expo(B)+1) > lg(B))
			goto FIX_PRECISION;
		
		Br = ceilr(B); // calculate answer
		
		// Check if answer is within a millionth of an integer
		if (cmprr(absr(mpsub(B, Br)), real2n(-20, DEFAULTPREC)) < 0) {
			Br = gerepilecopy(ltop, Br);
			
			// Sanity check on the floating-point arithmetic
			if (mod32(Br) != BellMod32[n % 96])
				pari_err(e_BUG, "Bell(%d) failed verification mod 32, please report", n);
			if (DEBUGLEVEL > 4) {
				pari_printf("Precision for Bell(%d): %d (%d digits), an excess of %d (%d digits)\n", n, sz - 2, prec2ndec(sz), sz - expi(Br) / BITS_IN_LONG - 3, prec2ndec(sz - expi(Br) / BITS_IN_LONG - 1));
			}
			return Br;
		}
		
FIX_PRECISION:
		sz += logf(sz);	// increase precion slightly
		pari_warn(warnprec, "Bell", sz);
		avma = ltop;
	}
}


double
lnBell(long n)
{
	if (n < 2) {
		if (n < 0)
			pari_err_DOMAIN("lnBell", "n", "<", gen_0, stoi(n));
		return 0.0;
	}
	double Blog = 0.0, flog = 0.0, tlog;
	
	long k = 2;
	while (1) {
		double logk = log((double)k);
		flog -= logk;
		tlog = n * logk + flog;
		if (tlog < -17)
			break;
		double tmp = tlog - Blog;
		if (tmp >= log(DBL_MAX))	// TODO: Should this be cut closer?
			Blog += tmp;
		else
			Blog += log1p(exp(tmp));
		++k;
	}
	return Blog - 1.0;
}


GEN
glnBell(long n)
{
	return dbltor(lnBell(n));
}


GEN
deBruijnXi(GEN x)
{
	double xx = rtodbl(x), left, right;
	if (xx < 1)
		pari_err_DOMAIN("deBruijnXi", "x", "<", gen_1, x);
	if (xx > 1)
		left = log(xx);
	else
		left = DBL_EPSILON;
	right = 1.35 * log(xx) + 1;	// Heuristic

	// Bisection
	while (right - left > left * DBL_EPSILON) {
		double m = (left + right) / 2;
		if (expm1(m) > xx * m)
			right = m;
		else
			left = m;
	}
	return dbltor((left + right) / 2);
}

GEN
rhoest(GEN x, long prec)
{
	pari_sp ltop = avma;
	GEN xi, ret, e;
	x = gtor(x, "rhoest", prec);
	xi = deBruijnXi(x);
	e = eint1(negr(xi), prec);
	if (typ(e) == t_COMPLEX)
		e = gel(e, 1);
	setsigne(e, -signe(e));
	ret = mpexp(subrr(e, mulrr(x, xi)));
	ret = divrr(divrr(ret, sqrtr(mulrr(mulsr(2, mppi(prec)), x))), xi);
	ret = gerepileupto(ltop, ret);
	return ret;
}


GEN
DickmanRho(GEN x, long prec)
{
	static const double rhoTable[] = {
		NEVER_USED, 1, 3.068528194e-1, 4.860838829e-2, 4.910925648e-3,
		3.547247005e-4, 1.964969635e-5, 8.745669953e-7, 3.232069304e-8,
		1.016248283e-9,	2.770171838e-11, 6.644809070e-13, 1.419713165e-14,
		2.729189030e-16, 4.760639989e-18, 7.589908004e-20
	};
	static const int rhoTableLen = 15;
	static const double rhoScale = 1.130709295873035782;
	// Last table entry, divided by rhoest at that point

	pari_sp ltop = avma;
	GEN ret, left, right, scale;
	x = gtor(x, "DickmanRho", prec);
	if (cmprs(x, 2) <= 0) {
		ret = gsubsg(1, glog(gmaxgs(x, 1), prec));
		ret = gerepileupto(ltop, ret);
		return ret;
	}
	if (gcmpgs(x, 3) <= 0) {
		ret = gadd(gadd(gsubsg(1, mulrr(subsr(1, mplog(subrs(x, 1))), mplog(x))), greal(dilog(subsr(1, x), prec))), divrs(sqrr(mppi(prec)), 12));
		ret = gerepileupto(ltop, ret);
		return ret;
	}
  
	double xx = rtodbl(x);
  
	// Asymptotic estimate (scaled for continuity)
	if (xx > rhoTableLen) {
		double sc = rhoScale;
		sc = (sc - 1) * sqrt(sqrt(rhoTableLen / xx)) + 1;
		/* Let the scale factor dwindle away, since the estimate is (presumably) */
		/* better in the long run than any scaled version of it.  The exponent */
		/* of 0.25 has been chosen to give the best results for 10 < x < 100 */
		/* with a table size of 10. */

		ret = precision0(mulrr(rhoest(x, prec), dbltor(sc)), 9);
		ret = gerepileupto(ltop, ret);
		return ret;
	}
  
	// Scaling factors: the factor by which the true value of rho differs from
	// the estimates at the endpoints.
	left = divrr(dbltor(rhoTable[(int)floor(xx)]), rhoest(floorr(x), prec));
	right = divrr(dbltor(rhoTable[(int)ceil(xx)]), rhoest(ceilr(x), prec));
	
	// Linear interpolation on the scale factors.
	scale = gadd(left, gmul(gsub(right, left), mpsub(x, floorr(x))));
	
	// Return a result based on the scale factor and the asymptotic formula.
	ret = precision0(gmul(rhoest(x, prec), scale), 9);
	ret = gerepileupto(ltop, ret);
	return ret;
}


// Convenience function: binary logarithm of x
GEN
log_2(GEN x, long prec)
{
	pari_sp ltop = avma;
	GEN ret;
	switch(typ(x)) {
		case t_INT:
			ret = mplog(itor(x, prec));
			break;
		case t_REAL:
			ret = mplog(x);
			break;
		case t_FRAC:
		case t_COMPLEX:
			ret = mplog(cxcompotor(x, prec));
			break;
		default:
			pari_err_TYPE("lg", x);
			__builtin_unreachable();
	}
	ret = divrr(ret, mplog2(prec));
	ret = gerepileupto(ltop, ret);
	return ret;
}


GEN
contfracback(GEN v, GEN terms)
{
	pari_sp ltop = avma;
	GEN x = gen_0;
	long tterms;
	if (!terms)
		tterms = glength(v) - 1;
	else if (typ(terms) == t_INT)
		tterms = itos(terms);
	else {
		pari_err_TYPE("contfracback", terms);
		__builtin_unreachable();
	}
	x = gel(v, tterms + 1);
	if (tterms == 1)
		x = gcopy(x);
	pari_sp btop = avma, st_lim = stack_lim(btop, 1);
	long i = 0;
	for (i = tterms; i >= 1; i--)
	{
		x = gadd(gel(v, i), ginv(x));
		if (low_stack(st_lim, stack_lim(btop, 1)))
			gerepileall(btop, 1, &x);
	}
	x = gerepileupto(ltop, x);
	return x;
}


double
W_small(double x)
{
	double e, w, t = 1.0;
	
	// Initial approximation for iteration
	if (x < 1)
	{
		double tmp = sqrt(2*M_E*(x + 1/M_E));
		w = (1 + (11/72*tmp - 1/3)*tmp)*tmp - 1;
		if (x < (-1/M_E + DBL_EPSILON * 1000000)) {
			if (x < -1/M_E)	// W(x) undefined for x < -1/e
				pari_err_DOMAIN("W_small", "x", "<", gneg(gexp(gen_m1, DEFAULTPREC)), dbltor(x));
			return w;
		}
	} else {
		w = log(x);
	}
	if (x > 3)
		w -= log(w);

	double ep = DBL_EPSILON * (1 + fabs(w));
	while (fabs(t) > ep)
	{
		// Halley loop
		e = exp(w);
		t = w*e - x;
		t /= e*(w+1) - 0.5*(w+2)*t/(w+1);
		w -= t;
	}
	return w;
}


/******************************************************************************/
/**															 Statistics																 **/
/******************************************************************************/

long
infinite(GEN x)
{
	if (typ(x) != t_VEC || glength(x) != 1)
		return 0;
	GEN e = gel(x, 1);	// Nothing is created, so no garbage... right?
	
	// If e is gen_0, then is_pm1 is unpredicatable, but that doesn't matter
	// because then both branches return 0.
	return (typ(e) == t_INT && is_pm1(e)) ? signe(e) : 0;
}


long
isExtendedReal(GEN x)
{
	long t = typ(x);
	if (t == t_INT || t == t_FRAC || t == t_REAL)
		return 1;
	return infinite(x);
}


// FIXME: Infinities are broken?
GEN
normd(GEN a, GEN b, long prec)
{
	// Error type follows that of intnum in language/intnum.c
	if (!isExtendedReal(a) || !isExtendedReal(b))
		pari_err(e_MISC, "incorrect endpoint in normd");
	pari_sp ltop = avma;
	long tmp;
	GEN ret;
	
	/* Infinities */
	if ((tmp = infinite(a)))	// Assignment and test-if-0
	{
pari_warn(warner, "Doesn't work properly with infinities");
		if (tmp < 0)	// (-oo, b)
		{
			tmp = infinite(b);
			if (tmp > 0)
				ret = gen_1;
			else if (tmp < 0)
				ret = gen_0;
			else
				ret = gdivgs(mpneg(gerfc(mpdiv(b, gsqrt(gen_2, prec)), prec)), 2);
		} else {		// (oo, b)
			if (infinite(b) == 1)
				ret = gen_1;
			else
				pari_err(e_MISC, "incorrect endpoint in normd");
				// Error type follows that of intnum in language/intnum.c
				__builtin_unreachable();
		}
	} else if ((tmp = infinite(b))) {	// Assignment and test-if-0
pari_warn(warner, "Doesn't work properly with infinities");
		if (tmp < 0)	// (a, -oo)
			pari_err(e_MISC, "incorrect endpoint in normd");
			// Error type follows that of intnum in language/intnum.c
		ret = gdivgs(gerfc(mpdiv(a, gsqrt(gen_2, prec)), prec), 2);
	} else {
		GEN root2 = gsqrt(gen_2, prec);
		ret = gdivgs(gsub(gerfc(gdiv(a, root2), prec), gerfc(gdiv(b, root2), prec)), 2);
	}
	ret = gerepileupto(ltop, ret);
	return ret;
}


// Use the Box-Muller transform to generate random normal variables. Caches
// values, so multiple calls at the same precision are fast.
GEN
rnormal(long prec)
{
	if (rnormal_cached) {
		long cached = precision(rnormal_cached);
		if (prec == cached) {
			GEN ret = gcopy(rnormal_cached);
			gunclone(rnormal_cached);
			rnormal_cached = 0;
			return ret;
		} else if (cached > prec) {
			GEN ret = gcopy(rnormal_cached);
			fixlg(ret, prec);
			gunclone(rnormal_cached);
			rnormal_cached = 0;
			return ret;
		} else {
			gunclone(rnormal_cached);
		}
	}
	
	pari_sp ltop = avma;
	GEN u1, u2, ret, left, rightS, rightC;
	u1 = randomr(prec);
	u2 = randomr(prec);
	left = sqrtr_abs(shiftr(mplog(u1), 1));
	mpsincos(mulrr(shiftr(mppi(prec), 1), u2), &rightS, &rightC);
	
	rnormal_cached = gclone(mulrr(left, rightS));	// Cache for later use
	ret = mulrr(left, rightC);
	ret = gerepileupto(ltop, ret);
	return ret;
}

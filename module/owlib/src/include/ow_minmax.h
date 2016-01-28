/**
 * Defines inline versions of min/max.
 *
 * Please use instead of MIN/MAX macros:
 * https://dustri.org/b/min-and-max-macro-considered-harmful.html
 */
#ifndef OW_MINMAX
#define OW_MINMAX

inline int min(int a, int b) {
	if (a > b)
		return b;
	return a;
}

inline int max(int a, int b) {
	if (a > b)
		return a;
	return b;
}

#endif

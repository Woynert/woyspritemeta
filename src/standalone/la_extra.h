#ifndef LA_EXTRA
#define LA_EXTRA

#include "la.h"

V2f v2f_translate_scale(V2i point, V2i translate, float scale) {
	return v2f_add(
		v2f_mul(
			v2i_2f(point),
			v2ff(scale)
		),
		v2i_2f(translate)
	);
}

V2i v2i_translate_scale(V2i point, V2i translate, float scale) {
	return v2i_add(
		v2f_2i(v2f_mul(
			v2i_2f(point),
			v2ff(scale)
		)),
		translate
	);
}


#endif // !LA_EXTRA

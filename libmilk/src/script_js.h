#if MOZJS_MAJOR_VERSION == 38
	#include "script_js38.h"
#elif defined JS17_FOUND
	#include "script_js17.h"
#endif

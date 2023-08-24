#pragma once

#ifdef USING_BML_PLUS
# include <BMLPlus/BMLAll.h>
typedef const char* iCKSTRING;
#else
# include <BML/BMLAll.h>
typedef CKSTRING iCKSTRING;
#endif

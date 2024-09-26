#pragma once

#ifdef USING_BML_PLUS
# include <BMLPlus/BMLAll.h>
# define m_bml m_BML
# define m_sprite m_Sprite
# define VT21_REF(x) &(x)
typedef const char* iCKSTRING;
#else
# include <BML/BMLAll.h>
# define VT21_REF(x) (x)
typedef CKSTRING iCKSTRING;
#endif

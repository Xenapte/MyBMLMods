#pragma once

#ifdef USING_BML_PLUS
# include <BMLPlus/BMLAll.h>
# ifndef m_bml
#  define m_bml m_BML
#  define m_sprite m_Sprite
#  define VT21_REF(x) &(x)
# endif
typedef const char* iCKSTRING;
#else
# include <BML/BMLAll.h>
# define VT21_REF(x) (x)
typedef CKSTRING iCKSTRING;
#endif
#if defined(USING_BML_PLUS) && BML_MINOR_VERSION >= 3
# define BML_BUILD_VAR_NAME(x) (x).patch
#else
# define BML_BUILD_VAR_NAME(x) (x).build
#endif
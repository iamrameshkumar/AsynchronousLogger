#pragma once

# include "tstring.h"

#if !(defined(__PRETTY_FUNCTION__))
#define __PRETTY_FUNCTION__   __FUNCTIONW__
#endif

#define MakeUnicodeStr(LITERAL)			_T(#LITERAL) 
#define MakeStr(LITERAL)				#LITERAL
#define MakeUnicode(LITERAL)			_T(LITERAL) 
#define MakeString(METHOD, LITERAL)		METHOD(LITERAL) 
#define $Line 							MakeString( MakeUnicodeStr, __LINE__ ) 
#define $File							MakeString( MakeUnicodeStr, __FILE__ ) 
#define $Function						MakeString( MakeUnicodeStr, __PRETTY_FUNCTION__ ) 

tstring splitFileName(const tstring& str);
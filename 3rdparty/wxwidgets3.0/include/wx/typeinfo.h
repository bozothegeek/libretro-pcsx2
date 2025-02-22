///////////////////////////////////////////////////////////////////////////////
// Name:        wx/typeinfo.h
// Purpose:     wxTypeId implementation
// Author:      Jaakko Salli
// Created:     2009-11-19
// Copyright:   (c) wxWidgets Team
// Licence:     wxWindows licence
///////////////////////////////////////////////////////////////////////////////

#ifndef _WX_TYPEINFO_H_
#define _WX_TYPEINFO_H_

//
// This file defines wxTypeId macro that should be used internally in
// wxWidgets instead of typeid(), for compatibility with builds that do
// not implement C++ RTTI. Also, type defining macros in this file are
// intended for internal use only at this time and may change in future
// versions.
//
// The reason why we need this simple RTTI system in addition to the older
// wxObject-based one is that the latter does not work in template
// classes.
//

#include "wx/defs.h"

#define wxTRUST_CPP_RTTI    0

//
// When C++ RTTI is not available, we will have to make the type comparison
// using pointer to a dummy static member function. This will fail if
// declared type is used across DLL boundaries, although using
// WX_DECLARE_TYPEINFO() and WX_DEFINE_TYPEINFO() pair instead of
// WX_DECLARE_TYPEINFO_INLINE() should fix this. However, that approach is
// usually not possible when type info needs to be declared for a template
// class.
//

typedef void (*wxTypeIdentifier)();

// Use this macro to declare type info with specified static function
// IDENTFUNC used as type identifier. Usually you should only use
// WX_DECLARE_TYPEINFO() or WX_DECLARE_TYPEINFO_INLINE() however.
#define _WX_DECLARE_TYPEINFO_CUSTOM(CLS, IDENTFUNC) \
public: \
    virtual wxTypeIdentifier GetWxTypeId() const \
    { \
        return reinterpret_cast<wxTypeIdentifier> \
            (&IDENTFUNC); \
    }

// Use this macro to declare type info with externally specified
// type identifier, defined with WX_DEFINE_TYPEINFO().
#define WX_DECLARE_TYPEINFO(CLS) \
private: \
    static CLS sm_wxClassInfo(); \
_WX_DECLARE_TYPEINFO_CUSTOM(CLS, sm_wxClassInfo)

// Use this macro to implement type identifier function required by
// WX_DECLARE_TYPEINFO().
// NOTE: CLS is required to have default ctor. If it doesn't
//       already, you should provide a private dummy one.
#define WX_DEFINE_TYPEINFO(CLS) \
CLS CLS::sm_wxClassInfo() { return CLS(); }

// Use this macro to declare type info fully inline in class.
// NOTE: CLS is required to have default ctor. If it doesn't
//       already, you should provide a private dummy one.
#define WX_DECLARE_TYPEINFO_INLINE(CLS) \
private: \
    static CLS sm_wxClassInfo() { return CLS(); } \
_WX_DECLARE_TYPEINFO_CUSTOM(CLS, sm_wxClassInfo)

#define wxTypeId(OBJ) (OBJ).GetWxTypeId()

// Because abstract classes cannot be instantiated, we use
// this macro to define pure virtual type interface for them.
#define WX_DECLARE_ABSTRACT_TYPEINFO(CLS) \
public: \
    virtual wxTypeIdentifier GetWxTypeId() const = 0;

#endif // _WX_TYPEINFO_H_

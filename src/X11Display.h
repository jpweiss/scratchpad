// -*- C++ -*-
// Header file for class X11Display
//
// Copyright (C) 2010-2011 by John Weiss
// This program is free software; you can redistribute it and/or modify
// it under the terms of the Artistic License, included as the file
// "LICENSE" in the source code archive.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//
// You should have received a copy of the file "LICENSE", containing
// the License John Weiss originally placed this program under.
//
// $Id$
//
#ifndef _X11Display_H_
#define _X11Display_H_

// Includes
//
#include <stdexcept>
#include <string>

#include <X11/Xlib.h>


// Enclosing namespace
//
namespace jpwTools {
 // Using decls.
 //
 using std::string;


 // Class X11Display
 /**
  * Simple wrapper class for automatically closing the X11 Display in the
  * event of an error or exception.
  */
 class X11Display
 {
     Display* m__x11DisplayPtr;
 public:
     struct FailToOpen : public std::runtime_error
     {
         explicit FailToOpen(const string& how)
             : std::runtime_error(how)
         {}
     };


     // C'tor
     explicit X11Display(const string& displayName)
         : m__x11DisplayPtr(XOpenDisplay(displayName.c_str()))
     {
         if(!m__x11DisplayPtr) {
             string errmsg("Invalid or unknown $DISPLAY==\"");
             errmsg += displayName;
             errmsg += "\"\nCannot continue.";
             throw FailToOpen(errmsg);
         }
     }

     // D'tor
     ~X11Display()
     {
         if(m__x11DisplayPtr) {
             XCloseDisplay(m__x11DisplayPtr);
         }
     }

     // Allow access via implicit cast.
     operator Display*() { return m__x11DisplayPtr; }
 };

}; //end namespace


#endif //_X11Display_H_
/////////////////////////
//
// End

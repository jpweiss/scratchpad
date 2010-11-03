// -*- C++ -*-
// Header file for class LinuxInputDevice
//
// Copyright (C) 2010 by John Weiss
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
#ifndef _LinuxInputDevice_H_
#define _LinuxInputDevice_H_

// Includes
//
#include <string>

#include <unistd.h> // For Unix/POSIX 'read()' fn.


// Enclosing namespace
//
namespace jpwTools {
 // Using decls.
 //
 using std::string;


 // Class LinuxInputDevice
 /**
  * <Document it here>
  */
 struct LinuxInputDevice
 {
     explicit LinuxInputDevice(const string& filename)
         : m__fd(-1)
     {
         if(filename == "auto") {
             m__fd = scanForDevice();
         }

         // Fallback in case scanning failed to turn up anything.
         if(m__fd < 0) {
             m__fd = openUnixReadFd(filename);
         }
     }

     template<typename T>
     ssize_t reinterpret_read(T* objPtr)
     {
         return read(m__fd, reinterpret_cast<void*>(objPtr), sizeof(T));
     }

 private:
     int m__fd;

     /// Calls the Unix/POSIX \c open() function for reading, throwing a
     /// std::ios_base::failure on error.
     static int openUnixReadFd(const string& filename);

     /// Scans the \c "/dev/input/event*" files for the desired device.
     static int scanForDevice();
 };

}; //end namespace


#endif //_LinuxInputDevice_H_
/////////////////////////
//
// End

// -*- C++ -*-
// Header file for class LinuxInputDevice
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
#ifndef _LinuxInputDevice_H_
#define _LinuxInputDevice_H_

// Includes
//
#include <string>
#include <vector>

#include <stdint.h>
#include <unistd.h> // For Unix/POSIX 'read()' fn.


// Enclosing namespace
//
namespace jpwTools {
 // Using decls.
 //
 using std::string;


 // Class LinuxInputDevice
 /**
  * A class for accessing a Linux input device.
  *
  * At its most basic, this class wraps an integer Unix/POSIX file descriptor,
  * opened for reading on one of the Linux input devices.
  *
  * In addition, this class has a function for scanning the \c
  * "/dev/input/event*" devices for a matching device.
  */
 struct LinuxInputDevice
 {
     typedef std::vector<uint32_t> cap_flag_vec_t;
     static const cap_flag_vec_t m__NULL_VEC;

     /// Constructor
     /**
      * Opens the device whose path is \a filename.
      *
      * \param filename
      * In addition to the full pathname to a device, this can also be the
      * string, \c "auto", which triggers autoscanning if both \a targVendor
      * and \a targProduct are also set.
      * \param targVendor
      * The vendor ID number of the device to search for.
      * \param targProduct
      * The product ID number of the device to search for.
      * \param fallback_byName
      * In the event that a device doesn't match by vendor & product ID, you
      * can specify a string to match against the device's display name.
      * \param requiredCapabilities
      * An optional vector of capability flags.  If specified, then any device
      * found by autoscanning must have these capabilities.
      * \param forbiddenCapabilities
      * An optional vector of capability flags.  If specified, then any device
      * found by autoscanning cannot have these capabilities.
      */
     LinuxInputDevice(const string& filename,
                      uint16_t targVendor = 0,
                      uint16_t targProduct = 0,
                      const string& fallback_byName = "",
                      const cap_flag_vec_t& requiredCapabilities=m__NULL_VEC,
                      const cap_flag_vec_t&
                      forbiddenCapabilities=m__NULL_VEC);

     /// Closes the file description if it was ever opened.
     ~LinuxInputDevice();

     /// Read an object.
     /**
      * Reads a block of data the size of the object directly into the
      * object's memory area.  This requires \c T to be a POD type (including
      * POD-structs).
      *
      * When reading into a POD-struct, be mindful of alignment issues.  You
      * will likely get garbage-results if any members of \c objPtr are padded
      * by the compiler.
      */
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
     static int scanForDevice(uint16_t targVendor,
                              uint16_t targProduct,
                              const string& fallback_byName,
                              const cap_flag_vec_t& requiredCapabilities,
                              const cap_flag_vec_t& forbiddenCapabilities);
 };

}; //end namespace


#endif //_LinuxInputDevice_H_
/////////////////////////
//
// End

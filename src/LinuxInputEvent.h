// -*- C++ -*-
// Header file for class LinuxInputEvent
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
#ifndef _LinuxInputEvent_H_
#define _LinuxInputEvent_H_

// Includes
//
#include <stdint.h>
#include "LinuxInputDevice.h"


// Enclosing namespace
//
namespace jpwTools {
 // Class LinuxInputEvent
 /**
  * A POD-struct for manipulating linux input events.
  *
  * Based, in part, on information from the Linux Kernel Documentation and
  * "linux/input.h".
  */
 struct LinuxInputEvent
 {
     // Notice that the members of this struct require no padding.  All
     // adjacent members smaller than a word fit within a single word.

     /// The time at which the input event occurred.
     timeval evTime;  // sizeof(timeval) == 2 x wordsize.
     /// The type of input event.  Use the \c event_type enum to convert to
     /// something meaningful.
     uint16_t evType;
     /// What event happened.  Its meaning depends on the value of \c evType.
     uint16_t evCode;
     /// The event "value".  Its meaning depends on the value of \c evType.
     uint32_t evValue;

     /// Enum for the evType field.  See "linux/input.h".
     enum event_type {
         SYNCHRONIZE=0x00,
         KEY=0x01,
         RELATIVE_MOTION=0x02,
         ABSOLUTE_MOTION=0x03,
         MISC=0x04,
         SWITCH=0x05,
         LED=0x11,
         SOUND=0x12,
         AUTOREPEAT=0x14,
         FORCE_FEEDBACK=0x15,
         POWER=0x16,
         FORCE_FEEDBACK_STATUS=0x17,
         MAX=0x1f
     };

     /// Compare the \c evType member to a specified \c event_type.
     bool operator==(event_type t) const
     { return (static_cast<event_type>(evType) == t); }

     /// Compare the \c evType member to a specified \c event_type.
     bool operator!=(event_type t) const
     { return !this->operator==(t); }

     /// Read an event from the specified device.
     bool read(LinuxInputDevice& ufd)
     {
         // Recall:  LinuxInputEvent is a POD-struct, whose members have no
         // padding between them.  "Consecutive" member fields all completely
         // fill a machine word, or occupy an integral number of machine words.
         //
         // Reading raw data like this will require one or more
         // reinterpret_cast<>()'s someplace.  That's unavoidable.
         ssize_t nRead = ufd.reinterpret_read(this);
         return (nRead > 0);
     }
 };

 /// Commuted \c LinuxInputEvent::operator==().
 bool operator==(LinuxInputEvent::event_type t, const LinuxInputEvent& kbd)
 { return kbd.operator==(t); }

 /// Commuted \c LinuxInputEvent::operator!=().
 bool operator!=(LinuxInputEvent::event_type t, const LinuxInputEvent& kbd)
 { return kbd.operator!=(t); }


}; //end namespace


#endif //_LinuxInputEvent_H_
/////////////////////////
//
// End

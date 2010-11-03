// -*- C++ -*-
// Implementation of class LinuxInputDevice
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
static const char* const
LinuxInputDevice_cc__="$Id: class.cc 2037 2010-10-26 22:27:48Z candide $";


// Includes
//
#include <ios>

#include <cstring>
#include <cerrno>

#include <sys/stat.h>
#include <fcntl.h>

#include "LinuxInputDevice.h"


using namespace jpwTools;


//
// Static variables
//


//
// Typedefs
//


/////////////////////////

//
// LinuxInputDevice Member Functions
//


int LinuxInputDevice::openUnixReadFd(const string& filename)
{
    int the_fd = open(filename.c_str(), O_RDONLY);
    if(the_fd == -1) {
        string errmsg("Failed to open file for reading:\n\t\"");
        errmsg += filename;
        errmsg += "\"\nReason:\n\t";
        errmsg += strerror(errno);
        throw std::ios_base::failure(errmsg);
    } // else
    return the_fd;
}


int LinuxInputDevice::scanForDevice()
{
    // TODO:  This is just a placeholder until I figure out how to
    // implement this feature.
    return -1;
}


/////////////////////////
//
// End

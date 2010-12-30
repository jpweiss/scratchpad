// -*- C++ -*-
// Implementation of class LinuxInputDevice
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
static const char* const
LinuxInputDevice_cc__="$Id: class.cc 2037 2010-10-26 22:27:48Z candide $";


// Includes
//
#include <iostream>
#include <string>
#include <vector>

#include <cstring>
#include <cstdlib>
#include <cerrno>

#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <linux/input.h>

#include "LinuxInputDevice.h"
#include "LibTrace.h"


using std::cerr;
using std::endl;
using std::vector;
using std::string;
using namespace jpwTools;


//
// Static variables
//


namespace {
 const string linuxInputDevDir("/dev/input");
 const string linuxInputDevFnameBase("event");
 const string msnek4kDevname("Microsoft Natural\u00A9 "
                             "Ergonomic Keyboard 4000");

 const unsigned MAX_EVIOGNAME_LEN(1024);
};


const LinuxInputDevice::cap_flag_vec_t LinuxInputDevice::m__NULL_VEC;


//
// Typedefs
//


typedef vector<string> stringvec_t;


/////////////////////////

//
// General Function Definitions
//


// Tests for a capability bit.  Taken from 
inline bool test_capability_bit(unsigned bit, uint32_t* array)
{
    return ( array[bit/EV_CNT] & (1 << (bit%EV_CNT)) );
}


inline bool matchedCapabilities(uint32_t* capabilities_bitmask,
                                const LinuxInputDevice::cap_flag_vec_t&
                                requiredCapabilities,
                                const LinuxInputDevice::cap_flag_vec_t&
                                forbiddenCapabilities)
{
    typedef LinuxInputDevice::cap_flag_vec_t::const_iterator cap_iter_t;

    cap_iter_t reqCap_end(requiredCapabilities.end());
    bool hasRequiredCaps(true);
    for(cap_iter_t reqCapIter=requiredCapabilities.begin();
        reqCapIter != reqCap_end; ++reqCapIter)
    {
        // All of the requiredCapabilities must be present.
        hasRequiredCaps = ( test_capability_bit(*reqCapIter,
                                                capabilities_bitmask)
                            && hasRequiredCaps );
    }

    cap_iter_t forbidCap_end(forbiddenCapabilities.end());
    bool hasAnyForbiddenCaps(false);
    for(cap_iter_t forbidCapIter=forbiddenCapabilities.begin();
        forbidCapIter != forbidCap_end; ++forbidCapIter)
    {
        // None of the forbiddenCapabilities must be present.  Check if even
        // one of them is present.
        hasAnyForbiddenCaps = ( test_capability_bit(*forbidCapIter,
                                                    capabilities_bitmask)
                                || hasAnyForbiddenCaps );
    }

    return (hasRequiredCaps && !hasAnyForbiddenCaps);
}


/////////////////////////

//
// LinuxInputDevice Member Functions
//


inline int LinuxInputDevice::openUnixReadFd(const string& filename)
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


inline int LinuxInputDevice::scanForDevice(uint16_t targVendor,
                                           uint16_t targProduct,
                                           const string& fallback_byName,
                                           const cap_flag_vec_t&
                                           requiredCapabilities,
                                           const cap_flag_vec_t&
                                           forbiddenCapabilities)
{
    stringvec_t evdevfiles;
    LibTrace lt("LinuxInputDevice::scanForDevice");

    // Scan "/dev/input/" for filenames matching "event*".
    DIR* devInput_dd = opendir(linuxInputDevDir.c_str());
    if(!devInput_dd) {
        cerr << "Error:  Failed to open path: \""
             << linuxInputDevDir
             << "\""
             << endl
             << "Reason: \"" << strerror(errno) << '\"'
             << endl
             << "Cannot perform autoscan." << endl;
        return -1;
    }

    // Scan the directory.  Make sure that we can identify an abort-on-error.
    errno = 0;
    for(dirent* dirChild_ptr = readdir(devInput_dd);
        dirChild_ptr;
        dirChild_ptr = readdir(devInput_dd))
    {
        string childName(dirChild_ptr->d_name);
        lt.Debug("    Checking directory entry: ", childName);
        if(childName.find(linuxInputDevFnameBase) == 0) {
            string childPath(linuxInputDevDir);
            childPath += '/';
            childPath += childName;
            evdevfiles.push_back(childPath);
        }
    }

    if(errno) {
        cerr << "Warning:  Error occurred while searching path: \""
             << linuxInputDevDir
             << "\" for devices."
             << endl
             << "Reason: \"" << strerror(errno) << '\"'
             << endl
             << endl
             << "Autoscan results may not be correct."
             << endl;
    }

    // Nothing matching found?  Nothing to do.
    if(evdevfiles.empty()) {
        cerr << "Error:  No event devices found in path: \""
             << linuxInputDevDir
             << "\""
             << endl
             << "Cannot perform autoscan." << endl;
        return -1;
    }

    // Iterate over the vector of event files, looking for a match.
    for(stringvec_t::const_iterator evFileIter = evdevfiles.begin();
        evFileIter != evdevfiles.end();
        ++evFileIter)
    {
        // Open the file
        int ev_fd(-1);
        try {
            ev_fd = openUnixReadFd(evFileIter->c_str());
        } catch(std::ios_base::failure& err) {
            cout << err.what() << endl;
        }

        bool foundMatch(false);
        bool inputIdIoctlFailed(false);

        // Check if the IDs match what we're looking for.
        if(ev_fd > 0) {
            lt.Debug("    Getting evdev info for: ", *evFileIter);
            input_id evdev_info;

            int ioctl_stat = ioctl(ev_fd, EVIOCGID, &evdev_info);
            if(ioctl_stat) {
                inputIdIoctlFailed = true;
                cerr << "Warning:  Error occurred while examining device: \""
                     << *evFileIter
                     << "\"."
                     << endl
                     << "Reason: \"" << strerror(errno) << '\"'
                     << endl;
            } else {
                lt.Debug("        bustype: ", evdev_info.bustype);
                cerr << std::hex;
                lt.Debug("        vendor: ", evdev_info.vendor);
                lt.Debug("        product: ", evdev_info.product);
                cerr << std::dec;
                lt.Debug("        version: ", evdev_info.version);

                foundMatch = ( (evdev_info.vendor == targVendor) &&
                               (evdev_info.product == targProduct) );
            }
        }

        // IDs didn't match.  Let's try a string match on the name as a
        // fallback.
        if( !foundMatch && (ev_fd > 0) && !fallback_byName.empty() ) {
            lt.Debug("    Getting name for: ", *evFileIter);
            char nameBuf[MAX_EVIOGNAME_LEN];

            int ioctl_stat = ioctl(ev_fd,
                                   EVIOCGNAME(MAX_EVIOGNAME_LEN),
                                   nameBuf);
            if(ioctl_stat < 0) {
                cerr << "Warning:  Error occurred while getting the name "
                     << "of the device:"
                     << endl
                     << "\""
                     << *evFileIter
                     << "\"."
                     << endl
                     << "Reason: \"" << strerror(errno) << '\"'
                     << endl
                     << "Skipping this device..."
                     << endl;
            } else {
                string evdevName(nameBuf);
                lt.Debug("        display name: ", evdevName);

                foundMatch
                    = (evdevName.find(fallback_byName) != string::npos);
            }
        } else if(inputIdIoctlFailed) {
            // If we're not performing fallback to name checking, print out
            // the remaining part of the error message for the failed EVIOCGID
            // ioctl from earlier.
            cerr << "Skipping this device..."
                 << endl;
        }

        // If we found the device we're looking for, start checking the
        // capability flags.  We want the one w/o LEDs and with
        // absolute+relative motion.
        if( foundMatch && !requiredCapabilities.empty()
            && !forbiddenCapabilities.empty() )
        {
            lt.Debug("    Device matched target.  Checking capabilities...");
            // I don't really care about errors here.  And the actual contents
            // of the bitmask for each ioctl call is irrelevant.
            uint32_t capabilities_bitmask[EV_CNT];
            int ioctl_stat = ioctl(ev_fd,
                                   EVIOCGBIT(0, sizeof(capabilities_bitmask)),
                                   capabilities_bitmask);
            if(ioctl_stat < 0) {
                cerr << "Warning:  Error occurred while getting "
                     << "capabilities of device:"
                     << endl
                     << "\""
                     << *evFileIter
                     << "\"."
                     << endl
                     << "Reason: \"" << strerror(errno) << '\"'
                     << endl;
            } else {
                if(matchedCapabilities(capabilities_bitmask,
                                       requiredCapabilities,
                                       forbiddenCapabilities))
                {
                    cerr << "Autoscan:  Using device \""
                         << *evFileIter
                         << '"'
                         << endl;
                    return ev_fd;
                }
            }

        } else if(foundMatch) {
            cerr << "Autoscan:  Using device \""
                 << *evFileIter
                 << '"'
                 << endl;
            return ev_fd;
        }
        // else:
        // We have to keep examining devices.

        if(ev_fd > 0) {
            close(ev_fd);
        }
    }

    // If we reach here, no device was found.
    return -1;
}


LinuxInputDevice::LinuxInputDevice(const string& filename,
                                   uint16_t targVendor,
                                   uint16_t targProduct,
                                   const string& fallback_byName,
                                   const cap_flag_vec_t&
                                   requiredCapabilities,
                                   const cap_flag_vec_t&
                                   forbiddenCapabilities)
    : m__fd(-1)
{
    if( (filename == "auto") && targVendor && targProduct )
    {
        m__fd = scanForDevice(targVendor,
                              targProduct,
                              fallback_byName,
                              requiredCapabilities,
                              forbiddenCapabilities);
        if(m__fd < 0) {
            cerr << endl
                 << "Scanning for the "
                 << msnek4kDevname
                 << " device failed."
                 << endl
                 << "You must manually specify the correct keyboard "
                 << "device on the commandline "
                 << endl
                 << "or in the configuration file."
                 << endl << endl
                 << "Cowardly refusing to continue."
                 << endl << endl;
            exit(1);
        }
    } else {
        m__fd = openUnixReadFd(filename);
    }
}


LinuxInputDevice::~LinuxInputDevice()
{
    if(m__fd >= 0) {
        close(m__fd);
    }
}


/////////////////////////
//
// End

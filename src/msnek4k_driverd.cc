// -*- C++ -*-
// Implementation of a userspace driver for the keys on a "MS Natural
// Ergonomic Keyboard 4000" not supported by Linux at present.
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
msnek4k_driverd_cc__="RCS $Id$";


// Includes
//
#include <iostream>
#include <string>
#include <vector>
#include <stdexcept>

#include <sys/time.h> // Required for keypress/release parsing.

// For Unix I/O
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <signal.h>

// For Error Handling (Unix/POSIX  calls)
#include <cstring>
#include <errno.h>

#include <X11/extensions/XTest.h>  // Requires -lXtst
#include <X11/Xlib.h>

#include <tr1/array>

// Requires '-lboost_program_options-mt'
#include "Daemonizer.h"
#include "ProgramOptions_Base.h"



//
// Using Decls.
//
using std::ios_base;
using std::string;
using std::vector;
using std::exception;
using std::cerr;
using std::cout;
using std::endl;
using std::flush;


//
// Static variables
//


//
// Typedefs
//


/////////////////////////

//
// Class:  ProgramOptions
//


class ProgramOptions : public jpwTools::boost_helpers::ProgramOptions_Base
{
    typedef jpwTools::boost_helpers::ProgramOptions_Base Base_t;
public:
    struct KbdMapping
    {
        uint16_t scancode;
        uint16_t x11Keycode;
        bool isMouseButton;
        bool isMouseWheel;

        KbdMapping()
            : scancode(0)
            , x11Keycode(0)
            , isMouseButton(false)
            , isMouseWheel(false) {}
    };

    // Member variables for individually storing program options.
    bool doNotDaemonize;
    string daemonLog;
    string kbdDriverDev;
    KbdMapping zoomUp;
    KbdMapping zoomDown;
    KbdMapping spell;


    // C'tor
    //
    // Usually, you can get away with omitting your public member variables.
    // It's better to use the:
    //
    //     value<T>(m__myMemberVar)->default_value(...)
    //
    // ...arg in the 'add_options()' call when defining your options.
    //
    explicit ProgramOptions(const string& programName)
        : Base_t(programName)
    {}

    // Complete the body of this function (see below).
    void defineOptionsAndVariables();

    virtual bool validateParsedOptions(b_po_varmap_t& varMap);
};


//
// Used by the parse_environment() function in ProgramOptions::parse().
//
struct DisplayMapper
{
    std::string operator()(const std::string& s)
    {
        string o;
        if(s == "DISPLAY") {
            o = "display";
        }
        return o;
    }
};


/////////////////////////

//
// ProgramOptions Member Functions
//


void ProgramOptions::defineOptionsAndVariables()
{
    using namespace boost::program_options;

    //
    // Commandline-only Options
    //

    addOpts()
        ("--dbg",
         bool_switch(&doNotDaemonize)->default_value(false),
         "Run in the foreground instead of as a daemon."
         )
        ("zoom-up,U",
         value<uint16_t>(&(zoomUp.x11Keycode)),
         "The X11 keycode to generate when the Zoom jog is pressed up.\n"
         "Equivalent to the \"ZoomUp.x11Keycode\" configuration file "
         "variable."
         )
        ("zoom-down,D",
         value<uint16_t>(&(zoomDown.x11Keycode)),
         "The X11 keycode to generate when the Zoom jog is pressed down."
         "Equivalent to the \"ZoomDown.x11Keycode\" configuration file "
         "variable."
         )
        ("spell,S",
         value<uint16_t>(&(spell.x11Keycode)),
         "The X11 keycode to map the Spell key to."
         "Equivalent to the \"Spell.x11Keycode\" configuration file "
         "variable."
         )
        ;

    //
    // The Configuration File Variables:
    //

    addCfgVars()
        ("ZoomUp.scancode",
         value<uint16_t>(&(zoomUp.scancode))->default_value(0x1A2),
         "The raw keyboard scancode returned when the Zoom jog is "
         "pressed up (or released from that position).")
        ("ZoomUp.x11Keycode",
         value<uint16_t>(&(zoomUp.x11Keycode)),
         "The X11 keycode to generate when the Zoom jog is pressed up.\n")
        ("ZoomUp.isMouseButton",
         value<bool>(&(zoomUp.isMouseButton))->default_value(false),
         "When set to 'true', the value specified to \"ZoomUp.x11Keycode\" "
         "is a mouse button number.  Binds to this mouse button instead of a "
         "keycode.")
        ("ZoomUp.isMouseWheel",
         value<bool>(&(zoomUp.isMouseWheel))->default_value(false),
         "When set to 'true', the value specified to \"ZoomUp.x11Keycode\" "
         "maps to a mouse wheel button.  Ignored unless "
         "\"ZoomUp.isMouseButton\" is set to true.")
        ("ZoomDown.scancode",
         value<uint16_t>(&(zoomDown.scancode))->default_value(0x1A3),
         "The raw keyboard scancode returned when the Zoom jog is "
         "pressed down (or released from that position).")
        ("ZoomDown.x11Keycode",
         value<uint16_t>(&(zoomDown.x11Keycode)),
         "The X11 keycode to generate when the Zoom jog is pressed down.\n")
        ("ZoomDown.isMouseButton",
         value<bool>(&(zoomDown.isMouseButton))->default_value(false),
         "When set to 'true', the value specified to \"ZoomDown.x11Keycode\" "
         "is a mouse button number.  Binds to this mouse button instead of a "
         "keycode.")
        ("ZoomDown.isMouseWheel",
         value<bool>(&(zoomDown.isMouseWheel))->default_value(false),
         "When set to 'true', the value specified to \"ZoomDown.x11Keycode\" "
         "maps to a mouse wheel button.  Ignored unless "
         "\"ZoomDown.isMouseButton\" is set to true.")
        ("Spell.scancode",
         value<uint16_t>(&(spell.scancode))->default_value(0x1B0),
         "The raw keyboard scancode generated by the Spell key.")
        ("Spell.x11Keycode",
         value<uint16_t>(&(spell.x11Keycode)),
         "The X11 keycode to map the Spell key to.")
        ;

    //
    // Configuration File Variables that can also be passed as Commandline
    // Options:
    //

    // Environment variable:  DISPLAY.  Will be added to the shared options.
    addEnvvars(Base_t::SHARED)
        ("display,d", value<string>(),
         "The X11 display to run on.  Overrides the DISPLAY environment "
         "variable.\n"
         "This option is required if DISPLAY is not set.");

    // The keyboard device.
    string deflDev_tmp("/dev/input/by-id/usb-Microsoft_Natural");
    deflDev_tmp += "\xC2\xAE"; // Unicode character '®' in UTF-8
    deflDev_tmp += "_Ergonomic_Keyboard_4000-event-kbd";
    // Note:  The uber-long default value confuses the
    // boost::program_options engine, destroying the auto-formatting.  So, put
    // it in its own subgroup, then document it elsewhere, via a hidden
    // option.
    options_description kbdDev_descr_wrapper(usageLineLength());
    kbdDev_descr_wrapper.add_options()
        ("kbd-dev,k",
         value<string>(&kbdDriverDev)->default_value(deflDev_tmp.c_str()),
         "The full pathname of the keyboard device."
         )
        ;
    addCfgVars(kbdDev_descr_wrapper, Base_t::SHARED);

    // This is another potentially-long default value.  So like kbd-dev, it
    // goes in its own group.
    string deflLogfile("/tmp/");
    deflLogfile += programName();
    deflLogfile += ".log";
    options_description logfile_descr_wrapper(usageLineLength());
    logfile_descr_wrapper.add_options()
        ("logfile,l",
         value<string>(&daemonLog)->default_value(deflLogfile.c_str()),
         "The name of the log file.  Ignored if \"--dbg\" is passed on the "
         "commandline. "
         )
        ;
    addCfgVars(logfile_descr_wrapper, Base_t::SHARED);

    // Because of the overly-long defaults for some options, we need to use a
    // special option group, or the doc strings for the following won't
    // properly line-wrap.
    options_description otherShared_wrapper(usageLineLength());
    otherShared_wrapper.add_options()
        ("Zoom.isMouseButton,b", bool_switch()->default_value(false),
         "Equivalent to setting both the \"ZoomUp.isMouseButton\" and "
         "\"ZoomDown.isMouseButton\" configuration file variables."
         )
        ("Zoom.isMouseWheel,w", bool_switch()->default_value(false),
         "Equivalent to setting both the \"ZoomUp.isMouseWheel\" and "
         "\"ZoomDown.isMouseWheel\" configuration file variables.  Ignored "
         "if \"Zoom.isMouseButton\" isn't also set to true."
         )
        ;
    addCfgVars(otherShared_wrapper, Base_t::SHARED);


    //
    // Define the additional/verbose/enhanced configuration file
    // documentation:
    //

    const char* kbd_dev_doc =
        "\n"
        "* Selecting a \"--kbd-dev <dev>\":\n"
        "\n  \t"
        "Usually, <dev> will be under \"/dev/input\" or "
        "\"/dev/input/by-id\".  To determine which one to use:"
        "\n"
        "  1. \tRun 'input-events <n>', replacing \"<n>\" with an integer "
        "corresponding to an \"event*\" file in \"/dev/input\"."
        "\n"
        "  2. \tMove the 'Zoom' key on your keyboard.  If you get no "
        "response, repeat step-1 with a different value of \"<n>\".  "
        "Otherwise, the file \"/dev/input/event<n>\" (where \"<n>\""
        "is the integer you used in step-1) is the device you want."
        "\n";
    addConfigHelpDetails(kbd_dev_doc);

    const char* mouseWheelDoc =
        "\n"
        "* \tThe \"ZoomUp.isMouseWheel\", and \"ZoomDown.isMouseWheel\" "
        "Settings:\n"
        "\n  \t"
        "Normally, holding down a mouse button generates a single event.  "
        "Releasing it generates another, separate event.  Under X11, a "
        "mouse wheel is mapped to two buttons, one for each wheel "
        "direction.  The mouse wheel itself produces a button-click (or so "
        "it appears to X11)."
        "\n  \t"
        "If you want to use the Zoom jog as a mouse wheel, this becomes a "
        "problem.  Suppose you've set \"ZoomUp.isMouseWheel\", and "
        "\"ZoomDown.isMouseWheel\" to true.  Holding the Zoom jog up or "
        "down produces the same event as holding _down_ a mouse button, "
        "which is the same as moving the mouse wheel _once_.\n"
        "\n  \t"
        "The main reason for mapping the Zoom jog to the mouse wheel "
        "buttons, however, is to use it as an auto-rolling mouse wheel.\n"
        "\n  \t"
        "Setting \"ZoomUp.isMouseWheel\" and \"ZoomDown.isMouseWheel\" to "
        "true causes holding the Zoom jog to click the mouse wheel button, "
        "just like an actual mouse wheel does.  Releasing the Zoom jog will "
        "be ignored.  So, holding the Zoom jog will act as if you're "
        "spinning the mouse wheel nonstop.\n"
        "\n  \t"
        "Remember:  Each of these settings are ignored if the corresponding "
        "\"Zoom*.isMouseButton\" isn't set to true."
        "\n";
    addConfigHelpDetails(mouseWheelDoc);

    const char* keycodeDoc =
        "\n"
        "* \tThe \"Spell.x11Keycode\", \"ZoomUp.x11Keycode\", and "
        "\"ZoomDown.x11Keycode\" Settings:\n"
        "\n  \t"
        "Note that these are not keySYMs, but keyCODEs.  X11 maps the raw "
        "keyboard scancodes to its own set of 8-bit codes.  These are the "
        "X11 keycodes.\n"
        "\n  \t"
        "You should choose a keycode (between 1 and 255) that isn't "
        "already in use by X11.  (You *could* use the same keycode as "
        "another key, causing this key to behave identical to that other "
        "key.  But then, why map it at all?)  To find unused keycodes, run "
        "the following from a terminal:\n\n"
        "      xmodmap -pke | grep ' = *$' | less\n"
        "\n  \t"
        "(Picking one of the higher unused keycodes should insulate you "
        "from future XFree86/Xorg or kernel changes.)\n"
        "\n"
        "  Note:  \tUsing this driver will not automagically map the Spell "
        "& Zoom keys to a keysym for you.  You'll need to do that, "
        "yourself, using the \"xmodmap\" utility."
        "\n";
    addConfigHelpDetails(keycodeDoc);
}


// Performs more complex program option validation.
//
// The boost::program_options engine handles most forms of validation.  More
// complicated processing, such as cross-option dependencies, should go in
// this member function.
//
bool ProgramOptions::validateParsedOptions(b_po_varmap_t& varMap)
{
    // Handle Zoom.isMouseButton and Zoom.isMouseWheel:
    if(varMap["Zoom.isMouseButton"].as<bool>()) {
        zoomDown.isMouseButton = zoomUp.isMouseButton = true;
    }

    if(varMap["Zoom.isMouseWheel"].as<bool>()) {
        zoomDown.isMouseWheel = zoomUp.isMouseWheel = true;
    }

    // Make sure that the specified keycodes are in range.
    require8BitSize(spell.x11Keycode, "Spell.x11Keycode");
    require8BitSize(zoomUp.x11Keycode, "ZoomUp.x11Keycode");
    require8BitSize(zoomDown.x11Keycode, "ZoomDown.x11Keycode");

    return true;
}


/////////////////////////

//
// Struct:  UnixFd
//


struct UnixInputFd
{
    explicit UnixInputFd(const string& filename)
        : m__fd(-1)
    {
        m__fd = open(filename.c_str(), O_RDONLY);
        if(m__fd == -1) {
            string errmsg("Failed to open file for reading:\n\t\"");
            errmsg += filename;
            errmsg += "\"\nReason:\n\t";
            errmsg += strerror(errno);
            throw std::ios_base::failure(errmsg);
        }
    }

    template<typename T>
    ssize_t reinterpret_read(T* objPtr)
    {
        return read(m__fd, reinterpret_cast<void*>(objPtr), sizeof(T));
    }

private:
    int m__fd;
};


/////////////////////////

//
// Struct:  KbdInputEvent
//


struct KbdInputEvent
{
    // Notice that the members of this struct require no padding.  All
    // adjacent members smaller than a word fit within a single word.

    timeval evTime;  // sizeof(timeval) == 2 x wordsize.
    uint16_t evType;
    uint16_t evCode;
    uint32_t evValue;

    // The Keypress (and key-release) data from a "/dev/input/event*" file is
    // 0x30 or 0x48 bytes long, for 32-bit or 64-bit platforms, respectively.
    // It comes in 3 groups of 0x10 or 0x18 bytes each.

    // Enum for the evType field.
    enum event_type {
        SYN=0x00,
        KEY=0x01,
        REL=0x02,
        ABS=0x03,
        MSC=0x04,
        LED=0x11,
        SND=0x12,
        REP=0x14,
        FF=0x15,
        PWR=0x16,
        FF_STATUS=0x17,
        MAX=0x1f
    };

    // Comparison Functions:
    // Compare the evType member of this object to a specified event_type
    // tag.

    bool operator==(event_type t) const
    { return (static_cast<event_type>(evType) == t); }

    bool operator!=(event_type t) const
    { return !this->operator==(t); }

    // I/O
    bool read(UnixInputFd& ufd)
    {
        // Recall:  the members of KbdInputEvent have no padding between them.
        // "Consecutive" member fields all completely fill a machine word, or
        // occupy an integral number of machine words.
        //
        // Reading raw data like this will require one or more
        // reinterpret_cast<>()'s someplace.  That's unavoidable.
        ssize_t nRead = ufd.reinterpret_read(this);
        return (nRead > 0);
    }
};


/////////////////////////

//
// Class:  X11Display
//


// Simple wrapper class for automatically closing the X11 Display in the event
// of an error or exception.
class X11Display
{
    Display* m__x11DisplayPtr;
public:
    struct FailToOpen : public std::runtime_error
    {
        explicit FailToOpen(const string& what)
            : std::runtime_error(what)
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


/////////////////////////

//
// General Function Definitions
//


// Commuted op==() and op!=() for KbdInputEvent.  Just in case.
bool operator==(KbdInputEvent::event_type t, const KbdInputEvent& kbd)
{ return kbd.operator==(t); }

bool operator!=(KbdInputEvent::event_type t, const KbdInputEvent& kbd)
{ return kbd.operator!=(t); }


bool processKbdEvent(const KbdInputEvent& kbdEvent,
                     X11Display& theDisplay,
                     const ProgramOptions& opts)
{
    if(kbdEvent != KbdInputEvent::KEY) {
        return true;
    }

    int verbose = opts["verbose"].as<int>();

    ProgramOptions::KbdMapping mapping;

    // We just have 3 keys to map; no need to use a generic data structure.
    if(kbdEvent.evCode == opts.zoomUp.scancode) {
        mapping = opts.zoomUp;
    } else if(kbdEvent.evCode == opts.zoomDown.scancode) {
        mapping = opts.zoomDown;
    } else if(kbdEvent.evCode == opts.spell.scancode) {
        mapping = opts.spell;
    } else if(verbose) {
        cerr << "Key "
             << (kbdEvent.evValue ? "Pressed" : "Released")
             << ":  unknown scancode==0x"
             << std::hex << kbdEvent.evCode << endl;
        return true;
    }

    if(verbose > 1) {
        cout << "Key "
             << (kbdEvent.evValue ? "Pressed" : "Released")
             << ":  scancode==0x"
             << std::hex << mapping.scancode << std::dec
             << (mapping.isMouseButton
                 ? " ==> mouse button #"
                 : " ==> X11 Key: ")
             << mapping.x11Keycode
             << endl;
    }

    // kbdEvent.evValue contains the pressed/released value.
    int sentOk;
    if(mapping.isMouseButton) {
        if(mapping.isMouseWheel && kbdEvent.evValue) {
            // When treating a button as a mouse wheel, ignore release
            // events.
            sentOk = XTestFakeButtonEvent(theDisplay, mapping.x11Keycode,
                                          true, CurrentTime);
            // 'And'-in the previous value of "sentOk" ... _after_ the
            // fn. call.
            sentOk = XTestFakeButtonEvent(theDisplay, mapping.x11Keycode,
                                          false, CurrentTime)
                && sentOk;
        } else if(!mapping.isMouseWheel) {
            sentOk = XTestFakeButtonEvent(theDisplay, mapping.x11Keycode,
                                          kbdEvent.evValue, CurrentTime);
        }
    } else {
        sentOk = XTestFakeKeyEvent(theDisplay, mapping.x11Keycode,
                                   kbdEvent.evValue, CurrentTime);
    }
    int flushOk = XFlush(theDisplay);

    if(!sentOk) {
        cerr << "Failed to send event for scancode==0x"
             << std::hex << mapping.scancode << std::dec
             << endl;
    }
    return (sentOk && flushOk);
}


/////////////////////////

//
// Functions "main()" and "cxx_main()"
//


// This is where all of your main handling should go.
int cxx_main(const string& myName,
             const string& myPath,
             const ProgramOptions& opts)
{
    // First things first:  daemonize yourself.
    if(!opts.doNotDaemonize) {
        jpwTools::process::daemonize(opts.daemonLog.c_str(), 0, true);
    }

    // X11/XTest Setup
    X11Display theDisplay(opts["display"].as<string>());
    int xtqeDummy;
    bool hasXTest = XTestQueryExtension(theDisplay,
                                        &xtqeDummy, &xtqeDummy,
                                        &xtqeDummy, &xtqeDummy);
    if(!hasXTest) {
        string errmsg("The XTest extension is not installed "
                      "or not available.\n");
        errmsg += myName;
        errmsg += " requires XTest in order to run.\n\n"
            "Cowardly refusing to continue.";
        throw std::runtime_error(errmsg);
    }

    // The Main Loop

    UnixInputFd kbd_fd(opts.kbdDriverDev);
    KbdInputEvent kbdEvent;
    while(true)
    {
        if(!kbdEvent.read(kbd_fd)) {
            sleep(1);
            continue;
        }

        processKbdEvent(kbdEvent, theDisplay, opts);
    }
}


int main(int argc, char* argv[])
{
    // Split off the name of the executable from its path.
    string myName(argv[0]);
    string::size_type last_pathsep = myName.find_last_of('/');
    string myPath;
    if(last_pathsep != string::npos) {
        myPath = myName.substr(0, last_pathsep+1);
        myName.erase(0, last_pathsep+1);
    }

    // Call cxx_main(), which is where almost all of your code should go.
    try {
        ProgramOptions myOpts(myName);
        myOpts.parse(argc, argv);

        return cxx_main(myName, myPath, myOpts);
    } catch(std::ios_base::failure& ex) {
        cerr << "I/O Error: " << ex.what() << endl;
        return 2;
    } catch(boost::program_options::duplicate_option_error& ex) {
        cerr << "Fatal Internal Programming Error:  "
             << endl
             << ex.what()
             << endl << endl;
        return 9;
    } catch(boost::program_options::error& ex) {
        cerr << "Error while parsing program options: " << ex.what()
             << endl << endl
             << "Rerun as \"" << myName << " --help\" for usage."
             << endl;
        return 1;
    } catch(exception& ex) {
         cerr << endl << "(Std) Exception caught: \""
              << ex.what() << "\"" << endl;
    } catch(...) {
        cerr << "Unknown exception caught." << endl;
    }
    return -1;
}


/////////////////////////
//
// End

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
msnek4k_driverd_cc__="$Id$";


// Includes
//
#include <iostream>
#include <csignal>

// Required for keypress/release parsing.
#include <sys/time.h>

// Requires -lXtst
#include <X11/extensions/XTest.h>


#include "X11Display.h"
#include "LinuxInputDevice.h"
#include "LinuxInputEvent.h"
#include "Daemonizer.h"
// Requires '-lboost_program_options-mt'
#include "ProgramOptions_Base.h"



//
// Using Decls.
//
using std::ios_base;
using std::string;
using std::exception;
using std::cerr;
using std::cout;
using std::endl;
using std::flush;

using jpwTools::X11Display;
using jpwTools::LinuxInputDevice;
using jpwTools::LinuxInputEvent;


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
    explicit ProgramOptions(const string& theProgramName)
        : Base_t(theProgramName)
    {}

    // Complete the body of this function (see below).
    void defineOptionsAndVariables();

    virtual bool validateParsedOptions(b_po_varmap_t& varMap);
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

    // The keyboard device.  Unicode character '®'=='\u00A9'
    string deflDev_tmp("/dev/input/by-id/usb-Microsoft_Natural\u00A9"
                       "_Ergonomic_Keyboard_4000-event-kbd");
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
// General Function Definitions
//


bool processKbdEvent(const LinuxInputEvent& kbdEvent,
                     X11Display& theDisplay,
                     const ProgramOptions& opts)
{
    if(kbdEvent != LinuxInputEvent::KEY) {
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
    int sentOk(0);
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
             const string& /*myPath*/,
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

    LinuxInputDevice kbdDev(opts.kbdDriverDev);
    LinuxInputEvent kbdEvent;
    while(true)
    {
        if(!kbdEvent.read(kbdDev)) {
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

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
#include <fstream> // Required for reading config files.
#include <sstream> // Required by --help-config.
#include <string>
#include <vector>
#include <stdexcept>

#include <sys/time.h> // Required for keypress/release parsing.
#include <stdint.h>   // For size-specific int types.

// For Unix I/O
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>

// For Error Handling (Unix/POSIX  calls)
#include <cstring>
#include <errno.h>

#include <X11/extensions/XTest.h>  // Requires -lXtst
#include <X11/Xlib.h>

#include <tr1/array>
// Requires '-lboost_program_options-mt'
#include <boost/program_options.hpp>



//
// Using Decls.
//
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


class ProgramOptions
{
    // The boost::program_options library always indents the documentation
    // strings by a minimum amount.  This messes up the display of
    // m__cfgfileDocDetails.
    //
    // To fix this problem, we will unindent by a fixed amount.
    static const unsigned m__B_PO_MIN_LEFT_MARGIN=20;

    // Internal Member Variables
    // No User Serviceable Parts
    string m__progName;
    bool m__showHelp;
    bool m__showHelpConfig;
    string m__cfgfile;
    char m__docDetails_FakeOption[2];
    boost::program_options::positional_options_description m__posnParams;
    boost::program_options::options_description m__posnParamOpts;
    boost::program_options::variables_map m__opts;
    unsigned m__lineLen;
    boost::program_options::options_description m__cmdlineOpts;
    boost::program_options::options_description m__hiddenCmdlineOpts;
    boost::program_options::options_description m__sharedOpts;
    boost::program_options::options_description m__envvarOpts;
    boost::program_options::options_description m__cfgfileOpts;
    boost::program_options::options_description m__cfgfileDocDetails;

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
    explicit ProgramOptions(const string& programName, int lineLength=78)
        : m__progName(programName)
        , m__showHelp(false)
        , m__showHelpConfig(false)
        , m__cfgfile()
        , m__docDetails_FakeOption()
        , m__opts()
        , m__lineLen(lineLength)
        , m__posnParams()
        , m__posnParamOpts("Positional Parameters", m__lineLen)
        , m__cmdlineOpts(m__lineLen)
        , m__hiddenCmdlineOpts("Hidden Options", m__lineLen)
        , m__sharedOpts(m__lineLen)
        , m__envvarOpts(m__lineLen)
        , m__cfgfileOpts("Configuration File Variables", m__lineLen)
          // The extra offset of 6, below, accounts for (A) the space added by
          // boost::program_options for the fake option; (B) the "# " prefix,
          // if the user requested it.
        , m__cfgfileDocDetails("Details",
                               m__lineLen + m__B_PO_MIN_LEFT_MARGIN - 6)
    {
        m__docDetails_FakeOption[0] = 0x21;
        m__docDetails_FakeOption[2] = 0;
    }

    // Complete the body of this function (see below).
    void defineCommandlineOptions();

    // Complete the body of this function (see below).
    void defineCfgfileOptions();

    // Complete the body of this function (see below).
    bool validateParsedOptions();

    void parse(int argc, char* argv[]);

    const boost::program_options::variable_value&
    operator[](const string& varName) const
    {
        return m__opts[varName];
    }

private:
    typedef boost::program_options::options_description b_po_opt_descr_t;
    typedef boost::program_options::value_semantic b_po_value_semantic_t;

    // Enhanced help output for configuration files.
    void showConfigHelp(const b_po_opt_descr_t& config_descr);

    // Add the named positional parameter.
    //
    void addPosnParam(const char* paramName,
                      const char* paramDocstring,
                      int max_count=1)
    {
        m__posnParams.add(paramName, max_count);
        m__posnParamOpts.add_options()
            (paramName, paramDocstring);
    }

    // Add the named positional parameter.
    //
    void addPosnParam(const char* paramName,
                      const b_po_value_semantic_t* value_obj,
                      const char* paramDocstring,
                      int max_count=1)
    {
        m__posnParams.add(paramName, max_count);
        m__posnParamOpts.add_options()
            (paramName, value_obj, paramDocstring);
    }

    // Enhanced help output for configuration files.
    void addConfigHelpDetails(const char* docstr)
    {
        m__cfgfileDocDetails.add_options()
            (m__docDetails_FakeOption, docstr);
        ++m__docDetails_FakeOption[0];
    }

    // Check that \a val is only 8 bits.  \a varName is the config variable
    // that's set to \val.
    //
    // Unfortunately, boost::program_options will not accept an integer value
    // for \c char or \c unsigned \c char.
    void require8BitSize(uint16_t val, const char* varName)
    {
        using namespace boost::program_options;
        if(!val || (val & 0xFF00)) {
            string errmsg("Value out of range:  \"");
            errmsg += varName;
            errmsg += "\" must be a number between 1 and 255, inclusive.";
            throw invalid_option_value(errmsg);
        }
    }
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


// Define your commandline options in this function.
//
// The options "--help", "--verbose", "--config", and "--help-config" will
// be defined for you.  No need to put them in here.
void ProgramOptions::defineCommandlineOptions()
{
    using namespace boost::program_options;

    m__cmdlineOpts.add_options()
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
}


// Define your configuration file variables in this function.
//
// If your program doesn't use a configfile, just delete everything in the
// body of this function.
//
void ProgramOptions::defineCfgfileOptions()
{
    using namespace boost::program_options;

    // Define the Configuration File Variables:

    m__cfgfileOpts.add_options()
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

    // Define the Configuration File Variables that can also be passed as
    // Commandline Options:

    string deflDev_tmp("/dev/input/by-id/usb-Microsoft_Natural");
    deflDev_tmp += "\xC2\xAE"; // Unicode character '®' in UTF-8
    deflDev_tmp += "_Ergonomic_Keyboard_4000-event-if01";
    // Note:  The uber-long default value confuses the
    // boost::program_options engine, destroying the auto-formatting.  So, put
    // it in its own subgroup, then document it elsewhere, via a hidden
    // option.
    options_description kbdDev_descr_wrapper(m__lineLen);
    kbdDev_descr_wrapper.add_options()
        ("kbd-dev,k",
         value<string>(&kbdDriverDev)->default_value(deflDev_tmp.c_str()),
         "The full pathname of the keyboard device."
         )
        ;
    m__sharedOpts.add(kbdDev_descr_wrapper);

    // Again, because of the overly-long default for kbdDriverDev, we need
    // to use a special option group, or the doc strings for the following
    // won't properly line-wrap.
    options_description otherShared_wrapper(m__lineLen);
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
    m__sharedOpts.add(otherShared_wrapper);

    // Environment variable:  DISPLAY.  Will be added to the shared options.
    m__envvarOpts.add_options()
        ("display,d", value<string>(),
         "The X11 display to run on.  Overrides the DISPLAY environment "
         "variable.\n"
         "This option is required if DISPLAY is not set.");
    m__sharedOpts.add(m__envvarOpts);

    // Define the additional/verbose/enhanced configuration file
    // documentation:

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
bool ProgramOptions::validateParsedOptions()
{
    // Handle Zoom.isMouseButton and Zoom.isMouseWheel:
    if(m__opts["Zoom.isMouseButton"].as<bool>()) {
        zoomDown.isMouseButton = zoomUp.isMouseButton = true;
    }

    if(m__opts["Zoom.isMouseWheel"].as<bool>()) {
        zoomDown.isMouseWheel = zoomUp.isMouseWheel = true;
    }

    // Make sure that the specified keycodes are in range.
    require8BitSize(spell.x11Keycode, "Spell.x11Keycode");
    require8BitSize(zoomUp.x11Keycode, "ZoomUp.x11Keycode");
    require8BitSize(zoomDown.x11Keycode, "ZoomDown.x11Keycode");
}


// No User Servicable Parts in this fn.
//
void ProgramOptions::showConfigHelp(const b_po_opt_descr_t& config_descr)
{
    const char* prefix("");
    bool verbose = m__opts["verbose"].as<int>();

    if(verbose) {
        prefix = "# ";
    }

    cout << prefix << m__progName << " - Configuration File:"
         << endl << prefix << endl << prefix
         << "Settings in the configuration file are of the form:"
         << endl << prefix << endl << prefix
         << "    settingName=value"
         << endl << prefix <<  endl << prefix
         << "Multiple settings can be grouped into sections.  "
         << "Each option in a group"
         << endl << prefix
         << "has the form \"sectionName.settingName\", and appears in "
         << "the configuration "
         << endl << prefix
         << "file as follows:"
         << endl << prefix << endl << prefix
         << "    [sectionName]"
         << endl << prefix
         << "    settingName=value"
         << endl << prefix << endl << prefix
         << "The comment delimiter is '#' and may appear anywhere "
         << "on a line."
         << endl << prefix
         << endl;

    // Unfortunately, boost::program_options doesn't provide a means of
    // printing out the configuration file variables as anything other
    // than options.  So, we'll fake it by printing to a stringstream and
    // editing each line before printing it out.
    std::stringstream configDoc_sst;
    configDoc_sst << config_descr << endl;

    string::size_type unindent(0);
    while(configDoc_sst) {
        string line;
        getline(configDoc_sst, line);

        // Look for lines beginning with an option name.  Ignore any lines
        // with a margin more than 1/3 of the size of the line.
        string::size_type leftMargin = line.find_first_not_of(' ');
        if( (leftMargin == string::npos) ||
            (leftMargin > line.length()/3) )
        {
            leftMargin = 0;
        }

        if(line.find("--") == leftMargin) {
            // Erase a leading '--'
            line[leftMargin] = ' ';
            line[leftMargin + 1] = ' ';
            unindent = 2;
        } else if( (line.find(" [ --") == (leftMargin+2)) &&
                   (line[leftMargin] == '-') &&
                   (line[leftMargin+1] != '-') )
        {
            // Erase the short option and remove the '['...']' surrounding
            // the long name.
            line.replace(leftMargin, 7, 7, ' ');
            string::size_type bracketPos = line.find(']', leftMargin);
            if(bracketPos != string::npos) {
                line.erase(bracketPos-1, 2);
                line.insert(0, 2, ' ');
            }
            unindent = 9;
        } else if(leftMargin = 0) {
            unindent = 0;
        }

        // Remove excess indentation, if any.
        if(unindent && (line.length() > unindent)) {
            line.erase(0, unindent);
        }

        cout << prefix << line << endl;
    }

    // Stop if there's no additional documentation.
    if (m__cfgfileDocDetails.options().empty()) {
        exit(0);
    } // else:

    configDoc_sst.clear();
    configDoc_sst << m__cfgfileDocDetails << endl;
    unindent = m__B_PO_MIN_LEFT_MARGIN;
    if(verbose) {
        unindent += 2;
    }
    while(configDoc_sst) {
        string line;
        getline(configDoc_sst, line);

        string::size_type leftMargin = line.find_first_not_of(' ');
        if(leftMargin == string::npos) {
            leftMargin = 0;
        }
        if(line.find("--") == leftMargin) {
            // Trim off the fake option.
            leftMargin = unindent;
        }
        // Ignore lines that haven't been indented.
        if (unindent <= leftMargin) {
            line.erase(0, unindent);
        }

        cout << prefix << line << endl;
    }
    cout << endl << endl;


    if(verbose) {
        cout << endl;
        boost::any defaultVal;
        for(unsigned ui=0; ui < config_descr.options().size(); ++ui) {
            cout << '#'
                 << config_descr.options()[ui]->long_name()
                 << " = ";
            if(config_descr.options()[ui]
               ->semantic()->apply_default(defaultVal))
            {
                // Ugh!  boost:any isn't OutputStreamable.  Need to resort to
                // nasty typeid-checking.
                if(defaultVal.type() == typeid(bool)) {
                    cout << boost::any_cast<bool>(defaultVal);
                } else if(defaultVal.type() == typeid(uint16_t)) {
                    cout << boost::any_cast<uint16_t>(defaultVal);
                } else if(defaultVal.type() == typeid(string)) {
                    cout << boost::any_cast<string>(defaultVal);
                }
            }
            cout << endl << endl;
        }
    } else {
        cout << "(To print this message as a sample configuration file, "
             << "rerun this program"
             << endl
             << " with both the \"--help-config\" and \"-v\" options.)"
             << endl;
    }

    exit(0);
}


// No User Servicable Parts in this fn.
//
void ProgramOptions::parse(int argc, char* argv[])
{
    using namespace boost::program_options;

    defineCommandlineOptions();
    defineCfgfileOptions();

    options_description cmdline_descr;
    options_description cmdline_documented_descr("Options");
    options_description config_descr;

    string deflCfgFile("/etc/");
    deflCfgFile += m__progName;
    deflCfgFile += ".conf";

    // Define the default/std. commandline options
    cmdline_documented_descr.add_options()
        ("help,h", bool_switch(&m__showHelp), "This message.")
        ("verbose,v",
         value<int>()->implicit_value(1)->default_value(0),
         "Make this program more verbose.")
        ("config",
         value<string>(&m__cfgfile)->default_value(deflCfgFile.c_str()),
         "Configuration file, containing additional options.")
        ;
    if(!m__cfgfileOpts.options().empty()) {
        cmdline_documented_descr.add_options()
            ("help-config", bool_switch(&m__showHelpConfig),
             "Additional information about the configuration file.")
            ;
    }

    // Set up the local 'options_description' vars from the members.

    // Comandline:
    cmdline_documented_descr.add(m__cmdlineOpts);
    if(!m__sharedOpts.options().empty()) {
        cmdline_documented_descr.add(m__sharedOpts);
    }
    if(!m__posnParamOpts.options().empty()) {
        cmdline_documented_descr.add(m__posnParamOpts);
    }

    cmdline_descr.add(cmdline_documented_descr);
    cmdline_descr.add(m__hiddenCmdlineOpts);

    // Config File:
    if(!m__sharedOpts.options().empty()) {
        config_descr.add(m__sharedOpts);
    }
    if(!m__cfgfileOpts.options().empty()) {
        config_descr.add(m__cfgfileOpts);
    }

    // Parse the Commandline:
    command_line_parser theParser(argc, argv);
    theParser.options(cmdline_descr);
    if(!m__posnParamOpts.options().empty()) {
        theParser.positional(m__posnParams);
    }
    store(theParser.run(), m__opts);

    // Read the Config File (if any):
    if(!config_descr.options().empty()) {
        // Unfortunately, m__cfgfile is still empty at this point.  Calling
        // notify() fixes that.
        notify(m__opts);

        // The --help* options override reading the configfile.
        if(m__showHelp || m__showHelpConfig) {
            m__cfgfile.erase();
        }
    }
    if(!config_descr.options().empty() && !m__cfgfile.empty()) {
        std::ifstream cfg_ifs(m__cfgfile.c_str());
        if(!cfg_ifs) {
            string errmsg("Invalid/unknown configuration file: \"");
            errmsg += m__cfgfile;
            errmsg += '"';
            throw invalid_option_value(errmsg);
        }
        try {
            store(parse_config_file(cfg_ifs, config_descr), m__opts);
            cfg_ifs.close();
        } catch(std::ios_base::failure& ex) {
            string errmsg("Failed to read configuration file: \"");
            errmsg += m__cfgfile;
            errmsg += "\"\nReason:\n\t\"";
            errmsg += ex.what();
            errmsg += '"';
            throw invalid_option_value(errmsg);
        }
    }

    // Read the environment vars:
    store(parse_environment(m__envvarOpts, DisplayMapper()), m__opts);

    // Finish up.
    notify(m__opts);

    // Print out the help message(s), as needed:

    if(m__showHelp)
    {
        cout << "usage: " << m__progName
             << " [options] [posn params]"
             << endl << endl
             << cmdline_documented_descr
             << endl;
        exit(0);
    }

    if(m__showHelpConfig) {
        showConfigHelp(config_descr);
    }

    validateParsedOptions();
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
        FD_ZERO(&m__readSet);

        m__fd = open(filename.c_str(), O_RDONLY);
        if(m__fd == -1) {
            string errmsg("Failed to open file for reading:\n\t\"");
            errmsg += filename;
            errmsg += "\"\nReason:\n\t";
            errmsg += strerror(errno);
            throw std::ios_base::failure(errmsg);
        }

        FD_SET(m__fd, &m__readSet);
    }

    template<typename T>
    ssize_t reinterpret_read(T* objPtr)
    {
        return read(m__fd, reinterpret_cast<void*>(objPtr), sizeof(T));
    }

private:
    int m__fd;
    fd_set m__readSet;
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


void processKbdEvent(const KbdInputEvent& kbdEvent,
                     X11Display& theDisplay,
                     const ProgramOptions& opts)
{
    if(kbdEvent != KbdInputEvent::KEY) {
        return;
    }

    bool verbose = opts["verbose"].as<int>();

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
        return;
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
//        sentOk = XTestFakeButtonEvent(theDisplay, mapping.x11Keycode,
//                                      kbdEvent.evValue, CurrentTime);
        // For Wheel Buttons
        // FIXME:  Make this a config/cmdline option.
        if(kbdEvent.evValue) {
            sentOk = XTestFakeButtonEvent(theDisplay, mapping.x11Keycode,
                                          true, CurrentTime);
            sentOk = XTestFakeButtonEvent(theDisplay, mapping.x11Keycode,
                                          false, CurrentTime);
        }
    } else {
        sentOk = XTestFakeKeyEvent(theDisplay, mapping.x11Keycode,
                                   kbdEvent.evValue, CurrentTime);
    }
    if(sentOk) {
        XFlush(theDisplay);
    } else {
        cerr << "Failed to send event for scancode==0x"
             << std::hex << mapping.scancode << std::dec
             << endl;
    }
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

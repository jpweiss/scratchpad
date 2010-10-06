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
#include <sys/time.h> // Required for keypress/release parsing.
#include <stdint.h>

// For Unix I/O
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>

// For Error Handling (Unix/POSIX  calls)
#include <cstring>
#include <errno.h>

#include <iostream>
#include <fstream> // Required for reading config files.
#include <sstream> // Required by --help-config.
#include <string>
#include <vector>
#include <exception>

#include <tr1/array>
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


// NOTE:
//
// When using boost::program_options, you need to compile or link using:
//   '-lboost_program_options-mt'
// (Check your Boost build to see if you have to remove the '-mt' suffix.)
class ProgramOptions
{
    // Internal Member Variables
    // No User Serviceable Parts
    string m__progName;
    bool m__showHelp;
    bool m__showHelpConfig;
    string m__cfgfile;
    boost::program_options::positional_options_description m__posnParams;
    boost::program_options::options_description m__posnParamOpts;
    boost::program_options::variables_map m__opts;
    boost::program_options::options_description m__cmdlineOpts;
    boost::program_options::options_description m__hiddenCmdlineOpts;
    boost::program_options::options_description m__sharedOpts;
    boost::program_options::options_description m__cfgfileOpts;

public:
    struct KbdMapping
    {
        uint16_t scancode;
        unsigned keysym;
        bool isMouseButton;

        KbdMapping() : scancode(0), keysym(0), isMouseButton(false) {}
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
    ProgramOptions(const string& programName, int lineLength=78)
        : m__progName(programName)
        , m__showHelp(false)
        , m__showHelpConfig(false)
        , m__cfgfile()
        , m__opts()
        , m__posnParams()
        , m__posnParamOpts("Positional Parameters", lineLength)
        , m__cmdlineOpts(lineLength)
        , m__hiddenCmdlineOpts("Hidden Options", lineLength)
        , m__sharedOpts(lineLength)
        , m__cfgfileOpts("Configuration File Variables", lineLength)
    {}

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
    typedef boost::program_options::value_semantic b_po_value_semantic_t;

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
        ("zoom-up,u",
         value<unsigned>(&(zoomUp.keysym)),
         "The X11 keysym to generate when the Zoom jog is pressed up.\n"
         "Equivalent to the \"ZoomUp.x11Keysym\" configuration file "
         "variable."
         )
        ("zoom-down,d",
         value<unsigned>(&(zoomDown.keysym)),
         "The X11 keysym to generate when the Zoom jog is pressed down."
         "Equivalent to the \"ZoomDown.x11Keysym\" configuration file "
         "variable."
         )
        ("zoom-is-mouse,b", bool_switch()->default_value(false),
         "Equivalent to setting both the \"ZoomUp.isMouseButton\" and "
         "\"ZoomDown.isMouseButton\" configuration file variables."
         )
        ("spell,s",
         value<unsigned>(&(zoomDown.keysym)),
         "The X11 keysym to map the Spell key to."
         "Equivalent to the \"Spell.x11Keysym\" configuration file "
         "variable."
         )
        ;

    m__hiddenCmdlineOpts.add_options()
        ("help-kbd-dev",
         bool_switch()->default_value(false),
         "Describe the \"--kbd-dev\" option in detail.")
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
        ("ZoomUp.x11Keysym",
         value<unsigned>(&(zoomUp.keysym)),
         "The X11 keysym to generate when the Zoom jog is pressed up.")
        ("ZoomUp.isMouseButton",
         value<bool>(&(zoomUp.isMouseButton))->default_value(false),
         "When set to 'true', the value specified to \"ZoomUp.x11Keysym\" is"
         "a mouse button number.  Binds to this mouse button instead of a "
         "keysym.")
        ("ZoomDown.scancode",
         value<uint16_t>(&(zoomDown.scancode))->default_value(0x1A3),
         "The raw keyboard scancode returned when the Zoom jog is "
         "pressed down (or released from that position).")
        ("ZoomDown.x11Keysym",
         value<unsigned>(&(zoomDown.keysym)),
         "The X11 keysym to generate when the Zoom jog is pressed down.")
        ("ZoomDown.isMouseButton",
         value<bool>(&(zoomDown.isMouseButton))->default_value(false),
         "When set to 'true', the value specified to \"ZoomDown.x11Keysym\" "
         "is a mouse button number.  Binds to this mouse button instead of a "
         "keysym.")
        ("Spell.scancode",
         value<uint16_t>(&(spell.scancode))->default_value(0x1B0),
         "The raw keyboard scancode generated by the Spell key.")
        ("Spell.x11Keysym",
         value<unsigned>(&(spell.keysym)),
         "The X11 keysym to map the Spell key to.")
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
    options_description kbdDev_descr_wrapper;
    kbdDev_descr_wrapper.add_options()
        ("kbd-dev,k",
         value<string>(&kbdDriverDev)->default_value(deflDev_tmp.c_str()),
         "\nThe full pathname of the keyboard device.\n"
         "Rerun with the \"--help-kbd-dev\" option for more details."
         )
        ;
    m__sharedOpts.add(kbdDev_descr_wrapper);
}


// Performs more complex program option validation.
//
// The boost::program_options engine handles most forms of validation.  More
// complicated processing, such as cross-option dependencies, should go in
// this member function.
//
bool ProgramOptions::validateParsedOptions()
{
    // Handle --zoom-is-mouse:
    if(m__opts["zoom-is-mouse"].as<bool>()) {
        zoomDown.isMouseButton = zoomUp.isMouseButton = true;
    }

    // The extended help-message for \"--kbd-dev\"
    if(m__opts["help-kbd-dev"].as<bool>())
    {
        cout << "--kbd-dev <dev>:" << endl
             <<
            "\tThe full pathname of the keyboard device.\n\n"
            "\tUsually, <dev> will be under \"/dev/input\" or "
            "\"/dev/input/by-id\".  To\n"
            "\tdetermine which one to use:\n"
            "\t1. Run 'input-events <n>', replacing \"<n>\" with an integer\n"
            "\t   corresponding to an \"event*\" file in \"/dev/input\".\n"
            "\t2. Move the 'Zoom' key on your keyboard.  If you get no "
            "response,\n"
            "\t   repeat step-1 with a different value of \"<n>\".\n"
            "\t   Otherwise, the file \"/dev/input/event<n>\" (where \"<n>\""
            "is the\n"
            "\t   integer you used in step-1) is the device you want.\n"
             << endl;
         exit(0);
    }
}


// There's very little to change in this function.
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
        ("verbose,v", value<int>()->default_value(0)->zero_tokens(),
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
    if(!m__cfgfileOpts.options().empty()) {
        config_descr.add(m__cfgfileOpts);
    }
    if(!m__sharedOpts.options().empty()) {
        config_descr.add(m__sharedOpts);
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

    if(m__showHelpConfig)
    {
        cout << m__progName << " - Configuration File:"
             << endl << endl
             << "Settings in the configuration file are of the form:"
             << endl << endl
             << "    settingName=value"
             << endl << endl
             << "Multiple settings can be grouped into sections.  "
             << "Each option in a group"
             << endl
             << "has the form \"sectionName.settingName\", and appears in "
             << "the configuration "
             << endl
             << "file as follows:"
             << endl << endl
             << "    [sectionName]"
             << endl
             << "    settingName=value"
             << endl << endl
             << "The comment delimiter is '#' and may appear anywhere "
             << "on a line."
             << endl << endl;

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

            cout << line << endl;
        }
        exit(0);
    }

    validateParsedOptions();
}


/////////////////////////

//
// Struct:  UnixFd
//


struct UnixInputFd
{
    UnixInputFd(const string& filename)
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
// General Function Definitions
//


// Commuted op==() and op!=() for KbdInputEvent
bool operator==(KbdInputEvent::event_type t, const KbdInputEvent& kbd)
{ return kbd.operator==(t); }

bool operator!=(KbdInputEvent::event_type t, const KbdInputEvent& kbd)
{ return kbd.operator!=(t); }


void processKbdEvent(const KbdInputEvent& kbdEvent,
                     const ProgramOptions& opts)
{
    if(kbdEvent != KbdInputEvent::KEY) {
        return;
    }

    ProgramOptions::KbdMapping mapping;

    // We just have 3 keys to map; no need to use a generic data structure.
    if(kbdEvent.evCode == opts.zoomUp.scancode) {
        mapping = opts.zoomUp;
    } else if(kbdEvent.evCode == opts.zoomDown.scancode) {
        mapping = opts.zoomDown;
    } else if(kbdEvent.evCode == opts.spell.scancode) {
        mapping = opts.spell;
    } else {
        cout << "Key "
             << (kbdEvent.evValue ? "Pressed" : "Released")
             << ":  unknown scancode==0x"
             << std::hex << kbdEvent.evCode << endl;
        return;
    }

    cout << "Key "
         << (kbdEvent.evValue ? "Pressed" : "Released")
         << ":  scancode==0x"
         << std::hex << mapping.scancode << std::dec
         << (mapping.isMouseButton
             ? " ==> mouse button #"
             : " ==> X11 Key: ")
         << mapping.keysym
         << endl;
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
    // How to retrieve entries from 'opts':
    //     opts["option"].as<type>()
    //
    // To check if the option is unset or wasn't passed:
    //     opts["option"].empty()
    //
    // To check if an option with a default value wasn't passed:
    //     opts["option"].defaulted()

    UnixInputFd kbd_fd(opts.kbdDriverDev);
    KbdInputEvent kbdEvent;
    while(true)
    {
        if(!kbdEvent.read(kbd_fd)) {
            sleep(1);
            continue;
        }

        processKbdEvent(kbdEvent, opts);
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
    } catch(boost::program_options::error& ex) {
        cerr << "Error while parsing program options: " << ex.what()
             << endl << endl
             << "Rerun \"" << myName << " --help\" for usage."
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

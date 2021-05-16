#include "eventref.hpp"
#include <string>

CoreGraphicsRefEvent::CoreGraphicsRefEvent(double timestamp, std::string op, uint64_t tid, uint64_t _event_addr, uint64_t _event_class, uint64_t _event_kind, uint64_t _keycode, uint32_t coreid, std::string procname)
:EventBase(timestamp, EVENTREF_EVENT, op, tid, coreid, procname)
{
    cgs_event_addr = _event_addr;
    cgs_event_class = _event_class;
    cgs_event_kind = _event_kind;
    key_code = _keycode;
}

const char *CoreGraphicsRefEvent::decode_keycode(int keyCode)
{
    switch (keyCode) {
        case 0: return("a");
        case 1: return("s");
        case 2: return("d");
        case 3: return("f");
        case 4: return("h");
        case 5: return("g");
        case 6: return("z");
        case 7: return("x");
        case 8: return("c");
        case 9: return("v");

        case 11: return("b");
        case 12: return("q");
        case 13: return("w");
        case 14: return("e");
        case 15: return("r");
        case 16: return("y");
        case 17: return("t");
        case 18: return("1");
        case 19: return("2");
        case 20: return("3");
        case 21: return("4");
        case 22: return("6");
        case 23: return("5");
        case 24: return("=");
        case 25: return("9");
        case 26: return("7");
        case 27: return("-");
        case 28: return("8");
        case 29: return("0");
        case 30: return("]");
        case 31: return("o");
        case 32: return("u");
        case 33: return("[");
        case 34: return("i");
        case 35: return("p");
        case 36: return("RETURN");
        case 37: return("l");
        case 38: return("j");
        case 39: return("'");
        case 40: return("k");
        case 41: return(";");
        case 42: return("\\");
        case 43: return(",");
        case 44: return("/");
        case 45: return("n");
        case 46: return("m");
        case 47: return(".");
        case 48: return("TAB");
        case 49: return("SPACE");
        case 50: return("`");
        case 51: return("DELETE");
        case 52: return("ENTER");
        case 53: return("ESCAPE");
        
        // some more missing codes abound, reserved I presume, but it would
        // have been helpful for Apple to have a document with them all listed
        
        case 65: return(".");
            
        case 67: return("*");
        
        case 69: return("+");
        
        case 71: return("CLEAR");
        
        case 75: return("/");
        case 76: return("ENTER");   // numberpad on full kbd
        
        case 78: return("-");
        
        case 81: return("=");
        case 82: return("0");
        case 83: return("1");
        case 84: return("2");
        case 85: return("3");
        case 86: return("4");
        case 87: return("5");
        case 88: return("6");
        case 89: return("7");
            
        case 91: return("8");
        case 92: return("9");
        
        case 96: return("F5");
        case 97: return("F6");
        case 98: return("F7");
        case 99: return("F3");
        case 100: return("F8");
        case 101: return("F9");

        case 103: return("F11");

        case 105: return("F13");

        case 107: return("F14");

        case 109: return("F10");

        case 111: return("F12");

        case 113: return("F15");
        case 114: return("HELP");
        case 115: return("HOME");
        case 116: return("PGUP");
        case 117: return("DELETE");  // full keyboard right side numberpad
        case 118: return("F4");
        case 119: return("END");
        case 120: return("F2");
        case 121: return("PGDN");
        case 122: return("F1");
        case 123: return("LEFT");
        case 124: return("RIGHT");
        case 125: return("DOWN");
        case 126: return("UP");

        default:
            return("unknownkey");
    }
}

const char *CoreGraphicsRefEvent::decode_keykind(int keykind)
{
    switch (keykind) {
        case kEventRawKeyDown:
            return "KeyDown";
        case kEventRawKeyRepeat:
            return "KeyRepeat";
        case kEventRawKeyUp:
            return "KeyUp";
        case kEventRawKeyModifiersChanged:
            return "KeyModifiersChanged";
        case kEventHotKeyPressed:
            return "HotKeyPressed";
        case kEventHotKeyReleased:
            return "HotKeyReleased";
        default:
            return "unknonwkey";
    }    
}

void CoreGraphicsRefEvent::decode_event(bool is_verbose, std::ofstream &outfile)
{
    EventBase::decode_event(is_verbose, outfile);
    char event_class_desc[5] = {0};
    strncpy(event_class_desc, (const char *)&cgs_event_class, 4);
    std::reverse(event_class_desc, event_class_desc + 4);
    outfile << "\t" << cgs_event_addr;
    outfile << "\t" << event_class_desc << std::endl;
}

void CoreGraphicsRefEvent::streamout_event(std::ofstream &outfile)
{
    EventBase::streamout_event(outfile);
    char event_class_desc[5] = {0};
    strncpy(event_class_desc, (const char *)&cgs_event_class, 4);
    std::reverse(event_class_desc, event_class_desc + 4);

    outfile << "\t" << cgs_event_addr;
    outfile << "\t" << event_class_desc;
    if (cgs_event_class == kEventClassKeyboard) {
        if (cgs_event_kind == kEventRawKeyDown)
            outfile << "\t" << decode_keycode(key_code);
        outfile << "\t" << decode_keykind(cgs_event_kind);
    }
    outfile << std::endl;
}

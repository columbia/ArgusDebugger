#include "nsappevent.hpp"
NSAppEventEvent::NSAppEventEvent(double timestamp, std::string op, uint64_t tid, bool _begin, uint32_t event_core, std::string procname)
:EventBase(timestamp, NSAPPEVENT_EVENT, op, tid, event_core, procname)
{
	begin = _begin;
}

const char *NSAppEventEvent::decode_event_type(int event_type)
{
    switch (event_type) {
        case NSLeftMouseDown:
            return "LeftMouseDown"; 
        case NSLeftMouseUp:
            return "LeftMouseUp";
        case NSRightMouseDown:
            return "RightMouseDown";
        case NSRightMouseUp:
            return "RingMouseUp";
        case NSMouseMoved:
            return "MouseMoved";
        case NSLeftMouseDragged:
            return "LeftMouseDragged";
        case NSRightMouseDragged:
            return "RightMouseDragged";
        case NSMouseEntered:
            return "MouseEntered";
        case NSMouseExited:
            return "MouseExited";
        case NSKeyDown:
            return "KeyDown";
        case NSKeyUp:
            return "KeyUp";
        case NSFlagsChanged:
            return "FlagsChanged";
        case NSAppKitDefined:
            return "NSAppKitDefined";
        case NSSystemDefined:
            return "SystemDefined";
        case NSApplicationDefined:
            return "NSApplicationDefined";
        case NSPeriodic:
            return "NSPeriodic";
        case NSCursorUpdate:
            return "CursorUpdate";
        case NSScrollWheel:
            return "ScrollWheel";
        case NSTabletPoint:
            return "TabletPoint";
        case NSTabletProximity:
            return "TableProximity";
        case NSOtherMouseDown:
            return "OtherMouseDown";
        case NSOtherMouseUp:
            return "OtherMouseUp";
        case NSOtherMouseDragged:
            return "OtherMouseDragged";
		case NSTypeGesture:
		case NSTypeMagnify:
		case NSTypeSwipe:
		case NSTypeRotate:
		case NSTypeBeginGesture:
		case NSTypeEndGesture:
		case NSTypeSmartMagnify:
		case NSTypeQuickLook:
		case NSTypePressure:
		case NSTyeDirectTouch:
			return "Other_Events";
        default:
            return "Unknown_Event_type";
    }
    
}

const char *NSAppEventEvent::decode_keycode(int keyCode)
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

void NSAppEventEvent::decode_event(bool is_verbose, std::ofstream &outfile)
{
    EventBase::decode_event(is_verbose, outfile);
    if (begin == true)
        outfile << "\tBegin";
    else
        outfile << "\tEnd";
    outfile << std::endl;
}
/*
std::string NSAppEventEvent::decode_event_to_string()
{
    std::string str(decode_event_type(event_class));
    if (event_class == NSKeyDown) {
        str += "\t";
        str += decode_keycode(key_code); 
    }
    return str;
}
*/

void NSAppEventEvent::streamout_event(std::ofstream &outfile)
{
    EventBase::streamout_event(outfile);
    if (begin == true)
        outfile << "\tBegin";
    else
        outfile << "\tEnd";

    outfile << "\t" << std::dec << event_class \
		<< "\t" << decode_event_type(event_class);
    if (event_class == NSKeyDown) 
        outfile << "\t" << decode_keycode(key_code); 
    outfile << std::endl;
}

void NSAppEventEvent::print_event()
{
    LOG_S(INFO) << "0x" << std::right << std::hex << get_group_id();

    if (begin == true)
        LOG_S(INFO) << "UIEvent in 0x" << std::hex << get_group_id() \
            << "\t" << decode_event_type(event_class) << " Begin";
    else
        LOG_S(INFO) << "UIEvent in 0x" << std::hex << get_group_id() \
            << "\t" << decode_event_type(event_class) << " End";

    if (event_class == NSKeyDown) 
        LOG_S(INFO) << "\tkey code =  " << std::dec << key_code \
			<< ",\t" << decode_keycode(key_code); 
}

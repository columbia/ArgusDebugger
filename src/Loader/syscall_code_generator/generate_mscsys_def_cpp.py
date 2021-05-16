#!/usr/bin/python
#pyton version is 2.7.10
import re
import sys
import getopt
import sets

class processor:
    def __init__(self, input_path, output_path):
        self.sys_sw_path = input_path + "/osfmk/mach/syscall_sw.h"
        self.sys_def_path = input_path + "/osfmk/mach/mach_traps.h"
        self.out = output_path
        self.sys_sw = dict()
    
    def get_sys_sw(self):
        infile = open(self.sys_sw_path, "r")
        while 1:
            line = infile.readline()
            if not line:
                break
            if re.search('^kernel_trap', line) == None:
                continue;
            fields = re.split("[(,)]", line)
            #self.sys_sw[fields[1]] = int(fields[2]) * -1
            if re.match("syscall_", fields[1].strip()):
                fields[1] = fields[1].strip().replace("syscall_", "")
            self.sys_sw[int(fields[2]) * -1] = fields[1]
        print self.sys_sw
        infile.close()
        
    def get_sys_define(self, func):
        infile = open(self.sys_def_path, "r")
        lines = []
        func_args = func + "_args{"
        func_trap_args = func + "_trap_args{"
        func__args = func + "_args {"
        func_trap__args = func + "_trap_args {"
        print "search function: ", func
        while 1:
            line = infile.readline()
            if not line:
                break
            if re.search(func, line):
                if re.search(func_args, line) or re.search(func__args, line)\
                or re.search(func_trap_args, line) or re.search(func_trap__args, line):
                    print "Args"
                    lines.append("(");
                    while 1:
                        line = infile.readline()
                        if not line:
                            return
                        if re.search("PAD_ARG_", line):
                            arg = re.search('(?<=\().*?(?=\))', line)
                            try:
                                arg_fields = re.split('[,]', arg.group(0))
                                lines.append(arg_fields[0] + " " + arg_fields[1] + ",")
                            except:
                                print "Not a valid arg line " + line
                        elif re.search("};", line):
                            lines[-1] = lines[-1].rstrip(",")
                            lines.append(");")
                            break;
                        else:
                            lines.append(line.strip().replace(";", ","))
                    
                elif re.search(func + r"\(.*?", line):
                    lines.append(line)
                    if re.search(";", line):
                        break;
                    while 1:
                        line = infile.readline()
                        if not line:
                            return;
                        if re.search(";", line) == None:
                            lines.append(line)
                        else:
                            lines.append(line)
                            break;
                else:
                    continue;
                break;
        infile.close()
        ret = ""
        for line in lines:
            ret += line.strip() 
        print "return ", ret
        return ret

    def process_syscall_def(self):
        self.get_sys_sw()
        out = open(self.out, "w")
        out.write("/* automatically generated with script */\n");
        out.write("#include \"loader.hpp\"\n");
        out.write("namespace LoadData\n{\n");
        out.write("\tconst struct syscall_entry mach_syscall_table[] = {\n")
        begin_comma = False
        for sysnum in self.sys_sw:
            func = self.sys_sw[sysnum]
            lines = self.get_sys_define(func)
            paramlist = re.search('(?<=\().*?(?=\))', lines)
            try:
                params = re.split('[,]', paramlist.group(0))
            except AttributeError:
                    print "No param find for: ", func
                    print "lines return: ", lines
                    continue;
            
            if begin_comma == True:
                out.write(",\n")
            else:
                begin_comma = True
            if re.match("_kernelrpc_", func):
                func = func.replace("_kernelrpc_", "")
            out.write("\t\t{" + str(sysnum) + "," + "\t\"" + func + "\",\n\t{")
            need_comma = False
            for param in params:
                s = re.sub(r"\s", '_', param)
                if need_comma == True:
                    out.write(",")
                    out.write("\"" + s + "\"")
                else:
                    need_comma = True
                    out.write("\"" + s + "\"")
            out.write("}}")
        out.write("\n\t};\n")
        out.write("\tconst uint64_t msc_size = sizeof(mach_syscall_table)/sizeof(struct syscall_entry);\n");
        out.write("}\n");
        out.close()


class Usage(Exception):
    """ -i kernel_source_code_path
        -o output file
    """
    def __init__(self, msg):
        self.msg  = msg

def main(argv = None):
    if argv is None:
        argv = sys.argv
    try:
        try:
            opts, args = getopt.getopt(argv[1:], "i:o:h", ["help"])
        except getopt.error, msg:
            raise Usage(msg)
    except Usage, err:
        print >> sys.stderr, err.msg
        print >> sys.stderr, "for help -h"
        return 2
    infile = None
    outfile = None
    for opt, val in opts:
        if opt in ("-h", "--help"):
            print Usage("").__doc__
            return 1
        if opt == "-i":
            infile = val
        if opt == "-o":
            outfile = val
    
    p = processor(infile, outfile)
    p.process_syscall_def()

if __name__ == "__main__":
    sys.exit(main())

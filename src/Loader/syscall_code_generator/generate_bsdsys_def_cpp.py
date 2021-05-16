#!/usr/bin/python
#pyton version is 2.7.10
import re
import sys
import getopt
def process(file0, file1) :
    infile = open(file0, "r")
    outfile = open(file1, "w")

    outfile.write("/* automatically generated with script */\n");
    outfile.write("#include \"loader.hpp\"\n");
    outfile.write("namespace LoadData\n{\n");
    outfile.write("const struct syscall_entry bsd_syscall_table[] = {\n")

    begin_comma = False;
    all_bsdsys =[]
    while 1:
        line = infile.readline()
        if not line:
            break;

        if re.search('nosys', line):
            continue

        if re.match('^[0-9]', line) == None:
            continue

        fields= re.split('[\s\(]+', line)
        sysnum = fields[0]
        func = re.sub('^[^A-Za-z]*', '', fields[5])
        paramlist = re.search('(?<=\().*?(?=\))', line)
        params = re.split('[,]', paramlist.group(0))
        #print paramlist.group(0)
        #print len(params)
        if sysnum in all_bsdsys:
            print "Repeatd sysnum " + sysnum

        all_bsdsys.append(sysnum);

        if begin_comma == True:
            outfile.write(",\n")
        else:
            begin_comma = True
        outfile.write("\t\t{" + sysnum + "," + "\t\"" + func + "\",\n\t{")
        need_comma = False
        for param in params:
            s = re.sub(r"\s", '_', param)
            if need_comma == True:
                outfile.write(",")
                outfile.write("\"" + s + "\"")
            else:
                need_comma = True
                outfile.write("\"" + s + "\"")
        outfile.write("}}")
    outfile.write("\n\t};\n")
    outfile.write("\tconst uint64_t bsc_size = sizeof(bsd_syscall_table)/sizeof(struct syscall_entry);\n");
    outfile.write("}\n");

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
    process(infile + "/bsd/kern/syscalls.master", outfile)

if __name__ == "__main__":
    sys.exit(main())

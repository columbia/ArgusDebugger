#!/usr/bin/python
#pyton version is 2.7.10
import subprocess
import sys
import getopt
import re

class fetcher:
    def __init__(self, source_path, output_path):
        self.path = source_path + "/BUILD/obj/RELEASE_X86_64/osfmk/RELEASE"
        self.out = open(output_path, "w")
        self.out.write("#include \"loader.hpp\"\n")
        self.out.write("namespace LoadData\n{\n")
        self.out.write("\tconst struct mig_service mig_table[] = {\n")
        self.mig = {}

    def fetch(self):
        p = subprocess.Popen(['/usr/bin/grep', '__DeclareRcvRpc([0-9]\{1,\}', '-r', self.path],stdout = subprocess.PIPE)
        lines = p.stdout
        begin_comma = False;
        for line in lines:
            fields = re.split('[(,)]', line)
            if re.match('\d+',fields[1]):
                self.mig[int(fields[1])] = fields[2]
                if begin_comma == True:
                    self.out.write(",\n")
                self.out.write("\t\t{" + fields[1] + ",\t" + fields[2] + "}")
                begin_comma = True
                #print int(fields[1]), "\t", fields[2]
                #print "0X", "{0:x}".format(int(fields[1])), "\t", fields[2]
        p.stdout.close()
        self.out.write("\n\t};\n")
        self.out.write("\tconst uint64_t mig_size = sizeof(mig_table)/sizeof(struct mig_service);\n")
        self.out.write("}\n")
        #print self.mig
        
        return self.mig

class Usage(Exception):
    """-s kernel_source_code_path -o mig_def.cpp
    """
    def __init__(self, msg):
        self.msg  = msg

def main(argv = None):
    if argv is None:
        argv = sys.argv
    try:
        try:
            opts, args = getopt.getopt(argv[1:], "s:o:h", ["help"])
        except getopt.error, msg:
            raise Usage(msg)
    except Usage, err:
        print >> sys.stderr, err.msg
        print >> sys.stderr, "for help --help"
        return 2
    
    sourced = None
    outfile = None
    for opt, val in opts:
        if opt in ("-h", "--help"):
            print Usage("").__doc__
            return 1
        if opt == "-s":
            sourced = val
        if opt == "-o":
            outfile = val

    if sourced == None or outfile == None:
        print Usage(""). __doc__
        return 1

    ks_fetcher = fetcher(sourced, outfile)
    ks_fetcher.fetch()

if __name__ == "__main__":
    sys.exit(main())

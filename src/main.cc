#include "ArgumentParser.h"
#include "Formatter.h"
#include "LAPCFGParser.h"
#include "Mapping.h"
#include "Timer.h"

#include <boost/algorithm/string.hpp>
#include <iostream>
#include <cstdio>
#include <cmath>
#include <fstream>
#include <memory>
#include <string>
#include <vector>

using namespace std;
using namespace AHCParser;

unique_ptr<ArgumentParser> parseArgs(int argc, char * argv[]) {
    unique_ptr<ArgumentParser> ap(new ArgumentParser("ahcparse -model <model-prefix> [options]"));

    ap->addSwitchArgument("help", "print this manual and exit");
    
    ap->addStringArgument("model", "", "prefix of model path", true);
    
    ap->addStringArgument("input", "/dev/stdin", "input file", false);
    ap->addStringArgument("output", "/dev/stdout", "output file", false);

    ap->addRealArgument("prune-threshold", 1e-5, "coarse-to-fine pruning threshold", false);
    ap->addRealArgument("smooth-unklex", 1e-10, "smoothing strength using UNK lexicon", false);

    ap->addSwitchArgument("add-root-tag", "add ROOT tag into output tree");

    bool ret = ap->parseArgs(argc, argv);

    if (ap->getSwitch("help")) {
        ap->printUsage();
        exit(0);
    }

    if (!ret) {
        exit(0);
    }

    return ap;
}

int main(int argc, char * argv[]) {
    unique_ptr<ArgumentParser> ap = ::parseArgs(argc, argv);
    
    string ifname = ap->getString("input");
    ifstream ifs(ifname);
    if (!ifs.is_open()) {
        cerr << "ERROR: cannot open file to read: " << ifname << endl;
        return 1;
    }

    string ofname = ap->getString("output");
    ofstream ofs(ofname);
    if (!ofs.is_open()) {
        cerr << "ERROR: cannot open file to write: " << ofname << endl;
        return 1;
    }

    shared_ptr<LAPCFGParser> parser = LAPCFGParser::loadFromBerkeleyDump(ap->getString("model"));
    parser->setPruningThreshold(ap->getReal("prune-threshold"));
    parser->setUNKLexiconSmoothing(ap->getReal("smooth-unklex"));

    Timer timer;

    cerr << "Ready" << endl;

    string line;
    int total_lines = 0;
    int total_words = 0;
    
    while (getline(ifs, line)) {
        boost::trim(line);
        vector<string> ls;
        if (!line.empty()) {
            boost::split(ls, line, boost::is_space(), boost::algorithm::token_compress_on);
        }
        
        ++total_lines;
        total_words += ls.size();
        
        cerr << "Input " << total_lines << ":";
        for (const string & s : ls) cerr << " " << s;
        cerr << endl;

        timer.start();
        shared_ptr<Tree<string> > parse = parser->parse(ls);
        double lap = timer.stop();

        string repr = Formatter::ToPennTreeBank(*parse, ap->getSwitch("add-root-tag"));
        cerr << "  Parse: " << repr << endl;
        fprintf(stderr, "  Time: %.3fs\n", lap);
        ofs << repr << endl;
    }

    cerr << endl;
    fprintf(stderr, "Parsed %d sentences, %d words.\n", total_lines, total_words);
    fprintf(stderr, "Total parsing time: %.3fs.\n", timer.elapsed());

    return 0;
}


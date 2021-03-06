/* 
 * File:   main.cpp
 * Author: Dennis Roberts
 *
 * A simple program for extracting phylogenetic trees from tree files.
 */

#include <cstdlib>
#include <iostream>
#include <fstream>

#include "boost/program_options.hpp"

#include "ncl/nxsmultiformat.h"

#include "tree_info.h"
#include "tree_parser.h"

using namespace std;

namespace po = boost::program_options;

// Static function declarations.
static void describe_options();

// The supported program options.
static po::options_description desc("Allowed options");
static bool is_valid_format(string &format);
static void display_valid_formats();
static void extract_trees(string &input, string &format, string &prefix);
static void parse_input(string &input, tree_parser &parser);
static void spit_newick(const string &contents, const string &filename);

/**
 * Attempts to parse the standard input as a trees file.  The format is specified using the -f command-line option.
 */
int main(int argc, char** argv) {

    // Parse the command-line options.
    describe_options();
    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    // Display the help and exit if help was requested.
    if (vm.count("help")) {
        cout << desc << endl;
        return 0;
    }

    // Validate the input file.
    if (!vm.count("input")) {
        cerr << "required option, --input, missing" <<endl;
        cerr << desc << endl;
        return 1;
    }
    string input = vm["input"].as<string>();

    // Validate the requested input format.
    if (!vm.count("format")) {
        cerr << "required option, --format, missing" << endl;
        cerr << desc << endl;
        return 1;
    }
    string format = vm["format"].as<string>();
    if (!is_valid_format(format)) {
        cerr << "invalid input format: " << endl << endl << "valid formats:" << endl;
        display_valid_formats();
        return 1;
    }

    // Determine the tree name prefix to use.
    string prefix = vm.count("prefix") ? vm["prefix"].as<string>() : "tree";

    // Attempt to extract trees from the data passed to us in the standard input stream.
    extract_trees(input, format, prefix);

    return 0;
}

/**
 * Describes the command-line options accepted by the program.
 */
static void describe_options() {
    desc.add_options()
            ("help,h", "display the help message")
            ("input,i", po::value<string> (), "specify the path to the input file")
            ("format,f", po::value<string> (), "specify the format of the input data")
            ("prefix,p", po::value<string> (), "the prefix to use for tree names")
            ;
}

/**
 * Determines if the requested format name is supported by the Nexus Class Library.
 * 
 * @param format the format name.
 * @return true if the requested format name is supported.
 */
static bool is_valid_format(string &format) {
    vector<string> format_names = MultiFormatReader::getFormatNames();
    for (unsigned i = 0; i < format_names.size(); i++) {
        if (format_names[i] == format) {
            return true;
        }
    }
    return false;
}

/**
 * Displays the list of input formats that are supported by the Nexus Class Library
 */
static void display_valid_formats() {
    vector<string> format_names = MultiFormatReader::getFormatNames();
    for (unsigned i = 0; i < format_names.size(); i++) {
        cerr << "\t" << format_names[i] << endl;
    }
}

/**
 * An exception that can be thrown when no trees are found in a file.
 */
class no_trees_exception: public exception {
    virtual const char *what() const throw() {
        return "the file was parsed successfully, but no trees were found";
    }
} no_trees;

/**
 * Extracts trees from the input passed to us in the standard input stream.
 * 
 * @param input the path to the input file.
 * @param format the expected format of the input file.
 * @param prefix the prefix to use for trees.
 */
static void extract_trees(string &input, string &format, string &prefix) {
    tree_parser parser(format);
    parse_input(input, parser);
    vector<tree_info> trees = parser.get_trees();
    if (trees.size() == 0) {
        throw no_trees;
    }
    for (unsigned i = 0; i < trees.size(); i++) {
        tree_info tree = trees[i];
        string name = tree.get_name();
        if (name.length() == 0) {
            name = prefix + "_" + boost::lexical_cast<string>(i);
        }
        spit_newick(tree.get_newick(), name + ".tre");
    }
}

/**
 * Writes a string to a file, appending a semicolon to the end of the string.
 * 
 * @param contents the contents of the file.
 * @param filename the name of the file.
 */
static void spit_newick(const string &contents, const string &filename) {
    ofstream out;
    out.open(filename.c_str());
    out << contents << ";";
    out.close();
}

/**
 * Parses the input passed to us in the standard input stream.
 * 
 * @param input the path to the input file.
 * @param parser the tree parser.
 */
static void parse_input(string &input, tree_parser &parser) {
    try {
        ifstream in(input.c_str());
        parser.parse(in);
    }
    catch (NxsException e) {
        cerr << e.nxs_what() << endl;
        exit(1);
    }
    catch (exception e) {
        cerr << e.what() << endl;
        exit(1);
    }
    catch (...) {
        cerr << "unknown exception occurred while parsing tree" << endl;
    }
}

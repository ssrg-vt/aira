/*
 * Implementation details of the ProgramOptions class, which wraps Boost's
 * Program Options library.
 */

#include "program_options.h"

namespace po = boost::program_options;

/*
 * Default constructor for the ProgramOptions class.  Takes in command-line
 * arguments and parses them using Boost's Program Options library.
 */
ProgramOptions::ProgramOptions(int argc, char** argv) : desc("Allowed options")
{
	//Add options and their description - new arguments can be added here
	desc.add_options()
		("help", "produce help message")
		("checkAST", "run the internal AST consistency tests.  This can be expensive compared to building the AST")
		("generateCallGraph", "generate DOT file representing the call graph")
		("generateAST", "generate DOT file representing the abstract syntax tree (AST)")
		("generateFullAST", "generate a PDF representing the entire AST")
		("mpiHeader", po::value<std::string>()->default_value("mpi.h"), "set the MPI library header (by default is \"mpi.h\")")
	;

	//Set up the variable map and parse the command-line arguments
	po::store(po::command_line_parser(argc, argv).options(desc).allow_unregistered().run(), vm);
	po::notify(vm);
}

/*
 * Returns whether or not the help option was specified.
 */
bool ProgramOptions::printHelp() {
	if(vm.count("help"))
		return true;
	else
		return false;
}

/*
 * Returns whether or not the user requested AST consistency checking.
 */
bool ProgramOptions::runASTConsistencyTests() {
	if(vm.count("checkAST"))	
		return true;
	else
		return false;
}

/*
 * Returns whether or not the user requested the call graph.
 */
bool ProgramOptions::printCallGraph() {
	if(vm.count("generateCallGraph"))
		return true;
	else
		return false;
}

/*
 * Returns whether or not the user requested the AST graph.
 */
bool ProgramOptions::printAST() {
	if(vm.count("generateAST"))
		return true;
	else
		return false;
}

/*
 * Returns whether or not the user requested the full AST graph.
 */
bool ProgramOptions::printFullAST() {
	if(vm.count("generateFullAST"))
		return true;
	else
		return false;
}

/*
 * Return either the default MPI Header ("<mpi.h>") or a user specified MPI
 * header
 */
std::string ProgramOptions::getMPIHeader() {
	if(vm.count("mpiHeader"))
		return vm["mpiHeader"].as<std::string>();
	else
		return "mpi.h";
}

/*
 * pragma_parser.cpp
 *
 * Pragma-parsing utilities to help with popcorn pragmas.
 *
 * Created on: Apr 17, 2013
 * Author: Rob Lyerly <rlyerly@vt.edu>
 */
#include "rose.h"

#include <sstream>
#include <algorithm>
#include "pragma_handling.h"

/*
 * Constructs the PragmaParser object.
 */
PragmaParser::PragmaParser(SgPragmaDeclaration* p_pragmaDeclaration) :
	pragmaDeclaration(p_pragmaDeclaration),
	pragma(""),
	pragmaKeyword(""),
	pragmaType(""),
	names()
{
	ROSE_ASSERT(pragmaDeclaration);
	pragma = pragmaDeclaration->get_pragma()->get_pragma();

	//Get the keyword of pragma
	int i = 0;
	istringstream ss(pragma);
	ss >> pragmaKeyword;

	if(isPopcornPragma())
	{
		//Get the pragma type
		istringstream typeStream;
		size_t location = pragma.find('(');
		if(location == string::npos)
		{
			pragmaType = PragmaBuilder::type2string(PARTITIONED);
			return;
		}

		string type = pragma.substr(0, location);
		typeStream.str(type);
		typeStream >> type; //get rid of keyword
		while(typeStream)
		{
			typeStream >> type;
			type = trim(type);
			if(type == "")
				continue;
			pragmaType += type + " ";
			type.clear();
		}
		pragmaType = trim(pragmaType);

		//Split into names
		istringstream nameStream;
		char removal[] = "() ";
		string name = pragma.substr(location, string::npos);
		for(i = 0; i < 3; i++)
			name.erase(std::remove(name.begin(), name.end(),
					removal[i]), name.end());
		name = trim(name);
		replace(name.begin(), name.end(), ',', ' ');
		nameStream.str(name);
		while(nameStream)
		{
			nameStream >> name;
			name = trim(name);
			if(name == "")
				continue;
			names.insert(name);
		}
	}
}

/*
 * Returns the pragma declaration encapsulated by this object.
 */
SgPragmaDeclaration* PragmaParser::getPragmaDeclaration()
{
	return pragmaDeclaration;
}

/*
 * Returns true if this is a popcorn pragma, or false otherwise.
 */
bool PragmaParser::isPopcornPragma()
{
	if(pragmaKeyword == popcornKeyword)
		return true;
	else
		return false;
}

/*
 * Returns the keyword of this pragma
 */
string PragmaParser::getPragmaKeyword()
{
	return pragmaKeyword;
}

/*
 * Returns the type of this pragma
 */
enum pragmaType PragmaParser::getPragmaType()
{
	if(!isPopcornPragma())
		throw string("Cannot get type because this is an unknown pragma");
	return PragmaBuilder::string2type(pragmaType);
}

/*
 * Get the names contained in the pragma, if there are any
 */
set<string> PragmaParser::getNames()
{
	if(!isPopcornPragma())
		throw string("Cannot get names because this is an unknown pragma");
	return names;
}

string& PragmaParser::trim(string &s)
{
	return ltrim(rtrim(s));
}

string& PragmaParser::ltrim(string &s)
{
	s.erase(s.begin(), std::find_if(s.begin(), s.end(),
			std::not1(std::ptr_fun<int, int>(std::isspace))));
	return s;
}

string& PragmaParser::rtrim(string &s)
{
	s.erase(std::find_if(s.rbegin(), s.rend(),
			std::not1(std::ptr_fun<int, int>(std::isspace))).base(),
			s.end());
	return s;
}

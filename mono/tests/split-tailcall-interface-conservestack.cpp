// This program is used to split tailcall-interface-conservestack.il into separate tests.
//
// Algorithm is just to split on special inserted markers.
//
// mkdir tailcall/interface-conservestack
// rm tailcall-interface-conservestack-*
// git rm tailcall-interface-conservestack-*
// g++ split-tailcall-interface-conservestack.cpp && ./a.out < tailcall-interface-conservestack.il
// git add tailcall/interface-conservestack/*.il
//
// Note This is valid C++98 for use with older compilers, where C++11 would be desirable.
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <assert.h>
using namespace std;

typedef vector<string> strings_t;
struct test_t
{
	strings_t content;
};
typedef vector<test_t> tests_t;

int main()
{
	string line;
	strings_t prefix;
	tests_t tests;
	test_t test_dummy;
	test_t *test = &test_dummy;
	strings_t suffix;

	while (getline(cin, line) && line != "// test-split-prefix do not remove or edit this line")
		prefix.push_back(line);

	while (getline (cin, line))
	{
		if (!line.length()) // tests are delimited by empty lines
		{
			tests.resize(tests.size() + 1);
			test = &tests.back();
			assert(getline(cin, line));
			if (line == "// test-split-suffix do not remove or edit this line")
				break;
		}
		test->content.push_back(line);
	}
	while (getline (cin, line))
		suffix.push_back(line);

	int i = 0;

	system("mkdir tailcall/interface-conservestack");

	for (tests_t::iterator test = tests.begin(); test != tests.end(); ++test)
	{
		char buffer[99];
		sprintf(buffer, "%d", ++i)'
		ofstream output("tailcall/interface-conservestack/" + buffer + ".il");
		for (strings_t::const_iterator s = prefix.begin(); s != prefix.end(); ++s)
			output << *s << endl;
		output << endl;
		for (strings_t::const_iterator s = test->content.begin(); s != test->content.end(); ++s)
			output << *s << endl;
		output << endl;
		for (strings_t::const_iterator s = suffix.begin(); s != suffix.end(); ++s)
			output << *s << endl;
	}
}

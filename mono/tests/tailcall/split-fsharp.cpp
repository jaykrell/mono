/*
This program is used to split fsharp-deeptail.il into separate tests.

Algorithm is just to split on the newline delimited ldstr opcodes
within the main@ function, and generate file names from the ldstr,
replacing special characters with underscores.

rm fsharp-deeptail-*
g++ split-fsharp.cpp -std=c++11 && ./a.out < fsharp-deeptail.il
*/
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <assert.h>
#include <string.h>
using namespace std;

int main ()
{
	struct test_t
	{
		string name;
		vector<string> content;
	};
	string line;
	vector<string> prefix;
	vector<test_t> tests;
	test_t test_dummy;
	test_t *test = &test_dummy;
	vector<string> suffix;

	while (getline (cin, line))
	{
		prefix.push_back (line);
		if (line == ".entrypoint")
			break;
	}

	string marker_ldstr = "ldstr \"";	// start of each test, and contains name
	string marker_ldc_i4_0 = "ldc.i4.0";	// start of suffix

	while (getline (cin, line))
	{
		// tests are delimited by empty lines
		if (line == "")
		{
			tests.resize (tests.size () + 1);
			test = &tests.back ();
			assert (getline (cin, line));
			// tests start with ldstr
			//printf("%s\n", line.c_str());
			bool ldstr = line.length () >= marker_ldstr.length () && memcmp(line.c_str (), marker_ldstr.c_str (), marker_ldstr.length ()) == 0;
			bool ldc_i4_0 = line.length () >= marker_ldc_i4_0.length () && memcmp(line.c_str (), marker_ldc_i4_0.c_str (), marker_ldc_i4_0.length ()) == 0;
			assert (ldstr || ldc_i4_0);
			if (ldc_i4_0)
			{
				suffix.push_back (line);
				break;
			}
			string name1 = &line [marker_ldstr.length ()];
			*strchr ((char*)name1.c_str (), '"') = 0; // truncate at quote
			test->name = name1.c_str ();
			for (auto& c: test->name)
			{
				if (strchr("<>", c))
					c = '_';
			}
			//printf("%s\n", test->name.c_str());
		}
		test->content.push_back (line);
	}
	while (getline (cin, line))
	{
		suffix.push_back (line);
	}
	for (auto& t: tests)
	{
		if (t.name == "")
			continue;
		//printf("%s\n", t.name.c_str());
		ofstream output("fsharp-deeptail-" + t.name + ".il");
		for (auto &a: prefix)
			output << a.c_str () << endl;
		output << endl;
		for (auto &a: t.content)
			output << a.c_str () << endl;
		output << endl;
		for (auto &a: suffix)
			output << a.c_str () << endl;

	}
}

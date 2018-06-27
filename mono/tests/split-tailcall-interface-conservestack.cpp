/*
This program is used to split tailcall-interface-conservestack.il into separate tests.

Algorithm is just to split on special inserted markers.

rm tailcall-interface-conservestack-*
g++ split-tailcall-interface-conservestack.cpp -std=c++11 && ./a.out < tailcall-interface-conservestack.il
*/
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <assert.h>
using namespace std;

int main()
{
	struct test_t
	{
		vector<string> content;
	};
	string line;
	vector<string> prefix;
	vector<test_t> tests;
	test_t test_dummy;
	test_t *test = &test_dummy;
	vector<string> suffix;

	while (getline (cin, line) && line != "// test-split-prefix do not remove or edit this line")
		prefix.push_back(line);

	while (getline (cin, line))
	{
		if (line == "") // tests are delimited by empty lines
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
	for (auto& t: tests)
	{
		ofstream output("tailcall-interface-conservestack-" + to_string(++i) + ".il");
		for (auto &a: prefix)
			output << a << endl;
		output << endl;
		for (auto &a: t.content)
			output << a << endl;
		output << endl;
		for (auto &a: suffix)
			output << a << endl;
	}
}

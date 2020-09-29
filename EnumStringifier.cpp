#include <filesystem>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

using namespace std::filesystem;
using namespace std;

#include "Example.hpp"

void GenerateOutput(const string& Path, vector<string>& EnumMembers, string& EnumName) {
	if (exists(Path) && is_regular_file(Path)) {
		cout << "\x1B[33m[INFO] Output file already exists. Overwriting...\033[0m\n";
		remove(Path);
	}

	ofstream OutputFile = ofstream(Path);
	cout << "\x1B[92m[INFO] Started writing the output to a file...\033[0m\n";

	auto CreateFunction = [&](const std::string& Variable) {
		OutputFile << "std::string " << EnumName << "ToString(" << Variable << " Member) {\n";
		OutputFile << "    switch(Member) {\n";

		for (string& Member : EnumMembers)
			OutputFile << "        case " << EnumName << "::" << Member << ": return \"" << EnumName << ":" << Member << "\";\n";

		OutputFile << "        default: return \"null\";\n";
		OutputFile << "    }\n";
		OutputFile << "}\n";
	};

	auto OverloadShift = [&](const std::string& Variable) {
		OutputFile << "std::ostream& operator<<(std::ostream& Stream, " << Variable << " Member) {\n";
		OutputFile << "    return Stream << " << EnumName << "ToString(Member);\n";
		OutputFile << "}\n";
	};

	OutputFile << "#include <string>\n";
	OutputFile << "#include <iostream>\n\n";

	const string ReferenceVar = "const " + EnumName + "&";
	const string PointerVar = EnumName + "*";
	
	CreateFunction(ReferenceVar);
	CreateFunction(PointerVar);

	OverloadShift(ReferenceVar);
	OverloadShift(PointerVar);

	cout << "\x1B[92m[INFO] File successfully written. Stopping...\033[0m\n";
	OutputFile.close();
}

void AnalyzeEnumFile(const path& Path, vector<string>& EnumMembers, string& EnumName) {
	vector<string> Tokens;
	{
		// Split the file into words, separated by spaces.
		// Encapsulated the code into a block so that `EnumFile` and `Line`
		// will go out of scope after they are no longer needed.
		fstream EnumFile = fstream(Path);
		string Line;
		while (getline(EnumFile, Line)) {
			stringstream LineStream = stringstream(Line);

			for (string Token; LineStream >> Token;)
				Tokens.push_back(Token);
		}
		EnumFile.close();
	}

	unsigned int Beginning = 0;
	unsigned int End = 0;
	bool EnumBlock = false;

	// Find out what the enum is called, at which word it starts, and
	// at which it ends. This will not work well if you have more than
	// one enum.
	for (unsigned int i = 0; i < Tokens.size(); i++) {
		if (Tokens[i] == "enum") {
			if (Tokens[i + 1] == "class") EnumName = Tokens[i + 2];
			else EnumName = Tokens[i + 1];
			EnumBlock = true;
		} else if (Tokens[i] == "{") {
			// Only set `Beginning` if the identifier `enum` was found,
			// otherwise if there's something else besides the enum itself
			// in the source file, the program will go crazy.
			if(EnumBlock) Beginning = i;
		} else if (Tokens[i] == "};") {
			// Same thing here.
			if(EnumBlock) End = i;
			break;
		}
	}
	cout << "\x1B[92m[INFO] Found enum " << EnumName << "..." << "\033[0m\n";

	for (unsigned int i = Beginning + 1; i < End; i++) {
		if (Tokens[i] == "=") {
			i = i + 1; // Ignore the value assigned to a member, we don't actually need it.
			continue;
		}
		EnumMembers.push_back(Tokens[i]);
	}

	// Remove any remaining commas in the members.
	for (string& Member : EnumMembers)
		Member.erase(remove(Member.begin(), Member.end(), ','), Member.end());

	if (EnumMembers.size() == 0) {
		cout << "\x1B[91m[ERROR] Couldn't find any enum members.\033[0m\n";
		cout << "\x1B[91m[ERROR] Make sure your file is valid.\033[0m\n";
	} else {
		cout << "\x1B[92m[INFO] Found a total of " << EnumMembers.size() << " enum members...\033[0m\n";
	}
}

int main(int argc, char* argv[]) {
	if (argc < 2) {
		cout << "\x1B[91m[ERROR] You haven't specified a file to open.\033[0m\n";
		return -1;
	}

	path Path = argv[1];
	if (!exists(Path) || !is_regular_file(Path)) {
		cout << "\x1B[91m[ERROR] The path specified is invalid.\033[0m\n";
		return -1;
	}
	string OutputPath = Path.parent_path().string() + "\\enum_output.hpp";

	vector<string> EnumMembers; string EnumName;
	AnalyzeEnumFile(Path, EnumMembers, EnumName);
	GenerateOutput(OutputPath, EnumMembers, EnumName);

	cout << "\x1B[92m[INFO] Done! File saved to '" << OutputPath << "'.\033[0m\n";
	return 0;
}
#include <string_view>
#include <filesystem>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <thread>
#include <random>

// Note: Considering this program only uses the standard library,
// I have decided to use the std namespace for better code
// readability.
using namespace std;
namespace fs = std::filesystem;

// This define enables console logging, shouldn't be enabled in
// most circumstances.
#define PROGRAM_DEBUG_MODE

// This solution is pretty clever considering `constexpr` if
// statements get resolved at compile-time AFAIK, and it makes
// the code look pretty clean.
#ifdef PROGRAM_DEBUG_MODE
#define DEBUG_ONLY if constexpr(1)
#else
#define DEBUG_ONLY if constexpr(0)
#endif

// Not really necessary, but why not.
#ifdef _WINDOWS
#undef CLEAR_CONSOLE
#define CLEAR_CONSOLE system("cls")
#endif

#ifdef __unix__
#undef CLEAR_CONSOLE
#define CLEAR_CONSOLE system("clear")
#endif

// Class declarations
// Note: Local copies are preffered over pointers to
// vector elements in cases in which the value is also part
// of the current class and it is not a heavy object.

// Note: I am also using structs instead of classes for
// containers, and classes for actual implementations.
// Using `public` in structs is deliberate, in order to
// (hopefully) make their purpose clear.

template <typename T>
struct VElementPtr { public:
	vector<T>* list = nullptr;
	int position = -1;

	inline T* get() const { if(null()) return nullptr; return &(*list)[position]; }
	inline bool null() const { return (position == -1 || list == nullptr); }
	inline bool equals(const VElementPtr<T>& other) const { return (this->position == other.position); }
	inline bool operator==(const VElementPtr<T>& other) const { return equals(other); }
};

struct Block { public:
	VElementPtr<Block> parent;
	string_view nmspace;
	unsigned start = 0;
	unsigned end = 0;
};

struct Token { public:
	VElementPtr<Block> block;
	string_view value;
	unsigned start = 0;
	unsigned end = 0;
};

struct Namespace { public:
	VElementPtr<Namespace> parent;
	string_view name;
	unsigned start = 0;
	unsigned end = 0;
	bool is_anon = false;
};

struct Enumerated { public:
	VElementPtr<Block> block;
	string_view name;
	Namespace nmspace;
	unsigned start = 0;
	unsigned end = 0;
	bool is_class = false;
};

// Global variable declarations
vector<Enumerated> Enums;
vector<Namespace> Namespaces;
vector<Token> Tokens;
vector<Block> Blocks;
string Source;

// Actual implementation

// This class cleans the source code of unnecessary spaces
// to make life easier for the scanner.
class Sanitizer { public:
	inline void RemoveCharacter(const char& Character) {
		Source.erase(remove(Source.begin(), Source.end(), Character), Source.end());
	}

	inline void RemoveConsecutive(const char& Character) {
		for(int i = 1; i < Source.size(); ) {
			if(Source[i] != Character) { i++; continue; }
			if(Source[i] == Source[i - 1]) { Source.erase(i, 1); }
			else { i++; }
		}
	}

	void SanitizeSource() {
		RemoveCharacter('\v'); RemoveCharacter('\t');

		for(int i = 0; i < Source.size() - 1; i++) {
			bool SkipLine = false;
			if(Source[i] == '#') SkipLine = true;
			if(Source[i] == '/' && Source[i + 1] == '/') SkipLine = true;
			if(!SkipLine) continue;

			unsigned Start = i;
			while(Source[i] != '\n') i++;
			Source.erase(Start, (i + 1) - Start);
			i = Start - 1;
		}

		// Remove multiple line comments
		for(int i = 0; i < Source.size() - 1; i++) {
			if(Source[i] == '/' && Source[i + 1] == '*') {
				unsigned Start = i;
				while(i != (Source.size() - 2)) { if(Source[i] == '*' && Source[i + 1] == '\\') break; i++; }
				Source.erase(Start, (i + 1) - Start);
				i = Start - 1;
			}
		}

		RemoveConsecutive(' '); RemoveConsecutive('\n');
	}
};

class Scanner { public:
	VElementPtr<Namespace> CurrentNamespace;
	VElementPtr<Block> CurrentBlock;
	unsigned Current = 0;
	char End = '\0';

	inline bool IsAtEnd() { return Current >= Source.size(); }
	inline bool IsAlpha(char& Character) { return (Character >= 'a' && Character <= 'z') || (Character >= 'A' && Character <= 'Z') || Character == '_'; }
	inline char* Peek() { if(IsAtEnd()) return &End; return &Source[Current]; }
	inline char* PeekNext() { if(Current + 1 >= Source.size()) return &End; return &Source[Current + 1];}
	inline char* Advance() { Current++; return &Source[Current - 1]; }

	inline void BeginBlock() {
		Block FoundBlock{};
		FoundBlock.nmspace = "";
		FoundBlock.start = Current;
		FoundBlock.parent.list = &Blocks;
		if(!CurrentBlock.null()) { FoundBlock.parent.position = CurrentBlock.position; }
		Blocks.push_back(FoundBlock);
		CurrentBlock.list = &Blocks;
		CurrentBlock.position = (Blocks.size() - 1);
	}

	inline void EndBlock() {
		if(CurrentBlock.null()) { cout << "Error - Found unexpected '}'.\n"; return; }
		CurrentBlock.get()->end = Current;
		CurrentBlock.position = CurrentBlock.get()->parent.position;
	}

	inline void Identifier() {
		unsigned Start = Current;
		while(IsAlpha(*Peek())) Advance();
		unsigned End = Current;

		Token FoundToken{};
		FoundToken.start = Start;
		FoundToken.end = End;
		FoundToken.value = Source;
		FoundToken.value.remove_prefix(Start - 1);
		FoundToken.value.remove_suffix(Source.size() - End);
		FoundToken.block = VElementPtr<Block>();
		Tokens.push_back(FoundToken);
	}

	inline void ScanCharacter() {
		char Character = *Advance();
		switch(Character) {
			case '{': BeginBlock(); break;
			case '}': EndBlock(); break;
			default: if(IsAlpha(Character)) Identifier(); break;
		}
	}

	void ScanSource() {
		while(!IsAtEnd()) { ScanCharacter(); }
	}
};

// class Scanner { public:
// 	VElementPtr<Namespace> CurrentNamespace;
// 	VElementPtr<Block> CurrentBlock;
// 	unsigned Current = 0;
// 	unsigned Line = 0;
// 	char End = '\0';

// 	inline bool IsAtEnd() { return Current >= Source.size(); }
// 	inline bool IsAlpha(char& Character) { return (Character >= 'a' && Character <= 'z') || (Character >= 'A' && Character <= 'Z') || Character == '_'; }
// 	inline bool IsNumeric(char& Character) { return Character >= '0' && Character <= '9'; }
// 	inline char* Peek() { if (IsAtEnd()) return &End; return &Source[Current]; }
// 	inline char* PeekNext() { if (Current + 1 >= Source.size()) return &End; return &Source[Current + 1]; }
// 	inline char* Advance() { Current++; return &Source[Current - 1]; }

// 	void Identifier() {
// 		unsigned Start = Current - 1;
// 		while (IsAlpha(*Peek())) Advance();
// 		Token FoundToken;
// 		FoundToken.block.position = CurrentBlock.position;
// 		FoundToken.block.list = &Blocks;
// 		FoundToken.start = Start;
// 		FoundToken.end = Current;
// 		FoundToken.value = Source;
// 		FoundToken.value.remove_prefix(Start);
// 		FoundToken.value.remove_suffix(Source.size() - Current);
// 		Tokens.push_back(FoundToken);

// 		if(FoundToken.value.compare("namespace") == 0) { BeginNamespace(); }
// 	}

// 	void BeginNamespace() {
// 		Namespace FoundNamespace;
// 		FoundNamespace.name.list = &Tokens;
// 		FoundNamespace.parent.list = &Namespaces;
// 		FoundNamespace.parent.position = CurrentNamespace.position;
// 		FoundNamespace.start = Current;
		
// 		while(*Peek() == ' ') Current++;

// 		if(*Peek() == '{') {
// 			Advance();
// 			FoundNamespace.is_anon = true;
// 			FoundNamespace.name.position = -1;
// 			DEBUG_ONLY cout << "Debug - Found anonymous namespace.\n";
// 		} else {
// 			while(*Peek() == ' ') { Current++; } Identifier();
// 			FoundNamespace.is_anon = false;
// 			FoundNamespace.name.position = (Tokens.size() - 1);
// 			DEBUG_ONLY cout << "Debug - Found namespace '" << FoundNamespace.name.get()->value << "'.\n";
// 		}

// 		Namespaces.push_back(FoundNamespace);
// 		CurrentNamespace.position = (Namespaces.size() - 1);
// 		CurrentNamespace.list = &Namespaces;
// 	}

// 	void TryEndNamespace() {
// 		if(CurrentNamespace.null()) return;
// 		DEBUG_ONLY if(!CurrentNamespace.get()->name.null()) cout << "Debug - Ended namespace '" << CurrentNamespace.get()->name.get()->value << "'.\n";
// 		CurrentNamespace.get()->end = Current;
// 		CurrentNamespace.position = CurrentNamespace.get()->parent.position;
// 	}

// 	void BeginBlock() {
// 		Block FoundBlock;
// 		FoundBlock.parent_namespace.position = CurrentNamespace.position;
// 		FoundBlock.parent_namespace.list = &Namespaces;
// 		FoundBlock.parent.position = CurrentBlock.position;
// 		FoundBlock.parent.list = &Blocks;
// 		FoundBlock.start = Current;
// 		Blocks.push_back(FoundBlock);
// 		CurrentBlock.list = &Blocks;
// 		CurrentBlock.position = (Blocks.size() - 1);
// 	}

// 	void EndBlock() {
// 		if (CurrentBlock.null()) { cout << "Error - Unexpected '}' found at line " << Line << ".\n"; return; }
// 		TryEndNamespace();
// 		CurrentBlock.get()->end = Current;
// 		CurrentBlock.position = CurrentBlock.get()->parent.position;
// 	}

// 	void SkipLine() {
// 		while (!IsAtEnd()) {
// 			char Character = *Advance();
// 			if (Character == '\n') { Line++; return; }
// 		}
// 	}

// 	inline void ScanCharacter() {
// 		char Character = *Advance();
// 		switch (Character) {
// 		case '/': if (*Peek() == '/') SkipLine(); break;
// 		case '#': SkipLine(); break;
// 		case '\n': Line++; break;
// 		case '{': BeginBlock(); break;
// 		case '}': EndBlock(); break;
// 		default: if (IsAlpha(Character)) Identifier(); break;
// 		}
// 	}

// 	void ScanSource() {
// 		DEBUG_ONLY "Debug - Scanner started.\n";
// 		CurrentNamespace.list = &Namespaces;
// 		CurrentBlock.list = &Blocks;
// 		CurrentNamespace.position = -1;
// 		CurrentBlock.position = -1;
// 		while (!IsAtEnd()) {
// 			ScanCharacter();
// 		}
// 		DEBUG_ONLY "Debug - Scanner finished.\n\n";
// 	}
// };

// class Parser { public:
// 	Enumerated CurrentEnum;
// 	unsigned Current = 0;

// 	inline bool IsAtEnd() { return Current >= Tokens.size(); }
// 	inline bool IsAlpha(char& Character) { return (Character >= 'a' && Character <= 'z') || (Character >= 'A' && Character <= 'Z') || Character == '_'; }
// 	inline Token* Advance() { Current++; return &Tokens[Current - 1]; }

// 	void Enumerator() {
// 		CurrentEnum = Enumerated();
// 		string_view Name;
// 		unsigned Start = Current - 1;
// 		unsigned Consumed = 0;

// 		CurrentEnum.name.list = &Tokens;

// 		if(!Tokens[Start].block.get()->parent_namespace.null()) {
// 			CurrentEnum.nmspace.position = Tokens[Start].block.get()->parent_namespace.position;
// 			CurrentEnum.nmspace.list = &Namespaces;
// 		}

// 		if (Tokens[Start + 1].value.compare("class") != 0) { CurrentEnum.name.position = Start + 1; CurrentEnum.is_class = false; Consumed = 1; }
// 		else { CurrentEnum.name.position = Start + 2; CurrentEnum.is_class = true; Consumed = 3; }
// 		CurrentEnum.block.position = Tokens[Start + Consumed + 1].block.position;
// 		CurrentEnum.block.list = &Blocks;
// 		Current += Consumed;

// 		DEBUG_ONLY cout << "Debug - Discovered enum '" << CurrentEnum.name.get()->value << "'. Is also class: " << (CurrentEnum.is_class ? "yes.\n" : "no.\n");
// 		while (!IsAtEnd()) {
// 			Token Next = *Advance();
// 			if (!Next.block.equals(CurrentEnum.block)) {
// 				DEBUG_ONLY cout << "Debug - Completed parsing enum.\n";
// 				Enums.push_back(CurrentEnum);
// 				Current--;
// 				return;
// 			}
// 			DEBUG_ONLY cout << "Debug - Parsing token '" << Next.value << "' inside enum.\n";
// 		}
// 		// This is called only when the file has ended.
// 		DEBUG_ONLY cout << "Debug - Completed parsing enum.\n";
// 		Enums.push_back(CurrentEnum);
// 	}

// 	void ParseToken() {
// 		Token Next = *Advance();
// 		if (Next.value.compare("enum") == 0) {
// 			DEBUG_ONLY cout << "Debug - Found keyword 'enum'.\n";
// 			Enumerator();
// 		}
// 	}

// 	void ParseTokens() {
// 		DEBUG_ONLY cout << "Debug - Parser started.\n";
// 		while (!IsAtEnd()) {
// 			ParseToken();
// 		}
// 		DEBUG_ONLY cout << "Debug - Parser finished.\n\n";
// 	}
// };

// class Writer { public:
// 	void WriteResult(const string_view& Path) {
// 		if (fs::exists(Path)) { fs::remove(Path); }
// 		ofstream FileIO = ofstream(Path.data());
// 		if (!FileIO.is_open()) {
// 			cout << "Error - Couldn't open file.\n";
// 			return;
// 		}

// 		if (Enums.size() == 0) {
// 			cout << "Error - There is nothing to write to the file.\n";
// 			FileIO.close();
// 			return;
// 		}

// 		FileIO << "#ifndef __ENUMTOSTRING_H\n";
// 		FileIO << "#define __ENUMTOSTRING_H\n\n";

// 		FileIO << "#include <ostream>\n";
// 		FileIO << "#include <string>\n\n";

// 		// Declaration of the enum itself, this is not very useful
// 		// because it doesn't keep track of the values assigned to
// 		// each enumerator in the source file (eg. MY_ENUMERATOR = 1)
// 		// but in most cases it is fine.
// 		for (Enumerated& FoundEnum : Enums) {
// 			DEBUG_ONLY cout << "Debug - Writing enum '" << FoundEnum.name.get()->value << "' to file.\n";
// 			FileIO << "// ----\n\n";

// 			if(!FoundEnum.nmspace.null()) {
// 				if(FoundEnum.nmspace.get()->is_anon) {
// 					FileIO << "namespace {\n";
// 				} else {
// 					FileIO << "namespace" << FoundEnum.nmspace.get()->name.get()->value << " {\n";
// 				}
// 			}

// 			FileIO << (FoundEnum.is_class ? "enum class " : "enum ") << FoundEnum.name.get()->value << " {\n";
// 			for (Token& Enumerator : Tokens) {
// 				if (!Enumerator.block.equals(FoundEnum.block)) continue;
// 				FileIO << "    " << Enumerator.value << ",\n";
// 			}
// 			FileIO << "};\n\n";

// 			FileIO << "std::string " << FoundEnum.name.get()->value << "ToString(" << FoundEnum.name.get()->value << " Enumerator) {\n";
// 			FileIO << "    switch(Enumerator) {\n";
// 			for (Token& Enumerator : Tokens) {
// 				if (!Enumerator.block.equals(FoundEnum.block)) continue;
// 				FileIO << "    case " << FoundEnum.name.get()->value << "::" << Enumerator.value << ": ";
// 				FileIO << "return \"" << Enumerator.value << "\";\n";
// 			}
// 			FileIO << "    }\n";
// 			FileIO << "}\n\n";

// 			FileIO << "std::ostream& operator<<(std::ostream& Stream, " << FoundEnum.name.get()->value << " Enumerator) {\n";
// 			FileIO << "    return Stream << " << FoundEnum.name.get()->value << "ToString(Enumerator);\n";
// 			FileIO << "}\n\n";

// 			if(!FoundEnum.nmspace.null()) {
// 				FileIO << "}\n\n";
// 			}
// 			FileIO << "// ----\n\n";
// 		}
// 		FileIO << "#endif\n";
// 		FileIO.close();
// 	}
// };

int main(int argc, char* argv[]) {
	string FilePath = " ";
	bool FoundPath = false;
	
	if (argc >= 2) {
		FilePath = argv[1];
	}

	if (!fs::exists(FilePath)) {
		cout << "Please enter the path of the file with your enum definitions.\n";
		while (!fs::exists(FilePath) || !fs::is_regular_file(FilePath)) {
			cout << ">> "; cin >> FilePath;
		}
		FoundPath = true;
	}

	fstream File = fstream(FilePath); 
	
	{
		stringstream Temporary;
		Temporary << File.rdbuf();
		Source = Temporary.str();
	}

	// This code is probably windows-only, I added it for testing purposes only
	DEBUG_ONLY CLEAR_CONSOLE;
	DEBUG_ONLY cout << "Debug - Source code:\n" << Source << "\n\n\n\n\n";

	Sanitizer MySanitizer = Sanitizer();
	MySanitizer.SanitizeSource();

	DEBUG_ONLY CLEAR_CONSOLE;
	DEBUG_ONLY cout << "Debug - Sanitized code:\n" << Source;

	Scanner MyScanner = Scanner();
	MyScanner.ScanSource();

	// string OutputPath = "";
	// cout << "Please enter the path to the file in which you'd like the output to be stored at.\n";
	// cout << ">> "; cin >> OutputPath;

	// MyWriter.WriteResult(OutputPath);
	// cout << "Info - Process finished.\n";
	return 0;
}
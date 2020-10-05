#include <string_view>
#include <filesystem>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <thread>
#include <random>

using namespace std;
namespace fs = std::filesystem;

#ifdef STRINGIFIER_DEBUG
#define DEBUG_ONLY if constexpr(1)
#else
#define DEBUG_ONLY if constexpr(0)
#endif

template <typename T>
class VElementPtr { public:
	vector<T>* list = nullptr;
	int position = -1;

	bool null() { return (position == -1 || list == nullptr); }
	bool equals(const VElementPtr<T>& Other) { return (this->list == Other.list && this->position == Other.position); }
	T* get() { if (null()) return nullptr;  return &(*list)[position]; }

	friend ostream& operator<<(ostream& Stream, VElementPtr<T> pointer) { return Stream << pointer.get(); }
	bool operator==(const VElementPtr<T>& Other) { return equals(Other); }
};

class Block { public:
	VElementPtr<Block> parent;
	unsigned start = 0;
	unsigned end = 0;
};

class Token { public:
	VElementPtr<Block> block;
	string_view value;
	unsigned start = 0;
	unsigned end = 0;
};

class Enumerated { public:
	VElementPtr<Block> block;
	VElementPtr<Token> name;
	bool is_class = false;
	unsigned start = 0;
	unsigned end = 0;
};

vector<Enumerated> Enums;
vector<Token> Tokens;
vector<Block> Blocks;
string Source;

class Scanner { public:
	VElementPtr<Block> CurrentBlock;
	unsigned Current = 0;
	unsigned Line = 0;
	char End = '\0';

	inline bool IsAtEnd() { return Current >= Source.size(); }
	inline bool IsAlpha(char& Character) { return (Character >= 'a' && Character <= 'z') || (Character >= 'A' && Character <= 'Z') || Character == '_'; }
	inline bool IsNumeric(char& Character) { return Character >= '0' && Character <= '9'; }
	inline char* Peek() { if (IsAtEnd()) return &End; return &Source[Current]; }
	inline char* PeekNext() { if (Current + 1 >= Source.size()) return &End; return &Source[Current + 1]; }
	inline char* Advance() { Current++; return &Source[Current - 1]; }

	void Identifier() {
		unsigned Start = Current - 1;
		while (IsAlpha(*Peek())) Advance();
		Token FoundToken;
		FoundToken.block.position = CurrentBlock.position;
		FoundToken.block.list = &Blocks;
		FoundToken.start = Start;
		FoundToken.end = Current;
		FoundToken.value = Source;
		FoundToken.value.remove_prefix(Start);
		FoundToken.value.remove_suffix(Source.size() - Current);
		Tokens.push_back(FoundToken);
	}

	void BeginBlock() {
		Block FoundBlock;
		FoundBlock.parent.list = &Blocks;
		FoundBlock.parent.position = CurrentBlock.position;
		FoundBlock.start = Current;
		Blocks.push_back(FoundBlock);
		CurrentBlock.list = &Blocks;
		CurrentBlock.position = (Blocks.size() - 1);
	}

	void EndBlock() {
		if (CurrentBlock.null()) { cout << "Error - Unexpected '}' found at line " << Line << ".\n"; return; }
		CurrentBlock.get()->end = Current;
		CurrentBlock.position = CurrentBlock.get()->parent.position;
	}

	void SkipLine() {
		while (!IsAtEnd()) {
			char Character = *Advance();
			if (Character == '\n') { Line++; return; }
		}
	}

	inline void ScanCharacter() {
		char Character = *Advance();
		switch (Character) {
		case '/': if (*Peek() == '/') SkipLine(); break;
		case '#': SkipLine(); break;
		case '\n': Line++; break;
		case '{': BeginBlock(); break;
		case '}': EndBlock(); break;
		default: if (IsAlpha(Character)) Identifier(); break;
		}
	}

	void ScanSource() {
		DEBUG_ONLY "Debug - Scanner started.\n";
		CurrentBlock.list = &Blocks;
		CurrentBlock.position = -1;
		while (!IsAtEnd()) {
			ScanCharacter();
		}
		DEBUG_ONLY "Debug - Scanner finished.\n";
	}
};

class Parser { public:
	Enumerated CurrentEnum;
	unsigned Current = 0;

	inline bool IsAtEnd() { return Current >= Tokens.size(); }
	inline bool IsAlpha(char& Character) { return (Character >= 'a' && Character <= 'z') || (Character >= 'A' && Character <= 'Z') || Character == '_'; }
	inline Token* Advance() { Current++; return &Tokens[Current - 1]; }

	void Enumerator() {
		CurrentEnum = Enumerated();
		string_view Name;
		unsigned Start = Current - 1;
		unsigned Consumed = 0;

		CurrentEnum.name.list = &Tokens;
		if (Tokens[Start + 1].value.compare("class") != 0) { CurrentEnum.name.position = Start + 1; CurrentEnum.is_class = false; Consumed = 1; }
		else { CurrentEnum.name.position = Start + 2; CurrentEnum.is_class = true; Consumed = 3; }
		CurrentEnum.block.position = Tokens[Start + Consumed + 1].block.position;
		CurrentEnum.block.list = &Blocks;
		Current += Consumed;

		DEBUG_ONLY cout << "Debug - Discovered enum '" << CurrentEnum.name.get()->value << "'. Is also class: " << (CurrentEnum.is_class ? "yes.\n" : "no.\n");
		while (!IsAtEnd()) {
			Token Next = *Advance();
			if (!Next.block.equals(CurrentEnum.block)) {
				DEBUG_ONLY cout << "Debug - Completed parsing enum.\n";
				Enums.push_back(CurrentEnum);
				Current--;
				return;
			}
			DEBUG_ONLY cout << "Debug - Parsing token '" << Next.value << "' inside enum.\n";
		}
		// This is called only when the file has ended.
		DEBUG_ONLY cout << "Debug - Completed parsing enum.\n";
		Enums.push_back(CurrentEnum);
	}

	void ParseToken() {
		Token Next = *Advance();
		if (Next.value.compare("enum") == 0) {
			DEBUG_ONLY cout << "Debug - Found keyword 'enum'.\n";
			Enumerator();
		}
	}

	void ParseTokens() {
		DEBUG_ONLY cout << "Debug - Parser started.\n";
		while (!IsAtEnd()) {
			ParseToken();
		}
		DEBUG_ONLY cout << "Debug - Parser finished.\n";
	}
};

class Writer { public:
	void WriteResult(const string_view& Path) {
		if (fs::exists(Path)) { fs::remove(Path); }
		ofstream FileIO = ofstream(Path.data());
		if (!FileIO.is_open()) {
			cout << "Error - Couldn't open file.\n";
			return;
		}

		if (Enums.size() == 0) {
			cout << "Error - There is nothing to write to the file.\n";
			FileIO.close();
			return;
		}

		FileIO << "#ifndef __ENUMTOSTRING_H\n";
		FileIO << "#define __ENUMTOSTRING_H\n";

		FileIO << "#include <ostream>\n";
		FileIO << "#include <string>\n\n";

		// Declaration of the enum itself, this is not very useful
		// because it doesn't keep track of the values assigned to
		// each enumerator in the source file (eg. MY_ENUMERATOR = 1)
		// but in most cases it is fine.
		for (Enumerated& FoundEnum : Enums) {
			DEBUG_ONLY cout << "Debug - Writing enum '" << FoundEnum.name.get()->value << "' to file.\n";
			FileIO << "// ----\n\n";
			FileIO << (FoundEnum.is_class ? "enum class " : "enum ") << FoundEnum.name.get()->value << " {\n";
			for (Token& Enumerator : Tokens) {
				if (!Enumerator.block.equals(FoundEnum.block)) continue;
				FileIO << "    " << Enumerator.value << ",\n";
			}
			FileIO << "};\n\n";

			FileIO << "std::string __" << FoundEnum.name.get()->value << "ToString(" << FoundEnum.name.get()->value << " Enumerator) {\n";
			FileIO << "    switch(Enumerator) {\n";
			for (Token& Enumerator : Tokens) {
				if (!Enumerator.block.equals(FoundEnum.block)) continue;
				FileIO << "    case " << FoundEnum.name.get()->value << "::" << Enumerator.value << ": ";
				FileIO << "return \"" << Enumerator.value << "\";\n";
			}
			FileIO << "    }\n";
			FileIO << "}\n\n";

			FileIO << "std::ostream& operator<<(std::ostream& Stream, " << FoundEnum.name.get()->value << " Enumerator) {\n";
			FileIO << "    return Stream << __" << FoundEnum.name.get()->value << "ToString(Enumerator);\n";
			FileIO << "}\n\n";
			FileIO << "// ----\n\n";
		}
		FileIO << "#endif\n";
		FileIO.close();
	}
};

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
	DEBUG_ONLY system("cls");
	DEBUG_ONLY cout << "Debug - Source code:\n" << Source << "\n";
	DEBUG_ONLY this_thread::sleep_for(chrono::seconds(1));
	DEBUG_ONLY system("cls");
	DEBUG_ONLY this_thread::sleep_for(chrono::seconds(1));

	Scanner MyScanner = Scanner();
	Parser MyParser = Parser();
	Writer MyWriter = Writer();
	MyScanner.ScanSource();
	MyParser.ParseTokens();
	
	string OutputPath = "";
	cout << "Please enter the path to the file in which you'd like the output to be stored at.\n";
	cout << ">> "; cin >> OutputPath;

	MyWriter.WriteResult(OutputPath);
	cout << "Info - Process finished.\n";
	return 0;
}
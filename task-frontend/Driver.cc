#include "Driver.h"
#include <fstream>
#include <llvm/Support/CommandLine.h>

using namespace llvm;
cl::opt<std::string> OutputFilename("o", cl::desc("<output file>"),
                                    cl::Required);
cl::opt<std::string> InputFilename(cl::Positional, cl::desc("<input file>"),
                                   cl::Required);

int main(int argc, char **argv) {
  cl::ParseCommandLineOptions(argc, argv);

  std::ifstream InputFile{InputFilename};
  std::error_code OsErr;
  raw_fd_ostream OutputFile{OutputFilename, OsErr};

  yy::Driver Driver{&InputFile};
  std::unique_ptr<AST::ASTModule> Root{Driver.parse()};
  if (!Root)
    return 0;

  auto Context = std::make_unique<LLVMContext>();
  auto TheModule = std::make_unique<Module>(InputFilename, *Context);
  IRBuilder<> Builder(*Context);
  AST::ValsT NamedValues;
  Root->genIR(*Context, *TheModule, Builder, NamedValues);
  TheModule->print(OutputFile, nullptr);
}

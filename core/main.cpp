#include "forge.hpp"
#include <fstream>
#include <iostream>
int main(int argc,char** argv){
 if(argc!=2){std::cerr<<"Usage: forgewin executable.exe\n";return 2;}
 std::ifstream f(argv[1],std::ios::binary|std::ios::ate);
 if(!f||f.tellg()<0||f.tellg()>64*1024*1024){std::cerr<<"Cannot read file (limit 64 MiB)\n";return 2;}
 forge::Bytes b(size_t(f.tellg()));f.seekg(0);if(!f.read(reinterpret_cast<char*>(b.data()),b.size()))return 2;
 auto report=forge::execute(b);std::cout<<report;return report.find("STOP:")==std::string::npos?0:1;
}

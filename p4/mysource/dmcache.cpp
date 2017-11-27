#include <iostream>
#include <fstream>
#include <iomanip>

#define NUM_TAGS 256
#define NUM_CACHE_LINES 32
#define LINE_SIZE 8

class CacheLine {
  public: 
    int tag;
    int dirtybit;    
    int data[LINE_SIZE];

  CacheLine() {
    tag = -1;
    dirtybit = 0;
    for (int i = 0; i < LINE_SIZE; i++) data[i] = 0;
  };
};

int main(int argc, char *argv[]){
  if (argc != 2) {
    std::cout << "Program takes the testing filename as input\n";
    return 0;
  }

  std::ifstream inFile;
  inFile.open(argv[1]);
  std::ofstream outFile;
  outFile.open("dm-out.txt");

  static int memory[NUM_TAGS][NUM_CACHE_LINES][LINE_SIZE] = {}; // 2^(16), 64kb ram
  static CacheLine cache[NUM_CACHE_LINES];                      // 2^(5+3) 256b cache

  int address = 0;
  int offset = 0;
  int linenum = 0;
  int inTag = 0;
  int operation = 0;
  int inData = 0;
  int hit = 0;  // 1 is hit; 0 is miss; only modified on read

  while (inFile >> std::hex >> address >> std::hex >> operation >> std::hex >> inData){
    inTag = (address & 0xFF00) >> 8;
    linenum = (address & 0xF8) >> 3;
    offset = (address & 0x7);

    /*
      WRITE:
        ON CACHE HIT:
          1. if cache line tag is -1, set cache line tag to inTag
          2. copy inData to cache line offset
          3. set cache line dirtybit
        ON CACHE MISS:
          1. copy cache line to memory
          2. copy correct line from memory into cache line
          3. copy inData to cache line offset
          4. set cache line tag to inTag
          5. set cache line dirtybit
      READ:
        ON CACHE HIT:
          1. print 'data 1 dirtybit'
        ON CACHE MISS:
          1. copy cache line to memory
          2. copy correct line from memory into cache line
          3. print 'data 0 dirtybit'
          4. set cache line tag to inTag
          5. set cache line dirtybit to 0
    */
    if (operation == 0xFF){ 
      if (cache[linenum].tag == -1 || inTag == cache[linenum].tag) {  // tag match => cache hit
        if (cache[linenum].tag == -1) cache[linenum].tag = inTag;
        cache[linenum].data[offset] = inData;
        cache[linenum].dirtybit = 1;
      } else {  // tag mismatch => cache miss
        for (int i = 0; i < LINE_SIZE; i++) {
          memory[cache[linenum].tag][linenum][i] = cache[linenum].data[i];
          cache[linenum].data[i] = memory[inTag][linenum][i];
        }
        cache[linenum].data[offset] = inData;
        cache[linenum].tag = inTag;
        cache[linenum].dirtybit = 1;
      }
    } else if (operation == 0x0){ 
      if (inTag == cache[linenum].tag) {
        hit = 1;
        outFile << std::uppercase << std::setw(2) << std::hex << std::setfill('0') << cache[linenum].data[offset] << " " << hit << " " << cache[linenum].dirtybit << "\n";
      } else {
        hit = 0;
        for (int i = 0; i < LINE_SIZE; i++) {
          memory[cache[linenum].tag][linenum][i] = cache[linenum].data[i];
          cache[linenum].data[i] = memory[inTag][linenum][i];
        }
        outFile << std::uppercase << std::setw(2) << std::hex << std::setfill('0') << cache[linenum].data[offset] << " " << hit << " " << cache[linenum].dirtybit << "\n";
        cache[linenum].tag = inTag;
        cache[linenum].dirtybit = 0;
      }
    }
  }

  inFile.close();
  outFile.close();
  return 0;
}

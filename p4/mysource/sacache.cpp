#include <iostream>
#include <fstream>
#include <iomanip>
#include <vector>
#include <algorithm>

#define CACHE_SIZE 512
#define NUM_WAYS 8
#define LINE_SIZE 4
#define NUM_LINES 128
#define NUM_SETS 16
#define NUM_TAGS 1024

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

class Set {
  public:
    CacheLine ways[NUM_WAYS];
    std::vector<int> LRU; // LRU line number will be at the front of the vector
  
  Set() {
    for (int i = 0; i < NUM_WAYS; i++) LRU.push_back(i);
  }
};

class SACache {
  public:
    Set sets[NUM_SETS];

  SACache() {}
};

int main(int argc, char *argv[]){

  if (argc != 2) {
    std::cout << "Program takes the testing filename as input\n";
    return 0;
  }

  std::ifstream inFile;
  inFile.open(argv[1]);
  std::ofstream outFile;
  outFile.open("sa-out.txt");

  static int memory[NUM_TAGS][NUM_SETS][LINE_SIZE] = {};
  static SACache cache;

  int address = 0;
  int offset = 0;
  int setNum = 0;
  int inTag = 0;
  int operation = 0;
  int inData = 0;
  int correctLine = -1;
  int lru = -1;
  int hit = 0; 
  
  while (inFile >> std::hex >> address >> std::hex >> operation >> std::hex >> inData){
    inTag = (address & 0xFFC0) >> 6;
    setNum = (address & 0x3C) >> 2;
    offset = (address & 0x3);
    hit = 0;
    correctLine = -1;
    lru = cache.sets[setNum].LRU[0];    

    for (int i = 0; i < NUM_WAYS; i++) {
      if (-1 == cache.sets[setNum].ways[i].tag || inTag == cache.sets[setNum].ways[i].tag) {
        hit = 1;
        correctLine = i;
        break;
      }
    }

    /*
      WRITE:
        ON CACHE HIT:
          1. if cache line tag was -1, update tag to inTag
          2. copy inData to correct line
          3. set the line's dirtybit to 1
          4. update LRU vector
        ON CACHE MISS:
          1. copy LRU cache line to memory
          2. copy in new line from memory
          3. update cache line tag to inTag
          4. copy inData to cache line
          5. update LRU vector
          6. set the line's dirtybit to 1
      READ:
        ON CACHE HIT:
          1. print 'data 1 dirtybit'
          2. update LRU vector
        ON CACHE MISS:
          1. copy LRU cache line to memory
          2. copy in new line from memory
          3. print 'data 0 dirtybit'
          4. update cache line tag to inTag
          5. set the line's dirtybit to 0
          6. update LRU vector
    */
    if (operation == 0xFF){ 
      if (hit == 1) {
        if (-1 == cache.sets[setNum].ways[correctLine].tag) cache.sets[setNum].ways[correctLine].tag = inTag;
        cache.sets[setNum].ways[correctLine].data[offset] = inData;
        cache.sets[setNum].ways[correctLine].dirtybit = 1;
        cache.sets[setNum].LRU.erase(std::remove(cache.sets[setNum].LRU.begin(), cache.sets[setNum].LRU.end(), correctLine), cache.sets[setNum].LRU.end());
        cache.sets[setNum].LRU.push_back(correctLine);
      } else {
        for (int i = 0; i < LINE_SIZE; i++) {
          memory[cache.sets[setNum].ways[lru].tag][setNum][i] = cache.sets[setNum].ways[lru].data[i];
          cache.sets[setNum].ways[lru].data[i] = memory[inTag][setNum][i];
        }
        cache.sets[setNum].ways[lru].data[offset] = inData;
        cache.sets[setNum].ways[lru].tag = inTag;
        cache.sets[setNum].ways[lru].dirtybit = 1;
        cache.sets[setNum].LRU.erase(std::remove(cache.sets[setNum].LRU.begin(), cache.sets[setNum].LRU.end(), lru), cache.sets[setNum].LRU.end());
        cache.sets[setNum].LRU.push_back(lru);
      }
    } else if (operation == 0x0){
      if (hit == 1) {
        outFile << std::uppercase << std::setw(2) << std::hex << std::setfill('0') << cache.sets[setNum].ways[correctLine].data[offset] << " " << hit << " " << cache.sets[setNum].ways[correctLine].dirtybit << "\n";
        cache.sets[setNum].LRU.erase(std::remove(cache.sets[setNum].LRU.begin(), cache.sets[setNum].LRU.end(), correctLine), cache.sets[setNum].LRU.end());
        cache.sets[setNum].LRU.push_back(correctLine);
      } else {
        for (int i = 0; i < LINE_SIZE; i++) {
          memory[cache.sets[setNum].ways[lru].tag][setNum][i] = cache.sets[setNum].ways[lru].data[i];
          cache.sets[setNum].ways[lru].data[i] = memory[inTag][setNum][i];
        }
        outFile << std::uppercase << std::setw(2) << std::hex << std::setfill('0') << cache.sets[setNum].ways[lru].data[offset] << " " << hit << " " << cache.sets[setNum].ways[lru].dirtybit << "\n";
        cache.sets[setNum].ways[lru].dirtybit = 0;
        cache.sets[setNum].ways[lru].tag = inTag;        
        cache.sets[setNum].LRU.erase(std::remove(cache.sets[setNum].LRU.begin(), cache.sets[setNum].LRU.end(), lru), cache.sets[setNum].LRU.end());
        cache.sets[setNum].LRU.push_back(lru);
      }
    }
  }

  inFile.close();
  outFile.close();
  return 0;
}

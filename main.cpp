#include <iostream>
#include <fstream>
#include <string>
#include <cstdint>
#include <sstream>
#include <vector>

using namespace std;

enum class AccessType { READ, WRITE };
enum class WritePolicy { WRITE_THROUGH, WRITE_BACK };

struct CacheLine {
    bool valid;
    uint32_t tag;
    bool dirty;  // Only used if write-back policy is set
    CacheLine() : valid(false), tag(0), dirty(false) {}
};

struct Cache {
    CacheLine* lines;
    uint32_t size;
    uint32_t lineSize;
    uint32_t numLines;
    uint32_t cacheAccessCycles;
    uint64_t accesses;
    uint64_t reads;
    uint64_t writes;
    uint64_t hits;
    uint64_t misses;
    WritePolicy hitPolicy;
    WritePolicy missPolicy;

    Cache(uint32_t s, uint32_t lSize, uint32_t accessCycles, WritePolicy hitPol, WritePolicy missPol)
    : size(s), lineSize(lSize), cacheAccessCycles(accessCycles), accesses(0), reads(0), writes(0), hits(0), misses(0), hitPolicy(hitPol), missPolicy(missPol) {
        numLines = size / lineSize;
        lines = new CacheLine[numLines];
    }

    ~Cache() {
        delete[] lines;
    }

    void printCacheState() {
        cout << "Cache State:" << endl;
        for (uint32_t i = 0; i < numLines; i++) {
            cout << "Line " << i << ": Valid = " << lines[i].valid << ", Tag = " << lines[i].tag << ", Flag = " << lines[i].dirty << endl;
        }
    }
};

uint32_t getLineIndex(uint32_t address, uint32_t numLines, uint32_t lineSize) {
    return (address / lineSize) % numLines;
}

uint32_t getTag(uint32_t address, uint32_t lineSize) {
    return address / lineSize;
}

void printStats(const Cache &cache) {
    double hitRatio = double(cache.hits) / cache.accesses;
    double missRatio = double(cache.misses) / cache.accesses;
    double AMAT = cache.cacheAccessCycles + (missRatio * 150);  // Assuming 150 cycles for memory access

    cout << "Total Accesses: " << cache.accesses << "\n";
    cout << "Reads: " << cache.reads << ", Writes: " << cache.writes << "\n";
    cout << "Hits: " << cache.hits << ", Misses: " << cache.misses << "\n";
    cout << "Hit Ratio: " << hitRatio << ", Miss Ratio: " << missRatio << "\n";
    cout << "Average Memory Access Time: " << AMAT << " cycles\n\n";
}

void accessCache(Cache &cache, uint32_t address, AccessType type) {
    uint32_t index = getLineIndex(address, cache.numLines, cache.lineSize);
    uint32_t tag = getTag(address, cache.lineSize);
    cache.accesses++;
    if (type == AccessType::READ) {
        cache.reads++;
    } else {
        cache.writes++;
    }

    if (cache.lines[index].valid && cache.lines[index].tag == tag) {
        cache.hits++;
        if (type == AccessType::WRITE) {
            if (cache.hitPolicy == WritePolicy::WRITE_BACK) {
                cache.lines[index].dirty = true;  // Mark line as dirty
            } else {
                // Simulate immediate write to memory
                cout << "Write-through: Writing to memory immediately." << endl;
            }
        }
    } else {
        cache.misses++;
        if (type == AccessType::WRITE) {
            if (cache.missPolicy == WritePolicy::WRITE_BACK) {
                // Load to cache and mark dirty
                cache.lines[index].valid = true;
                cache.lines[index].tag = tag;
                cache.lines[index].dirty = true;
            } else {
                // Write-through: write directly to memory, optionally load line
                cout << "Write-through on miss: Writing to memory." << endl;
                cache.lines[index].valid = true;
                cache.lines[index].tag = tag;
            }
        } else {
            // Load the line for a read miss
            cache.lines[index].valid = true;
            cache.lines[index].tag = tag;
            cache.lines[index].dirty = false;
        }
    }

    cache.printCacheState();
    printStats(cache);
}

void processAccesses(Cache &cache, const string &filePath) {
    ifstream accessFile(filePath);
    if (!accessFile.is_open()) {
        cerr << "Failed to open file: " << filePath << endl;
        exit(1);
    }

    string line;
    getline(accessFile, line);
    istringstream iss(line);
    string accessEntry;
    while (getline(iss, accessEntry, ',')) {
        stringstream ss(accessEntry);
        string addressStr, typeStr;
        getline(ss, addressStr, ':');
        getline(ss, typeStr);
        uint32_t address = stoi(addressStr);
        AccessType type = (typeStr == "W") ? AccessType::WRITE : AccessType::READ;
        cout << "Processing " << (type == AccessType::WRITE ? "write" : "read") << " access at address: " << address << endl;
        accessCache(cache, address, type);
    }
}

int main() {
    cout << "Starting the program..." << endl;

    // Cache parameters and write policies
    uint32_t cacheSize, lineSize, cacheCycles;
    string hitPolicyStr, missPolicyStr;
    cout << "Enter cache size (bytes): ";
    cin >> cacheSize;
    cout << "Enter line size (bytes): ";
    cin >> lineSize;
    cout << "Enter cache access cycles: ";
    cin >> cacheCycles;
    cout << "Enter hit write policy (WT/WB): ";
    cin >> hitPolicyStr;
    WritePolicy hitPolicy = (hitPolicyStr == "WB") ? WritePolicy::WRITE_BACK : WritePolicy::WRITE_THROUGH;
    cout << "Enter miss write policy (WT/WB): ";
    cin >> missPolicyStr;
    WritePolicy missPolicy = (missPolicyStr == "WB") ? WritePolicy::WRITE_BACK : WritePolicy::WRITE_THROUGH;

    // Cache initialization
    Cache cache(cacheSize, lineSize, cacheCycles, hitPolicy, missPolicy);
    string accessFilePath = "access_sequence.txt";  // Path to the access file

    // Process access sequence
    processAccesses(cache, accessFilePath);

    cout << "Program completed successfully." << endl;
    return 0;
}

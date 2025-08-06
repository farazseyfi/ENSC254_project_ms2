/* Name : Faraz Seyfi , Niyousha Amin Afshari
 * Student ID: 301543610, 301465214
 * Course name: ENSC 254 - Summer 2025
 * Institution: Simon Fraser University
 * This file contains solutions for the Data Lab assignment.
 * Use is permitted only for educational and non-commercial purposes.
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>
#include <math.h>
#include <time.h>
#include <unistd.h>
#include "dogfault.h"
#include "cache.h"

// DO NOT MODIFY THIS FILE. INVOKE AFTER EACH ACCESS FROM runTrace
void print_result(result r) {
  if (r.status == CACHE_EVICT)
    printf(" [status: miss eviction, victim_block: 0x%llx, insert_block: 0x%llx]",
           r.victim_block_addr, r.insert_block_addr);
  if (r.status == CACHE_HIT)
    printf(" [status: hit]");
  if (r.status == CACHE_MISS)
    printf(" [status: miss, insert_block: 0x%llx]", r.insert_block_addr);
}

/* This is the entry point to operate the cache for a given address in the trace file.
 * First, is increments the global lru_clock in the corresponding cache set for the address.
 * Second, it checks if the address is already in the cache using the "probe_cache" function.
 * If yes, it is a cache hit:
 *     1) call the "hit_cacheline" function to update the counters inside the hit cache 
 *        line, including its lru_clock and access_counter.
 *     2) record a hit status in the return "result" struct and update hit_count 
 * Otherwise, it is a cache miss:
 *     1) call the "insert_cacheline" function, trying to find an empty cache line in the
 *        cache set and insert the address into the empty line. 
 *     2) if the "insert_cacheline" function returns true, record a miss status and the
          inserted block address in the return "result" struct and update miss_count
 *     3) otherwise, if the "insert_cacheline" function returns false:
 *          a) call the "victim_cacheline" function to figure which victim cache line to 
 *             replace based on the cache replacement policy (LRU and LFU).
 *          b) call the "replace_cacheline" function to replace the victim cache line with
 *             the new cache line to insert.
 *          c) record an eviction status, the victim block address, and the inserted block
 *             address in the return "result" struct. Update miss_count and eviction_count.
 */

 result operateCache(const unsigned long long address, Cache *cache) {
  // Declaring & initialzing results to store the values searching for 
  result r = {.status = 0, .victim_block_addr = 0, .insert_block_addr = 0};

  unsigned long long setIndex = cache_set(address, cache); // using implemeted functions
  Set *set = &cache -> sets[setIndex]; // creating a pointer to correct set value within the cache 
  
  set -> lru_clock++; // Everytime the set is accessed global clock increases 
                     // Making sure the the most recently accessed value has the highest clock value attatched to it 

  // Want to check if a certain adress already exist in a set -- CACHE_HIT
  if (probe_cache(address, cache)){
    // if we have a hit, the address already exists in the cache, if not it matches the data to the matching line 
    hit_cacheline(address, cache); 
    cache -> hit_count++; // if a hit occurs, it increments the hit count 
    r.status = CACHE_HIT; // if a hit occurs, it stores the result 
    return r; 
  }              

  // Checking if the value is inserted in an invalid empty location -- CACHE_MISS
  if (insert_cacheline(address, cache)){
    // if cache line is empty, it updates the cache miss count 
    cache -> miss_count++; 
    r.status = CACHE_MISS; // stores the values in the cache_miss location 
    r.insert_block_addr = address_to_block(address, cache); // stores the address of the value 
    return r; 
  }

  // checking which line is valid, finds vicitm address and replaces it -- CACHE_EVICT
  // creating a replacement for the victim block 
  unsigned long long vict_block = victim_cacheline(address, cache); 
  replace_cacheline(vict_block, address, cache); // replaces the victim block with the new block 

  // Incrementing the counter satuses 
  cache -> miss_count++; 
  cache -> eviction_count++; 
  
  // Replacing values to show thay an eviction has occured 
  r.status = CACHE_EVICT; 
  r.victim_block_addr = vict_block;// storing the address of the ecvited line 
  r.insert_block_addr = address_to_block(address, cache); // stores the block address of the inserted line 
  return r;
}

// HELPER FUNCTIONS USEFUL FOR IMPLEMENTING THE CACHE
// Given an address, return the block (aligned) address,
// i.e., byte offset bits are cleared to 0
unsigned long long address_to_block(const unsigned long long address, const Cache *cache) {

    // create a mask that clears the block offset
    // 1ULL: Unsigned long long value 1
     unsigned long long mask = ~((1ULL << cache->blockBits) - 1);

     // Apply the mask to zero out the block offset
     // By bitwise AND operation between the mask and the given input memory adress
     return address & mask;

}

// Return the cache tag of an address
unsigned long long cache_tag(const unsigned long long address, const Cache *cache) {

  // Shift the given input memory adress to the right to remove block offset and set index 
  unsigned long long tag = address >> ((cache->blockBits + cache->setBits));                    
  
  // Return the tag of the given memory address
  return tag;
}

// Return the cache set index of the address
unsigned long long cache_set(const unsigned long long address, const Cache *cache) {

  // Remove the block offset bits by shifting right
  unsigned long long shifted = address >> cache->blockBits;
  
  // Create a mask to only extract the set index
  unsigned long long mask = (1ULL << cache->setBits) - 1;

  // Return the cache set index of the given input memory address
  return shifted & mask;
}


// Check if the address is found in the cache. If so, return true. else return false.
bool probe_cache(const unsigned long long address, const Cache *cache) {

  // Use the function we implemented earlier to get the set index of a given memory adress
  unsigned long long setIndex = cache_set(address, cache);

  // Use the function we implemented earlier to get the tag of a given memory adress
  unsigned long long tag = cache_tag(address, cache);

  // Get a pointer to the correct set in the cache using setIndex
  Set *targetSet = &cache->sets[setIndex];

  // Loop through all the lines in the set for the selected cache set
  for (int i = 0; i < cache->linesPerSet; i++) {
    Line *line = &targetSet->lines[i]; // This line gets a pointer to the i-th cache line in the selected set

    // Check if the line is valid - Meaning the line containes valid data
    // Check if the tag stored in cache line matches the tag extracted from current memory address
    if (line->valid && line->tag == tag) {
      return true; // If both conditions match - return true since the given input memory address is in the cache
    }
  }

  // Return false since no matching line was found, therfor the address is NOT found in the cache
  return false;
}





// Access address in cache. Called only if probe is successful.
// Update the LRU (least recently used) or LFU (least frequently used) counters.
void hit_cacheline(const unsigned long long address, Cache *cache){
    // using implemeted functions
    unsigned long long setIndex = cache_set(address, cache);
    unsigned long long tag = cache_tag(address, cache);
    
    // creating a target for set 
    Set *set = &cache ->sets[setIndex];
    // if a matching one is found, line (empty pointer) points to it 
    Line *line = NULL; 

    // Looping through the lines to find the matching tag needed + storing the address in line 
    for (int i= 0; i < cache ->linesPerSet; i++) { 
      if (set -> lines[i].valid && set -> lines[i].tag == tag){ 
        line = &set -> lines[i]; 
        break;
      }
    }

    // Case where no matching tags are found, line with be set to NULL 
    if (line == NULL){
      fprintf(stderr, "Error - hit_cacheline could not find matching line\n"); 
      return; 
    }

  line -> lru_clock = set -> lru_clock; // The current timestamp is loads into lru_clock, indicating that it was used most recently
  line -> access_counter++; // incrementing the counter 


 }





/* This function is only called if probe_cache returns false, i.e., the address is
 * not in the cache. In this function, it will try to find an empty (i.e., invalid)
 * cache line for the address to insert. 
 * If it found an empty one:
 *     1) it inserts the address into that cache line (marking it valid).
 *     2) it updates the cache line's lru_clock based on the global lru_clock 
 *        in the cache set and initiates the cache line's access_counter.
 *     3) it returns true.
 * Otherwise, it returns false.  
 */ 

bool insert_cacheline(const unsigned long long address, Cache *cache) {

  // Compute the set index, tag, and the block offset from the given memory address input
  // We have already implemnted the logic to extract these info using helper functions

  unsigned long long setIndex = cache_set(address, cache);
  unsigned long long tag = cache_tag(address, cache);
  unsigned long long block_addr = address_to_block(address, cache);

  // Get a pointer to the correct cache set, given we have the set index of the memory address
  Set *targetSet = &cache->sets[setIndex];

  // Loop through the lines in the set to to find an empty (invalid) line to insert the new cache block
  for (int i = 0; i < cache->linesPerSet; i++) {
    Line *line = &targetSet->lines[i]; // Create a pointer to the i-th cache line in the selected set for easier access



    // Check to see if the line is invalid (empty line)
    if (!line->valid) {

      // Insert the new cache block inside the Empty line

      line->valid = true; // Assign the line as valid - since are inserting and it's not empty anymore
      line->tag = tag; //  Assign the tag we calculated already using the helper function
      line->block_addr = block_addr; // Assign the aligned block address
      line->access_counter = 1; // Intial access count
      line->lru_clock = targetSet->lru_clock; // updating the line's lru_clock based on the global lru_clock in the cache set

      return true; // Successful insertion

    }

  }

  // No empty line found to insert a new cache block
   return false; 
}



// If there is no empty cacheline, this method figures out which cacheline to replace
// depending on the cache replacement policy (LRU and LFU). It returns the block address
// of the victim cacheline; note we no longer have access to the full address of the victim
unsigned long long victim_cacheline(const unsigned long long address, const Cache *cache) {
                                
  unsigned long long setIndex = cache_set(address, cache); // What we are looking for 
  Set *set = &cache -> sets[setIndex]; // Creating the target for function 

  int victIndex = 0; // Assuming the first line encountered is the vicitim 

  for (int i = 1; i < cache -> linesPerSet; i++){ // looping to find the matching line to the victim pointer 
    Line *current = &set -> lines[i]; // Pointer to the current value being inspected 
    Line *vict = &set -> lines[victIndex]; // pointer to the assumed victim value 

    if (cache -> lfu == 0){ 
      // finding line with the oldest timestamp 
      if (current -> lru_clock < vict -> lru_clock)
        victIndex = i; 
    } else { // if two lines are use frequinlty for oldest accessed, chooses the smaller rlu_clock 
      if (current -> access_counter < vict -> access_counter || 
         (current -> access_counter == vict -> access_counter &&
          current -> lru_clock < vict -> lru_clock ))
          victIndex = i; 
    }
  }
   // returns address of the victim found 
   return set -> lines[victIndex].block_addr; 
}




/* Replace the victim cacheline with the new address to insert. Note for the victim cachline,
 * we only have its block address. For the new address to be inserted, we have its full address.
 * Remember to update the new cache line's lru_clock based on the global lru_clock in the cache
 * set and initiate the cache line's access_counter.
 */


void replace_cacheline(const unsigned long long victim_block_addr, const unsigned long long insert_addr, Cache *cache) {
               

unsigned long long setIndex = cache_set(insert_addr, cache); // The new address we are looking for 
Set *set = &cache -> sets[setIndex]; 

// Looping through set to find the victim line 
Line *vict = NULL;

for (int i = 0; i < cache -> linesPerSet; i++) {
  // finds the block_addr that matches the vitim line looking for 
  if (set -> lines[i].valid && set -> lines[i].block_addr == victim_block_addr){
    vict = &set -> lines[i]; 
    break;
  }
}

// if nothing is found, sets the vict to null (empty)
if (vict == NULL){ 
  // if victim line is not found, prints out an error 
  fprintf(stderr, "Error - replace_cacheline could not find the victim line"); 
  return;
}

// Replacing the vicitm like with a new block 
vict -> tag = cache_tag(insert_addr, cache); // stores the pieces of the address in the line 
vict -> block_addr = address_to_block(insert_addr, cache); // stores the aligned block address 
vict -> valid = true; // line can now be used 
vict -> access_counter = 1; // setting it to the first access 
vict -> lru_clock = set -> lru_clock; // sets set to the recently used value 

}




// allocate the memory space for the cache with the given cache parameters
// and initialize the cache sets and lines.
// Initialize the cache name to the given name 
void cacheSetUp(Cache *cache, char *name) {
  
  // Calculate the total number of sets
  // Since setBits represents the number of bits used for the set index
  // Total number of sets = 2 ^ (setBits)
  int numSets = 1 << cache->setBits;

  // Dynamically allocate memory for the array of cache sets
  // Each set will later contain an array of cache lines
  cache->sets = (Set *)malloc(numSets * sizeof(Set));


  // Now we loop through each set to allocate and intialize the lines for each set
  for (int i = 0; i < numSets; i++) {

    // Dynamically Allocate memory for the array of lines for the current set
    cache->sets[i].lines = (Line *)malloc(cache->linesPerSet * sizeof(Line));

    // Intialize each line in the current set
    for (int j = 0; j < cache->linesPerSet; j++) {
      cache->sets[i].lines[j].valid = false; // The line starts invalid - Meaning initially, does NOT contain data
      cache->sets[i].lines[j].tag = 0; // Intializing the line as it containes NO tag
      cache->sets[i].lines[j].lru_clock = 0; // LRU_clock tracks how this recently this line was used - intialzing as 0
      cache->sets[i].lines[j].access_counter = 0; // Tracks how many times this line has been accessed - intialzing as 0
      cache->sets[i].lines[j].block_addr = 0; // block_ addr represents the memory block address the line holds - intialzing as 0

    }

    // Intialize the Global LRU clock for the current set
    // NOTE: this is different than the list lru_clock
    cache->sets[i].lru_clock = 0;

  }

  // Set the cache name to the one given as input parameter of the function
  // For indentification
  cache->name = name;

}

// deallocate the memory space for the cache
void deallocate(Cache *cache) {
  
  // Calculate the total number of sets
  // Since setBits represents the number of bits used for the set index
  // Total number of sets = 2 ^ (setBits)
  int numSets = 1 << cache->setBits;

  // Loop through each set
  for (int i = 0; i < numSets; i++) {
    
    // Free the dynamically allocated array of lines in each set
    // we are using free(), which is std library fucntion to releases  dynamic memory, kind of like a destructor
    free(cache->sets[i].lines); 
  }

  // Free the array of sets after free the array of lines
  free(cache->sets);

  // We don't simply free(cache) because cache is not a dynamically allocated
  // But instead it is decleared as a local stack variable
}

// print out summary stats for the cache
void printSummary(const Cache *cache) {
  printf("%s hits: %d, misses: %d, evictions: %d\n", cache->name, cache->hit_count,
         cache->miss_count, cache->eviction_count);
}
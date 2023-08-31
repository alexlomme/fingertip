#include <cstdint>
#include <initializer_list>
#include <sys/mman.h>

#include "debug.hpp"

inline void* allocZeros(uint64_t size) {
  return mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
}

inline uint64_t hashKey(uint64_t k) {
  // MurmurHash64A
  const uint64_t m = 0xc6a4a7935bd1e995LLU;
  const int r = 47;
  uint64_t h = 0x8445d61a4e774912 ^ (8 * m);
  k *= m;
  k ^= k >> r;
  k *= m;
  h ^= k;
  h *= m;
  h ^= h >> r;
  h *= m;
  h ^= h >> r;
  return h;
}

struct Hashtable {
   struct Entry {
      uint64_t key;
      uint64_t value;
      Entry* next;
   };

   uint64_t htSize;
   uint64_t mask;
   Entry** ht;

   Hashtable(uint64_t size) {
      htSize = 1ull << ((8*sizeof(uint64_t)) - __builtin_clzl(size)); // next power of two
      mask = htSize - 1;
      ht = reinterpret_cast<Entry**>(allocZeros(sizeof(uint64_t) * htSize));
   }

   ~Hashtable() {
       for (Entry* e=*ht; e; e=e->next) {
           free(e);
       }
      munmap(reinterpret_cast<void*>(ht), sizeof(uint64_t) * htSize);
   }

   Entry* lookup(uint64_t key) {
      for (Entry* e=ht[hashKey(key)&mask]; e; e=e->next)
         if (e->key==key)
            return e;
      return nullptr;
   }

   bool insert(uint64_t key, uint64_t value) {
      Entry* e = lookup(key);
      if (e) {
         e->value = value;
         return false;
      } else {
         uint64_t pos = hashKey(key) & mask;
         Entry* newEntry = new Entry();
         ht[pos] = newEntry;
         newEntry->key = key;
         newEntry->value = value;
         uint64_t nextPos = pos + 1;
         while (nextPos < htSize && ht[nextPos] == nullptr) {
             nextPos++;
         }
         if (nextPos == htSize) {
             newEntry->next = nullptr;
         }
         else {
             newEntry->next = ht[nextPos];
         }
         if (pos == 0) {
             return true;
         }
         uint64_t prevPos = pos - 1;
         while (prevPos != 0ULL && ht[prevPos] == nullptr) {
             prevPos--;
         }
         if (ht[prevPos]) {
             ht[prevPos]->next = newEntry;
         }
         return true;
      }
   }

   bool erase(uint64_t key) {
      uint64_t pos = hashKey(key) & mask;
      Entry** ePtr = &ht[pos];
      Entry* prevPtr = nullptr;
      while (Entry* e=(*ePtr)) {
         if (e->key==key) {
            prevPtr->next = e->next;
            delete e;
            return true;
         }
         prevPtr = *ePtr;
         ePtr = &e->next;
      }
      return false;
   }
};

int main() {
   for (uint64_t size : {10, 99}) { //, 837, 48329, 384933
      Hashtable h(size);

      // insert
      for (uint64_t i=0; i<size; i++) {
         Ensure(h.insert(i, 42));
      }
      // update
      for (uint64_t i=0; i<size; i++) {
         Ensure(!h.insert(i, i));
      }
      // lookup
      for (uint64_t i=0; i<size; i++) {
         Hashtable::Entry* e = h.lookup(i);
         Ensure(e);
         Ensure(e->value==i);
      }
      // erase some
      for (uint64_t i=0; i<size/2; i+=3) {
         Ensure(h.erase(i));
      }
      // erase twice
      for (uint64_t i=0; i<size/2; i+=3) {
         Ensure(!h.erase(i));
      }
      // lookup
      for (uint64_t i=0; i<size/2; i++) {
         Hashtable::Entry* e = h.lookup(i);
         if ((i%3)==0) {
            Ensure(!e);
         } else {
            Ensure(e);
            Ensure(e->value==i);
         }
      }
      // erase some more
      for (uint64_t i=0; i<size/2; i++) {
         if ((i%3)==0) {
            Ensure(!h.erase(i));
         } else {
            Ensure(h.erase(i));
         }
      }
      // lookup
      for (uint64_t i=0; i<size/2; i++) {
         Hashtable::Entry* e = h.lookup(i);
         Ensure(!e);
      }
   }
   return 0;
}

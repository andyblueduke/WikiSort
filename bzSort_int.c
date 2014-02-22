// this file is identical to bzSort.c, but instead of a #define it uses a function that only operates on 'int' arrays
// this should make it easier to read and modify the underlying algorithm.

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

uint64_t floor_power_of_two(uint64_t x) {
   x |= (x >> 1); x |= (x >> 2); x |= (x >> 4);
   x |= (x >> 8); x |= (x >> 16); x |= (x >> 32);
   x -= (x >> 1) & 0x5555555555555555;
   x = (x & 0x3333333333333333) + ((x >> 2) & 0x3333333333333333);
   x = (x + (x >> 4)) & 0x0F0F0F0F0F0F0F0F;
   x += x >> 8; x += x >> 16; x += x >> 32;
   x &= 0x7F; return (x == 0) ? 0 : (1 << (x - 1));
}

// this assumes that if a <= b and b <= c, then a <= c
void bzSort(int ints[], const uint64_t array_count) {
   uint64_t i; int temp;
   
   if (array_count < 32) {
      // insertion sort the array
      for (i = 1; i < array_count; i++) {
         temp = ints[i]; uint64_t j = i;
         while (j > 0 && ints[j - 1] > temp) { ints[j] = ints[j - 1]; j--; }
         ints[j] = temp;
      }
      return;
   }
   
   const uint64_t swap_size = 1024; // change this as desired
   int swap[swap_size];
   
   // calculate how to scale the index value to the range within the array
   uint64_t pot = floor_power_of_two(array_count);
   double scale = array_count/(double)pot; // 1.0 <= scale < 2.0
   
   uint64_t index = 0, start, mid, end, iteration, merge, length;
   while (index < pot) {
      start = index * scale; mid = (index + 16) * scale; end = (index + 32) * scale;
      
      // insertion sort from start to mid and mid to end
      for (i = start + 1; i < mid; i++) {
         temp = ints[i]; uint64_t j = i;
         while (j > start && ints[j - 1] > temp) { ints[j] = ints[j - 1]; j--; }
         ints[j] = temp;
      }
      
      for (i = mid + 1; i < end; i++) {
         temp = ints[i]; uint64_t j = i;
         while (j > mid && ints[j - 1] > temp) { ints[j] = ints[j - 1]; j--; }
         ints[j] = temp;
      }
      
      merge = index; index += 32; iteration = (index >> 4); length = 16;
      while ((iteration & 0x1) == 0x0) {
         start = merge * scale; mid = (merge + length) * scale; end = (merge + length + length) * scale;
         
         // don't merge if they're already in order
         if (ints[mid - 1] > ints[mid]) {
            // see if the two segments are in the wrong order, like in this example: 4 5 6 7 | 0 1 2 3
            // ints[start] = 4 and ints[end - 1] = 3, and 4 > 3, so we only need to swap the segments rather than perform a full merge
            if (ints[start] > ints[end - 1]) {
               // the size of the two sides will never differ by more than 1, so we can just have a separate swap here for a single variable
               if (mid - start >= end - mid) {
                  uint64_t a_from = start, a_to = mid, b_from = mid, b_to = start, count = end - mid;
                  if (mid - start != end - mid) temp = ints[a_to = mid - 1];
                  while (count > 0) {
                     // copy values from the left side into swap
                     uint64_t read = (swap_size < count) ? swap_size : count;
                     memcpy(&swap[0], &ints[a_from], read * sizeof(ints[0]));
                     memmove(&ints[b_to], &ints[b_from], read * sizeof(ints[0]));
                     memcpy(&ints[a_to], &swap[0], read * sizeof(ints[0]));
                     a_from += read; a_to += read; b_from += read; b_to += read; count -= read;
                  }
                  if (mid - start != end - mid) ints[end - 1] = temp;
               } else {
                  uint64_t a_from = end, a_to = mid, b_from = mid, b_to = end, count = mid - start;
                  if (mid - start != end - mid) { temp = ints[mid]; a_to++; }
                  while (count > 0) {
                     // copy values from the left side into swap
                     uint64_t read = (swap_size < count) ? swap_size : count;
                     a_from -= read; a_to -= read; b_from -= read; b_to -= read; count -= read;
                     memcpy(&swap[0], &ints[a_from], read * sizeof(ints[0]));
                     memmove(&ints[b_to], &ints[b_from], read * sizeof(ints[0]));
                     memcpy(&ints[a_to], &swap[0], read * sizeof(ints[0]));
                  }
                  if (mid - start != end - mid) ints[start] = temp;
               }
            } else {
               // add the smaller of the two values to swap
               uint64_t insert = 0, count = 0, index1 = start, index2 = mid, swap_to = start, swap_from = 0;
               while (index1 < mid && index2 < end) {
                  count++; swap[insert++] = (ints[index1] <= ints[index2]) ? ints[index1++] : ints[index2++];
                  if (insert >= swap_size) insert = 0;
                  if (count >= swap_size) { // if the buffer is full, we need to shift over the values on the left side and write part of the swap buffer back to the array
                     if (index1 - swap_to <= count/4) { // if there's at least 1/4 free space to the left of a, write it out right away
                        // shift the values all the way to the right and write out the entire buffer
                        memmove(&ints[index2 - (mid - index1)], &ints[index1], (mid - index1) * sizeof(ints[0]));
                        index1 = index2 - (mid - index1); mid = index2; count = 0;
                     } else count -= (index1 - swap_to); // write out (index1 - start) values from the swap buffer
                     while (swap_to < index1) { ints[swap_to++] = swap[swap_from++]; if (swap_from >= swap_size) swap_from = 0; }
                  }
               }
               
               if (mid < index2) { memmove(&ints[index2 - (mid - index1)], &ints[index1], (mid - index1) * sizeof(ints[0])); index1 = index2 - (mid - index1); }
               while (swap_to < index1) { ints[swap_to++] = swap[swap_from++]; if (swap_from >= swap_size) swap_from = 0; }
            }
         }
         
         length *= 2; merge -= length; iteration /= 2;
      }
   }
}

int main(int argc, char **argv) {
   // the algorithm was previously tested with arrays containing millions of elements with various properties
   // (in order, reverse order, random, mostly ascending, etc.), but this is just random mashing on the keyboard:
   int test[] = { 2,4,678,9,2,7,0,4,32,5,43,34,7,94,21,2,4,7,90,9,6,3,2,5,8,43,459,3,78,72,2,70,87,42,578,9,85,3,5,78,74,2,346,8,9 };
   long count = sizeof(test)/sizeof(test[0]); int i;
   printf("before: "); for (i = 0; i < count; i++) printf("%d ", test[i]); printf("\n");
   bzSort(test, count);
   printf("after: "); for (i = 0; i < count; i++) printf("%d ", test[i]); printf("\n");
   return 0;
}

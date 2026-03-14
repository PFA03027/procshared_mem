# ipsm_mem
 class of shared memory b/w processes and it startup asyncronous and cooperative and/or non-cooperative


# utility classes
 * ipsm_mem is a shared memory construction class
 * ipsm_malloc is a memory allocator on ipsm_mem. memory allocation algorithm is K&R memory allocator.
 * ipsm_mutex / ipsm_recursive_mutex is the mutex that is able to be shared b/w process.
 * ipsm_condition_variable is conditional variable that is able to be shared b/w process.
 * offset_ptr is offset base pointer.
 * atomic_offset_ptr supports atomic behavior of offset_ptr.

# Header file dependency layering

* ipsm_logger.hpp / offset_memory_util.hpp
* offset_ptr.hpp
* offset_unique_ptr.hpp / offset_shared_ptr.hpp
* offset_malloc.hpp
* offset_allocator.hpp( / offset_memory_resource.hpp未実装)
* offset_list.hpp
* ipsm_mutex.hpp
* ipsm_condition_variable.hpp
* ipsm_mem.hpp
* ipsm_malloc.hpp
* ipsm_allocator.hpp / ipsm_memory_resource.hpp

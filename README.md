# procshared_mem
 class of shared memory b/w processes and it startup asyncronous

# using resources
 in case that shared memory name is "/test_shm", below resource are used;
 * semaphore file by sem_open() that names "/test_shm"
 * id file by open() that names "/tmp/test_shm"  This is empry file, but inode number is refered.
 * shared memory by shm_open() that names "/tmp_shm"  Length of this shard memory is set by class procshared_mem's constructor argument.

# utility classes
 * procshared_mutex / procshared_recursive_mutex is the mutex that is able to be shared b/w process.
 * procshared_condition_variable is conditional variable that is able to be shared b/w process.
 * offset_ptr is offset base pointer.
 * atomic_offset_ptr supports atomic behavior of offset_ptr.

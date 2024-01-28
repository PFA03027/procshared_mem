# ipsm_mem
 class of shared memory b/w processes and it startup asyncronous and cooperative and/or non-cooperative

# using resources
 in case that shared memory name is "/test_shm" and id directory name is "/tmp", below resource are used;
 * semaphore file by sem_open() that names "/test_shm"
 * id file by open() that names "/tmp/test_shm"  This is empry file, but inode number is refered.
 * shared memory by shm_open() that names "/tmp_shm"  Length of this shard memory is set by class ipsm_mem's constructor argument.

# utility classes
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
* ipsm_mutex_base.hpp
* ipsm_mutex.hpp / ipsm_recursive_mutex.hpp
* ipsm_condition_variable.hpp
* ipsm_mem.hpp
* ipsm_malloc.hpp
* ipsm_allocator.hpp / ipsm_memory_resource.hpp

# AI memo
* 非協調で共有メモリの起動が出来たら、ver 0.5 (done)
* 共有メモリからメモリ割り当て対応ができたら、0.6 (done)
* 共有メモリ間でのメッセージやり取りができたら、+0.1 (done)
* 複数の共有メモリに対し、それぞれの共有メモリに対して解放処理ができるようになったら、0.8
* 上記が満足出来たら、1.0
* プロセス異常終了対応は、初期化中(コンストラクタ処理中)/終了中(デストラクタ処理中)以外に対応できたら、+0.1
* 初期化中、終了中にも対応できたら、+0.1


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

# AI
* ipsm_mallocクラスが保持する共有メモリの寿命管理に対し、allocatorやdeleterなど、参照中のリソースの開放完了まで、寿命をキープできるようにする。
 ー＞　取り下げ。shared_ptrでの管理は、不可能。寿命キープの責務はipsm_mallocでは担わない。
  * なお、ipsm_memクラスの段階では、リソースオーナーとしての役割を成立させるため、move onlyクラスとする点は変更しない。
  * 変更案
    * ipsm_mallocクラスに対し、実装クラスを導入し、現在の実装を実装クラス側に移動する。(済)
     ー＞　取り下げ。 shared_ptrでの管理は、不可能。なぜならば、共有メモリ内に保持する場合も生じるため。
    * そのうえで、ipsm_mallocクラスは、std::shared_ptrで、実装クラスを保持するように変更する。(済)
     ー＞　取り下げ。 shared_ptrでの管理は、不可能。なぜならば、共有メモリ内に保持する場合も生じるため。
    * 上記の対応の後、allocatorとdeleterを生成するファクトリ機能を担うメンバ関数を導入。(変更)
    　ー＞　offset_mallocを取得できるようにして、ファクトリの役割を持つメンバ関数の導入は取りやめ。
    * allocatorとdeleterには、実装クラスへのstd::shared_ptrを保持させる。（キャンセル）
* 上記が終わったら、offset_unique_ptrとoffset_shared_ptrのアロケータやデリーターの扱いを変更し、配列型の保持をできるようにする。(済)
ー＞ offset_mallocを保持する方式で実現。
* その後、ipsm_mallocのエリアにリスト型ベースのキューを作成するサンプルを作り、使いやすさを確認する。



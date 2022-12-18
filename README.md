# Inter-VM-Communication
- 在QEMU启用虚拟机时需要添加指令-object memory-backend-file,id=shmmem-shmem0,mem-path=/dev/shm/lzh,size=33554432,share=yes -device ivshmem-plain,id=shmem0,memdev=shmmem-shmem0  
- -object memory-backend-file,id=shmmem-shmem0,mem-path=/dev/shm/lzh,size=33554432,share=yes创建一个指定大小为32M的内存后端设备文件, 创建文件在Host OS的路径为/dev/shm/lzh。
-device ivshmem-plain,id=shmem0,memdev=shmmem-shmem0表示QEMU通过上述内存后端设备创建一个ivshmem-plain设备。  
- 在通过添加上述指令QEMU启用虚拟机之后，会创建一个32M内存大小的PCI设备。我们仍需要在虚拟机中添加ivshmem设备驱动以启用该设备。通过make指令执行makefile文件编译ivshmem.c文件得出内核模块文件ivshmem.ko文件，也就是ivshmem的设备驱动。通过sudo insmod ivshmem.ko加载该内核模块到内核中运行。从而创建一个文件设备/dev/ivshmem0提供编程使用，通过对该文件进行读写以实现共享内存的读写。   

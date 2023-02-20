ivshmem.c and uio.c are interrupt implementations.Both adopt the msi-x interrupt mode.   
To enable the virtual machine, you need to first enable the server of qemu, that is, use the command:   
     **sudo ivshmem-server -S /tmp/nahanni  -m /dev/shm/ -l 2M**  
Then enable the virtual machine in a way similar to the following command ：   
     **qemu-system-x86_64 -name ubuntu20 -m 1024M -smp 2 --enable-kvm -cpu host -boot d -hda ubuntu_20.04.qcow2  -chardev socket,path=/tmp/nahanni,id=myst -device ivshmem-doorbell,vectors=4,chardev=myst**  

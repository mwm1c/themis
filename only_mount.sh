echo 'insmod'
insmod nova.ko

echo 'mount'
mount -t NOVA -o init /dev/pmem0 /mnt/nova

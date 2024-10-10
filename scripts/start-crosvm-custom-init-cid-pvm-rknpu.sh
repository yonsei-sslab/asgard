./crosvm run \
	--cid 3 \
	--cpus 1 \
	--cpu-affinity 4 \
	--mem 512 \
	--protected-vm-without-firmware \
	--vfio-platform /sys/bus/platform/devices/fdab0000.npu-guest \
	--disable-sandbox \
	--serial stdout,hardware=virtio-console,console,stdin \
	--params init=/files/guest_inference_init \
	--rwroot rootfs_minimal.ext4 \
	Image_minimal

mv /dev/vfio /dev/vfio.tmp
mkdir /dev/vfio
mv /dev/vfio.tmp /dev/vfio/vfio

chmod 777 /sys/module/vfio_iommu_type1/parameters/allow_unsafe_interrupts
echo Y > /sys/module/vfio_iommu_type1/parameters/allow_unsafe_interrupts
chmod 777 /sys/bus/platform/devices/fdab0000.npu-guest/driver_override
echo vfio-platform > /sys/bus/platform/devices/fdab0000.npu-guest/driver_override
chmod 777 /sys/bus/platform/drivers/vfio-platform/bind
echo fdab0000.npu-guest > /sys/bus/platform/drivers/vfio-platform/bind

ls -a /dev/vfio
ls -a /sys/bus/platform/drivers/vfio-platform

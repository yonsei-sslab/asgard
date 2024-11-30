# NDSS'25 Fall Artifact #41

### ASGARD: Protecting On-Device Deep Neural Networks with Virtualization-Based Trusted Execution Environments

***Artifact Abstract***—ASGARD is a new on-device deep neural network (DNN) model protection solution based on a virtualization-based trusted execution environment.
While the virtual machine abstraction used by ASGARD brings the benefit of strong compatibility with existing proprietary software including NPU drivers and secure monitors at EL3, it introduces both trust computing base (TCB) and run-time overheads.
ASGARD aggressively minimizes both platform- and application-level TCB overheads, and reduces run-time overheads through our proposed DNN execution planning technique.
Our evaluation includes (i) a qualitative yet comprehensive security analysis of ASGARD, and (ii) a quantitative analysis of ASGARD's TCB and run-time overheads on our prototype implementation of ASGARD on RK3588S.
We outline below how to build our prototype, and how to reproduce our quantitative analysis results of TCB and run-time overheads.

### Directory Structure

```
.
├── README.md
├── asgard-buildroot            # Used to build the enclave root file system.
├── asgard-manifest-*           # Manifest file to download the remaining Android, kernel, and CROSVM source.
├── asgard-tensorflow
│   └── asgard                  # DNN application source.
├── assets
│   ├── images                  # Images to be used in the experiments (E5) and (E6).
│   └── models                  # DNN models to be used in the experiments (E5) and (E6).
├── bin
│   ├── librknnrt               # User-mode Rockchip NPU drivers for the host and enclave (Rockchip-compiled for either arm64 linux or android).
│   ├── adb                     # Tool to access the development board via terminal.
│   ├── upgrade_tool            # Tool to install the host image on the development board.
│   └── rk3588_bl31_v1.26.elf   # Proprietary RK3588S EL3 secure monitor.
├── configs                     # Enclave kernel config files.   
├── scripts                     # Scripts used to run this artifact.
└── src
    ├── guest-init              # Enclave init application.
    ├── rknpu-power             # Small program that prepares Rockchip NPU before enclave boot.
    ├── rknpu                   # Original kernel-mode Rockchip NPU driver code.
    └── hyp                     # Original pKVM hypervisor code.
```

### Prerequisites

- A 64-bit x86 host machine with 20.04 LTS installed, at least 1TB of free disk space, 64GB of RAM, and preferably a multi-core CPU.
- A [Khadas Edge2 development board](https://www.khadas.com/edge2) with 16GB (or more) of RAM.

## 1. Downloading Sources

The repository includes multiple git submodules, all of which must be cloned by following (1.1).
Due to the large size of the Android source, we include a submodule called `asgard-manifest-*`, which points to all the remaining repositories for the host Android, host kernel, enclave kernel, and CROSVM.
These can be downloaded by following (1.2), (1.3), and (1.4), which **may take more than a day** depending on the network quality.

### 1.1 Git Submodules

```bash
# If you have already cloned this repository.
git submodule update --init --recursive

# If you have not yet cloned this repository.
git clone --recurse-submodules https://github.com/yonsei-sslab/asgard-artifact.git
```

### 1.2 [Host Kernel](https://github.com/yonsei-sslab/asgard-linux/tree/host) and Android

```bash
# Install toolchains.
sudo apt-get update
sudo apt-get install git-lfs
sudo apt-get install git-core gnupg build-essential gcc-multilib libc6-dev-i386 lib32ncurses5-dev
sudo apt-get install x11proto-core-dev libx11-dev lib32z1-dev libgl1-mesa-dev xsltproc unzip fontconfig
sudo apt-get install bison g++-multilib git gperf libxml2-utils python3-networkx make zip
sudo apt-get install flex curl libncurses5 libncurses5-dev libssl-dev zlib1g-dev gawk minicom
sudo apt-get install exfat-fuse exfat-utils device-tree-compiler liblz4-tool
sudo apt-get install openjdk-8-jdk
sudo apt-get install gcc-aarch64-linux-gnu
sudo apt-get install python3-matplotlib

# Create new directory in the main repository and get the source.
mkdir host-android && cd host-android
curl https://storage.googleapis.com/git-repo-downloads/repo-1 > repo
chmod a+x repo
python3 repo init -u https://github.com/yonsei-sslab/asgard-manifest.git -b host-android
python3 repo sync -c

# Pull some large files that were skipped.
cd external/camera_engine_rkaiq
git lfs pull
cd prebuilts/module_sdk
git lfs pull
cd device/khadas/rk3588
git lfs pull
```

### 1.3 [Enclave Kernel](https://github.com/yonsei-sslab/asgard-linux/tree/guest)

```bash
# Create new directory in the main repository and get the source.
mkdir guest-linux && cd guest-linux
curl https://storage.googleapis.com/git-repo-downloads/repo-1 > repo
chmod a+x repo
python3 repo init -u https://github.com/yonsei-sslab/asgard-manifest.git -b guest-linux
python3 repo sync -c
```

### 1.4 [CROSVM](https://github.com/yonsei-sslab/asgard-crosvm/tree/main)

```bash
# Create new directory in the main repository and get the source.
mkdir crosvm-android && cd crosvm-android
curl https://storage.googleapis.com/git-repo-downloads/repo-1 > repo
chmod a+x repo
python3 repo init -u https://github.com/yonsei-sslab/asgard-manifest.git -b crosvm-android
python3 repo sync -c
```

## 2. Building Sources *(20 human-minutes + 85 compute-minutes)*

The host Android, host kernel, enclave kernel, CROSVM, and DNN applications must be compiled on the host machine.
This process is expected to take a total 20 human-minutes and 85 compute-minutes on a machine with Intel i9-12900K CPU (16 physical cores) and 64GB of RAM.

### 2.1 [Host Kernel](https://github.com/yonsei-sslab/asgard-linux/tree/host) and Android *(2 human-minutes + 70 compute-minutes)*

```bash
# Create a new directory called 'build' in the main repository.
mkdir build

# Go to host android directory.
cd host-android

# Do some warmups.
source build/envsetup.sh
lunch kedge2-eng

# Build everything.
./build.sh -AUCKu -J $(nproc)

# Check the newly-built image and then move it to the build directory.
ls -al rockdev/Image-kedge2/update.img
cp rockdev/Image-kedge2/update.img ../build/update.img

# Move kernel modules to the build directory. 
cp kernel-5.10/drivers/rknpu/rknpu.ko ../build/rknpu.ko
cp kernel-5.10/drivers/iommu/rockchip-iommu.ko ../build/rockchip-iommu.ko
cp kernel-5.10/drivers/iommu/pkvm-rockchip-iommu.ko ../build/pkvm-rockchip-iommu.ko
```

### 2.2 [Enclave Kernel](https://github.com/yonsei-sslab/asgard-linux/tree/guest) *(7 human-minutes + 3 compute-minutes)*

```bash
# NOTE: We assume that the build directory has been created in (2.1).

cd guest-linux

# Build kernel. This is expected to fail during link-time due to our custom Makefile.
BUILD_CONFIG=common/build.config.protected_vm.aarch64 build/build.sh -j $(nproc)

# Now, replace the .config file in guest-linux with our asgard.config file.
rm out/android13-5.10/common/.config
cp ../configs/asgard.config out/android13-5.10/common/.config

# Build kernel with asgard.config file. Make sure to use 'build/build_asgard.sh'.
BUILD_CONFIG=common/build.config.protected_vm.aarch64 build/build_asgard.sh -j $(nproc)

# Check the newly built outputs and then move them to the build directory.
ls -al out/android13-5.10/dist/Image
ls -al out/android13-5.10/dist/vmlinux
cp out/android13-5.10/dist/Image ../build/Image_minimal
cp out/android13-5.10/dist/vmlinux ../build/vmlinux_minimal

# Finally, replace the .config file in guest-linux with our asgard_for_android_lib.config file.
rm out/android13-5.10/common/.config
cp ../configs/asgard_for_android_lib.config out/android13-5.10/common/.config

# Build kernel again with asgard_for_android_lib.config file. Make sure to use 'build/build_asgard.sh'.
BUILD_CONFIG=common/build.config.protected_vm.aarch64 build/build_asgard.sh -j $(nproc)

# Check the newly built outputs and then move them to the build directory.
ls -al out/android13-5.10/dist/Image
ls -al out/android13-5.10/dist/vmlinux
cp out/android13-5.10/dist/Image ../build/Image
cp out/android13-5.10/dist/vmlinux ../build/vmlinux
```

### 2.3 [CROSVM](https://github.com/yonsei-sslab/asgard-crosvm/tree/main) *(2 human-minutes + 7 compute-minutes)*

```bash
cd crosvm-android

# Do some warmups.
source build/envsetup.sh
lunch armv8-eng

# Build crosvm.
m crosvm -j $(nproc)

# Check the newly built output and then move it to the build directory.
ls -al out/target/product/armv8/system/bin/crosvm
cp out/target/product/armv8/system/bin/crosvm ../build/crosvm
```

### 2.4 [DNN Applications](https://github.com/yonsei-sslab/asgard-tensorflow/tree/main) *(7 human-minutes + 5 compute-minutes)*

```bash
cd asgard-tensorflow

# Download and unzip Android NDK 20.
wget https://dl.google.com/android/repository/android-ndk-r20b-linux-x86_64.zip
unzip android-ndk-r20b-linux-x86_64.zip

# Create and .tf_configure.bazelrc.
vim .tf_configure.bazelrc

# Configure and save .tf_configure.bazelrc as the following.
build --action_env ANDROID_NDK_HOME="./android-ndk-r20b"
build --action_env ANDROID_NDK_API_LEVEL="20"

# Build linux app.
# If bazel is not installed: https://bazel.build/install/ubuntu#install-on-ubuntu
bazel build -c opt --config=elinux_aarch64 //asgard:guest_inference_minimal

# Check output and move the binary to the build directory.
ls -al bazel-bin/asgard
cp bazel-bin/asgard/guest_inference_minimal ../build/guest_inference_minimal

# Now, build android apps.
bazel build -c opt --config=android_arm64 --build_tag_filters=-no_android //asgard/...

# Check output and move all binaries to the build directory.
ls -al bazel-bin/asgard
cp bazel-bin/asgard/vm_service ../build/vm_service
cp bazel-bin/asgard/guest_inference ../build/guest_inference
cp bazel-bin/asgard/guest_inference_tsdp ../build/guest_inference_tsdp
cp bazel-bin/asgard/guest_inference_ssd_mobilenetv1_coalesced ../build/guest_inference_ssd_mobilenetv1_coalesced
cp bazel-bin/asgard/guest_inference_ssd_inceptionv2_coalesced ../build/guest_inference_ssd_inceptionv2_coalesced
cp bazel-bin/asgard/guest_inference_lite_transformer_encoder_coalesced ../build/guest_inference_lite_transformer_encoder_coalesced
cp bazel-bin/asgard/guest_inference_lite_transformer_decoder_coalesced ../build/guest_inference_lite_transformer_decoder_coalesced
cp bazel-bin/asgard/host_inference ../build/host_inference
cp bazel-bin/asgard/host_inference_tsdp ../build/host_inference_tsdp
cp bazel-bin/asgard/host_inference_native ../build/host_inference_native
cp bazel-bin/asgard/host_inference_lite_transformer ../build/host_inference_lite_transformer
cp bazel-bin/asgard/host_inference_native_ssd_mobilenetv1_baseline ../build/host_inference_native_ssd_mobilenetv1_baseline
cp bazel-bin/asgard/host_inference_native_ssd_inceptionv2_baseline ../build/host_inference_native_ssd_inceptionv2_baseline
cp bazel-bin/asgard/host_inference_native_lite_transformer_encoder_baseline ../build/host_inference_native_lite_transformer_encoder_baseline
cp bazel-bin/asgard/host_inference_native_lite_transformer_decoder_baseline ../build/host_inference_native_lite_transformer_decoder_baseline
```

### 2.5 Miscellaneous Programs *(2 human-minutes)*

```bash
# Build NPU-related program.
cd src/rknpu-power
make

# Copy the binary to the build directory.
cp build/rknpu_power_on ../../build/rknpu_power_on

# Build guest init program.
cd ../guest-init
make

# Copy the binary to the build directory.
cp build/guest_inference_init ../../build/guest_inference_init
```

## 3. Configuring and Installing the Artifact *(27 human-minutes + 37 compute-minutes)*

We now install the new host image on the development board by following (3.1).
Then, in (3.2), we use Buildroot to create the enclave root file system image, to which we add DNN models, DNN applications, and user-mode NPU driver to the image, and from which remove the unnecessary binaries.
Finally, in (3.3), we move all the necessary files to the development board.

### 3.1 Install New Image on Dev. Board *(10 human-minutes + 5 compute-minutes)*

1. Boot the development board into upgrade mode.
```bash
# In a new terminal window, access the development board using adb.
./bin/adb shell

# Reboot the development board into upgrade mode.
reboot loader
```

2. Check that the development board has booted into upgrade mode.
```bash
# In a terminal window, check the device status with upgrade_tool.
sudo ./bin/upgrade_tool

# It should output 'Mode=Loader' like the following:
Not found config.ini
Program Data in /your/path/to/upgrade_tool
List of rockusb connected
DevNo=1	Vid=0x2207,Pid=0x350b,LocationID=1b	Mode=Loader	SerialNo=....
Found 1 rockusb,Select input DevNo,Rescan press <R>,Quit press <Q>:
```

3. Flash the development board with the new host image.
``` bash
# In a terminal window, flash the development board. This will take few minutes.
sudo ./bin/upgrade_tool uf build/update.img

# The flashing might sometimes fail due to the buggy (and proprietary) flashing tool (e.g., 'Download Firmware Fail').
# Please first try resetting the device.
sudo ./bin/upgrade_tool rd

# Then, after wating for a minute, reboot the development board into upgrade mode.
./bin/adb shell
reboot loader
```

4. The development board should boot with the new host image. After wating for a minute, run `./bin/adb shell` in a terminal window.

### 3.2 Build Root File System *(15 human-minutes + 30 compute-minutes)*

Buildroot is used to create the enclave root file system image.

1. Add necessary files to the root file system image.
```bash
# Create a new directory 'asgard-buildroot/files/files'.
cd asgard-buildroot && mkdir files
cd files && mkdir files
cd ..

# Move all necessary files to the new directory. These include:
# DNN models, user-mode NPU driver, DNN inference apps, and DNN inference app launch scripts.
cp -r ../assets/models files/files/
cp -r ../bin/librknnrt files/files/
cp ../build/guest_inference* files/files/
cp ../scripts/run-inference-guest* files/files/
```

2. Build the file system.
```bash
# Cross-compile at x86 host machine.
make crosvm_aarch64_virt_rknpu_defconfig
make ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- -j $(nproc)

# Check that the files added in Step 1 have been correctly added to the file system.
ls -al output/target/files

# Check the newly built image and move it to the build directory.
ls -al output/images/rootfs.ext4
cp output/images/rootfs.ext4 ../build/rootfs.ext4
```

3. Build the file system again with the minimal config.
```bash
# Clean up the old build.
make clean

# Cross-compile at x86 host machine. Make sure to use 'crosvm_aarch64_virt_rknpu_minimal_defconfig'.
make crosvm_aarch64_virt_rknpu_minimal_defconfig
make ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- -j $(nproc)

# Check that the files added in Step 1 have been correctly added to the file system.
ls -al output/target/files

# Check the newly built image and move it to the build directory as 'rootfs_minimal.ext4'.
ls -al output/images/rootfs.ext4
cp output/images/rootfs.ext4 ../build/rootfs_minimal.ext4
```

4. Remove unnecessary binaries from the minimal root file system.
```bash
# In your host machine, create /mnt/rootfs.
sudo mkdir -p /mnt/rootfs

# Mount rootfs.
sudo mount ../build/rootfs_minimal.ext4 /mnt/rootfs
ls -l /mnt/rootfs

# Remove files from rootfs.
sudo rm -r /mnt/rootfs/etc
sudo rm /mnt/rootfs/lib/libanl.so.1
sudo rm /mnt/rootfs/lib/libatomic.so.1.2.0
sudo rm /mnt/rootfs/lib/libcrypt.so.1
sudo rm /mnt/rootfs/lib/libgcc_s.so
sudo rm /mnt/rootfs/lib/libnss_dns.so.2
sudo rm /mnt/rootfs/lib/libnss_files.so.2
sudo rm /mnt/rootfs/lib/libresolv.so.2
sudo rm /mnt/rootfs/lib/librt.so.1
sudo rm /mnt/rootfs/lib/libutil.so.1
sudo rm /mnt/rootfs/usr/lib/libstdc++.so.6.0.29-gdb.py

# Unmount rootfs.
sudo umount /mnt/rootfs
```

### 3.3 Move Files to Dev. Board *(2 human-minutes + 2 compute-minute)*

```bash
# In the host machine, go to the main repository.

# Push all necessary files to /data/local.
./bin/adb push build/host_* /data/local
./bin/adb push build/Image* /data/local
./bin/adb push build/*.ko /data/local
./bin/adb push build/*.ext4 /data/local
./bin/adb push build/crosvm /data/local
./bin/adb push build/vm_service /data/local
./bin/adb push build/rknpu_power_on /data/local
./bin/adb push bin/librknnrt /data/local
./bin/adb push scripts /data/local
./bin/adb push assets/models /data/local
./bin/adb push assets/images /data/local
```

## 4. Running Evaluation *(105 human-minutes + 20 compute-minutes)*

The artifact contains a total of six experiments.
The first four experiments involve verifying the source code and proprietary binaries used in the artifact, all of which can be executed on the host machine.
The last two experiments involve running DNN inference on the development board.

### (E1) Verify Secure Monitor and User-Mode NPU Driver *(10 human-minutes)*

ASGARD does not require any modifications to the closed-source secure monitor or the user-mode NPU driver.
The experiment involves verifying that these components will be used throughout the evaluation.

#### Preparation

The directory `bin/rk3588_bl31_v1.26.elf` contains the proprietary RK3588S secure monitor from the [device vendor](https://github.com/rockchip-linux/rkbin/blob/ae710c9ebffa72785f162a397303cd63425b65a3/bin/rk35/rk3588_bl31_v1.26.elf), and `bin/librknnrt/linux/librknnrt.so` contains the proprietary user-mode Rockchip NPU driver from the [device vendor](https://github.com/rockchip-linux/rknn-toolkit2/blob/1f4415eafe8f578787822fc836f38df3ef3b5627/rknpu2/runtime/Linux/librknn_api/aarch64/librknnrt.so).
In the host machine, go to the main repository.

#### Execution

To verify that we are running the unmodified proprietary secure monitor:
1. Check that the bootloader (`host-android/u-boot`) is [configured](https://github.com/khadas/u-boot/blob/bb307d6a09d1de5d0fc001584e3f62c0d73bad95/configs/kedge2_defconfig#L16) to [load](https://github.com/khadas/android_rkbin/blob/35c3e2e438d2a6983a958fd2d3a66076112bea23/RKTRUST/RK3588TRUST.ini#L8) the proprietary secure monitor (`host-android/rkbin/bin/rk35/rk3588_bl31_v1.26.elf`). (Unfortunately, this cannot be checked during device runtime.)
2. In the main repository, run `cmp -l bin/rk3588_bl31_v1.26.elf host-android/rkbin/bin/rk35/rk3588_bl31_v1.26.elf` to compare the two binary files byte to byte.

To verify that we are running the unmodified proprietary user-mode NPU driver, we will run protected DNN inference during experiment (E5) using the proprietary driver (`bin/librknnrt/linux/librknnrt.so`), which was embedded into the root file system in (3.2).

#### Results

For the secure monitor, the `cmp` command does not produce any output if no differences are found.
For the user-mode NPU driver, the experiment (E5) must be completed successfully.

### (E2) Verify Kernel-Mode NPU Driver *(10 human-minutes)*

ASGARD employs an unmodified kernel-mode NPU driver in both the REE and the enclave.
The experiment involves verifying that the unmodified driver will be used throughout the evaluation.

#### Preparation

The directory `src/rknpu` contains an original driver code obtained from the [device vendor](https://github.com/khadas/linux/tree/973dd55ebe493404d72d646d53d434ed412e68f1/drivers/rknpu).
In the host machine, go to the main repository.

#### Execution

To verify the drivers:
```bash
# Compare with enclave RKNPU driver.
diff -r src/rknpu guest-linux/common/drivers/rknpu

# Compare with REE RKNPU driver.
diff -r src/rknpu host-android/kernel-5.10/drivers/rknpu
```

#### Results

The `diff` command does not produce any output if no differences are found.

However, in this artifact submission, we have included our custom performance measurement framework, which will be used in experiments (E5) and (E6).
This framework **does not introduce any functional changes** to the driver (i.e., affect the driver’s original behavior).

The framework consists of:
- A component that measures the NPU task completion time using the program counter register (`CNTPCT_EL0`).
- IOCTL handlers and header definitions to acquire and clear the performance measurement data.

### (E3) Verify TEEvisor TCB Size *(5 human-minutes)*

ASGARD introduces 2 kLoC to the TEEvisor (see Section VI.B).
The experiment involves measuring the LoC of the original, unmodified TEEvisor and ASGARD’s TEEvisor.

#### Preparation

The directory `src/hyp` contains an original TEEvisor (pKVM EL2 hypervisor) obtained from the [Android common kernel](https://android.googlesource.com/kernel/common/+/refs/heads/deprecated/android13-5.10-2022-11/arch/arm64/kvm/hyp/).
In the host machine, go to the main repository.

#### Execution

To verify the LoC changes with *cloc*:
```bash
# Check the LoC of the original TEEvisor. Exclude code that are not compiled.
cloc src/hyp --exclude-list=\
src/hyp/vhe,\
src/hyp/include/nvhe/debug,\
src/hyp/include/nvhe/iommu/s2mpu.c

# Check the LoC of our TEEvisor. Exclude code that are not compiled.
cloc host-android/kernel-5.10/arch/arm64/kvm/hyp --exclude-list=\
host-android/kernel-5.10/arch/arm64/kvm/hyp/vhe,\
host-android/kernel-5.10/arch/arm64/kvm/hyp/include/nvhe/debug,\
host-android/kernel-5.10/arch/arm64/kvm/hyp/include/nvhe/iommu/s2mpu.c
```

#### Results

The value in the `code` column and the `SUM` row represents the total LoC for the TEEvisor.
Subtract the original TEEvisor’s value from ASGARD’s value, which should be about 2 kLoC.

### (E4) Verify Enclave Image Size *(10 human-minutes)*

ASGARD achieves an enclave image size of 17.439 MB (see Table III), which consists of the kernel and the root file system.
The experiment involves measuring the size of the kernel and file system images.

#### Preparation

In the host machine, go to the main repository.

#### Execution

To verify the kernel image size:
```bash
aarch64-linux-gnu-size build/vmlinux_minimal
```

To verify the root file system image size:
```bash
cd asgard-buildroot
make graph-size

# Check all sizes of core utilities and C/C++ standard libraries and linkers that are included in the image.
cat output/graphs/file-size-stats.csv

# The files that we removed from the image in (3.2) should not be counted.
# Run our python script, which adds values in the `File size` column for the selected rows.
python3 ../scripts/get_rootfs_size.py output/graphs/file-size-stats.csv

# Finally, check the size of the DNN applications and user-mode NPU driver.
ls -l files/files/guest_inference
ls -l files/files/guest_inference_init
ls -l files/files/librknnrt/linux/librknnrt.so
```

#### Results

For the kernel image, the `aarch64-linux-gnu-size` command should output 7.936 MB in the `dec` column.

For the root file system image, we must add the outputs for the C/C++ libraries and linkers (3.870 MB), DNN applications (0.014 and 0.009 MB), and user-mode NPU driver (5.610 MB).
The sum of these should be 9.503 MB.

### (E5) Compare Inference Latency with REE *(60 human-minutes + 10 compute-minutes)*

ASGARD achieves near-zero DNN inference latency overhead compared to that in the REE (see Figure 8b).
The experiment involves running unprotected DNN inference in the REE and protected inference within the ASGARD enclave, using all six DNN models.

#### Preparation (REE)

```bash
# Access the development board using adb.
./bin/adb shell
cd /data/local

# Load the original Rockchip IOMMU driver.
insmod rockchip-iommu.ko
insmod rknpu.ko

# Do some setups.
./scripts/enable-cpu4-7-only.sh
export LD_LIBRARY_PATH=librknnrt/android

# Run inference in the REE by following the instruction in the Execution section.
```

#### Execution (REE)

```bash
# Run inference with MobileNetV1 and InceptionV3 for 100 iterations.
# After running inference with each model, check the results by following the instruction in the Results section.
./host_inference_native models/mobilenetv1.rknn images/dog_224x224.jpg 100
./host_inference_native models/inceptionv3.rknn images/dog_224x224.jpg 100

# Run inference with SSD models for 100 iterations.
# After running inference with each model, check the results by following the instruction in the Results section.
./scripts/run-inference-host-native-ssd-mobilenetv1-baseline.sh
./scripts/run-inference-host-native-ssd-inceptionv2-baseline.sh

# Run inference with lite transformer models for 100 iterations.
# After running inference with each model, check the results by following the instruction in the Results section.
./scripts/run-inference-host-native-lite-transformer-encoder-baseline.sh
./scripts/run-inference-host-native-lite-transformer-decoder-baseline.sh
```

#### Results (REE)

The unprotected inference in the REE should exhibit the latency value shown by the 'REE' bar in Figure 8b.

```bash
# Access the development board using adb.
./bin/adb shell
cd /data/local

# Print results to the console.
cat output.csv

# For MobileNetV1 and InceptionV3: Check the numbers in the 'inference only' column. The numbers are in milliseconds (ms).
# For SSD and lite transformer models: Check the numbers in the 'inference total' column. The numbers are in milliseconds (ms).
```

#### Preparation (ASGARD w/ Minimal Enclave Image)

```bash
# Reboot development board.
./bin/adb shell
reboot

# After waiting for few seconds, access the development board using adb.
./bin/adb shell
cd /data/local

# Load ASGARD's Rockchip IOMMU driver.
insmod pkvm-rockchip-iommu.ko
insmod rknpu.ko

# Boot the enclave with NPU support enabled.
./scripts/enable-cpu4-7-only.sh
./rknpu_power_on
./scripts/setup-vfio-platform-rknpu-android.sh
./scripts/start-crosvm-custom-init-cid-pvm-rknpu.sh

# The model is automatically loaded with the guest init program.
```

#### Execution (ASGARD w/ Minimal Enclave Image)

```bash
# NOTE: The ASGARD enclave must be running.

# Access the development board from a separate terminal window.
./bin/adb shell
cd /data/local

# Set up a connection to the enclave.
rm 3.sock
./vm_service 3 /data/local/3.sock &

# Run inference for 100 iterations.
./host_inference images/dog_224x224.jpg /data/local/3.sock /dev/io-mem-0 100
```

#### Results (ASGARD w/ Minimal Enclave Image)

To verify that (E1) and (E4) are valid (i.e., can run inference with proprietary user-mode NPU driver and with minimal enclave image), the inference must be completed successfully.

However, to precisely measure ASGARD's inference latency overhead, we use the proprietary user-mode NPU driver compiled for android (`bin/librknnrt/android/librknnrt.so`).
This is because the driver compiled for linux (`bin/librknnrt/linux/librknnrt.so`) is compiled with different optimization option and will behave slightly differently.
Below, we measure the latency overhead of ASGARD with the android driver and with different enclave file system image (android system library added to `/system`) enclave kernel image (`sysfs` and `procfs` enabled).

#### Preparation (ASGARD)

```bash
# Reboot development board.
./bin/adb shell
reboot

# After waiting for few seconds, access the development board using adb.
./bin/adb shell
cd /data/local

# Load ASGARD's Rockchip IOMMU driver.
insmod pkvm-rockchip-iommu.ko
insmod rknpu.ko

# Boot the enclave with NPU support enabled.
./scripts/enable-cpu4-7-only.sh
./rknpu_power_on
./scripts/setup-vfio-platform-rknpu-android.sh
./scripts/start-crosvm-cid-pvm-rknpu.sh

# Do some setups.
cd files
mount -t proc proc /proc && mount -t sysfs sys /sys
export LD_LIBRARY_PATH=librknnrt/android

# Only one model can be loaded at a time.
# To load MobileNetV1 or InceptionV3:
./guest_inference models/mobilenetv1.rknn
./guest_inference models/inceptionv3.rknn

# To load one of the SSD models:
./run-inference-guest-ssd-mobilenetv1-coalesced.sh
./run-inference-guest-ssd-inceptionv2-coalesced.sh

# To load one of the lite transformer models:
./run-inference-guest-lite-transformer-encoder-coalesced.sh
./run-inference-guest-lite-transformer-decoder-coalesced.sh

# Run ASGARD-protected inference by following the instruction in the Execution section.

# Loading a different model requires rebooting the enclave. This requires rebooting the development board.
# Access the development board from a separate terminal window.
./bin/adb shell
reboot

# NOTE: If the development board gets stuck in a non-recoverable state, please try to power-cycle the development board.
```

#### Execution (ASGARD)

```bash
# NOTE: The ASGARD enclave must be running.

# Access the development board from a separate terminal window.
./bin/adb shell
cd /data/local

# Set up a connection to the enclave.
rm 3.sock
./vm_service 3 /data/local/3.sock &

# For MobileNetV1, InceptionV3, and SSD models: Run inference for 100 iterations.
# NOTE: The user-mode NPU driver is buggy and could sometimes fail. This will require rebooting the development board.
./host_inference images/dog_224x224.jpg /data/local/3.sock /dev/io-mem-0 100

# For lite transformer models: Run inference for 100 iterations.
./host_inference_lite_transformer /data/local/3.sock /dev/io-mem-0 100

# Check the results by following the instruction in the Results section.

# NOTE: Running inference with a different model requires rebooting the enclave. This requires rebooting the development board.
```

#### Results (ASGARD)

The ASGARD-protected inference with MobileNetV1 and InceptionV3 should exhibit the latency value shown by the 'ASGARD w/ Default Planning' bar in Figure 8b.
The inference with SSD and Lite Transformer models should exhibit the latency value shown by the 'ASGARD w/ Exit-Coalescing Planning' bar in Figure 8b.

```bash
# Access the development board using adb.
./bin/adb shell
cd /data/local

# Print results to the console.
cat output.csv

# Check the numbers in the 'inference only' column. The numbers are in milliseconds (ms).
# This does not include the latency for acquiring and releasing the NPU.
```

### (E6) Compare Inference Latency with ShadowNet *(10 human-minutes + 10 compute-minutes)*

ASGARD achieves DNN inference latency overhead that is significantly lower than that of existing REE-offloading approaches (see Figures 5b and 5c).
The experiment involves running DNN inference simulating ShadowNet and inference within the ASGARD enclave, using MobileNetV1.

#### Preparation (ShadowNet)

```bash
# Access the development board using adb.
./bin/adb shell
cd /data/local

# Load the original Rockchip IOMMU driver.
insmod rockchip-iommu.ko
insmod rknpu.ko

# Boot the enclave with NPU support disabled.
./scripts/enable-cpu4-7-only.sh
./scripts/start-crosvm-cid-pvm.sh

# Load the model inside the enclave.
cd files
mount -t proc proc /proc && mount -t sysfs sys /sys
./run-inference-guest-tsdp-mobilenetv1.sh
```

#### Execution (ShadowNet)

```bash
# NOTE: Only the ShadowNet enclave must be running.

# Access the development board from a separate terminal window.
./bin/adb shell
cd /data/local

# Set up a connection to the enclave.
rm 3.sock
./vm_service 3 /data/local/3.sock &

# Run inference for 100 iterations.
export LD_LIBRARY_PATH=librknnrt/android
./scripts/run-inference-host-tsdp-mobilenetv1.sh
```

#### Results (ShadowNet)

The inference latency numbers should match that in Figure 5b.

```bash
# Access the development board using adb.
./bin/adb shell
cd /data/local

# Print results to the console.
cat output.csv

# Check the numbers in the 'inference total' column. The numbers are in milliseconds (ms).
```

#### Preparation (ASGARD)

```bash
# Reboot development board.
./bin/adb shell
reboot

# After waiting for few seconds, access the development board using adb.
./bin/adb shell
cd /data/local

# Load ASGARD's Rockchip IOMMU driver.
insmod pkvm-rockchip-iommu.ko
insmod rknpu.ko

# Boot the enclave with NPU support enabled.
./scripts/enable-cpu4-7-only.sh
./rknpu_power_on
./scripts/setup-vfio-platform-rknpu-android.sh
./scripts/start-crosvm-cid-pvm-rknpu.sh

# Load the model inside the enclave.
cd files
mount -t proc proc /proc && mount -t sysfs sys /sys
export LD_LIBRARY_PATH=librknnrt/android
./guest_inference models/mobilenetv1.rknn

# NOTE: If the development board gets stuck in a non-recoverable state, please try to power-cycle the development board.
```

#### Execution (ASGARD)

```bash
# NOTE: Only the ASGARD enclave must be running.

# Access the development board from a separate terminal window.
./bin/adb shell
cd /data/local

# Set up a connection to the enclave.
rm 3.sock
./vm_service 3 /data/local/3.sock &

# Run inference for 100 iterations.
./host_inference images/dog_224x224.jpg /data/local/3.sock /dev/io-mem-0 100
```

#### Results (ASGARD)

The inference latency numbers should match that in Figure 5c.

```bash
# Access the development board using adb.
./bin/adb shell
cd /data/local

# Print results to the console.
cat output.csv

# Check the numbers in the 'inference and hypercall' column. The numbers are in milliseconds (ms).
# This includes the latency for acquiring and releasing the NPU.
```

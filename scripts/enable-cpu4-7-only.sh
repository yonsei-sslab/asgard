for x in /sys/devices/system/cpu/cpu[0-3]*/online; do
       echo 0 > "$x"
done

cat /sys/devices/system/cpu/online

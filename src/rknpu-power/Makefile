RKNPU_POWER_ON		= rknpu_power_on

OUT_DIR	= build
CC		= aarch64-linux-gnu-gcc
RM		= rm -f

all: $(RKNPU_POWER_ON)

$(RKNPU_POWER_ON): | $(OUT_DIR)
	$(CC) $(RKNPU_POWER_ON).c -o $(OUT_DIR)/$(RKNPU_POWER_ON) -static

$(OUT_DIR):
	mkdir $(OUT_DIR)

clean:
	$(RM) $(OUT_DIR)/*

.PHONY: all clean $(RKNPU_POWER_ON)

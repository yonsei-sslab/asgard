GUEST_INF_INIT = guest_inference_init

OUT_DIR	= build
CC		= aarch64-linux-gnu-gcc
RM		= rm -f

all: $(GUEST_INF_INIT)

$(GUEST_INF_INIT): | $(OUT_DIR)
	$(CC) $(GUEST_INF_INIT).c -o $(OUT_DIR)/$(GUEST_INF_INIT)

$(OUT_DIR):
	mkdir $(OUT_DIR)

clean:
	$(RM) $(OUT_DIR)/*

.PHONY: all clean $(GUEST_INF_INIT)

FALCO_BUILD_PATH = $(TOPDIR)/../linx-behavior-detection/build
FALCO_LIBS_PATH = $(TOPDIR)/../linx-behavior-detection/thirdparty/libs

PLUGIN_SRC		= $(wildcard $(TOPDIR)/plugin/*/*.c $(TOPDIR)/plugin/*.c)
PLUGIN_OBJ		= $(PLUGIN_SRC:.c=.o)

C_FLAGS			= -fPIC -Wall -g -O2
INCLUDE			= -I$(TOPDIR)/include -I$(TOPDIR)/plugin/plugin_config/include \
				  -I$(TOPDIR)/plugin/plugin/include -I$(TOPDIR)/ebpf/include \
				  -I$(TOPDIR)/plugin/ebpf_user/include -I$(TOPDIR)/plugin/linx_event_rich/include \
				  -I$(TOPDIR)/plugin/linx_event_putfile/include \
				  -I$(TOPDIR)/plugin/linx_log/include -I$(FALCO_LIBS_PATH) \
				  -I$(FALCO_LIBS_PATH)/userspace -I$(FALCO_LIBS_PATH)/userspace/libscap
SHARED			= -shared
LIBS			= -lbpf -lcjson -static \
				  -Wl,--whole-archive \
				  $(FALCO_BUILD_PATH)/libscap/libscap_event_schema.a \
				  $(FALCO_BUILD_PATH)/libscap/libscap_error.a \
				  -Wl,--no-whole-archive

.PHONY: all clean

%.o: %.c
	@echo "[CC] $<"
	@$(CC) $(C_FLAGS) $(INCLUDE) -c $< -o $@

all: $(PLUGIN_OUT)

$(PLUGIN_OUT): $(PLUGIN_OBJ)
	@echo "[CC] $@"
	@$(CC) $(C_FLAGS) $(SHARED) $^ -o $@ $(LIBS)

clean:
	@echo "[RM] $(PLUGIN_OBJ) $(PLUGIN_OUT)"
	@rm -rf $(PLUGIN_OBJ) $(PLUGIN_OUT)

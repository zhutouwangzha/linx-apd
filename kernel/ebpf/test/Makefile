FALCO_BUILD_PATH = $(TOPDIR)/../linx-behavior-detection/build
FALCO_LIBS_PATH = $(TOPDIR)/../linx-behavior-detection/thirdparty/libs

TEST_SRC		= $(wildcard $(TOPDIR)/test/*.c)
TEST_OBJ		= $(TEST_SRC:.c=.o)
TEST_OUT		= $(BUILD_PATH)/syscall_test

C_FLAGS			= -fPIC -Wall -g -O2
INCLUDE			= 
SHARED			= 
LIBS			= 

.PHONY: all clean

%.o: %.c
	@echo "[CC] $<"
	@$(CC) $(C_FLAGS) $(INCLUDE) -c $< -o $@

all: $(TEST_OUT)

$(TEST_OUT): $(TEST_OBJ)
	@echo "[CC] $@"
	@$(CC) $(C_FLAGS) $(SHARED) $^ -o $@ $(LIBS)

clean:
	@echo "[RM] $(TEST_OBJ) $(TEST_OUT)"
	@rm -rf $(TEST_OBJ) $(TEST_OUT)

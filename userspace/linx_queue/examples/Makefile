CC := gcc
CFLAGS := -Wall -Wextra -g -std=c99 -D_GNU_SOURCE
INCLUDES := -I../include
LDFLAGS := -pthread

# 目标文件
TARGETS := log_example alert_example event_example

# 源文件
LOG_SOURCES := linx_log_queue_example.c ../linx_queue.c
ALERT_SOURCES := linx_alert_queue_example.c ../linx_queue.c
EVENT_SOURCES := linx_event_queue_example.c ../linx_queue.c

.PHONY: all clean run-all run-log run-alert run-event

all: $(TARGETS)

log_example: $(LOG_SOURCES)
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $^ $(LDFLAGS)

alert_example: $(ALERT_SOURCES)
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $^ $(LDFLAGS)

event_example: $(EVENT_SOURCES)
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $^ $(LDFLAGS)

run-log: log_example
	@echo "=== Running Log Queue Example ==="
	./log_example

run-alert: alert_example
	@echo "=== Running Alert Queue Example ==="
	./alert_example

run-event: event_example
	@echo "=== Running Event Queue Example ==="
	./event_example

run-all: all
	@echo "Running all examples..."
	@echo ""
	@make run-log
	@echo ""
	@make run-alert  
	@echo ""
	@make run-event

clean:
	rm -f $(TARGETS)

help:
	@echo "Available targets:"
	@echo "  all        - Build all examples"
	@echo "  log_example - Build log queue example"
	@echo "  alert_example - Build alert queue example" 
	@echo "  event_example - Build event queue example"
	@echo "  run-log    - Run log queue example"
	@echo "  run-alert  - Run alert queue example"
	@echo "  run-event  - Run event queue example"
	@echo "  run-all    - Run all examples"
	@echo "  clean      - Clean built files"
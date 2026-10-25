CC = gcc
CFLAGS = -Wall -Wextra -O2 -Iinclude -std=gnu99 -D_WIN32_WINNT=0x0600 -D__USE_MINGW_ANSI_STDIO=1
LDFLAGS =

ifeq ($(OS),Windows_NT)
    TARGET = bin/os_platform_backend.exe
    TEST_TARGET = bin/test_monitoring.exe
    TEST2_TARGET = bin/test_phase2.exe
    TEST3_TARGET = bin/test_phase3.exe
    LDFLAGS += -lws2_32 -lpsapi
    MKDIR = if not exist bin mkdir bin
    RM = del /Q /F
else
    TARGET = bin/os_platform_backend
    TEST_TARGET = bin/test_monitoring
    TEST2_TARGET = bin/test_phase2
    TEST3_TARGET = bin/test_phase3
    LDFLAGS += -lpthread -lm
    MKDIR = mkdir -p bin
    RM = rm -rf
endif

SRCS = src/main.c \
       src/api/server.c \
       src/api/routes.c \
       src/monitoring/process_monitor.c \
       src/monitoring/cpu_monitor.c \
       src/monitoring/memory_monitor.c \
       src/scheduling/cpu_scheduler.c \
       src/memory/memory_prediction.c \
       src/page_replacement/page_replacement.c \
       src/deadlock/deadlock.c \
       src/logs/log_analyzer.c \
       src/advisory/resource_advisory.c \
       src/utils/json_utils.c

TEST_SRCS = tests/test_monitoring.c \
            src/monitoring/process_monitor.c \
            src/monitoring/cpu_monitor.c \
            src/monitoring/memory_monitor.c \
            src/utils/json_utils.c

TEST2_SRCS = tests/test_phase2.c \
             src/scheduling/cpu_scheduler.c \
             src/memory/memory_prediction.c \
             src/page_replacement/page_replacement.c \
             src/monitoring/memory_monitor.c \
             src/utils/json_utils.c

TEST3_SRCS = tests/test_phase3.c \
             src/deadlock/deadlock.c \
             src/logs/log_analyzer.c \
             src/advisory/resource_advisory.c \
             src/monitoring/cpu_monitor.c \
             src/monitoring/memory_monitor.c \
             src/monitoring/process_monitor.c \
             src/memory/memory_prediction.c \
             src/utils/json_utils.c

all: $(TARGET)

$(TARGET): $(SRCS)
	@$(MKDIR)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRCS) $(LDFLAGS)
	@echo [Build complete] $(TARGET)

test: $(TEST_TARGET) $(TEST2_TARGET) $(TEST3_TARGET)
	@echo [Running Phase 1 Monitoring Tests]
	$(TEST_TARGET)
	@echo.
	@echo [Running Phase 2 Algorithmic Tests]
	$(TEST2_TARGET)
	@echo.
	@echo [Running Phase 3 Advanced Systems Tests]
	$(TEST3_TARGET)

$(TEST_TARGET): $(TEST_SRCS)
	@$(MKDIR)
	$(CC) $(CFLAGS) -o $(TEST_TARGET) $(TEST_SRCS) $(LDFLAGS)

$(TEST2_TARGET): $(TEST2_SRCS)
	@$(MKDIR)
	$(CC) $(CFLAGS) -o $(TEST2_TARGET) $(TEST2_SRCS) $(LDFLAGS)

$(TEST3_TARGET): $(TEST3_SRCS)
	@$(MKDIR)
	$(CC) $(CFLAGS) -o $(TEST3_TARGET) $(TEST3_SRCS) $(LDFLAGS)

clean:
ifeq ($(OS),Windows_NT)
	-$(RM) bin\*.exe 2>nul
	-$(RM) bin\*.o 2>nul
else
	$(RM) bin
endif

run: $(TARGET)
	$(TARGET) --port 8080

.PHONY: all test clean run

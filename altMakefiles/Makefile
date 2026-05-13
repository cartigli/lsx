CC      := clang
TARGET  := lx_menu
SRC     := lx_menu.c
CFLAGS  := -Wall -Wextra -Wpedantic -g -O1 -fno-omit-frame-pointer
REPORTS := ./reports

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $@

$(REPORTS):
	mkdir -p $(REPORTS)

# ===== Static analysis =====
.PHONY: analyze-tidy
analyze-tidy: $(REPORTS)
	@echo ">>> clang-tidy"
	@clang-tidy $(SRC) \
	    --checks='clang-analyzer-*,bugprone-*,cert-*' \
	    --quiet \
	    -- $(CFLAGS) 2>/dev/null \
	    | tee $(REPORTS)/tidy.txt \
	    | grep -E '^(/|.*warning:|.*error:)' || true

.PHONY: analyze-cppcheck
analyze-cppcheck: $(REPORTS)
	@echo ">>> cppcheck"
	@cppcheck --enable=warning,performance,portability \
	    --inconclusive --std=c11 \
	    --suppress=missingIncludeSystem \
	    --template='{file}:{line}: {severity}: {message} [{id}]' \
	    $(SRC) 2> $(REPORTS)/cppcheck.txt; \
	    cat $(REPORTS)/cppcheck.txt

.PHONY: analyze-scan-build
analyze-scan-build: $(REPORTS)
	@echo ">>> scan-build (HTML report in $(REPORTS)/scan-build/)"
	@scan-build -o $(REPORTS)/scan-build --status-bugs \
	    $(CC) $(CFLAGS) $(SRC) -o /tmp/$(TARGET).scan 2>&1 \
	    | grep -E 'scan-build:' || true

.PHONY: analyze
analyze: analyze-tidy analyze-cppcheck analyze-scan-build
	@echo ""
	@echo "===== Summary ====="
	@echo "tidy:      $$(grep -c 'warning:' $(REPORTS)/tidy.txt 2>/dev/null || echo 0) warnings"
	@echo "cppcheck:  $$(grep -cE '(warning|error|performance|portability):' $(REPORTS)/cppcheck.txt 2>/dev/null || echo 0) issues"
	@echo "scan-build HTML: $(REPORTS)/scan-build/"

# ===== Dynamic analysis =====
.PHONY: asan
asan: $(REPORTS)
	@echo ">>> AddressSanitizer + UBSan"
	@$(CC) $(CFLAGS) -fsanitize=address,undefined $(SRC) -o $(TARGET)-asan
	@ASAN_OPTIONS=detect_leaks=1:abort_on_error=0 \
	 UBSAN_OPTIONS=print_stacktrace=1 \
	 ./$(TARGET)-asan 2>&1 | tee $(REPORTS)/asan.txt

.PHONY: valgrind
valgrind: $(REPORTS)
	@echo ">>> Valgrind"
	@$(CC) -Wall -Wextra -g -O0 $(SRC) -o $(TARGET)-dbg
	@valgrind --leak-check=full --show-leak-kinds=all \
	          --track-origins=yes --error-exitcode=0 \
	          ./$(TARGET)-dbg 2> $(REPORTS)/valgrind.txt; \
	          tail -30 $(REPORTS)/valgrind.txt

.PHONY: check
check: analyze asan valgrind
	@echo ""
	@echo "===== All reports in $(REPORTS)/ ====="
	@ls -la $(REPORTS)/

.PHONY: clean
clean:
	rm -f $(TARGET) $(TARGET)-asan $(TARGET)-dbg
	rm -rf $(REPORTS) scan-resutls lx.dSYM

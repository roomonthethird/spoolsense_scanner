CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -DNATIVE_TEST -I../../src -I.

.PHONY: all clean test

all: test_printer_manager

test_printer_manager: test_printer_manager.cpp MockPrinterStrategy.h NativePlatform.h test_helpers.h ../../src/IPrinterStrategy.h
	$(CXX) $(CXXFLAGS) -o $@ $<

test: test_printer_manager
	@echo ""
	@echo "=== Running PrinterManager tests ==="
	@./test_printer_manager
	@echo ""

clean:
	rm -f test_printer_manager

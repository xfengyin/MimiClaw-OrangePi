.PHONY: all libs plugins tests clean test docs coverage

all: libs plugins

libs:
	$(MAKE) -C libs

plugins:
	$(MAKE) -C plugins

tests:
	$(MAKE) -C tests

test: libs plugins
	$(MAKE) -C libs test

clean:
	$(MAKE) -C libs clean
	$(MAKE) -C plugins clean
	$(MAKE) -C tests clean

docs:
	cd docs/doxygen && doxygen Doxyfile

coverage: clean
	$(MAKE) -C libs COVERAGE=1
	$(MAKE) -C libs test
	lcov --capture --directory libs --output-file coverage.info
	genhtml coverage.info --output-directory coverage-report

install: all
	$(MAKE) -C libs install
	$(MAKE) -C plugins install

.PHONY: help
help:
	@echo "MimiClaw-OrangePi v2.0 Build System"
	@echo ""
	@echo "Targets:"
	@echo "  all       - Build libraries and plugins (default)"
	@echo "  libs      - Build core libraries"
	@echo "  plugins   - Build plugins"
	@echo "  test      - Run all tests"
	@echo "  clean     - Clean build files"
	@echo "  docs      - Generate API documentation"
	@echo "  coverage  - Generate coverage report"
	@echo "  install   - Install to system"
	@echo "  help      - Show this help"

BIN = ./node_modules/.bin
REPORTER = spec

clean:
	@rm -rf lib-cov test/fixtures/*/build.css

lint:
	@$(BIN)/jshint bin lib test

node_modules: package.json
	@npm install
	@touch node_modules

test: clean lint node_modules
	@$(BIN)/_mocha \
	  --reporter $(REPORTER)

test-cov: clean lint node_modules
	@$(BIN)/jscoverage lib lib-cov
	@NODESASS_COV=1 $(BIN)/_mocha \
	  --reporter mocha-lcov-reporter | $(BIN)/coveralls

.PHONY: test clean

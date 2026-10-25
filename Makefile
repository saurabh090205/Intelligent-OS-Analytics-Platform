all:
	$(MAKE) -C backend all

backend:
	$(MAKE) -C backend all

test:
	$(MAKE) -C backend test

clean:
	$(MAKE) -C backend clean

frontend-build:
	cd frontend && npm run build

.PHONY: all backend test clean frontend-build

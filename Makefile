test:test.c coroutine.c
	gcc $^ -o $@

.PHONY:clean
clean:
	rm test

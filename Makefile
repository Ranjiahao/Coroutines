all:tcp_srv tcp_cli

tcp_srv:coroutine.c tcp_srv.c
	gcc $^ -o $@

tcp_cli:tcp_cli.c
	gcc $^ -o $@

.PHONY:clean
clean:
	rm tcp_srv tcp_cli

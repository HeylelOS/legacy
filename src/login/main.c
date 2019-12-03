#include <stdio.h>
#include <unistd.h>
#include <pwd.h>
#include <err.h>

struct login_args {
	char *logname;
	char *motd;
};

static const struct login_args
login_parse_args(int argc, char **argv) {
	struct login_args args = {
		.logname = NULL,
		.motd = NULL
	};

	return args;
}

int
main(int argc, char **argv) {
	const struct login_args args = login_parse_args(argc, argv);

	return 0;
}


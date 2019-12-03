#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <libgen.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <termios.h>
#include <alloca.h>
#include <err.h>

/* The following are useful ANSI escape codes to initialize terminal state */
#define CSI        "\033[" /* Control Sequence Introducer */
#define CSI_CUP_00 CSI"H"  /* Cursor Position (0, 0) */
#define CSI_ED     CSI"J"  /* Erase in Display */

struct getty_args {
	char *logname;
	char *path;
	char *tty;
	speed_t ispeed;
	speed_t ospeed;
};

static const struct baud_rate {
	speed_t speed;
	unsigned long lspeed;
} baudrates[] = {
	{ B0,     0     },
	{ B50,    50    },
	{ B75,    75    },
	{ B110,   110   },
	{ B134,   134   },
	{ B150,   150   },
	{ B200,   200   },
	{ B300,   300   },
	{ B600,   600   },
	{ B1200,  1200  },
	{ B1800,  1800  },
	{ B2400,  2400  },
	{ B4800,  4800  },
	{ B9600,  9600  },
	{ B19200, 19200 },
	{ B38400, 38400 },
};

void
getty_open(const struct getty_args *args) {
	const size_t namelen = strlen(args->tty);
	char teletype[6 + namelen];
	int fd;

	stpncpy(stpcpy(teletype, "/dev/"), args->tty, namelen);

	close(STDIN_FILENO);
	close(STDOUT_FILENO);
	close(STDERR_FILENO);

	/* Open it for reading and writing, we'll acquire it and make it blocking later */
	if((fd = open(teletype, O_RDWR | O_NOCTTY | O_NONBLOCK)) < 0) {
		err(1, "open %s", teletype);
	}

	/* Let's check this quickly */
	if(isatty(fd) == 0) {
		err(1, "isatty %s", teletype);
	}

	/* We should not have any file descriptor opened except fd which should be 0,
	any error or incompatibility is detected here anyway */
	if(dup(STDIN_FILENO) != STDOUT_FILENO
		|| dup(STDOUT_FILENO) != STDERR_FILENO) {
		err(1, "Unable to setup stdout and stderr for %s", teletype);
	}

	/* Unbufferize output */
	setvbuf(stdout, NULL, _IONBF, 0);
}

static void
getty_setup(const struct getty_args *args) {
	const pid_t pgid = getpgrp();
	struct termios termios;
	pid_t tcsid;
	int flags;

	/* To force the controlling terminal */
	if(((tcsid = tcgetsid(STDIN_FILENO)) == -1 || tcsid != pgid)
		&& ioctl(STDIN_FILENO, TIOCSCTTY, 1) != 0) {
		err(1, "Unable to set the controlling terminal to %s", args->tty);
	}

	/* Usually pid and pgid *should* be the same as we should be a session leader */
	if(tcsetpgrp(STDIN_FILENO, pgid) != 0) {
		err(1, "Unable to set %s process group to %d", args->tty, pgid);
	}

	if(tcgetattr(STDIN_FILENO, &termios) != 0) {
		err(1, "tcgetattr");
	}

	if(args->ispeed != 0) {
		cfsetispeed(&termios, args->ispeed);
	}

	if(args->ospeed != 0) {
		cfsetospeed(&termios, args->ospeed);
	}

	if(tcflush(STDIN_FILENO, TCIOFLUSH) != 0) {
		err(1, "tcflush");
	}

	if(tcsetattr(STDIN_FILENO, TCSANOW, &termios) != 0) {
		err(1, "tcsetattr");
	}

	if((flags = fcntl(STDIN_FILENO, F_GETFL)) == -1) {
		err(1, "fcntl F_GETFL");
	}

	if(fcntl(STDIN_FILENO, F_SETFL, flags & ~O_NONBLOCK) == -1) {
		err(1, "fcntl F_SETFL");
	}
}

static void
getty_execv(const struct getty_args *args) {
	char *programname;
	char **argv;

	if(setenv("TERM", args->tty, 1) != 0) {
		err(1, "setenv %s", args->tty);
	}

	if((programname = strrchr(args->path, '/')) == NULL) {
		programname = args->path;
	} else {
		programname++;
	}

	if(args->logname != NULL) {
		argv = alloca(sizeof(*argv) * 3);
		argv[0] = programname;
		argv[1] = args->logname;
		argv[2] = NULL;
	} else {
		argv = alloca(sizeof(*argv) * 2);
		argv[0] = programname;
		argv[1] = NULL;
	}

	execv(args->path, argv);
	err(1, "execv %s", args->path);
}

static void
getty_usage(const char *gettyname) {
	fprintf(stderr, "usage: %s [-i speed] [-o speed] [-u logname] [-p path] [tty]\n", gettyname);
	exit(1);
}

static int
getty_speed(const char *speedstr, speed_t *speedp) {
	char *speedstrend;
	unsigned long lspeed = strtoul(speedstr, &speedstrend, 10);

	if(*speedstrend == '\0' && speedstrend != speedstr) {
		const struct baud_rate *current = baudrates,
			* const baudratesend = baudrates + sizeof(baudrates) + sizeof(*baudrates);

		while(current != baudratesend
			&& current->lspeed < lspeed) {
			current++;
		}

		if(current != baudratesend
			&& current->lspeed == lspeed) {
			*speedp = current->speed;
			return 0;
		}
	}

	return -1;
}

const struct getty_args
getty_parse_args(int argc, char **argv) {
	struct getty_args args = {
		.logname = NULL,
		.path = "/bin/login",
		.tty = "tty",
		.ispeed = 0,
		.ospeed = 0
	};
	int c;

	while((c = getopt(argc, argv, ":i:o:u:p:")) != -1) {
		switch(c) {
		case 'i':
			if(getty_speed(optarg, &args.ispeed) != 0) {
				errx(1, "Unable to parse input speed %s", optarg);
			}
			break;
		case 'o':
			if(getty_speed(optarg, &args.ospeed) != 0) {
				errx(1, "Unable to parse output speed %s", optarg);
			}
			break;
		case 'u':
			args.logname = optarg;
			break;
		case 'p':
			args.path = optarg;
			break;
		case ':':
			warnx("-%c: Missing argument", optopt);
			getty_usage(*argv);
		default:
			warnx("Unknown argument -%c", c);
			getty_usage(*argv);
		}
	}

	switch(argc - optind) {
	case 2:
		args.path = argv[optind + 1];
	case 1:
		args.tty = argv[optind];
	case 0:
		break;
	default:
		getty_usage(*argv);
	}

	return args;
}

int
main(int argc, char **argv) {
	const struct getty_args args = getty_parse_args(argc, argv);

	getty_open(&args);

	getty_setup(&args);

	getty_execv(&args);

	return 0;
}


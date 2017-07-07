#include "Defs.h"
#include "LogF.h"

static FILE *logfile = NULL;
static const char *logpath = "/tmp/auplog.tmp";
static bool enable_logging = false;

static void init(void)
{
	if (logfile == NULL && (logfile = fopen(logpath, "a")) != NULL) {
		fcntl(fileno(logfile), F_SETFD, FD_CLOEXEC);
		setbuf(logfile, NULL);
		fputc('\n', logfile);
		logfmt("Log file %s opened", logpath);
	}
	if (logfile == NULL) {
		fprintf(stderr, "Can't open logfile \"%s\"\n", logpath);
		exit(EXIT_FAILURE);
	}
}

void logfmt(const char *format, ...)
{
	va_list ap;
	char timebuf[30];
	time_t tm;

	if (enable_logging) {
		init();
		va_start(ap, format);
		tm = time(NULL);
		strftime(timebuf, sizeof(timebuf), "%Y-%b-%d %X", localtime(&tm));
		fprintf(logfile, "%s [%d]: ", timebuf, (int)getpid());
		vfprintf(logfile, format, ap);
		fputc('\n', logfile);
		va_end(ap);
	}
}

void logfmt_setpath(const char *path)
{
	logpath = path;
	init();
}

void logfmt_args(int argc, char *argv[])
{
	int i;

	for (i = 1; i < argc; i++)
		if (strcmp(argv[i], "-l") == 0) {
			enable_logging = true;
			break;
		}
}

void logfmt_enable(bool enable)
{
	enable_logging = enable;
}

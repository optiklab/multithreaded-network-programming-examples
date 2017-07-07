#include "Defs.h"
#include <pthread.h>

static void ec_mutex(bool lock)
{
    static pthread_mutex_t ec_mtx = PTHREAD_MUTEX_INITIALIZER;
    int errnum;
    char *msg;

    if (lock) {
        if ((errnum = pthread_mutex_lock(&ec_mtx)) == 0)
            return;
    }
    else {
        if ((errnum = pthread_mutex_unlock(&ec_mtx)) == 0)
            return;
    }
    if ((msg = strerror(errnum)) == NULL)
        fprintf(stderr, "Mutex error in ec_* function: %d\n", errnum);
    else
        fprintf(stderr, "Mutex error in ec_* function: %s\n", msg);
    exit(EXIT_FAILURE);
}

/*[ec_atexit_fcn]*/
static void ec_atexit_fcn(void)
{
    ec_print();
}
/*[ec_node]*/
static struct ec_node {
    struct ec_node *ec_next;
    int ec_errno;
    EC_ERRTYPE ec_type;
    char *ec_context;
} *ec_head, ec_node_emergency;
static char ec_s_emergency[100];

const bool ec_in_cleanup = false;
/*[ec_push]*/
#define SEP1 " ["
#define SEP2 ":"
#define SEP3 "] "

void ec_push(const char *fcn, const char *file, int line,
  const char *str, int errno_arg, EC_ERRTYPE type)
{
    struct ec_node node, *p;
    size_t len;
    static bool attexit_called = false;

    ec_mutex(true);
    node.ec_errno = errno_arg;
    node.ec_type = type;
    if (str == NULL)
        str = "";
    len = strlen(fcn) + strlen(SEP1) + strlen(file) + strlen(SEP2) +
      6 + strlen(SEP3) + strlen(str) + 1;
    node.ec_context = (char *)calloc(1, len);
    if (node.ec_context == NULL) {
        if (ec_s_emergency[0] == '\0')
            node.ec_context = ec_s_emergency;
        else
            node.ec_context = "?";
        len = sizeof(ec_s_emergency);
    }
    if (node.ec_context != NULL)
        snprintf(node.ec_context, len, "%s%s%s%s%d%s%s", fcn, SEP1,
          file, SEP2, line, SEP3, str);
    p = (struct ec_node *)calloc(1, sizeof(struct ec_node));
    if (p == NULL && ec_node_emergency.ec_context == NULL)
        p = &ec_node_emergency; /* use just once */
    if (p != NULL) {
        node.ec_next = ec_head;
        ec_head = p;
        *ec_head = node;
    }
    if (!attexit_called) {
        attexit_called = true;
        ec_mutex(false);
        if (atexit(ec_atexit_fcn) != 0) {
            ec_push(fcn, file, line, "atexit failed", errno, EC_ERRNO);
            ec_print(); /* so at least the error gets shown */
        }
    }
    else
        ec_mutex(false);
}
/*[ec_print]*/
void ec_print(void)
{
    struct ec_node *e;
    int level = 0;

    ec_mutex(true);
    for (e = ec_head; e != NULL; e = e->ec_next, level++) {
        char buf[200], buf2[25 + sizeof(buf)];

        if (e == &ec_node_emergency)
            fprintf(stderr, "\t*** Trace may be incomplete ***\n");
        ////
        ////syserrmsgtype(buf, sizeof(buf), e->ec_context, e->ec_next == NULL ? e->ec_errno : 0, e->ec_type);
        ////
        
        snprintf(buf2, sizeof(buf2), "%s\t%d: %s",
          (level == 0? "ERROR:" : ""), level, buf);
        fprintf(stderr, "%s\n", buf2);
        logfmt(buf2);
    }
    ec_mutex(false);
}
/*[ec_reinit]*/
void ec_reinit(void)
{
    struct ec_node *e, *e_next;

    ec_mutex(true);
    for (e = ec_head; e != NULL; e = e_next) {
        e_next = e->ec_next;
        if (e->ec_context != ec_s_emergency)
            free(e->ec_context);
        if (e != &ec_node_emergency)
            free(e);
    }
    ec_head = NULL;
    memset(&ec_node_emergency, 0, sizeof(ec_node_emergency));
    memset(&ec_s_emergency, 0, sizeof(ec_s_emergency));
    ec_mutex(false);
}

// This function is a mark of error: so our Cleanup Procedure will be executed ONLY if error actually happened.
void ec_warn(void)
{
    fprintf(stderr, "***WARNING: Control flowed into EC_CLEANUP_BGN\n");
}

char *errsymbol(const char *cat, int code, char **desc)
{
    for (int i = 0; errcodes[i].cat != NULL; i++)
    {
        if (strcmp(errcodes[i].cat, cat) == 0 && errcodes[i].code == (intptr_t)code)
        {
            if (desc != NULL)
                *desc = errcodes[i].desc;

            return errcodes[i].str;
        }
    }
    if (desc != NULL)
        *desc = "?";
    return "?";
}

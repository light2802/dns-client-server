#ifndef XGETOPT_H
#define XGETOPT_H


enum { xargument_no = 0, xargument_required, xargument_optional };

struct xoption {
    char        opt;
    const char *name;
    int         has_arg;
    int *       flag;
    int         val;
};

int xgetopt(int                   argc,
            const char *          argv[],
            const struct xoption *options,
            int *                 optind,
            const char **         optarg);

#endif

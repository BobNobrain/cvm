#ifndef HODULI_H
#define HODULI_H

#define MUST(x) { int err = (x); if (err != 0) return err; }

#ifdef DEBUG
#define ASSERT(x, msg) { if (!(x)) { printf(msg); exit(-1); } }
#else
#define ASSERT(x, msg)
#endif

#endif

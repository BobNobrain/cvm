#ifndef HODULI_H
#define HODULI_H

#define MUST(x) { int err = (x); if (err != 0) return err; }

#define ERR_DECL int err;
#define ERR_PASS(EXPR) err = (EXPR); if (err != 0) { return err; }
#define ERR_RET(RET, EXPR) err = (EXPR); if (err != 0) { return E; }

#ifdef DEBUG
#define ASSERT(x, msg) { if (!(x)) { printf(msg); exit(-1); } }
#else
#define ASSERT(x, msg)
#endif

#endif

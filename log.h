#ifndef LOG_H
#define LOG_H

#include <cassert>
#include <cstdio>

extern bool verbose;

#define LOG(format, ...) do{ \
								if (!verbose) break; \
                                printf(\
                                        "%s:%d: " \
                                        format "\n", \
                                        __FUNCTION__, \
                                        __LINE__, \
                                        ##__VA_ARGS__); \
                                }while(0)

#define ERR(format, ...) do{ \
                                printf(\
                                        "%s:%d: " \
                                        format "\n", \
                                        __FUNCTION__, \
                                        __LINE__, \
                                        ##__VA_ARGS__); \
                                }while(0)

#ifndef NDEBUG
#define ASSERT(X) assert(X)
#else
#define ASSERT(X) do{ if(X){}else{ERR("ASSERT failed: " #X); std::abort(); } }while(0)
#endif

#endif  // LOG_H

#ifndef DOOR_COMPILER_H_
#define DOOR_COMPILER_H_

#ifdef LOG
#define RAM_CODE
#else
#define RAM_CODE __attribute__((used, long_call, section(".data")))
#endif

#endif
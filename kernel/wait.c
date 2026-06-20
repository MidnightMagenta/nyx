#include <nyx/current.h>
#include <nyx/list.h>
#include <nyx/proc.h>
#include <nyx/wchan.h>

struct list_head waittab[NR_SLEEP_BUCKETS];

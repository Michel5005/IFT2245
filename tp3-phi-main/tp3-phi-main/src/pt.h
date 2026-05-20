#ifndef PT_H
#define PT_H

#include <stdio.h>
#include <stdbool.h>

#include "conf.h"

void pt_init (FILE *log);
int pt_lookup (unsigned int page_number);
void pt_set_entry (unsigned int page_number, unsigned int frame_number);
void pt_set_invalid (unsigned int page_number);
bool pt_is_valid (unsigned int page_number);
void pt_set_dirty (unsigned int page_number);
bool pt_is_dirty (unsigned int page_number);
void pt_clean (void);

#endif

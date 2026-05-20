#ifndef TLB_H
#define TLB_H

#include <stdio.h>
#include <stdbool.h>

#include "conf.h"

void tlb_init (FILE *log);
int tlb_lookup (unsigned int page_number, bool write);
void tlb_add_entry (unsigned int page_number, unsigned int frame_number, bool dirty);
void tlb_clean (void);

void tlb_invalid (unsigned int page_number);
#endif

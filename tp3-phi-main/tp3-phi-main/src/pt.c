#include <stdint.h>
#include <stdio.h>

#include "pt.h"
#include "conf.h"

struct page
{
  unsigned int frame_number : 16;
  bool valid : 1;
  bool dirty : 1;
};
static struct page page_table[NB_PAGES];

static FILE *pt_log = NULL;

static unsigned int pt_lookup_count = 0;
static unsigned int pt_page_fault_count = 0;
static unsigned int pt_set_count = 0;

// Initialise la page table et indique où envoyer le log des accès
void pt_init (FILE *log)
{
  for (unsigned int i = 0; i < NB_PAGES; i++) {
    page_table[i].valid = false;
  }
  pt_log = log;
}

/********************** ¡ NE RIEN CHANGER CI-DESSUS !  ***********************/


// Retourne le `frame_number` contenu à l'entrée `page_number` de la page table
// si l'entrée est valide
static int pt_lookup_(unsigned int page_number)
{
  if (page_table[page_number].valid) {
    return page_table[page_number].frame_number;
  }
  return -1;
}

// Modifie l'entrée `page_number` dans la page table pour qu'elle pointe vers
// `frame_number`
static void pt_set_entry_(unsigned int page_number, unsigned int frame_number)
{
  page_table[page_number].frame_number = frame_number;
  page_table[page_number].valid = true;
  page_table[page_number].dirty = false;
}

// Marque l'entrée `page_number` dans la page table comme invalide
void pt_set_invalid (unsigned int page_number)
{
  page_table[page_number].valid = false;
}

// Retourne `true` si l'entrée `page_number` dans la page table est valide
bool pt_is_valid (unsigned int page_number)
{
  return page_table[page_number].valid;
}

// Marque l'entrée `page_number` dans la page table comme dirty
void pt_set_dirty (unsigned int page_number)
{
  page_table[page_number].dirty = true;
}

// Retourne `true` si l'entrée `page_number` dans la page table est dirty
bool pt_is_dirty (unsigned int page_number)
{
  return page_table[page_number].dirty;
}


/********************** ¡ NE RIEN CHANGER CI-DESSOUS !  **********************/

void pt_set_entry (unsigned int page_number, unsigned int frame_number)
{
  pt_set_count++;
  pt_set_entry_(page_number, frame_number);
}

int pt_lookup (unsigned int page_number)
{
  pt_lookup_count++;
  int fn = pt_lookup_(page_number);
  if (fn < 0) pt_page_fault_count++;
  return fn;
}

// Imprime un sommaires des accès
void pt_clean (void)
{
  fprintf (stdout, "PT lookups   : %3u\n", pt_lookup_count);
  fprintf (stdout, "PT changes   : %3u\n", pt_set_count);
  fprintf (stdout, "Page Faults  : %3u\n", pt_page_fault_count);

  if (pt_log) {
    for (unsigned int i = 0; i < NB_PAGES; i++) {
      fprintf (pt_log,
               "%d -> {%d,%s%s}\n",
               i,
               page_table[i].frame_number,
               page_table[i].valid ? "" : " invalid",
               page_table[i].dirty ? " dirty" : "");
    }
  }
}

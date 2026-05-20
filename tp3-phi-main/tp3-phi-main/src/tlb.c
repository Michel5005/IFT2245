#include <stdint.h>
#include <stdio.h>

#include "tlb.h"
#include "conf.h"

struct tlb_entry
{
  unsigned int page_number;
  int frame_number;
  bool dirty : 1;
};
static struct tlb_entry tlb_entries[TLB_NB_ENTRIES];

static FILE *tlb_log = NULL;

static unsigned int tlb_hit_count = 0;
static unsigned int tlb_miss_count = 0;
static unsigned int tlb_mod_count = 0;

// Initialise le TLB et indique où envoyer le log d'accès
void tlb_init (FILE *log)
{
  for (int i = 0; i < TLB_NB_ENTRIES; i++){
    tlb_entries[i].frame_number = -1;
  }
  tlb_log = log;
}

/********************** ¡ NE RIEN CHANGER CI-DESSUS !  ***********************/

static int tlb_clock = 0; // Pointeur circulaire pour Clock algo
static int tlb_bit_used[TLB_NB_ENTRIES]; // Tableau de bits utilisé pour chaque entrée du TLB

// Retourne le `frame_number` associé avec `page_number` si `page_number` est
// contenu dans le TLB
static int tlb_lookup_(unsigned int page_number, bool write)
{
  // Parcourt toutes les entrées du TLB
  for (int i = 0; i < TLB_NB_ENTRIES; i++) {
    // Si l'entrée est valide et que le numéro de la page y correspond
    if (tlb_entries[i].frame_number != -1 && tlb_entries[i].page_number == page_number) {
      // Si l'accès est une écriture
      if (write) {
        tlb_entries[i].dirty = true;
      }

      tlb_bit_used[i] = 1; // Marquée comme utilisé récemment

      return tlb_entries[i].frame_number;
    }
  }
  return -1; // Si aucune entrée n'y correspond pas
}

// Ajoute une entrée qui associe `frame_number` à `page_number` dans le TLB
static void tlb_add_entry_(unsigned int page_number, unsigned int frame_number, bool dirty)
{
  // Parcourt toutes les entrées du TLB
  for (int i = 0; i < TLB_NB_ENTRIES; i++) {
    // Si l'entrée est invalide
    if (tlb_entries[i].frame_number == -1) {
      tlb_entries[i].page_number = page_number;
      tlb_entries[i].frame_number = frame_number;
      tlb_entries[i].dirty = dirty;
      tlb_bit_used[i] = 1;
      return;
    }
  }

  // Second-chance algo

  // Boucle à l'infinie jusqu'à ce qu'on remplace une entrée
  while (1) {
    // Si le bit utilisé n'a pas été utilisé récemment
    if (tlb_bit_used[tlb_clock] == 0) {
      tlb_entries[tlb_clock].page_number = page_number;
      tlb_entries[tlb_clock].frame_number = frame_number;
      tlb_entries[tlb_clock].dirty = dirty;
      tlb_bit_used[tlb_clock] = 1;

      // Remplace par une nouvelle
      tlb_clock = (tlb_clock + 1) % TLB_NB_ENTRIES;
      return;
    }

    // Sinon on donne une seconde chance

    // Remet à 0
    tlb_bit_used[tlb_clock] = 0;

    // Avance l'horloge
    tlb_clock = (tlb_clock + 1) % TLB_NB_ENTRIES;
  }
}

// Invalide une entrée pour une page donnée
void tlb_invalid (unsigned int page_number) {
  // Parcourt toutes les entrées du TLB
  for (int i = 0; i < TLB_NB_ENTRIES; i++) {
    // Si l'entrée est valide et correspond à la page qu'on doit invalider
    if (tlb_entries[i].frame_number != -1 && tlb_entries[i].page_number == page_number) {
      tlb_entries[i].frame_number = -1;
      tlb_entries[i].dirty = false;
      tlb_bit_used[i] = 0;
    }
  }
}

/********************** ¡ NE RIEN CHANGER CI-DESSOUS !  **********************/

int tlb_lookup (unsigned int page_number, bool write)
{
  int fn = tlb_lookup_(page_number, write);
  (*(fn < 0 ? &tlb_miss_count : &tlb_hit_count))++;
  return fn;
}

void tlb_add_entry (unsigned int page_number, unsigned int frame_number, bool dirty)
{
  tlb_mod_count++;
  tlb_add_entry_(page_number, frame_number, dirty);
}

// Imprime un sommaires des accès
void tlb_clean (void)
{
  fprintf (stdout, "TLB misses   : %3u\n", tlb_miss_count);
  fprintf (stdout, "TLB hits     : %3u\n", tlb_hit_count);
  fprintf (stdout, "TLB changes  : %3u\n", tlb_mod_count);
  fprintf (stdout, "TLB miss rate: %.1f%%\n",
           100 * tlb_miss_count
           /* Ajoute 0.01 pour éviter la division par 0.  */
           / (0.01 + tlb_hit_count + tlb_miss_count));
}

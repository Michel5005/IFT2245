#include <fcntl.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include "conf.h"
#include "common.h"
#include "vmm.h"
#include "tlb.h"
#include "pt.h"
#include "pm.h"

static unsigned int read_count = 0;
static unsigned int write_count = 0;
static FILE* vmm_log;

// Initialise le fichier de journal
void vmm_init (FILE *log)
{
  vmm_log = log;
}

static void vmm_log_command (FILE *out, const char *command,
                             unsigned int laddress, // Logical address
                             unsigned int page,
                             unsigned int offset,
                             unsigned int frame,
                             unsigned int paddress, // Physical address
                             char c) 
{
  if (out) {
    fprintf(out, "%s[%c]@%05d: p=%d, o=%d, f=%d, pa=%d\n", command, c,
            laddress, page, offset, frame, paddress);
  }
}

/********************** ¡ NE RIEN CHANGER CI-DESSUS !  ***********************/


#define PAGE_MASK (~OFFSET_MASK)
#define OFFSET_MASK (PAGE_FRAME_SIZE - 1)

static int frame_owner[NB_FRAMES]; // Indique la page qui occupe la frame
static int next_frame = 0; // Pointeur pour le remplacement
static int initial_fifo = 0; // Pour initialiser frame_owner une seule fois

// Retourne une frame libre ou remplace une frame
static int get_frame (unsigned int page) {
  // Marque toutes les frames comme étant libre
  if (!initial_fifo) {
    for (int i = 0; i < NB_FRAMES; i++) {
      frame_owner[i] = -1;
    }
    initial_fifo = 1;
  }
  // Attribution des frames libres
  for (int frame = 0; frame < NB_FRAMES; frame++) {
    if (frame_owner[frame] == -1) {
      frame_owner[frame] = page;
      return frame;
    }
  }

  int replacement = next_frame; // Frame à remplacer
  next_frame = (next_frame + 1) % NB_FRAMES;

  // L'ancienne page
  unsigned int old_page = frame_owner[replacement];

  // Si l'ancienne page était dirty
  if (pt_is_dirty(old_page)) {
    // Save sur le disque
    pm_backup_page(old_page, replacement);
  }

  // Invalide l'ancienne page
  pt_set_invalid(old_page);

  // Invalide l'entrée TLB qui y correspond
  tlb_invalid(old_page);

  // Assignation de la nouvelle page au frame
  frame_owner[replacement] = page;

  return replacement;
}

// Fait une lecture d'un caractère à l'adresse logique `laddress`
static char vmm_read_(unsigned int laddress)
{
  unsigned int page = (laddress & PAGE_MASK) / PAGE_FRAME_SIZE;
  unsigned int offset = (laddress & OFFSET_MASK);

  int frame = tlb_lookup(page,false); // Si on trouve, marque comme pas dirty

  // Si TLB échoue
  if (frame < 0) {
    // Cherche dans page table
    int fn = pt_lookup(page);

    // Si erreur de page
    if (fn < 0) {
      frame = get_frame(page); // Obtient la frame
      pm_download_page(page,frame); // Charge la page
      pt_set_entry(page, frame); // Met à jour page table
    } else { // Sinon
      frame = fn; // Page déjà en mémoire
    }

    bool is_dirty = pt_is_dirty(page);

    tlb_add_entry(page, frame, is_dirty); // Ajout au TLB
  }

  unsigned int paddress = frame * PAGE_FRAME_SIZE + offset; // Adresse physique

  char c = pm_read(paddress); // Lecture physique

  vmm_log_command (vmm_log, "READING", laddress, page, offset, frame, paddress, c);

  return c;
}

// Fait une écriture à l'adresse logique `laddress`
static void vmm_write_(unsigned int laddress, char c)
{
  unsigned int page = (laddress & PAGE_MASK) / PAGE_FRAME_SIZE;
  unsigned int offset = (laddress & OFFSET_MASK);

  int frame = tlb_lookup(page, true); // Si on trouve, marque comme dirty

  // Si TLB échoue
  if (frame < 0) {
    // Cherche dans page table
    int fn = pt_lookup(page);

    // Si erreur de page
    if (fn < 0) {
      frame = get_frame(page);  // Obtient la frame
      pm_download_page(page,frame); // Charge la page
      pt_set_entry(page, frame); // Met à jour page table
    } else { // Sinon
      frame = fn; // Page déjà en mémoire
    }

    tlb_add_entry(page, frame, true);
  }

  pt_set_dirty(page); // Marque la page comme dirty

  unsigned int paddress = frame * PAGE_FRAME_SIZE + offset; // Adresse physique
  pm_write(paddress, c); // Écriture physique

  vmm_log_command (vmm_log, "WRITING", laddress, page, offset, frame, paddress, c);
}


/********************** ¡ NE RIEN CHANGER CI-DESSOUS !  **********************/

char vmm_read (unsigned int laddress)
{
  read_count++;
  return vmm_read_(laddress);
}

void vmm_write (unsigned int laddress, char c)
{
  write_count++;
  vmm_write_(laddress, c);
}

void vmm_clean (void)
{
  fprintf (stdout, "VM reads : %4u\n", read_count);
  fprintf (stdout, "VM writes: %4u\n", write_count);
}

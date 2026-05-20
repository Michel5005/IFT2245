#include <stdio.h>
#include <string.h>

#include "conf.h"
#include "pm.h"

static FILE *pm_backing_store;
static FILE *pm_log;
static unsigned int download_count = 0;
static unsigned int backup_count = 0;
static unsigned int read_count = 0;
static unsigned int write_count = 0;

// Initialise la mémoire physique
void pm_init (FILE *backing_store, FILE *log)
{
  pm_backing_store = backing_store;
  pm_log = log;
  memset (pm_memory, '\0', sizeof (pm_memory));
}

/********************** ¡ NE RIEN CHANGER CI-DESSUS !  ***********************/


// Charge la page demandée du backing store
static void pm_download_page_(unsigned int page_number, unsigned int frame_number)
{
  // Déplace le curseur dans le fichier
  fseek(pm_backing_store, page_number * PAGE_FRAME_SIZE, SEEK_SET);

  unsigned int physical_address = frame_number * PAGE_FRAME_SIZE;

  // Lit une page du fichier et la copie dans le frame qui correspond dans la mémoire
  fread(pm_memory + physical_address, PAGE_FRAME_SIZE, 1, pm_backing_store);
}

// Sauvegarde la frame spécifiée dans la page du backing store
static void pm_backup_page_(unsigned int page_number, unsigned int frame_number)
{
  backup_count++;
  
  // Déplace le curseur dans le fichier
  fseek(pm_backing_store,page_number * PAGE_FRAME_SIZE,SEEK_SET);

  unsigned int physical_address = frame_number * PAGE_FRAME_SIZE;

  // Écrit en fichier et copie la frame mémoire vers la page qui correspond sur le disque
  fwrite(pm_memory + physical_address, PAGE_FRAME_SIZE, 1, pm_backing_store);
}

static char pm_read_(unsigned int physical_address)
{
  

  // Return le caractère stocké à cette adresse dans la mémoire physique
  return pm_memory[physical_address];
}

static void pm_write_(unsigned int physical_address, char c)
{

  // Écrit la valeur c à l'adresse physique dans la mémoire
  pm_memory[physical_address] = c;
}


/********************** ¡ NE RIEN CHANGER CI-DESSOUS !  **********************/

void pm_download_page (unsigned int page_number, unsigned int frame_number)
{
  download_count++;
  pm_download_page_(page_number, frame_number);
}

void pm_backup_page (unsigned int page_number, unsigned int frame_number)
{
  pm_backup_page_(page_number, frame_number);
}

char pm_read (unsigned int physical_address)
{
  read_count++;
  return pm_read_(physical_address);
}

void pm_write (unsigned int physical_address, char c)
{
  write_count++;
  pm_write_(physical_address, c);
}

// Enregiste l'état de la mémoire physiques
void pm_clean (void)
{
  if (pm_log) {
    for (unsigned int i = 0; i < PHYSICAL_MEMORY_SIZE; i++) {
      if (i % 80 == 0) {
        fprintf(pm_log, "%c\n", pm_memory[i]);
      } else {
        fprintf(pm_log, "%c", pm_memory[i]);
      }
    }
  }
  fprintf(stdout, "Page downloads: %2u\n", download_count);
  fprintf(stdout, "Page backups  : %2u\n", backup_count);
  fprintf(stdout, "PM reads : %4u\n", read_count);
  fprintf(stdout, "PM writes: %4u\n", write_count);
}

#pragma clang diagnostic push
#pragma ide diagnostic ignored "OCUnusedGlobalDeclarationInspection"
#pragma ide diagnostic ignored "hicpp-signed-bitwise"
#pragma ide diagnostic ignored "readability-non-const-parameter"

#include "main.h"

uint8_t ilog2(uint32_t n) {
    uint8_t i = 0;
    while ((n >>= 1U) != 0)
        i++;
    return i;
}

//--------------------------------------------------------------------------------------------------------
//                                           DEBUT DU CODE
//--------------------------------------------------------------------------------------------------------

/**
 * Exercice 1
 *
 * Prend cluster et retourne son addresse en secteur dans l'archive
 * @param cluster le cluster à convertir en LBA (logical block address)
 * @param block le block de paramètre du BIOS
 * @param first_data_sector le premier secteur de données, donnée par la formule dans le document
 * @return le LBA
 */
uint32_t cluster_to_logical_block_addr(uint32_t cluster, BPB *block, uint32_t first_data_sector) {
    // Calcul de l'adresse LBA
    uint32_t logical_block_addr = first_data_sector + (cluster - 2) * block->BPB_SectorPerCluster;

    return logical_block_addr;
}

/**
 * Exercice 2
 *
 * Va chercher une valeur dans la cluster chain
 * @param cluster le cluster qu'on veut aller lire
 * @param block le block de paramètre du système de fichier
 * @param value un pointeur ou on retourne la valeur
 * @param archive le fichier de l'archive
 * @return un src d'erreur
 */
error_code get_cluster_chain_value(uint32_t cluster, BPB *block, uint32_t *value, FILE *archive) {

    // Si un des pointeurs est NULL, on return une erreur
    if (!archive || !block || !value) {
        return GENERAL_ERR;
    }

    // Taille d'un secteur
    uint32_t bytes_sector = as_uint16(block->BPB_BytesPerSector);

    // Calcule le début de la FAT
    uint32_t fat_start = as_uint16(block->BPB_RsvdSectorCnt) * bytes_sector;

    // Calcule l'offset de l'entrée FAT
    uint32_t fat_offset = fat_start + cluster * 4;

    // Donne le secteur
    uint32_t sector = fat_offset / bytes_sector;

    // Calcule le offset dans le secteur
    uint32_t sector_offset = fat_offset % bytes_sector;

    // Allocation du buffer pour lire le secteur FAT
    uint8_t *buffer = malloc(bytes_sector);

    // Si l'allocation échoue
    if (!buffer)
        return OUT_OF_MEM;

    // Lit le secteur FAT
    if (fseek(archive,sector * bytes_sector,SEEK_SET) || fread(buffer, bytes_sector, 1, archive) !=1) {
        free(buffer);
        return GENERAL_ERR;
    }

    // Extrait valeur de l'entrée FAT
    *value = as_uint32(buffer + sector_offset) & 0x0FFFFFFF;

    // Libère la mémoire
    free(buffer);

    return NO_ERR; // Pas d'erreur
}

static void build_fat_name(char *src, uint8_t out[11]) {
    // Nom du FAT (8 caracteres pour le nom et 3 caracteres pour le type)
    for (int i = 0; i < 11; i++)
        out[i] = ' ';

    // Ignore le "/" initial du path
    if (*src == '/')
        src++;

     // Cas spéciaux pour "." et ".."
    if (strcmp(src, ".") == 0) {
        out[0] = '.';
        return;
    }
     if (strcmp(src, "..") == 0) {
        out[0] = '.';
        out[1] = '.';
        return;
    }
        
        
    // Trouve le "." qui sépare le nom et le type
    char *dot = strchr(src, '.');

    int i = 0;

    // Copie le nom, ce qui ce trouve avant le "."
    while (src[i] && src + i != dot && i < 8) {

        // Convertit en majuscules
        out[i] = toupper((unsigned char) src[i]);
        i++;
    }

    // Si on a un ".", on copie le type, ce qui ce trouve apres le "."
    if (dot) {
        dot++;
        int j = 8;
        int k = 0;
        while (dot[k] && j < 11) {
            // Convertit en majuscules
            out[j++] = toupper((unsigned char) dot[k++]);
        }
    }
}

/**
 * Exercice 3
 *
 * Vérifie si un descripteur de fichier FAT identifie bien fichier avec le nom name
 * @param entry le descripteur de fichier
 * @param name le nom de fichier
 * @return 0 ou 1 (faux ou vrai)
 */
bool check_name(FAT_entry *entry, char *name) {

    // Si un des pointeurs est NULL
    if (!entry || !name)
        return false;

    // Construit le nom FAT à partir de la fonction auxiliaire
    uint8_t fat_name[11];
    build_fat_name(name,fat_name);

    // Compare en binaire les deux noms FAT et retourne 0 si les deux sont pareils
    return memcmp(entry->DIR_Name, fat_name, 11) == 0;
}


/**
 * Exercice 4
 *
 * Prend un path de la forme "/dossier/dossier2/fichier.ext et retourne la partie
 * correspondante à l'index passé. Le premier '/' est facultatif.
 * @param path l'index passé
 * @param level la partie à retourner (ici, 0 retournerait dossier)
 * @param output la sortie (la string)
 * @return -1 si les arguments sont incorrects, -2 si le path ne contient pas autant de niveaux
 * -3 si out of memory
 */
error_code split_up_path(uint8_t level, char *path, char **output) {

    // Si un des arguments est NULL
    if (!path || !output)
        return -1; // Les arguments sont incorrects

    *output = NULL; // Initialise la sortie a NULL

    // Ignore le "/" initial du path
    if (path[0] == '/')
        path++;

    // Copie la chaine
    char *copy = strdup(path);

    // Si strdup echoue
    if (!copy)
        return -3; // Out of memory

    // Découpage de la chaine

    uint8_t current = 0;

    char *save = NULL;
    char *token = strtok_r(copy, "/", &save);


    while (token) {

        // Si on est au bon niveau
        if (current == level) {

            // Copie le segment
            *output = strdup(token);

            // Libère la mémoire
            free(copy);

            // Si l'allocation échoue
            if (!*output)
                return -3; // Out of memory

            // Convertit en majuscules
            for (char *p = *output; *p; p++)
                *p = (char) toupper((unsigned char)*p);

            return NO_ERR;
        }

        // Sinon on passe au prochain segment

        current++;
        token = strtok_r(NULL, "/", &save);
    }

    // Libère la mémoire
    free(copy);

    return -2; // Path ne contient pas assez de niveaux
}


/**
 * Exercice 5
 *
 * Lit le BIOS parameter block
 * @param archive fichier qui correspond à l'archive
 * @param block le block alloué
 * @return un src d'erreur
 */
error_code read_boot_block(BPB **block, FILE *archive) {

    // Si un des arguments est NULL
    if (!archive || !block)
        return GENERAL_ERR;

    // Allocation du BPB
    *block = malloc(sizeof(BPB));

    // Si l'allocation échoue
    if (!*block)
        return OUT_OF_MEM; // Out of memory



    // Si fseek échoue
    if (fseek(archive, 0, SEEK_SET) !=0) {

        // Libère la mémoire
        free(*block);
        *block = NULL;

        return GENERAL_ERR; // Erreur
    }

     if (fread(*block, sizeof(BPB), 1, archive) != 1) {
        free(*block);
        *block = NULL;

        return GENERAL_ERR; // Erreur
    }

    return NO_ERR; // Pas d'erreur
}

/**
 * Fonction auxiliaire qui extrait le numéro de cluster de départ d'un fichier ou d'un dossier avec une entrée FAT
 *
 * @param entry Entrée FAT32
 * @return Cluster de départ
 */
static uint32_t cluster_entry(FAT_entry *entry) {

    // Lit les 16 bits haut du numéro de cluster
    uint32_t cluster_hi = as_uint16(entry->DIR_FstClusterHI);

    // Lit les 16 bits bas du numéro de cluster
    uint32_t cluster_lo = as_uint16(entry->DIR_FstClusterLO);

    // Décale les 16 bits hauts de 16 positions vers la gauche et combine avec les 16 bits bas
    return (cluster_hi << 16) | cluster_lo;
}

/**
 * Fonction auxiliare qui parcourt la FAT et lit les fichiers
 *
 * @param archive fichier qui correspond à l'archive
 * @param block le block de paramètre du système de fichier
 * @param cluster le cluster à convertir en LBA
 * @param buffer Buffer où écrire les données
 * @return NO_ERR si la lecture réussit, sinon code d'erreur
 */
static error_code read_cluster(FILE *archive, BPB *block, uint32_t cluster, uint8_t *buffer) {

    // Taille d'un secteur
    uint32_t bytes_sector = as_uint16(block->BPB_BytesPerSector);

    // Nombre de secteur par cluster
    uint32_t sector = block->BPB_SectorPerCluster;

    // Calcule le premier secteur de données
    uint32_t first_data_sector = as_uint16(block->BPB_RsvdSectorCnt) + block->BPB_NumFATs
            * as_uint32(block->BPB_FATSz32);

    // Convertit le numéro de cluster en adresse LBA
    uint32_t logical_block_addr = cluster_to_logical_block_addr(cluster, block, first_data_sector);

    // Si fseek échoue
    if (fseek(archive, logical_block_addr * bytes_sector, SEEK_SET) != 0)
        return GENERAL_ERR; // Erreur

    // Si fread retourne 0
    if (fread(buffer, bytes_sector * sector, 1, archive) != 1)
        return GENERAL_ERR; // Erreur

    // Sinon

    return NO_ERR; // Pas d'erreur
}

/**
 * Fonction auxiliaire qui calcule le premier secteur de données
 *
 * @param block le block de paramètre du système de fichier
 * @return Le numéro du premier secteur de données
 */
static uint32_t get_first_data_sector(BPB *block) {
    return as_uint16(block->BPB_RsvdSectorCnt) + block->BPB_NumFATs * as_uint32(block->BPB_FATSz32);
}

/**
 * Fonction auxiliaire qui calcule la taille d'un cluster en octets
 *
 * @param block le block de paramètre du système de fichier
 * @return La taille d'un cluster en octets
 */
static uint32_t get_cluster_size(BPB *block) {
    return as_uint16(block->BPB_BytesPerSector) * block->BPB_SectorPerCluster;
}

/**
 * Fonction auxiliaire qui recherche une entrée FAT dans un répertoire
 *
 * @param archive fichier qui correspond à l'archive
 * @param block le block de paramètre du système de fichier
 * @param cluster_start premier cluster à parcourir du répertoire
 * @param name nom FAT à rechercher
 * @return Une copie de l'entrée FAT trouvée ou NULL si aucune entrée correspond
 */
static FAT_entry *find_entry(FILE *archive, BPB *block, uint32_t cluster_start, char *name) {

    // Cluster actuel
    uint32_t cluster = cluster_start;

    // Taille du cluster
    uint32_t cluster_size = get_cluster_size(block);

    // Allocation du buffer
    uint8_t *buffer = malloc(cluster_size);

    // Si l'allocation échoue
    if (!buffer) {
        return NULL;
    }

    // Parcourt la chaine de clusters jusqu'à la fin de la chaine FAT
    while (cluster < FAT_EOC_TAG) {

        // Si la lecture échoue
        if (read_cluster(archive, block, cluster, buffer) < 0)
            break;

        FAT_entry *entry = (FAT_entry *)buffer;

        // Nombre d'entrées dans le cluster
        size_t entries = cluster_size / sizeof(FAT_entry);

        // Boucle sur toutes les entrées du cluster
        for (size_t i = 0; i < entries; i++, entry++) {

            // Si entrée vide
            if (entry->DIR_Name[0] == 0x00)
                break;

            // Si entrée supprimée
            if (entry->DIR_Name[0] == 0xE5)
                continue;


            // Si le nom correspond
            if (check_name(entry, name)) {

                // Allocation d'une nouvelle entrée
                FAT_entry *e = malloc(sizeof(FAT_entry));
                if (!e) {
                    free(buffer);
                    return NULL;
                }

                // Copie l'entrée trouvée
                memcpy(e, entry, sizeof(FAT_entry));

                // Libère la mémoire
                free(buffer);

                return e; // Retourne une copie
            }
        }

        // Prochain cluster
        uint32_t next_cluster;

        // Si on trouve
        if (get_cluster_chain_value(cluster, block, &next_cluster, archive) < 0)
            break;

        // Passe au prochain cluster
        cluster = next_cluster;
    }

    // Libère la mémoire
    free(buffer);

    return NULL;
}

/**
 * Exercice 6
 *
 * Trouve un descripteur de fichier dans l'archive
 * @param archive le descripteur de fichier qui correspond à l'archive
 * @param path le chemin du fichier
 * @param entry l'entrée trouvée
 * @return un src d'erreur
 */
error_code find_file_descriptor(FILE *archive, BPB *block, char *path, FAT_entry **entry) {
    if (!archive || !block || !path || !entry) return GENERAL_ERR;
    *entry = NULL; 

    uint32_t cluster = as_uint32(block->BPB_RootCluster);

    // Index du segment dans le path
    uint32_t level = 0;

    // Segment extrait par split_up_path
    char *segment = NULL;

    // Parcourt chaque segment du path
    while (split_up_path(level, path, &segment) >= 0) {

        // Cherche une entrée qui correspond au cluster actuel
        FAT_entry *e = find_entry(archive, block, cluster, segment);

        // Libère la mémoire
        free(segment);

        // Si l'entrée n'existe pas
        if (!e)
            return RES_NOT_FOUND;

        char *next_segment = NULL;
        error_code next_err = split_up_path(level + 1, path, &next_segment);
        if (next_err == OUT_OF_MEM) {
            free(e);
            return OUT_OF_MEM;
        }
        if (next_err >= 0) {
            free(next_segment);
            if (!(e->DIR_Attr & 0x10)) {
                free(e);
                return RES_NOT_FOUND;
            }
        }
             
        // Met à jour le cluster actuel
        cluster = cluster_entry(e);
        if (*entry) free(*entry);            //  libère l'ancienne entrée

        // Garde l'entrée trouvée
        *entry = e;

        // Passe au prochain segment
        level++;
    }
    if (!*entry) return RES_NOT_FOUND; // path vide
    

    return NO_ERR; // Pas d'erreur

}

/**
 * Exercice 7
 *
 * Lit un fichier dans une archive FAT
 * @param entry l'entrée du fichier
 * @param buff le buffer ou écrire les données
 * @param max_len la longueur du buffer
 * @return un src d'erreur qui va contenir la longueur des donnés lues
 */
error_code read_file(FILE *archive, BPB *block, FAT_entry *entry, void *buff, size_t max_len) {
    if (!archive || !block || !entry || !buff) return GENERAL_ERR;        // validation

    // Si l'entrée est un dossier
    if (entry->DIR_Attr & 0x10)
        return GENERAL_ERR; // Erreur

    // Taille du fichier
    uint32_t file_size = as_uint32(entry->DIR_FileSize);

    // Nombre d'octets à lire
    uint32_t read = file_size < max_len ? file_size : max_len;

    // Cluster de départ du fichier
    uint32_t cluster = cluster_entry(entry);

    // Taille d'un cluster
    uint32_t cluster_size = get_cluster_size(block);

    if (read == 0)
    return 0;

    // Allocation de buffer
    uint8_t *buffer = malloc(cluster_size);

    // Si l'allocation échoue
    if (!buffer)
        return OUT_OF_MEM; // Out of memory

    uint32_t total = 0;

    // Lit cluster par cluster
    while (cluster < FAT_EOC_TAG && total < read) {
        if (read_cluster(archive, block, cluster, buffer) < 0) {     //  vérifie l'erreur
            free(buffer);
            return GENERAL_ERR;
        }


        uint32_t chunk = cluster_size;

        // Si le cluster dépasse la taille restante à lire
        if (total + chunk > read)
            chunk = read - total; // on coupe

        // Copie le buffer
        memcpy((uint8_t *)buff + total, buffer, chunk);

        // Ajoute au total
        total += chunk;
         if (total < read) {
             if (get_cluster_chain_value(cluster, block, &cluster, archive) < 0) {
                 free(buffer);
                 return GENERAL_ERR;
             }
         }
    }
    free(buffer);
    return (error_code)total;
}

        


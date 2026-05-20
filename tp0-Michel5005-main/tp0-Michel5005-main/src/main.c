#include <stdlib.h>
#include <stdio.h>
#include "main.h"


typedef unsigned char byte;
typedef int error_code;

#define ERROR (-1)
#define HAS_ERROR(code) ((code) < 0)
#define HAS_NO_ERROR(code) ((code) >= 0)

/**
 * Cette fonction compare deux chaînes de caractères.       
 * @param p1 la première chaîne
 * @param p2 la deuxième chaîne
 * @return le résultat de la comparaison entre p1 et p2. Un nombre plus petit que
 * 0 dénote que la première chaîne est lexicographiquement inférieure à la deuxième.
 * Une valeur nulle indique que les deux chaînes sont égales tandis qu'une valeur positive
 * indique que la première chaîne est lexicographiquement supérieure à la deuxième.
 */
int strcmp(const char *p1, const char *p2) {
    char c1, c2;
    do {
        c1 = (char) *p1++;
        c2 = (char) *p2++;
        if (c1 == '\0')
            return c1 - c2;
    } while (c1 == c2);
    return c1 - c2;
}

/**
 * Ex. 1: Calcul la longueur de la chaîne passée en paramètre selon
 * la spécification de la fonction strlen standard
 * @param s un pointeur vers le premier caractère de la chaîne
 * @return le nombre de caractères dans le code d'erreur, ou une erreur
 * si l'entrée est incorrecte
 */
error_code strlen2(const char *s) {
    // Si le pointeur est null
    if (s == NULL)
        return ERROR;

    // Compteur de caractère
    error_code length = 0;

    // Tant que le pointeur ne pointe pas vers le caractère nul
    while (*s != '\0') {
        // Incrémente le compteur de caractères et le pointeur
        length++;
        s++;
    }

    return length;
}

/**
 * Ex.2 :Retourne le nombre de lignes d'un fichier sans changer la position
 * courante dans le fichier.
 * @param fp un pointeur vers le descripteur de fichier
 * @return le nombre de lignes, ou -1 si une erreur s'est produite
 */
error_code no_of_lines(FILE *fp) {
    // Si le pointeur est null
    if (fp == NULL)
        return ERROR;

    long pos = ftell(fp);

    // Si ftell échoue
    if (pos == -1)
        return ERROR;

    int counter = 0; // Nb de lignes
    int current; // Caractère courant lu
    int previous = '\n';

    // Lit les caractères jusqu'à la fin du fichier
    while ((current = getc(fp)) != EOF) {
        // Si le caractère courant lu est un saut de ligne
        if (current == '\n') {
            counter++;
        }
        previous = current; // Garde le dernier caractère lu
    }
    // Si le fichier ne termine pas avec un saut de ligne
    if (previous != '\n') {
        counter++;
    }

    // Si fseek échoue
    if (fseek(fp, pos,SEEK_SET) == -1)
        return ERROR;

    return counter;
}

/**
 * Ex.3: Lit une ligne au complet d'un fichier
 * @param fp le pointeur vers la ligne de fichier
 * @param out le pointeur vers la sortie
 * @param max_len la longueur maximale de la ligne à lire
 * @return le nombre de caractère ou ERROR si une erreur est survenue
 */
error_code readline(FILE *fp, char **out, size_t max_len) {
    // Si les pointeurs sont null
    if (fp == NULL || out == NULL)
        return ERROR;

    // Allocation dynamique de caractères
    char *adresse = malloc(max_len + 1);

    // Si l'allocation échoue
    if (adresse == NULL)
        return ERROR;

    int current; // Caractère lu
    int i = 0; // Index

    // Lit les caractères tant qu'on est pas en fin de ligne, fin de fichier ou qu'on a pas atteint la limite maximale
    while ((current = getc(fp)) != '\n' && current != EOF && i < max_len) {
        adresse[i++] = current;
    }

    // Ajout du fin de caractère
    adresse[i] = '\0';

    // Stocke l'adresse
    *out = adresse;

    return (error_code) i;
}

/**
 * Ex.4: Copie un bloc mémoire vers un autre
 * @param dest la destination de la copie
 * @param src  la source de la copie
 * @param len la longueur (en byte) de la source
 * @return nombre de bytes copiés ou une erreur s'il y a lieu
 */
error_code memcpy2(void *dest, const void *src, size_t len) {
    // Si destination ou source sont null
    if (dest == NULL || src == NULL)
        return ERROR;

    // Conversion de pointeurs void en byte
    byte *d = (byte *) dest;
    byte *s = (byte *) src;

    // Parcourt la longueur de la source
    for (size_t i = 0; i < len; i++) {
        d[i] = s[i];
    }

    return (int) len;
}

/**
 * Ex.5: Analyse une ligne de transition
 * @param line la ligne à lire
 * @param len la longueur de la ligne
 * @return la transition ou NULL en cas d'erreur
 */
transition *parse_line(char *line, size_t len) {
    size_t i = 1;

    // Allocation de 6 caractères pour l'état actuel
    char *current_state = malloc(6);

    // Vérifie l'allocation
    if (!current_state)
        return NULL;

    size_t j = 0;

    // Copie les caractères de l'état actuel jusqu'à la virgule et limite à 5 caractères
    while (line[i] != ',' && j < 5) {
        current_state[j++] = line[i++];
    }


    current_state[j] = '\0';
    i++; // Incrémente pour passer la virgule

    // Symbole lu est le caractère suivant
    char read = line[i++];
    i += 4;

    // Allocation de 6 caractères pour le prochain état
    char *next_state = malloc(6);

    // Vérifie l'allocation
    if (!next_state) {
        free(current_state);
        return NULL;
    }

    j = 0;

    // Copie les caractères de l'état suivant jusqu'à la virgule et limite à 5 caractères
    while (line[i] != ',' && j < 5) {
        next_state[j++] = line[i++];
    }

    next_state[j] = '\0';
    i++;

    // Symbole suivant est le caractère à écrire
    char write = line[i++];
    i++;

    // Lit le caractère
    char move = line[i++];
    char movement;

    // Mouvement de la tête de lecture sur le ruban
    switch (move) {
        case 'G':
            movement = -1;
            break;
        case 'R':
            movement = 0;
            break;
        case 'D':
            movement = 1;
            break;
        default:
            movement = 2;
            break;
    }

    // Si le mouvement n'est pas bon ou dépasse la ligne
    if (movement == 2 || i > len) {
        // Libère la mémoire et retourne null
        free(current_state);
        free(next_state);
        return NULL;
    }

    // Allocation de la structure de transition qui auront les données
    transition *trans = malloc(sizeof(transition));

    // Vérification de l'allocation
    if (!trans) {
        free(current_state);
        free(next_state);
        return NULL;
    }

    // Stocke l'information que l'on extrait dans la structure
    trans -> current_state = current_state;
    trans -> next_state = next_state;
    trans -> read = read;
    trans -> write = write;
    trans -> movement = movement;

    return trans;
}

/**
 * Ex.6: Execute la machine de turing dont la description est fournie
 * @param machine_file le fichier de la description
 * @param input la chaîne d'entrée de la machine de turing
 * @return le code d'erreur
 */
error_code execute(char *machine_file, char *input) {
    // Si le fichier ou la chaine d'entrée est nulle
    if (machine_file == NULL || input == NULL)
        return ERROR;

    // Ouvre le fichier
    FILE *fp = fopen(machine_file, "r");

    // Si l'ouverture du fichier échoue
    if (!fp)
        return ERROR;

    int nb_lines = no_of_lines(fp);

    // Si no_of_lines échoue
    if (nb_lines == ERROR) {

        // Ferme le fichier et retourne ERROR
        fclose(fp);
        return ERROR;
    }

    // Pointeurs d'états initial, acceptant et rejetant
    char *start_state, *accept_state, *reject_state;

    // Si lecture échoue
    if (readline(fp, &start_state, 6) == ERROR ||
        readline(fp, &accept_state, 6) == ERROR ||
        readline(fp, &reject_state,6) == ERROR) {

        // Libère la mémoire et retourne ERROR
        free(start_state);
        free(accept_state);
        free(reject_state);
        fclose(fp);
        return ERROR;
    }

    int input_length = strlen2(input);

    // Si strlen2 échoue
    if (input_length == ERROR) {

        // Libère la mémoire et retourne ERROR
        free(start_state);
        free(accept_state);
        free(reject_state);
        fclose(fp);
        return ERROR;
    }

    // Taille initiale du ruban
    int tape_length = input_length * 2;

    // Met une taille minimale de 256 pour le ruban
    if (tape_length < 256)
        tape_length = 256;

    // Allocation dynamique du ruban
    char *tape = malloc(tape_length);

    // Si l'allocation du ruban échoue
    if (!tape) {

        // Libère la mémoire et retourne ERROR
        free(start_state);
        free(accept_state);
        free(reject_state);
        fclose(fp);
        return ERROR;
    }

    // Remplie tout le ruban avec le caractère vide
    for (int i = 0; i < tape_length; i++)
        tape[i] = ' ';

    // Position initiale de la tête du ruban
    int head = tape_length / 2 - input_length / 2;

    // Copie l'entrée sur le ruban à partir de la position de la tête du ruban
    memcpy2(&tape[head], input, input_length);

    // Nombre de transitions (3 = états spéciaux)
    int nb_trans = nb_lines - 3;

    // Allocation d'un tableau de pointeurs de taille nb_trans
    transition **trans = malloc(nb_trans * sizeof(transition *));

    // Si l'allocation échoue
    if (!trans) {

        // Libère la mémoire, ferme le fichier et retourne ERROR
        free(start_state);
        free(accept_state);
        free(reject_state);
        free(tape);
        fclose(fp);
        return ERROR;
    }

    // Passe chaque ligne de transition
    for (int i = 0; i < nb_trans; i++) {

        // Lecture d'une ligne
        char *line = NULL;
        int length = readline(fp, &line, 256);

        // Si la lecture d'une ligne échoue
        if (length == ERROR) {

            for (int j = 0; j < i; j++) {

                // Libère la mémoire des transitions déjà lues
                free(trans[j] -> current_state);
                free(trans[j] -> next_state);
                free(trans[j]);
            }

            // Libère la mémoire, ferme le fichier et retourne ERROR
            free(trans);
            free(start_state);
            free(accept_state);
            free(reject_state);
            free(tape);
            fclose(fp);
            return ERROR;
        }

        // Transforme la ligne brute lue en structure transition
        trans[i] = parse_line(line, length);

        // Libère la mémoire
        free(line);

        // Si le parsing échoue
        if (!trans[i]) {

            for (int k = 0; k < i; k++) {

                // Libère la mémoire des transitions déjà crées
                free(trans[k] -> current_state);
                free(trans[k] -> next_state);
                free(trans[k]);
            }

            // Libère la mémoire, ferme le fichier et retourne ERROR
            free(trans);
            free(start_state);
            free(accept_state);
            free(reject_state);
            free(tape);
            fclose(fp);
            return ERROR;
        }
    }

    // Ferme le fichier
    fclose(fp);

    // Parcourt chaque transition pour vérifier leur validité
    for (int i = 0; i <nb_trans; i++) {

        transition *t = trans[i];

        // Si une des conditions échoue, la transition n'est pas valide
        if (strlen2(t-> current_state) == 0 ||
            strlen2(t -> next_state) == 0 ||
            t-> movement < -1 || t->movement > 1 ||
            t-> read == '\0' || t-> write == '\0') {

            for (int j = 0; j < nb_trans; j++) {

                // Libère la mémoire des transitions
                free(trans[j] -> current_state);
                free(trans[j]-> next_state);
                free(trans[j]);
            }

            // Libère la mémoire et retourne ERROR
            free(trans);
            free(start_state);
            free(accept_state);
            free(reject_state);
            free(tape);
            return ERROR;
        }
    }

    char current_state[6];
    memcpy2(current_state,start_state,strlen2(start_state) + 1);

    // Boucle de la machine de Turing
    while (strcmp(current_state, accept_state) != 0 && strcmp(current_state,reject_state) != 0) {

        // Lit le symbole sous la tête du ruban
        char symbol = tape[head];
        transition *t = NULL;

        // Cherche la transition applicable
        for (int i = 0; i < nb_trans; i++) {

            if (strcmp(trans[i] -> current_state, current_state) == 0 && trans[i] -> read == symbol) {
                t = trans[i];
                break;
            }
        }

        // Si aucune transition ne s'applique
        if (!t) {
            for (int i = 0; i < nb_trans; i++) {

                // Libère la mémoire des transitions
                free(trans[i] -> current_state);
                free(trans[i] -> next_state);
                free(trans[i]);
            }

            // Libère la mémoire et retourne ERROR
            free(trans);
            free(start_state);
            free(accept_state);
            free(reject_state);
            free(tape);
            return ERROR;
        }

        // Application de la transition
        tape[head] = t -> write;
        head += t -> movement;
        memcpy2(current_state, t-> next_state, strlen2(t-> next_state) + 1);

        // Si la tête est dehors des limites du ruban
        if (head < 0 || head >= tape_length) {

            // Double la taille du ruban et alloue un nouveau ruban
            int new_length = tape_length * 2;
            char *new_tape = malloc(new_length);

            // Si l'allocation du nouveau ruban échoue
            if (!new_tape) {
                for (int i =0; i < nb_trans; i++) {

                    // Libère la mémoire des transitions
                    free(trans[i] ->current_state);
                    free(trans[i] -> next_state);
                    free(trans[i]);
                }

                // Libère la mémoire et retoure ERROR
                free(trans);
                free(start_state);
                free(accept_state);
                free(reject_state);
                free(tape);
                return ERROR;
            }

            // Remplit le nouveau ruban avec le caractère vide
            for (int i = 0; i < new_length; i++)
                new_tape[i] = ' ';

            // Copie l'ancien ruban au centre du nouveau avec un décalage
            int offset = new_length / 2 - tape_length / 2;
            memcpy2(&new_tape[offset], tape, tape_length);

            // Ajuste la position de la tête du ruban et on met à jour la longueur du ruban
            head += offset;
            tape_length = new_length;

            free(tape);
            tape = new_tape;
        }
    }

    // Si l'état actuel est l'état acceptant, result vaut 1 sinon vaut 0
    int result = (strcmp(current_state, accept_state) == 0) ? 1 : 0;

    for (int i = 0; i < nb_trans; i++) {

        // Libère la mémoire des transitions
        free(trans[i] -> current_state);
        free(trans[i] -> next_state);
        free(trans[i]);
    }

    // Libère la mémoire et retourne le resultat
    free(trans);
    free(start_state);
    free(accept_state);
    free(reject_state);
    free(tape);

    return result;

}

// ATTENTION! TOUT CE QUI EST ENTRE LES BALISES ༽つ۞﹏۞༼つ SERA ENLEVÉ! N'AJOUTEZ PAS D'AUTRES ༽つ۞﹏۞༼つ

// ༽つ۞﹏۞༼つ

int main() {
    // Vous pouvez ajouter des tests pour les fonctions ici

    printf("Exercice-6\n");

    printf(execute("../has_five_ones", "0000") == 0 ? "true" : "false");
    printf(execute("../has_five_ones", "101010101") == 1 ? "true" : "false");
    printf(execute("../has_five_ones", "111111111") == 1 ? "true" : "false");

    return 0;
}

// ༽つ۞﹏۞༼つ
